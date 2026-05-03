/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/

module;
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sw/redis++/redis++.h>
export module server;
import log;
import net;
import database;
import std;

Database db("root", "123456", "game");
sw::redis::Redis redis("tcp://127.0.0.1:6379");

export class Server {
public:

};


// 控制类型
// 000 000 查询密码是否正确, 密码正确直接表示上线了

// 错误码
// err 000  失败，无原因
void split_message(std::string_view msg, Socket &socket) {
    std::string type(msg.begin(), msg.begin() + 6);

    // login
    if (type == "000000") {
        Log().push_log("type is login");
        auto pos = msg.find(':');
        if (pos == std::string::npos)
            throw std::invalid_argument("invalid server type");
        std::string number(msg.begin() + 6, msg.begin() + pos);
        // + 1 跳过 ':'
        std::string password_hash(msg.begin() + pos + 1, msg.end());

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


}
export void server_main() {
    Log().push_log("Server start");
    Socket socket(Address {"0.0.0.0", 8080});


    redis.set("hello", "world");

    socket.bind();
    socket.listen();
    auto client = socket.accept();

    Buffer buf;
    while (true) {
        auto n = client.recv(buf.span());
        if (n <= 0)
            break;
        split_message(buf.span().data(), client);
        // std::string msg(buf);
        // std::cout << msg << std::endl;
        // auto res = db->select("id", "name")
        //              .from("users")
        //              .where("number = '{}'", number)
        //              .exec();
        //
        // std::string send_str;
        // for (auto user : res) {
        //     send_str = std::format("{}:{}", user[0], user[1]);
        // }
        //
        // client.send(std::span<char>(send_str.data(), send_str.size()));
    }
}
