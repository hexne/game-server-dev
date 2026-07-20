/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/15 16:21:39
********************************************************************************/

module;
export module room;
import std;

import id_generator;

// 房间
enum class RoomStatus {
    free,
    matching,
    matched,
    battle
};

export class Room {
    int id_{};
    int master_{};
    std::vector<int> users_{};  // @NOTE, users_中不保存master_
    RoomStatus status = RoomStatus::free;
    friend class RoomList;
public:
    static Room room_create(const int master) {
        static RoomIDGenerator id_generator;
        Room room(id_generator.next(), master);
        return room;
    }

    explicit Room(const int id, const int master) : id_(id), master_(master) {  }
    void add_user(const int id) {
        if (id == master_)
            return;
        if (std::ranges::find(users_, id) == users_.end())
            users_.push_back(id);
    }
    void remove_user(const int id) {
        if (id == master_ && !users_.empty()) {
            master_ = users_.back();
            users_.pop_back();
        }
        else if (id == master_ && users_.empty()) {
            // TODO, 解散房间
        }
        else {
            if (const auto it = std::ranges::find(users_, id); it != users_.end()) {
                std::swap(*it, users_.back());
                users_.pop_back();
            }
        }



    }
    int id() const {
        return id_;
    }
    int master() const {
        return master_;
    }
    std::vector<int> users() const {
        auto ret = users_;
        ret.push_back(master_);
        return ret;
    }
    void start_match() {
        status = RoomStatus::matching;
    }

    void stop_match() {
        status = RoomStatus::free;
    }

    bool operator < (const Room &that) const {
        return id_ < that.id_;
    }
};