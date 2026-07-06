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


Database db("root", "123456", "game");
std::mutex db_mutex;

OnlineUserList online_user_list([](int id) {
});

std::set<std::shared_ptr<Room>> online_rooms;

export class Server {
public:

};


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

// room_create "user_id"
void room_create(std::span<char> msg, TCP *socket) {
    int user_id = std::stoi(std::string(msg.data(), msg.size()));

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

// invite "user_id1" "user_id2"
// user1 invite user2, 需要先找到user1 在哪个房间？？？
void invite_room(std::span<char> msg, TCP *socket) {


}



// server 的事件分发
Router events_router {
    { header::type::login, login },
    { header::type::heart, heart },
    { header::type::room_create, room_create },
};

std::vector<std::unique_ptr<TCP>> clients;
std::vector<std::jthread> client_threads;
std::mutex clients_mutex;

// void handle_client(TCP& client) {
//     char buf[1024];
//     while (true) {
//         auto msg = client.recv(buf);
//         if (msg.empty())
//             break;
//
//         auto type = header::read(msg);
//         if (!events.contains(type))
//             throw std::invalid_argument("invalid server type");
//         events[type](msg.subspan(header::header_size()), client);
//     }
// }

export void server_main() {
    Log().push_log("Server start");

    Epoll epoll;
    TCP server(Address("0.0.0.0", 8080));
    server.bind();
    server.listen();

    epoll.add(server.fd(), epoll_in, &server);
    Log().push_log("epoll ADD");

    std::unordered_map<int, std::unique_ptr<TCP>> clients;

    epoll_event events[64];
    while (true) {
        int n = epoll.wait(events, 64);

        for (int i = 0;i < n; ++i) {
            TCP *tcp = static_cast<TCP *>(events[i].data.ptr);

            // 如果是监听者
            if (tcp->is_listener()) {
                TCP client = tcp->accept();
                int client_fd = client.fd();
                Log().push_log("Server accept a new connect");

                clients[client_fd] = std::make_unique<TCP>(std::move(client));

                epoll.add(client_fd, epoll_in | epoll_out | epoll_et, clients[client_fd].get());
                continue;
            }

            // 如果是普通连接
            if (events[i].events & epoll_in) {
                tcp->get_message_impl();
            }
            if (events[i].events & epoll_out) {
                tcp->send_message_impl();
            }

            while (auto msg = tcp->get_message()) {
                auto span = std::span{msg->data(), msg->size()};
                auto type = message::read_header(span);
                if (!events_router.contains(type))
                    throw std::invalid_argument("invalid server type");
                events_router[type](span.subspan(header::header_size), tcp);
            }

        }


    }



}
