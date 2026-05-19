/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:49:03
********************************************************************************/

module;
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
export module client;
import log;
import std;
import net;
import user;
import message;
import timer;

export class Client {
    std::optional<User> user_;
    TCP tcp_;
    Timer timer_;

public:
    explicit Client(Address address = Address{"127.0.0.1", 8080}) : tcp_(std::move(address)) {
        auto res = tcp_.connect();
        Log().push_log(std::format("connect res : {}", res));
        if (res == -1 && errno != EINPROGRESS) {
            Log().push_log(std::format("connect error : {}", strerror(errno)));
        }
    }

    ~Client() = default;

    auto fd() {
        return tcp_.fd();
    }
    std::optional<User> login(std::string_view number, std::string_view password) {
        auto hash = sha256(password);
        char msg[1024]{};
        auto login_msg = std::format("{}:{}", number, hash);
        auto msg_size = message::write(msg, header::type::login, std::span{login_msg.data(), login_msg.size()});
        tcp_.send_message(std::span{msg, msg_size});
        tcp_.writable();

        char buf[1024]{};
        tcp_.readable();
        auto recv_msg = tcp_.get_message();
        if (recv_msg == std::nullopt || recv_msg->empty())
            return std::nullopt;

        auto span = std::span<char>{recv_msg->data(), recv_msg->size()};
        auto type = header::read(span);
        auto payload = span.subspan(header::header_size());
        if (type == header::type::login_err) {
            Log().push_log("login error");
            return std::nullopt;
        }
        if (type != header::type::login_true)
            throw std::invalid_argument("invalid login response type");

        std::string user_info(payload.begin(), payload.end());
        std::array<std::string, 4> infos{};
        size_t start = 0;
        size_t idx = 0;

        while (idx < 3) {
            size_t pos = user_info.find(':', start);
            if (pos == std::string::npos)
                throw std::invalid_argument("invalid user info");
            infos[idx++] = user_info.substr(start, pos - start);
            start = pos + 1;
        }

        infos[3] = user_info.substr(start);
        user_ = User(infos[0], infos[1], infos[2], infos[3]);
        Log().push_log("login ok");

        auto id = user_->id();
        timer_.add_repeat_task([this, id] {
            send_heart(id);
        }, std::chrono::seconds{5});

        return user_;
    }

    void register_user(std::string_view name, std::string_view number, std::string_view password) {
        auto password_hash = sha256(password);
        char msg[1024]{};
        auto register_msg = std::format("{}:{}:{}", name, number, password_hash);
        auto msg_size = message::write(msg, header::type::register_user, std::span{register_msg.data(), register_msg.size()});
        tcp_.send_message(std::span{msg, msg_size});
    }

    void send_heart(int id) {
        char buf[16]{};
        auto id_span = std::span{reinterpret_cast<char*>(&id), sizeof(id)};
        auto size = message::write(buf, header::type::heart, id_span);
        tcp_.send_message(std::span{buf, size});
    }

};


export void client_main() {
    sleep(1);
    Log().push_log("Client start");
    Client client;
    Log().push_log("Client init");
    client.login("num10", "pass10");

}
