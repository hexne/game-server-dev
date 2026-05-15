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
export class Room {
    int room_id{};
    std::vector<int> users_{};
    RoomStatus status{};
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


export class RoomList {
    // 管理rides
    std::unordered_map<int, Room> rooms_;

    Room &create_room() {
        // @TODO, 生成一个id
        int id{};
        rooms_[id] = Room(id);
        return rooms_[id];
    }

    std::vector<Room> match() {
        // 根据一系列结果返回匹配队列

        // 被返回的队列状态从match -> gaming

    }

    




};

export std::vector<int> match(Room &room) {
    // 匹配队列
    // 1 ->
        // room_id, ....
    // 2 ->
        // room_id, ....
    // 3 ->
        // room_id, ....
    // 4 ->
        // room_id, ....
    // 5 ->
        // room_id, ....
    // 评估当前room中用户的平均得分
    // 将当前room中的用户全部加入match 队列



}