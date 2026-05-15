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
import std;


Database db("root", "123456", "game");
std::mutex db_mutex;

OnlineUserList online_user_list([](int id) {
    
});

export class Server {
public:

};


void login(std::span<char> msg, TCP &socket) {
    auto pos = std::ranges::find(msg, ':');
    if (pos == msg.end())
        throw std::invalid_argument("invalid server type");
    std::string number(msg.begin(), pos);
    std::cout << "login : " << number << std::endl;
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
        char buf[header::header_size()]{};
        auto size = message::write(buf, header::type::login_err, {});
        socket.send(std::span{buf, size});
    }
    else {
        auto send_msg = std::format("{}:{}:{}:{}", (*user)[1], (*user)[2], (*user)[3], (*user)[4]);
        char buf[1024]{};
        auto size = message::write(buf, header::type::login_true, std::span{send_msg.data(), send_msg.size()});
        socket.send(std::span{buf, size});

        // 登录成功，直接注册到在线列表
        online_user_list.update(std::stoi((*user)[1]), Time::now());
    }
}



void heart(std::span<char> msg, TCP &socket) {
    if (msg.size() != sizeof(int))
        throw std::invalid_argument("invalid heart message");

    int id{};
    std::memcpy(&id, msg.data(), sizeof(id));

    online_user_list.update(id, Time::now());
    Log().push_log(std::format("Server get {} heart", id));
}

// server 的事件分发
Router events {
    { header::type::login, login },
    { header::type::heart, heart }
};

std::vector<std::unique_ptr<TCP>> clients;
std::vector<std::jthread> client_threads;
std::mutex clients_mutex;

void handle_client(TCP& client) {
    char buf[1024];
    while (true) {
        auto msg = client.recv(buf);
        if (msg.empty())
            break;

        auto type = header::read(msg);
        if (!events.contains(type))
            throw std::invalid_argument("invalid server type");
        events[type](msg.subspan(header::header_size()), client);
    }
}

export void server_main() {
    Log().push_log("Server start");
    TCP socket(Address {"0.0.0.0", 8080});

    socket.bind();
    socket.listen();

    while (true) {
        auto client = std::make_unique<TCP>(socket.accept());
        auto client_ptr = client.get();
        {
            std::lock_guard lock(clients_mutex);
            clients.emplace_back(std::move(client));
            Log().push_log(std::format("Client connected: {}", clients.size()));
        }

        client_threads.emplace_back([client_ptr] {
            handle_client(*client_ptr);
        });
    }
}
