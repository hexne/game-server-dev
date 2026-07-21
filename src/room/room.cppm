/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/15 16:21:39
********************************************************************************/

module;
export module room;
import std;

import id_generator;
import timer;

// 房间
enum class RoomStatus {
    free,
    matching_0, // @NOTE, 匹配等级从0 - 2逐渐宽松, 默认为0
    matching_1, // 匹配等级1
    matching_2, // 匹配等级2
    matched,
    battle
};
export class RoomManager;
export class Room {
    int id_{};
    int master_{};
    int match_rank_{};
    std::vector<int> users_{};  // @NOTE, users_中不保存master_
    RoomStatus status = RoomStatus::free;
    friend class RoomList;
    friend class RoomManager;
public:
    // @TODO, 房间创建、邀请、移除人员都应该修改rank分
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
        status = RoomStatus::matching_0;
    }

    void stop_match() {
        status = RoomStatus::free;
    }

    bool operator < (const Room &that) const {
        return id_ < that.id_;
    }
};


struct PendingMatch {
    std::shared_ptr<Room> room_a, room_b;
    std::set<int> confirmed{};
};

class RoomManager {
    // 空闲房间列表
    std::vector<std::shared_ptr<Room>> free_rooms_;
    // 匹配中的房间列表
    std::vector<std::shared_ptr<Room>> matching_rooms_;
    // 等待确认的房间
    std::vector<std::shared_ptr<PendingMatch>> pending_matches_;

    Timer match_level_timer_;

    std::mutex free_rooms_mutex_;
    std::mutex matching_rooms_mutex_;
    std::mutex pending_matches_mutex_;

    constexpr static int rank_0 = 100, rank_1 = 200, rank_2 = 300;
    bool try_match(const std::shared_ptr<Room> &room_a, const std::shared_ptr<Room> &room_b, int rank_distance) {
        if (std::abs(room_a->match_rank_ - room_b->match_rank_) < rank_distance)
            return true;
        return false;
    }

public:
    RoomManager() = default;

    std::shared_ptr<Room> search_room_by_id(const int id) const {

        auto find_lam = [](const std::vector<std::shared_ptr<Room>> &rooms, int id) -> std::shared_ptr<Room> {
            auto it = std::ranges::find_if(rooms, [id](const std::shared_ptr<Room> &room) {
                return room->id() == id;
            });
            if (it == rooms.end())
                return nullptr;
            return *it;
        };

        std::shared_ptr<Room> ret = find_lam(free_rooms_, id);
        if (ret)
            return ret;
        ret = find_lam(matching_rooms_, id);
        if (ret)
            return ret;

        for (const auto& pending : pending_matches_) {
            const auto &[room_a, room_b, _] = *pending;
            if (room_a->id() == id)
                return room_a;
            if (room_b->id() == id)
                return room_b;
        }

        return nullptr;
    }

    void add_free_room(const std::shared_ptr<Room> &room) {
        std::lock_guard lock(free_rooms_mutex_);
        free_rooms_.push_back(room);
    }
    void add_matching_room(const std::shared_ptr<Room> &room) {
        // @FIXME, 应该从pending或free中操作room
        std::lock_guard lock(matching_rooms_mutex_);
        matching_rooms_.push_back(room);

        auto lam = [room, this] -> bool {
            if (!room)
                return false;


            if (room->status == RoomStatus::matching_0) {
                room->status = RoomStatus::matching_1;
                return true;
            }
            if (room->status == RoomStatus::matching_1) {
                room->status = RoomStatus::matching_2;
                return false;
            }
            return false;
        };

        match_level_timer_.add_task([room, lam, this] {
            if (lam())
                match_level_timer_.add_task(lam, std::chrono::minutes{1});
        }, std::chrono::minutes{1});
    }
    void add_pending_room(const std::shared_ptr<PendingMatch> &room) {
        std::lock_guard lock(pending_matches_mutex_);
        pending_matches_.push_back(room);
    }

    // @TODO, 先简易匹配把逻辑整通
    std::vector<std::shared_ptr<PendingMatch>> try_match() {
        std::lock_guard lock(matching_rooms_mutex_);
        std::vector<std::shared_ptr<PendingMatch>> ret;

        for (int i = 0;i < matching_rooms_.size();i++) {
            for (int j = 0;j < matching_rooms_.size();j++) {
                if (i == j)
                    continue;
                auto room_a = matching_rooms_[i];
                auto room_b = matching_rooms_[j];
                int rank_distance{};
                if (room_a->status == RoomStatus::matching_2
                    || room_b->status == RoomStatus::matching_2)
                    rank_distance = rank_2;
                else if (room_a->status == RoomStatus::matching_1
                    || room_b->status == RoomStatus::matching_1)
                    rank_distance = rank_1;
                else
                    rank_distance = rank_0;

                bool res = try_match(room_a, room_b, rank_distance);
                if (!res)
                    continue;

                // @TODO
                room_a->status = RoomStatus::matched;
                room_b->status = RoomStatus::matched;
                ret.emplace_back(std::make_shared<PendingMatch>(room_a, room_b));
            }
        }
        return ret;
    }

};