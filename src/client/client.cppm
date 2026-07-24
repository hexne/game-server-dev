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
import hash256;
import hero;

struct RoomInfo {
    int id;
    int master;
};

export class Client {
    std::optional<User> user_;
    int battle_id_{};
    std::optional<RoomInfo> room_;
    TCP tcp_;
    Timer timer_;
    std::map<header::type, void (Client::*)(std::span<char>)> rounter_ = {
        { header::type::login_true, &Client::login_true },
        { header::type::login_false, &Client::login_false },
        { header::type::room_create_true, &Client::room_create_true },
        { header::type::room_invite_message, &Client::room_invite_message },
        { header::type::room_join, &Client::room_join },
        { header::type::room_leave, &Client::room_leave },
        { header::type::room_chat, &Client::room_chat },
        { header::type::match_success, &Client::match_success },
        { header::type::match_cancel, &Client::match_cancel },
        { header::type::battle_pick_hero, &Client::battle_pick_hero },
        { header::type::battle_start_load, &Client::battle_start_load },
        { header::type::battle_start, &Client::battle_start },
    };

    void login_false(std::span<char> msg) {
        Log().push_log("login error");
    }

    // id:name:number
    void login_true(std::span<char> msg) {
        std::println("login true");
        std::string user_info(msg.begin(), msg.end());
        user_ = User(user_info);

        int user_id = user_->id();
        timer_.add_repeat_task([this, user_id] {
            send_heart(user_id);
        }, std::chrono::seconds{5});

    }

    void room_create_true(std::span<char> msg) {
        auto room_id = message::read(msg.data());
        auto master_id = message::read(msg.data() + sizeof(int));
        room_ = RoomInfo{.id = room_id, .master = master_id};
    }

    // 收到房间邀请信息
    void room_invite_message(std::span<char> msg) {
        int user1 = message::read(msg.data());
        int user2 = message::read(msg.data() + sizeof(int));
        int room_id = message::read(msg.data() + sizeof(int) * 2);

        if (user2 != user_id()) {
            throw std::runtime_error(std::format("{} != {}", user2, user_id()));
        }

        // 收到一个邀请信息
        // room_invite_accept(room_id);
        room_invite_reject(user2, user1);
    }
    void room_invite_accept(int room_id) {
        char buf[1024]{};
        auto size = message::write(buf, header::type::room_invite_accept, user_id(), room_id);
        tcp_.send_now(std::span{buf, size});
    }
    // 发送拒绝邀请
    void room_invite_reject(int user1, int user2) {
        char buf[1024]{};
        auto size = message::write(buf, header::type::room_invite_reject, user_id(), user1, user2);
        tcp_.send_now(std::span{buf, size});
    }

    // 用户拒绝了你的房间邀请信息
    void room_invite_reject(std::span<char> msg) {
        int id = message::read(msg);
        std::println("{} rejected you", id);
    }

    // 有用户加入房间
    void room_join(std::span<char> msg) {
        int user = message::read(msg.data());
        int room_id = message::read(msg.data() + sizeof(int));
        int room_master = message::read(msg.data() + sizeof(int) * 2);

        // 进入房间的是当前用户， 更新房间号
        if (user == user_id()) {
            room_ = RoomInfo{.id = room_id, .master = room_master};
        }
        // 否则查询用户信息并显示
    }

    // 有用户离开房间
    void room_leave(std::span<char> msg) {
        auto id = message::read(msg.data());

        if (id == user_id())
            room_ = std::nullopt;// 换主页ui
        else
            ;// 换房间内展示
    }

    void room_chat(std::span<char> msg) {
        auto user_id = message::read(msg.data());
        auto room_id = message::read(msg.data() + sizeof(int));
        auto message = msg.subspan(sizeof(int) * 2);

        if (user_id == this->user_id())
            return;
        std::println("{} : {}", user_id, message);
    }

    // 收到匹配成功信息
    void match_success(std::span<char> msg) {
        int id = message::read(msg.data());
        match_accept(id);
    }
    // 接受对局
    void match_accept(int id) {
        char buf[512]{};
        auto size = message::write(buf, header::type::match_accept, user_id(), id);
        tcp_.send_now(std::span{buf, size});
    }
    // 拒绝对局
    void match_reject(int id) {
        char buf[512]{};
        auto size = message::write(buf, header::type::match_reject, user_id(), id);
        tcp_.send_now(std::span{buf, size});
    }

    // 对局取消
    void match_cancel(std::span<char> msg) {
        ; // 取消对局，修改客户端UI
    }

    void battle_pick_hero(std::span<char> msg) {
        // UI修改，进入英雄选择界面
        battle_id_ = message::read(msg.data());
    }

    void battle_start_load(std::span<char> msg) {
        battle_load();
    }

    void battle_start(std::span<char> msg) {
        // 开始战斗
        // 显示地图，角色，界面等战斗信息
    }
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

    int user_id() {
        if (!user_)
            return -1;
        return user_->id();
    }
    std::string user_name() {
        if (!user_)
            return "";
        return user_->name();
    }
    std::string user_number() {
        if (!user_)
            return "";
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
        return std::to_string(room_->id);
    }

    // 创建房间 发送一个room_create user_id,
    // 让服务器创建房间，把user_id加进去，然后返回 room_create_true room_id即可
    // 或者room_create_false即可
    void room_create() {
        if (user_ == std::nullopt)
            return;
        char msg[1024]{};

        auto size = message::write(msg, header::type::room_create, user_id());
        tcp_.send_now(std::span{msg, size});
    }

    // room_invite current_user_id user_id current_room_id
    void room_invite(int user) {
        char msg[1024]{};
        auto size = message::write(msg, header::type::room_invite, user_id(), user, room_->id);
        tcp_.send_now(std::span{msg, size});
    }

    // 主动触发离开房间
    void room_leave() {
        char buf[512]{};
        auto size = message::write(buf, header::type::room_leave, user_id(), room_->id);
        tcp_.send_now(std::span{buf, size});
    }

    // 主动在房间中发送消息
    void room_message(std::string msg) {
        char buf[512]{};
        if (!user_ || !room_)
            return;
        auto size = message::write(buf, header::type::room_chat, user_id(), room_->id, std::span(msg));
        tcp_.send_now(std::span{buf, size});
    }

    // 开始匹配
    void match_join() {
        if (room_ == std::nullopt)
            return;

        if (user_id() != room_->master)
            return;

        char buf[512]{};
        auto size = message::write(buf, header::type::match_join, room_->id);
        tcp_.send_now(std::span{buf, size});
    }


    void battle_load() {
        auto send_load_msg = [&] (int val) {
            char buf[512]{};
            auto size = message::write(buf, header::type::battle_load, battle_id_, user_id(), val);
            tcp_.send_now(std::span{buf, size});
        };

        // 加载地图...
        send_load_msg(50);

        // 加载模型...
        send_load_msg(100);
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

    // 手动发送选择的英雄
    // battle_id, user_id, hero_name
    void battle_pick_hero(HeroName hero_name) {
        char buf[512]{};
        auto size = message::write(buf, header::type::battle_pick_hero, battle_id_, user_id(), static_cast<int>(hero_name));
        tcp_.send_now(std::span{buf, size});
    }
};


export void client_main() {
    sleep(1);
    Log().push_log("Client start");
    Client client(Address{"127.0.0.1", 8080});
    Log().push_log("Client init");
    client.login("num10", "pass10");

}