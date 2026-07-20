/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/15 14:41:27
********************************************************************************/

module;
export module online_user;
import std;
import time;

export class OnlineUserList {
    std::unordered_map<int, Time> online_;
    std::jthread thread_;
    std::function<void(int)> out_online_callback_;  // @TODO，离线事件调用
    std::mutex mutex_;

    // 这个函数只有读取
    void check() {
        while (!thread_.get_stop_token().stop_requested()) {
            Time cur = Time::now();
            std::vector<int> out_online_users;
            {
                std::lock_guard lock(mutex_);
                for (auto &[id, last_time] : online_) {
                    auto end = last_time + Time(std::chrono::seconds{15});
                    if (end < cur) {
                        out_online_users.push_back(id);
                    }
                }
                for (int id : out_online_users) {
                    online_.erase(id);
                }
            }
            for (int id : out_online_users) {
                out_online_callback_(id);
            }
            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
    }
public:
    explicit OnlineUserList(const std::function<void(int)> &callback)
        : out_online_callback_(callback) {
        thread_ = std::jthread(&OnlineUserList::check, this);
    }

    // 这里只写
    void update(const int id,const Time &last_time) {
        std::lock_guard lock(mutex_);
        online_[id] = last_time;
    }

    // 在线玩家数量
    std::size_t size() {
        std::lock_guard lock(mutex_);
        return online_.size();
    }

    ~OnlineUserList() {
        thread_.request_stop();
        thread_.join();
    }
};