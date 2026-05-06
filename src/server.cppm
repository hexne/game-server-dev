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
import std;

Database db("root", "123456", "game");
sw::redis::Redis redis("tcp://127.0.0.1:6379");

export class Server {
public:

};


void login(std::span<char> msg, TCP &socket) {
    auto pos = std::ranges::find(msg, ':');
    if (pos == msg.end())
        throw std::invalid_argument("invalid server type");
    std::string number(msg.begin(), pos);
    // + 1 跳过 ':'
    std::string password_hash(pos + 1, msg.end());

    auto res = db->select("password_hash", "id", "name", "number", "create_time")
                 .from("users")
                 .where("number = '{}'", number)
                 .exec();

    if (res.empty()) {
        char error[] = "err000";
        socket.send(std::span{error, sizeof(error) - 1});
    }
    else {
        auto send_msg = std::format("{}:{}:{}:{}", res[1], res[2], res[3], res[4]);
        socket.send(std::span{send_msg.data(), send_msg.size()});
        redis.sadd("online_users", res[1]);
    }
}

void heart(std::span<char> msg, TCP &socket) {
    if (msg.size() != sizeof(int))
        throw std::invalid_argument("invalid heart message");

    int id{};
    std::memcpy(&id, msg.data(), sizeof(id));
    Log().push_log(std::format("Server get {} heart", id));
}
std::map<header::type, std::function<void(std::span<char>, TCP&)>> events {
    { header::type::login, login },
    { header::type::heart, heart }
};

// 分发事件，根据事件不同调用不同的函数
void distribute(std::span<char> msg, TCP &socket) {
    auto type = header::read(msg);
    if (!events.contains(type))
        throw std::invalid_argument("invalid server type");
    events[type](msg.subspan(header::header_size()), socket);
}

export void server_main() {
    Log().push_log("Server start");
    TCP socket(Address {"0.0.0.0", 8080});


    redis.set("hello", "world");

    socket.bind();
    socket.listen();
    auto client = socket.accept();

    char buf[1024];
    while (true) {
        auto msg = client.recv(buf);
        if (msg.empty())
            break;
        distribute(msg, client);
    }
}
