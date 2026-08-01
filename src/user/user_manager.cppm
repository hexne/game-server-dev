/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 00:40:16
********************************************************************************/

module;
export module user_manager;

import net;
import timer;
import std;
import user;
import online_user_list;
import disconnect_users_list;

export class UserManager {
    Timer timer_;
    OnlineUserList online_user_list_;
    std::vector<std::shared_ptr<User>> users_;
    DisconnectUsersList disconnect_users_list_;

    std::mutex online_user_list_mutex_;
    std::mutex users_mutex_;

    // fd, user
    // user, fd
public:
    UserManager() {
        timer_.add_repeat_task([this] {
            std::lock_guard lock(online_user_list_mutex_);
            auto timeout_users = online_user_list_.check_timeout_user();

            for (const auto user : timeout_users) {
                online_user_list_.remove_user(user);
                user_offline(user);
            }

        }, std::chrono::seconds{5});
    }

    void user_online(std::shared_ptr<User> user) {
        std::lock_guard lock(users_mutex_);
        users_.emplace_back(user);
    }
    void add_fd(int fd, std::unique_ptr<TCP> tcp) {
        std::lock_guard lock(users_mutex_);
        users_.emplace_back(std::make_shared<User>(fd, std::move(tcp)));
    }
    void user_offline(const int user_id) {
        auto user = search_user_by_id(user_id);
        // 当前用户在对局中
        if (user->status() == UserStatus::in_battle) {
            disconnect_users_list_.add_disconnect_user(user_id, user->battle_id().value(), user->room_id().value());
        }
        {
            std::lock_guard lock(online_user_list_mutex_);
            online_user_list_.remove_user(user_id);
        }
        {
            std::lock_guard lock(users_mutex_);
            std::erase_if(users_, [user_id](const std::shared_ptr<User>& cur_user) {
                return user_id == cur_user->id();
            });
        }
    }
    void user_reconnect(const int user_id) {
        disconnect_users_list_.remove_disconnect_user(user_id);
    }
    TCP* search_user_tcp(const int user_id) {
        auto user = search_user_by_id(user_id);
        if (!user)
            return nullptr;
        if (!user->tcp())
            return nullptr;
        return user->tcp().get();
    }

    // 更新心跳
    void heart_update(const int user_id) {
        std::lock_guard lock(online_user_list_mutex_);
        online_user_list_.update(user_id);
    }

    std::shared_ptr<User> search_user_by_id(const int user_id) {
        std::lock_guard lock(users_mutex_);
        for (auto user : users_) {
            if (user->id() != user_id)
                continue;
            return user;
        }
        return nullptr;
    }

    std::shared_ptr<User> search_user_by_fd(const int fd) {
        std::lock_guard lock(users_mutex_);
        for (auto user : users_) {
            if (!user->tcp_fd())
                continue;
            if (user->tcp_fd() != fd)
                continue;
            return user;
        }
        return nullptr;
    }

    void remove_user_by_fd(const int fd) {
        std::lock_guard lock(users_mutex_);
        std::erase_if(users_, [fd](const std::shared_ptr<User>& user) {
            return user->tcp_fd() && user->tcp_fd() == fd;
        });
    }

    std::optional<std::tuple<int, int>> reconnect_info(const int user_id) {
        return disconnect_users_list_.get_battle_id_and_room_id(user_id);
    }
};
