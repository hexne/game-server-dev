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
    std::optional<int> room_;
    TCP tcp_;
    Timer timer_;
    std::map<header::type, void (Client::*)(std::span<char>)> rounter_ = {
        { header::type::login_true, &Client::login_true },
        { header::type::login_false, &Client::login_false },
        { header::type::create_room_true, &Client::create_room_true },

    };

    void login_false(std::span<char> msg) {
        Log().push_log("login error");
    }
    void login_true(std::span<char> msg) {
        std::string user_info(msg.begin(), msg.end());
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
        user_->status(UserStatus::online);

        auto id = user_->id();
        timer_.add_repeat_task([this, id] {
            send_heart(id);
        }, std::chrono::seconds{5});

    }

    // 联网的只需要
    void create_room_true(std::span<char> msg) {
        room_ = std::stoi(std::string(msg.data(), msg.size()));
    }
    // user_to_room_true
    // user_to_room_false


public:
    explicit Client(const Address &address) : tcp_(std::move(address)) {
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

    void login(std::string_view number, std::string_view password) {
        auto hash = sha256(password);
        char msg[1024]{};
        auto login_msg = std::format("{}:{}", number, hash);
        auto msg_size = message::write(msg, header::type::login, std::span{login_msg.data(), login_msg.size()});
        tcp_.send_now(std::span{msg, msg_size});
    }

    // void register_user(std::string_view name, std::string_view number, std::string_view password) {
    //     auto password_hash = sha256(password);
    //     char msg[1024]{};
    //     auto register_msg = std::format("{}:{}:{}", name, number, password_hash);
    //     auto msg_size = message::write(msg, header::type::register_user, std::span{register_msg.data(), register_msg.size()});
    //     tcp_.send_message(std::span{msg, msg_size});
    // }

    void send_heart(int id) {
        char buf[16]{};
        auto id_span = std::span{reinterpret_cast<char*>(&id), sizeof(id)};
        auto size = message::write(buf, header::type::heart, id_span);
        tcp_.send_now(std::span{buf, size});
    }

    auto user_id() {
        if (user_ == std::nullopt)
            return -1;
        return user_->id();
    }

    auto user_name() {
        if (user_ == std::nullopt)
            return std::string{};
        return user_->name();
    }
    auto user_number() {
        if (user_ == std::nullopt)
            return std::string{};
        return user_->number();
    }
    auto user_status() {
        if (user_ == std::nullopt)
            return std::string{};
        return user_->status();
    }
    auto room_id() {
        if (room_ == std::nullopt)
            return std::string{};
        return std::to_string(room_.value());
    }

    // 创建房间 发送一个room_create user_id,
    // 让服务器创建房间，把user_id加进去，然后返回 room_create_true room_id即可
    // 或者room_create_false即可
    void create_room() {
        if (user_ == std::nullopt)
            return;
        char msg[1024]{};

        auto size = message::write(msg, header::type::create_room, user_id());
        tcp_.send_now(std::span{msg, size});
    }

    auto rounter(std::span<char> msg) {
        auto header = message::read_header(msg);
        auto context = msg.subspan(header::header_size);

        if (!rounter_.contains(header))
            return;

        (this->*rounter_[header])(context);
    }

    auto &tcp() {
        return tcp_;
    }
};


export void client_main() {
    sleep(1);
    Log().push_log("Client start");
    Client client(Address{"127.0.0.1", 8080});
    Log().push_log("Client init");
    client.login("num10", "pass10");

}