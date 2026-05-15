/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/15 16:21:39
********************************************************************************/

module;
export module room;
import std;

// 房间
enum class RoomStatus {
    free,
    match,
    gaming
};
export class RoomList;
export class Room {
    int room_id{};
    std::vector<int> users_{};
    RoomStatus status{};
    friend class RoomList;
public:
    explicit Room(const int id) : room_id(id) {  }
    void add_user(const int id) {
        if (std::ranges::find(users_, id) == users_.end())
            users_.push_back(id);
    }
    void remove_user(const int id) {
        if (const auto it = std::ranges::find(users_, id); it != users_.end()) {
            std::swap(*it, users_.back());
            users_.pop_back();
        }
    }
    void start_match() {

    }

    void stop_match() {
    }
};


class RoomList {
    // 管理rides
    std::unordered_map<int, Room> rooms_;
    static inline int rooms_id = 1;

    Room &create_room() {
        // @TODO, 生成一个id
        const int id = rooms_id++;
        rooms_[id] = Room(id);
        return rooms_[id];
    }

    std::vector<Room> match() {
        // 根据一系列结果返回匹配队列


        return {};
    }

};