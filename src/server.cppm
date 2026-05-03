/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/

module;
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
export module server;
import log;
import net;
import database;
import std;

export class Server {
public:

};


// 000 000 查询密码是否正确, 密码正确直接表示上线了

void split_message(std::string_view msg) {
    std::string type(msg.begin(), msg.begin() + 6);

    // login
    if (type == "000000") {
        auto pos = msg.find(':');
        if (pos == std::string::npos)
            throw std::invalid_argument("invalid server type");
        std::string number(6, pos);
        // + 1 跳过:
        std::string password_hash(msg.begin() + pos + 1, msg.end());
        std::cout << "number" << std::endl;
        std::cout << password_hash << std::endl;
    }


}
export void server_main() {
    Log().push_log("Server start");
    Database db("root", "arch", "game");
    Socket socket(Address {"0.0.0.0", 8080});

    socket.bind();
    socket.listen();
    auto client = socket.accept();

    Buffer buf;
    while (true) {
        auto n = client.recv(buf.span());
        if (n <= 0)
            break;
        split_message(buf.span().data());
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
