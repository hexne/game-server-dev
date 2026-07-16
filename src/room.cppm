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
    matching,
    battle
};

// 一个房间号
// 一个房主
// 一个其他用户列表
//
export class Room {
    int id_{};
    int master_{};
    std::vector<int> users_{};
    RoomStatus status = RoomStatus::free;
    friend class RoomList;
public:
    static Room room_create(const int master) {
        static int id{};
        Room room(id ++, master);
        return room;
    }

    explicit Room(const int id, const int master) : id_(id), master_(master) {  }
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
    int id() const {
        return id_;
    }
    std::vector<int> users() {
        return users_;
    }
    void start_match() {

    }

    void stop_match() {

    }

    bool operator < (const Room &that) const {
        return id_ < that.id_;
    }
};
//
//
// class RoomList {
//     // 管理rides
//     std::unordered_map<int, Room> rooms_;
//     static inline int rooms_id = 1;
//
//     Room &room_create() {
//         // @TODO, 生成一个id
//         const int id = rooms_id++;
//         rooms_[id] = Room(id);
//         return rooms_[id];
//     }
//
//     std::vector<Room> match() {
//         // 根据一系列结果返回匹配队列
//
//
//         return {};
//     }
//
// };