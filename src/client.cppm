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

std::optional<User> current_user;
Timer timer;

void send_heart(int id, TCP &tcp) {
    char buf[16]{};
    auto id_span = std::span{reinterpret_cast<char*>(&id), sizeof(id)};
    auto size = message::write(buf, header::type::heart, id_span);
    tcp.send(std::span{buf, size});
}

void login_true(std::span<char> msg, TCP& socket) {
    std::string user_info(msg.begin(), msg.end());
    std::array<std::string, 4> infos{};
    size_t start = 0;
    size_t idx = 0;

    while (idx < 3) {
        size_t pos = user_info.find(':', start);
        infos[idx++] = user_info.substr(start, pos - start);
        start = pos + 1;
    }

    infos[3] = user_info.substr(start);
    current_user = User(infos[0], infos[1], infos[2], infos[3]);
    Log().push_log("login ok");

    auto id = current_user->id();
    timer.add_repeat_task([&socket, id] {
        send_heart(id, socket);
    }, std::chrono::seconds{5});
}

void login_false(std::span<char>, TCP&) {
    Log().push_log("login error");
}

void check_user_password(const std::string& number, const std::string& password, TCP &socket) {

    auto hash = sha256(password);
    char msg[1024]{};
    header::write(msg, header::type::login);

    auto login_msg = std::format("{}:{}", number, hash);
    auto msg_size = message::write(msg, header::type::login, std::span{login_msg.data(), login_msg.size()});
    socket.send(std::span{msg, msg_size});
}

Router router = {
    { header::type::login_true, login_true },
    { header::type::login_err, login_false }
};


export void client_main() {
    sleep(1);
    Log().push_log("Client start");

    TCP socket(Address{"127.0.0.1", 8080});
    socket.connect();


    //@TODO 用户登录界面填写账号和密码
    check_user_password("num10", "pass10", socket);

    char buf[1024]{};
    while (true) {
        auto msg = socket.recv(std::span{buf, sizeof(buf)});
        if (msg.empty()) {
            break;
        }

        auto type = header::read(msg);
        if (!router.contains(type))
            throw std::invalid_argument("invalid client type");
        router[type](msg.subspan(header::header_size()), socket);
    }

}
