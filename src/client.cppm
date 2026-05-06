/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:49:03
********************************************************************************/

module;
#include <arpa/inet.h>
#include <unistd.h>
export module client;
import log;
import std;
import std.compat;
import net;
import user;
import message;
import timer;

export class Client {
public:

};

std::optional<User> check_user_password(const std::string& number, const std::string& password, TCP &socket) {

    auto hash = sha256(password);
    char msg[1024]{};
    header::write(msg, header::type::login);

    auto login_msg = std::format("{}:{}", number, hash);
    auto msg_size = message::write(msg, header::type::login, std::span{login_msg.data(), login_msg.size()});
    socket.send(std::span{msg, msg_size});
    char buf[1024]{};
    auto recv_msg = socket.recv(std::span{buf, sizeof(buf)});
    std::string user_info(recv_msg.begin(), recv_msg.end());
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


void send_heart(int id, TCP &tcp) {
    char buf[16]{};
    auto id_span = std::span{reinterpret_cast<char*>(&id), sizeof(id)};
    auto size = message::write(buf, header::type::heart, id_span);
    tcp.send(std::span{buf, size});
}

Router router;


export void client_main() {
    sleep(1);
    Log().push_log("Client start");

    TCP socket(Address{"127.0.0.1", 8080});
    socket.connect();
    Timer timer;

    //@TODO 用户登录界面填写账号和密码
    auto user = check_user_password("num10", "pass10", socket);
    if (!user) {
        Log().push_log("login error");
    }
    else {
        Log().push_log("login ok");
        auto id = user->id();
        timer.add_repeat_task([&socket, id] {
            send_heart(id, socket);
        }, std::chrono::seconds{5});
    }
    while (true)
        ;

}
