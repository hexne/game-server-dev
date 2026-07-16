/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/
module;
#include <sw/redis++/redis++.h>
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
public:

};
UserStateManager user_state_manager;
OnlineUserList online_user_list([](int id) {
    auto user = user_state_manager.search_user_state_by_user_id(id);
    if (!user)
        return;
    auto fd = user->fd;
    user_state_manager.remove_fd(fd);

    // @TODO, 如果在房间里，通知房间中的其他人
});


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
    // auto it = online_rooms.find(room);
    // it->get()->add_user(user_id);

    // int room_id = it->get()->id();
    auto room_id_str = std::to_string(room->id());

    char buf[1024]{};
    auto size = message::write(buf, header::type::room_create_true, std::span{room_id_str.data(), room_id_str.size()});
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
        auto size = message::write(buf, header::type::room_join, id, room_id);

        user_socket->send_now(std::span{buf, size});
    }
}


// server 的事件分发
Router events_router {
    { header::type::login, login },
    { header::type::heart, heart },
    { header::type::room_create, room_create },
    { header::type::room_invite, room_invite },
    { header::type::room_invite_accept, room_invite_accept },
};

export void server_main() {
    Log().push_log("Server start");

    Epoll epoll;
    TCP server_listen(Address("0.0.0.0", 8080));
    server_listen.bind();
    server_listen.listen();

    epoll.add(server_listen.fd(), epoll_in | epoll_et);
    Log().push_log("epoll ADD");


    epoll_event events[64];
    while (true) {
        // 有几个消息
        int n = epoll.wait(events, 64);

        for (int i = 0;i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == server_listen.fd()) {
                TCP client = server_listen.accept();
                int client_fd = client.fd();
                Log().push_log("Server accept a new connect");
                user_state_manager.add_fd(client_fd, std::make_unique<TCP>(std::move(client)));

                epoll.add(client_fd, epoll_in | epoll_out | epoll_et);
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
                events_router[type](span.subspan(header::header_size), tcp);
            }

            if (events[i].events & (epoll_hup | epoll_err)) {
                user_state_manager.remove_fd(fd);
            }

        }
    }
}
