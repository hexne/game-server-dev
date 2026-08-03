/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 18:08:30
********************************************************************************/

module;
#include <meta>
export module user;
import std;
import net;
import time;
import disconnect_users_list;

export enum class UserStatus {
    online,
    offline,
    matching,
    in_room,
    in_battle
};

struct UserProfile {
    std::string password_hash{};
    std::string name{};
    std::string number{};
    int level{};
    int exp{};
    int rank{};
    Time create_time{};
};

export class User {
    int id_{};
    UserStatus status_{};
    UserProfile profile_{};

    std::optional<int> battle_id_{};
    std::optional<int> room_id_{};
    std::optional<int> tcp_fd_{};
    std::unique_ptr<TCP> tcp_{};

    std::string enum_to_string(UserStatus status) const {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^UserStatus))) {
            if ([:e:] == status) {
                return std::string(std::meta::display_string_of(e));
            }
        }
        return std::string("UnKnow Status");
    }

    std::vector<std::string> split(const std::string& str, char delimiter) {
        auto view = str
            | std::views::split(delimiter)
            | std::views::transform([](auto&& range) {
                return std::string(range.begin(), range.end());
            });

        return std::vector<std::string>(view.begin(), view.end());
    }
public:
    User(int tcp_fd, std::unique_ptr<TCP> tcp) : tcp_fd_(tcp_fd), tcp_(std::move(tcp)) {  }
    User(int id, int tcp_fd, std::unique_ptr<TCP> tcp)
        : id_(id), tcp_fd_(tcp_fd), tcp_(std::move(tcp)) {  }

    // "id", "name", "number", "password_hash", "create_time", "level", "exp", "rank"
    explicit User(const std::string &user_string) {
        update_user_info(user_string);
    }
    explicit User(const std::vector<std::string> user_infos) {
        update_user_info(user_infos);
    }
    void update_user_info(const std::string &user_string) {
        const auto vec = split(user_string, '|');
        update_user_info(vec);
    }
    void update_user_info(const std::vector<std::string> &info) {
        id_ = std::stoi(info[0]);
        profile_.name = info[1];
        profile_.number = info[2];
        profile_.password_hash = info[3];
        profile_.create_time = Time(info[4]);
        profile_.level = std::stoi(info[5]);
        profile_.exp = std::stoi(info[6]);
        profile_.rank = std::stoi(info[7]);

    }

    std::string to_string() const {
        return std::format("{}|{}|{}|{}|{}|{}|{}|{}",
            id_, name(), number(), password_hash(), create_time().get_string(), level(), exp(), rank());
    }

    void id(int id) {
        id_ = id;
    }
    int id() const {
        return id_;
    }
    std::string name() const {
        return profile_.name;
    }
    std::string number() const {
        return profile_.number;
    }
    UserStatus status() const {
        return status_;
    }
    std::string status_string() const {
        return enum_to_string(status_);
    }
    void status(const UserStatus status) {
        status_ = status;
    }
    std::string password_hash() const {
        return profile_.password_hash;
    }
    int level() const {
        return profile_.level;
    }
    void level(int level) {
        profile_.level = level;
    }
    int exp() const {
        return profile_.exp;
    }
    void exp(int exp) {
        profile_.exp = exp;
    }
    int rank() const {
        return profile_.rank;
    }
    void rank(int rank) {
        profile_.rank = rank;
    }
    Time create_time() const {
        return profile_.create_time;
    }


    std::unique_ptr<TCP>& tcp() {
        return tcp_;
    }
    void tcp(std::unique_ptr<TCP> tcp) {
        tcp_ = std::move(tcp);
    }
    void tcp_fd(const int fd) {
        tcp_fd_ = fd;
    }
    std::optional<int> tcp_fd() const {
        return tcp_fd_;
    }
    void battle_id(const std::optional<int>& battle_id) {
        battle_id_ = battle_id;
    }
    std::optional<int> battle_id() const {
        return battle_id_;
    }
    void room_id(const std::optional<int>& room_id) {
        room_id_ = room_id;
    }
    std::optional<int> room_id() const {
        return room_id_;
    }
};