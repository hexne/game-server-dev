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
import online_user_list;
import time;
import room;
import std;
import user;
import user_manager;
import timer;
import battle_manager;


export class Server {
    std::unique_ptr<TCP> server_listen_;
    UserManager user_manager_;
    RoomManager room_manager_;
    BattleManager battle_manager_;
    int match_timer_fd_;
    int remove_closed_rooms_fd_;
    int pending_match_timeout_fd_;
    int tick_fd_;
    Timer timer_;
    Database db_;
    std::mutex db_mutex;

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
        { header::type::battle_pick_hero, &Server::battle_pick_hero },
        { header::type::battle_start_load, &Server::battle_load },
    };

    // 匹配成功, 向id发送匹配成功信息
    void match_success(int user_id, int pending_match_id) {
        auto user = user_manager_.search_user_by_id(user_id);
        if (!user)
            return;
        auto &tcp = user->tcp();
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

        // 匹配成功之后，除了告诉用户匹配状态，同时插入匹配超时定时器
        for (auto &pending_match : res) {
            const auto &[pending_match_id, _, room_a, room_b, _] = *pending_match;
            for (auto &user_id : room_a->users())
                match_success(user_id, pending_match_id);
            for (auto &user_id : room_b->users())
                match_success(user_id, pending_match_id);

            timer_.add_task([pending_match, pending_match_id, this] {
                // @FIXME 需要考虑pending_match 后续不存在的问题
                pending_match->is_confirm_timeout = true;
                message::send_signal(pending_match_timeout_fd_, pending_match_id);
            }, PendingMatch::confirm_timeout);
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
            auto &[_, _, room_a, room_b, _] = *pending_match;

            auto users_a = room_a->users();
            auto users_b = room_b->users();
            int battle_id = battle_manager_.add_battle(users_a, users_b);

            char buf[512]{};
            auto size = message::write(buf, header::type::battle_pick_hero, battle_id);

            for (const auto cur_user_id : std::views::concat(users_a, users_b)) {
                auto user = user_manager_.search_user_by_id(cur_user_id);
                if (!user)
                    return;
                auto &tcp = user->tcp();
                if (!tcp)
                    return;
                tcp->send_now(std::span{buf, size});
            }
        }
    }

    // 用户拒绝匹配, 所有用户取消匹配
    void match_reject(std::span<char> msg, TCP *socket) {
        int user_id = message::read(msg.data());
        int pending_match_id = message::read(msg.data() + sizeof(int));
        match_cancel(pending_match_id);
    }

    // 通知某个已经匹配成功的对局取消
    void match_cancel(int pending_match_id) {
        auto pending_match = room_manager_.search_pending_match(pending_match_id);

        auto all_users = pending_match->room_a->users();
        all_users.append_range(pending_match->room_b->users());

        room_manager_.pending_match_cancel(pending_match_id);

        // @NOTE, 对局取消只有可能是有用户拒绝了对局，显示某用户拒绝对局即可，甚至无需该用户id
        char buf[512]{};
        const auto size = message::write(buf, header::type::match_cancel);
        for (auto user_id : all_users) {

            auto user = user_manager_.search_user_by_id(user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }

    void tick() {
        // @TODO, 遍历所有对局，发送快照
    }
public:

    Server(): db_("root", "123456", "game") {
        match_timer_fd_ = eventfd(0, EFD_NONBLOCK);
        remove_closed_rooms_fd_ = eventfd(0, EFD_NONBLOCK);
        pending_match_timeout_fd_ = eventfd(0, EFD_NONBLOCK);
        tick_fd_ = eventfd(0, EFD_NONBLOCK);

        timer_.add_repeat_task([this] {
            message::send_signal(match_timer_fd_);
        }, std::chrono::seconds{10});

        timer_.add_repeat_task([this] {
            message::send_signal(remove_closed_rooms_fd_);
        }, std::chrono::minutes{1});

        timer_.add_repeat_task([this] {
            message::send_signal(tick_fd_);
        }, std::chrono::milliseconds{1000 / 64});

    }

    // login "number"
    void login(std::span<char> msg, TCP *socket) {
        auto pos = std::ranges::find(msg, ':');
        if (pos == msg.end())
            throw std::invalid_argument("invalid server type");
        std::string number(msg.begin(), pos);
        // + 1 跳过 ':'
        std::string password_hash(pos + 1, msg.end());

        std::string user_info;
        {
            std::lock_guard lock(db_mutex);
            user_info = search_user_profile(db_, number);
        }

        auto login_false = [socket] {
            char buf[header::header_size]{};
            auto size = message::write(buf, header::type::login_false);
            socket->send_now(std::span{buf, size});
            Log().push_log("login error");
        };
        if (user_info.empty()) {
            login_false();
            return;
        }

        auto user = std::make_shared<User>(user_info);
        if (user->password_hash() != password_hash) {
            login_false();
            return;
        }
        char buf[512]{};
        auto size = message::write(buf, header::type::login_true, std::span{user_info.data(), user_info.size()});
        socket->send_now(std::span{buf, size});

        auto update_user = user_manager_.search_user_by_fd(socket->fd());
        if (!update_user)
            return;
        update_user->update_user_info(user_info);
    }

    // heart <id>
    void heart(std::span<char> msg, TCP *socket) {
        if (msg.size() != sizeof(int))
            throw std::invalid_argument("invalid heart message");

        int user_id = message::read(msg);
        user_manager_.heart_update(user_id);
    }

    // room_create <user_id>
    void room_create(std::span<char> msg, TCP *socket) {
        int user_id = message::read(msg);
        // 创建一个room, 将id放到这个room中
        auto room = std::make_shared<Room>(Room::room_create(user_id));
        room_manager_.add_free_room(room);

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

        auto user = user_manager_.search_user_by_id(user2);
        if (!user)
            return;
        auto &tcp = user->tcp();
        if (!tcp)
            return;

        tcp->send_now(std::span{buf, size});
    }

    void room_invite_accept(std::span<char> msg, TCP *socket) {
        int user = message::read(msg);
        int room_id = message::read(msg.data() + sizeof(int));

        auto room = room_manager_.search_room(room_id);

        room->add_user(user);

        // @TODO 房间应该加锁
        auto users = room->users();
        for (auto id : users) {
            auto cur_user = user_manager_.search_user_by_id(id);
            if (!cur_user)
                continue;
            auto &tcp = cur_user->tcp();
            if (!tcp)
                continue;

            char buf[512]{};
            auto size = message::write(buf, header::type::room_join, id, room_id, room->master());

            tcp->send_now(std::span{buf, size});
        }
    }

    void room_invite_reject(std::span<char> msg, TCP *socket) {
        auto user1 = message::read(msg.data());
        auto user2 = message::read(msg.data() + sizeof(int));

        auto user = user_manager_.search_user_by_id(user2);
        if (!user)
            return;
        auto &tcp = user->tcp();
        if (!tcp)
            return;

        char buf[512]{};
        auto size = message::write(buf, header::type::room_invite_reject, user1);
        tcp->send_now(std::span{buf, size});
    }

    // room_leave <user> <room_id>
    void room_leave(std::span<char> msg, TCP *socket) {
        auto id = message::read(msg.data());
        auto room_id = message::read(msg.data() + sizeof(int));

        auto room = room_manager_.search_room(room_id);
        if (!room)
            return;
        auto users = room->users();

        room->remove_user(id);

        for (auto user_id : users) {
            char buf[512]{};
            auto size = message::write(buf, header::type::room_leave, id, room_id);

            auto user = user_manager_.search_user_by_id(user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }

    void room_chat(std::span<char> msg, TCP *socket) {

        auto id = message::read(msg.data());
        auto room_id = message::read(msg.data() + sizeof(int));

        auto room = room_manager_.search_room(room_id);
        if (!room)
            return;

        auto users = room->users();
        for (auto user_id : users) {
            char buf[512]{};
            auto size = message::write(buf, header::type::room_chat, msg);

            auto user = user_manager_.search_user_by_id(user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }
    void match_join(std::span<char> msg, TCP *socket) {

        int room_id = message::read(msg.data());
        auto room = room_manager_.search_room(room_id);

        // 把房间添加到匹配队列中
        room_manager_.add_matching_room(room);

        try_match();
    }

    // 收到用户选择英雄后，如果所有用户都选择完成，通知客户端开始加载
    void battle_pick_hero(std::span<char> msg, TCP *socket) {
        auto battle_id = message::read(msg.data());
        auto user_id = message::read(msg.data() + sizeof(int));
        auto hero_name = static_cast<HeroName>(message::read(msg.data() + sizeof(int) * 2));
        battle_manager_.user_pick_hero(battle_id, user_id, hero_name);

        if (!battle_manager_.all_players_picked(battle_id))
            return; // 等待其他用户选择英雄, 此处不写超时逻辑，匹配已经写过了

        // 通知所有用户开始加载
        char buf[512]{};
        auto size = message::write(buf, header::type::battle_start_load);
        auto all_user = battle_manager_.all_users(battle_id);
        for (auto cur_user_id : all_user) {
            auto user = user_manager_.search_user_by_id(cur_user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }

    void battle_load(std::span<char> msg, TCP *socket) {
        int battle_id = message::read(msg.data());
        int user_id = message::read(msg.data() + sizeof(int));
        int val = message::read(msg.data() + sizeof(int) * 2);

        battle_manager_.battle_load(battle_id, user_id, val);

        if (!battle_manager_.all_players_picked(battle_id))
            ;   // 没有全部加载完就不管


        // 全部加载完广播所有用户开始战斗
        char buf[512]{};
        auto size = message::write(buf, header::type::battle_start);
        auto all_user = battle_manager_.get_battle(battle_id)->all_users();
        for (auto cur_user_id : all_user) {
            auto user = user_manager_.search_user_by_id(cur_user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }

    }

    void run() {
        Epoll epoll;
        server_listen_ = std::make_unique<TCP>(Address("0.0.0.0", 8080));
        server_listen_->bind();
        server_listen_->listen();

        epoll.add(server_listen_->fd(), epoll_in | epoll_et);
        epoll.add(match_timer_fd_, epoll_in);
        epoll.add(remove_closed_rooms_fd_, epoll_in);
        epoll.add(pending_match_timeout_fd_, epoll_in);

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
                    user_manager_.add_fd(client_fd, std::make_unique<TCP>(std::move(client)));

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
                if (fd == pending_match_timeout_fd_) {
                    int pending_match_id = message::consume_signal(fd);
                    match_cancel(pending_match_id);
                    continue;
                }
                if (fd == tick_fd_) {
                    message::consume_signal(fd);
                    tick();
                    continue;
                }

                auto user = user_manager_.search_user_by_fd(fd);
                if (!user)
                    continue;
                auto &tcp = user->tcp();
                if (!tcp)
                    continue;
                if (events[i].events & epoll_in | epoll_et)
                    tcp->get_message_impl();

                if (events[i].events & epoll_out)
                    tcp->send_message_impl();

                while (auto msg = tcp->get_message()) {
                    auto span = std::span{msg->data(), msg->size()};
                    auto type = message::read_header(span);
                    if (!events_router.contains(type))
                        throw std::invalid_argument("invalid server type");
                    (this->*events_router[type])(span.subspan(header::header_size), tcp.get());
                }

                if (events[i].events & (epoll_hup | epoll_err)) {
                    user_manager_.remove_user_by_fd(fd);
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
