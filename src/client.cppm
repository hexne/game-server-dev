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
import net;
import user;
import message;
import timer;

export class Client {
    std::optional<User> user_;
    TCP tcp_;
    Timer timer_;

    using Handler = void (Client::*)(std::span<char>);
    std::map<header::type, Handler> router_;
public:
    explicit Client(Address address = Address{"127.0.0.1", 8080}) : tcp_(std::move(address)) {
        router_ = {
            { header::type::login_true, &Client::login_true },
            { header::type::login_err, &Client::login_false }
        };
        tcp_.connect();
    }

    ~Client() = default;

    void login(std::string_view number, std::string_view password) {
        auto hash = sha256(password);
        char msg[1024]{};
        auto login_msg = std::format("{}:{}", number, hash);
        auto msg_size = message::write(msg, header::type::login, std::span{login_msg.data(), login_msg.size()});
        tcp_.send(std::span{msg, msg_size});
    }

    void register_user(std::string_view name, std::string_view number, std::string_view password) {
        auto password_hash = sha256(password);
        char msg[1024]{};
        auto register_msg = std::format("{}:{}:{}", name, number, password_hash);
        auto msg_size = message::write(msg, header::type::register_user, std::span{register_msg.data(), register_msg.size()});
        tcp_.send(std::span{msg, msg_size});
    }

    void login_true(std::span<char> msg) {
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
        user_ = User(infos[0], infos[1], infos[2], infos[3]);
        Log().push_log("login ok");

        auto id = user_->id();
        timer_.add_repeat_task([this, id] {
            send_heart(id);
        }, std::chrono::seconds{5});
    }

    void login_false(std::span<char>) {
        Log().push_log("login error");
    }
    void send_heart(int id) {
        char buf[16]{};
        auto id_span = std::span{reinterpret_cast<char*>(&id), sizeof(id)};
        auto size = message::write(buf, header::type::heart, id_span);
        tcp_.send(std::span{buf, size});
    }

    void route() {
        char buf[1024]{};
        while (true) {
            auto msg = tcp_.recv(std::span{buf, sizeof(buf)});
            if (msg.empty()) {
                break;
            }

            auto type = header::read(msg);
            if (!router_.contains(type))
                throw std::invalid_argument("invalid client type");
            (this->*router_[type])(msg.subspan(header::header_size()));
        }
        
    }


};


export void client_main() {
    sleep(1);
    Log().push_log("Client start");
    Client client;

    client.login("num10", "pass10");
    client.route();

}
