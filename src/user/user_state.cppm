/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/16 11:04:35
********************************************************************************/

module;
export module user_state;
import std;
import net;
import time;


export struct UserState {
    int id = -1;
    std::unique_ptr<TCP> tcp{};
    Time last_time{};
    bool is_online{};

    int fd{};
    int room{};
};

export class UserStateManager {
public:
    std::unordered_map<int, std::shared_ptr<UserState>> fd_map;   // fd → UserState
    std::unordered_map<int, std::shared_ptr<UserState>> id_map;   // user_id → UserState

    std::shared_ptr<UserState> search_user_state_by_fd(int fd) {
        if (auto it = fd_map.find(fd); it != fd_map.end())
            return it->second;
        return nullptr;
    }

    std::shared_ptr<UserState> search_user_state_by_user_id(int id) {
        if (auto it = id_map.find(id); it != id_map.end())
            return it->second;
        return nullptr;
    }

    void add_fd(int fd, std::unique_ptr<TCP> tcp) {
        auto state = std::make_shared<UserState>();
        state->fd = fd;
        state->tcp = std::move(tcp);
        state->is_online = true;
        fd_map[fd] = state;
    }

    void bind_user(int user_id, int fd) {
        auto state = search_user_state_by_fd(fd);
        if (!state) return;
        state->id = user_id;
        id_map[user_id] = state;
    }

    void remove_fd(int fd) {
        auto state = search_user_state_by_fd(fd);
        if (!state) return;

        if (state->id != -1)
            id_map.erase(state->id);

        fd_map.erase(fd);
    }
};
