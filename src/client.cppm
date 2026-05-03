/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:49:03
********************************************************************************/

module;
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
export module client;
import log;
import std;
import std.compat;
import net;
import user;

export class Client {
public:

};

std::optional<User>check_user_password(std::string_view number, std::string_view password) {

    auto hash = sha256(password);
    auto msg = std::format("000000{}:{}", number, hash);
    Socket socket(Address{"127.0.0.1", 8080});
    socket.connect();
    socket.send(std::span{msg.data(), msg.size()});

    char buf[1024]{};
    int n = socket.recv(std::span{buf, sizeof(buf)});
    std::string user_info(buf, n);
    if (user_info == "err000") {
        Log().push_log("login error");
        return std::nullopt;
    }
    std::array<std::string, 4> infos{};
    size_t start = 0;
    size_t idx = 0;

    while (idx < 3) {
        size_t pos = user_info.find(':', start);
        infos[idx++] = user_info.substr(start, pos - start);
        start = pos + 1;
    }

    infos[3] = user_info.substr(start);
    User user(infos[0], infos[1], infos[2], infos[3]);
    return user;
}

export void client_main() {
    sleep(1);
    Log().push_log("Client start");

    //@TODO 用户登录界面填写账号和密码
    auto user = check_user_password("num10", "pass10");
    if (!user) {
        Log().push_log("login error");
    }
    else {
        Log().push_log("login ok");
        user.value();
    }

    // Address addr("127.0.0.1", 8080);
    //
    // Socket socket(addr);
    //
    // socket.connect();
    //
    // socket.send("num10");
    //
    // Buffer buf;
    // socket.recv(buf.span());
    // if (buf.span().size() > 0) {
    //     std::string ret_info = buf;
    //     std::cout << ret_info << std::endl;
    // }

}