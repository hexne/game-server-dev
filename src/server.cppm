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
        char buf[header::header_size()]{};
        auto size = message::write(buf, header::type::login_err, {});
        socket.send(std::span{buf, size});
    }
    else {
        auto send_msg = std::format("{}:{}:{}:{}", res[1], res[2], res[3], res[4]);
        char buf[1024]{};
        auto size = message::write(buf, header::type::login_true, std::span{send_msg.data(), send_msg.size()});
        socket.send(std::span{buf, size});
        auto key = std::format("online:user:{}", res[1]);
        redis.set(key, "1", std::chrono::seconds{15});
    }
}



void heart(std::span<char> msg, TCP &socket) {
    if (msg.size() != sizeof(int))
        throw std::invalid_argument("invalid heart message");

    int id{};
    std::memcpy(&id, msg.data(), sizeof(id));
    auto key = std::format("online:user:{}", id);
    redis.set(key, "1", std::chrono::seconds{15});
    Log().push_log(std::format("Server get {} heart", id));
}
Router events {
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

        std::vector<std::string> keys;
        sw::redis::Cursor cursor = 0;
        do {
            cursor = redis.scan(cursor, "online:user:*", 10, std::back_inserter(keys));
        } while (cursor != 0);

        std::string online_users;
        for (const auto& key : keys) {
            if (!online_users.empty())
                online_users += ", ";
            online_users += key.substr(std::string{"online:user:"}.size());
        }
        Log().push_log(std::format("Online users: [{}]", online_users));
    }
}
