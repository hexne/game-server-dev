/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/
module;
#include <sw/redis++/redis++.h>
#include <unistd.h>
#include <sys/eventfd.h>
export module server;
import log;
import net;
import database;
import message;
import online_user;
import time;
import room;
import std;
import user_state;
import timer;


Database db("root", "123456", "game");
std::mutex db_mutex;


std::set<std::shared_ptr<Room>> online_rooms;

std::shared_ptr<Room> search_room_by_id(int id) {
    for (auto room : online_rooms) {
        if (room->id() == id)
            return room;
    }
    return {};
}

export class Server {
    std::unique_ptr<TCP> server_listen_;
    UserStateManager user_state_manager;
    OnlineUserList online_user_list;
    RoomManager room_manager_;
    int match_timer_fd_;
    int remove_closed_rooms_fd_;
    Timer timer_;

    // server 的事件分发
    std::map<header::type, void (Server::*)(std::span<char>, TCP *)> events_router {
        { header::type::login, &Server::login },
        { header::type::heart, &Server::heart },
        { header::type::room_create, &Server::room_create },
        { header::type::room_invite, &Server::room_invite },
        { header::type::room_invite_accept, &Server::room_invite_accept },
        { header::type::room_invite_reject, &Server::room_invite_reject },
        { header::type::room_leave, &Server::room_leave },
        { header::type::room_chat, &Server::room_chat },
        { header::type::match_join, &Server::match_join },
        { header::type::match_accept, &Server::match_accept },
        { header::type::match_reject, &Server::match_reject },
    };

    // 匹配成功, 向id发送匹配成功信息
    void match_success(int user_id, int pending_match_id) {
        auto status = user_state_manager.search_user_state_by_user_id(user_id);
        if (!status)
            return;
        auto &tcp = status->tcp;
        if (!tcp)
            return;

        char buf[512]{};
        auto size = message::write(buf, header::type::match_success, pending_match_id);
        tcp->send_now(std::span{buf, size});
    }

    // 尝试匹配
    void try_match() {
        auto res = room_manager_.try_match();
        if (res.empty())
            return;

        for (auto &pending_match : res) {
            const auto &[pending_match_id, room_a, room_b, _] = *pending_match;
            for (auto &user_id : room_a->users())
                match_success(user_id, pending_match_id);
            for (auto &user_id : room_b->users())
                match_success(user_id, pending_match_id);
        }
    }

    // 用户同意匹配
    void match_accept(std::span<char> msg, TCP *socket) {
        int user_id = message::read(msg.data());
        int pending_match_id = message::read(msg.data() + sizeof(int));

        // 插入用户
        auto pending_match = room_manager_.search_pending_match(pending_match_id);
        pending_match->confirmed.insert(user_id);
        int all_user_count = pending_match->room_a->users().size() + pending_match->room_b->users().size();

        // 还有用户没有确认
        if (pending_match->confirmed.size() < all_user_count) {
            ; // 继续等待
        }
        // 所有用户都确认了
        else if (pending_match->confirmed.size() == all_user_count) {
            ; // @TODO 开始对局
        }
    }

    // 用户拒绝匹配, 所有用户取消匹配
    void match_reject(std::span<char> msg, TCP *socket) {
        int user_id = message::read(msg.data());
        int pending_match_id = message::read(msg.data() + sizeof(int));

        auto pending_match = room_manager_.search_pending_match(pending_match_id);

        auto all_users = pending_match->room_a->users();
        all_users.append_range(pending_match->room_b->users());

        room_manager_.pending_match_cancel(pending_match_id);

        char buf[512]{};
        const auto size = message::write(buf, header::type::match_cancel);
        for (auto user : all_users) {

            auto user_status = user_state_manager.search_user_state_by_user_id(user);
            if (!user_status)
                continue;
            auto &tcp = user_status->tcp;
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }

public:

    Server() : online_user_list([this](int id) {
        auto user = user_state_manager.search_user_state_by_user_id(id);
         if (!user)
             return;
         auto fd = user->fd;
         user_state_manager.remove_fd(fd);
    }) {
        match_timer_fd_ = eventfd(0, EFD_NONBLOCK);
        remove_closed_rooms_fd_ = eventfd(0, EFD_NONBLOCK);

        timer_.add_repeat_task([this] {
            message::send_signal(match_timer_fd_);
        }, std::chrono::seconds{10});

        timer_.add_repeat_task([this] {
            message::send_signal(remove_closed_rooms_fd_);
        }, std::chrono::minutes{1});

    }

    // login "number"
    void login(std::span<char> msg, TCP *socket) {
        auto pos = std::ranges::find(msg, ':');
        if (pos == msg.end())
            throw std::invalid_argument("invalid server type");
        std::string number(msg.begin(), pos);
        // + 1 跳过 ':'
        std::string password_hash(pos + 1, msg.end());

        std::optional<std::array<std::string, 5>> user;
        {
            std::lock_guard lock(db_mutex);
            auto res = db.query(std::format(
                "SELECT password_hash, id, name, number, create_time FROM users WHERE number = '{}'",
                number
            ));

            if (!res.empty()) {
                user = std::array<std::string, 5> {
                    res[0],
                    res[1],
                    res[2],
                    res[3],
                    res[4]
                };
            }
        }

        if (!user) {
            char buf[header::header_size]{};
            auto size = message::write(buf, header::type::login_false);
            socket->send_now(std::span{buf, size});
        }
        else {
            auto send_msg = std::format("{}:{}:{}:{}", (*user)[1], (*user)[2], (*user)[3], (*user)[4]);
            char buf[1024]{};
            auto size = message::write(buf, header::type::login_true, std::span{send_msg.data(), send_msg.size()});
            socket->send_now(std::span{buf, size});

            // 登录成功，直接注册到在线列表
            online_user_list.update(std::stoi((*user)[1]), Time::now());
            user_state_manager.bind_user(std::stoi((*user)[1]), socket->fd());
        }
    }

    // heart <id>
    void heart(std::span<char> msg, TCP *socket) {
        if (msg.size() != sizeof(int))
            throw std::invalid_argument("invalid heart message");

        int id = message::read(msg);

        online_user_list.update(id, Time::now());
        // Log().push_log(std::format("Server get {} heart", id));
    }

    // room_create <user_id>
    void room_create(std::span<char> msg, TCP *socket) {
        int user_id = message::read(msg);
        // 创建一个room, 将id放到这个room中
        auto room = std::make_shared<Room>(Room::room_create(user_id));
        online_rooms.insert(room);
        char buf[1024]{};
        auto size = message::write(buf, header::type::room_create_true, room->id(), room->master());
        socket->send_now(std::span{buf, size});
    }

    // invite <user_id1> <user_id2> <room_id>
    // user1 invite user2 to room_id
    void room_invite(std::span<char> msg, TCP *socket) {
        int user1 = message::read(msg);
        int user2 = message::read(msg.data() + sizeof(int));
        int room_id = message::read(msg.data() + sizeof(int) * 2);

        char buf[1024]{};
        auto size = message::write(buf, header::type::room_invite_message, user1, user2, room_id);
        std::println(std::cout, "{} invite {} to {}", user1, user2, room_id);

        auto user_state = user_state_manager.search_user_state_by_user_id(user2);
        auto tcp = user_state->tcp.get();

        tcp->send_now(std::span{buf, size});
    }

    void room_invite_accept(std::span<char> msg, TCP *socket) {
        int user = message::read(msg);
        int room_id = message::read(msg.data() + sizeof(int));

        auto room = search_room_by_id(room_id);

        room->add_user(user);

        // @TODO 房间应该加锁
        auto users = room->users();
        for (auto id : users) {
            auto user_socket = user_state_manager.search_user_state_by_user_id(id)->tcp.get();

            char buf[512]{};
            auto size = message::write(buf, header::type::room_join, id, room_id, room->master());

            user_socket->send_now(std::span{buf, size});
        }
    }

    void room_invite_reject(std::span<char> msg, TCP *socket) {
        auto user1 = message::read(msg.data());
        auto user2 = message::read(msg.data() + sizeof(int));

        auto user = user_state_manager.search_user_state_by_user_id(user2);
        if (!user)
            return;

        char buf[512]{};
        auto size = message::write(buf, header::type::room_invite_reject, user1);
        user->tcp->send_now(std::span{buf, size});
    }

    // room_leave <user> <room_id>
    void room_leave(std::span<char> msg, TCP *socket) {
        auto id = message::read(msg.data());
        auto room_id = message::read(msg.data() + sizeof(int));

        auto room = search_room_by_id(room_id);
        if (!room)
            return;
        auto users = room->users();

        room->remove_user(id);

        for (auto user : users) {
            char buf[512]{};
            auto size = message::write(buf, header::type::room_leave, id, room_id);

            auto tcp = user_state_manager.search_user_state_by_user_id(user)->tcp.get();
            tcp->send_now(std::span{buf, size});
        }
    }

    void room_chat(std::span<char> msg, TCP *socket) {

        auto id = message::read(msg.data());
        auto room_id = message::read(msg.data() + sizeof(int));

        auto room = search_room_by_id(room_id);
        if (!room)
            return;

        auto users = room->users();
        for (auto user : users) {
            char buf[512]{};
            auto size = message::write(buf, header::type::room_chat, msg);

            auto user_state = user_state_manager.search_user_state_by_user_id(user);
            if (!user_state || !user_state->tcp)
                return;

            user_state->tcp->send_now(std::span{buf, size});
        }
    }
    void match_join(std::span<char> msg, TCP *socket) {

        int room_id = message::read(msg.data());
        auto room = search_room_by_id(room_id);

        // 把房间添加到匹配队列中
        room_manager_.add_matching_room(room);

        try_match();
    }


    void run() {
        Epoll epoll;
        server_listen_ = std::make_unique<TCP>(Address("0.0.0.0", 8080));
        server_listen_->bind();
        server_listen_->listen();

        epoll.add(server_listen_->fd(), epoll_in | epoll_et);
        epoll.add(match_timer_fd_, epoll_in);
        epoll.add(remove_closed_rooms_fd_, epoll_in);

        Log().push_log("epoll ADD");


        epoll_event events[64];
        while (true) {
            // 有几个消息
            int n = epoll.wait(events, 64);

            for (int i = 0;i < n; ++i) {
                int fd = events[i].data.fd;

                if (fd == server_listen_->fd()) {
                    TCP client = server_listen_->accept();
                    int client_fd = client.fd();
                    Log().push_log("Server accept a new connect");
                    user_state_manager.add_fd(client_fd, std::make_unique<TCP>(std::move(client)));

                    epoll.add(client_fd, epoll_in | epoll_out | epoll_et);
                    continue;
                }
                if (fd == match_timer_fd_) {
                    message::consume_signal(fd);
                    try_match();
                    continue;
                }
                if (fd == remove_closed_rooms_fd_) {
                    message::consume_signal(fd);
                    room_manager_.remove_closed_rooms();
                    continue;
                }

                auto state = user_state_manager.search_user_state_by_fd(fd);
                if (!state) continue;

                TCP *tcp = state->tcp.get();

                if (events[i].events & epoll_in | epoll_et)
                    tcp->get_message_impl();

                if (events[i].events & epoll_out)
                    tcp->send_message_impl();

                while (auto msg = tcp->get_message()) {
                    auto span = std::span{msg->data(), msg->size()};
                    auto type = message::read_header(span);
                    if (!events_router.contains(type))
                        throw std::invalid_argument("invalid server type");
                    (this->*events_router[type])(span.subspan(header::header_size), tcp);
                }

                if (events[i].events & (epoll_hup | epoll_err)) {
                    user_state_manager.remove_fd(fd);
                }

            }
        }

    }
};






export void server_main() {
    Log().push_log("Server start");

    Server server;
    server.run();


}
