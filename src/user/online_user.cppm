/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/15 14:41:27
********************************************************************************/

module;
export module online_user_list;
import std;
import time;

export class OnlineUserList {
    std::unordered_map<int, Time> online_list_;
    Time timeout_{std::chrono::seconds{15}};
public:
    OnlineUserList() = default;

    std::vector<int> check_timeout_user() const {
        std::vector<int> ret;
        auto now = Time::now();
        for (auto &[id, last_time] : online_list_) {
            if (now - last_time < timeout_)
                continue;;
            ret.push_back(id);
        }
        return ret;
    }

    void remove_user(const int user) {
        online_list_.erase(user);
    }

    void update(int user_id) {
        online_list_[user_id] = Time::now();
    }
};