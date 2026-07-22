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
    battle,
    closed,

};
export class RoomManager;
class MatchTree;

export class Room {
    int id_{};
    int master_{};
    int match_rank_{};
    bool need_relax_{};
    std::vector<int> users_{};  // @NOTE, users_中不保存master_
    RoomStatus status = RoomStatus::free;
    friend class RoomList;
    friend class RoomManager;
    friend class MatchTree;
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
            status = RoomStatus::closed;
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

class MatchTree {
    Timer match_level_timer_;
    std::map<int, std::vector<std::shared_ptr<Room>>> tree_;
    std::mutex match_tree_mutex_;
    constexpr static int rank_size = 100;
    constexpr static int rank_0 = 100, rank_1 = 200, rank_2 = 300;
public:
    void add_matching_room(std::shared_ptr<Room> &room) {
        room->status = RoomStatus::matching_0;
        std::lock_guard lock(match_tree_mutex_);
        int rank = room->match_rank_;
        int index = rank / rank_size;
        tree_[index].push_back(room);

        match_level_timer_.add_task([room, this] {
            room->need_relax_ = true;
        }, std::chrono::minutes{1});
    }

    void remove_matched_rooms(std::shared_ptr<Room> &room) {
        room->status = RoomStatus::matched;
        int rank = room->match_rank_;

        std::lock_guard lock(match_tree_mutex_);
        int index = rank / rank_size;
        auto vec = tree_[index];
        auto it = std::ranges::find_if(vec, [&room](std::shared_ptr<Room> &cur) {
            return cur->id_ == room->id_;
        });

        if (it == vec.end())
            return;
        vec.erase(it);

        if (vec.empty())
            tree_.erase(index);
    }

    void relax_match_level() {
        std::lock_guard lock(match_tree_mutex_);
        for (auto &[index, vec] : tree_) {
            for (auto &room : vec) {
                if (!room->need_relax_)
                    continue;
                if (room->status == RoomStatus::matching_0)
                    room->status = RoomStatus::matching_1;
                else if (room->status == RoomStatus::matching_1)
                    room->status = RoomStatus::matching_2;

                room->need_relax_ = false;
            }
        }
    }

    std::vector<std::shared_ptr<PendingMatch>> try_match() {
        relax_match_level();
        std::lock_guard lock(match_tree_mutex_);
        std::vector<std::shared_ptr<PendingMatch>> ret;

        std::vector<std::shared_ptr<Room>> all_rooms;
        std::vector<std::shared_ptr<Room>> matched_rooms;
        for (auto &[index, vec] : tree_)
            all_rooms.append_range(vec);

        // 匹配等级高的在前面
        std::ranges::sort(all_rooms, [](const auto &room_a, const auto &room_b) {
            return static_cast<int>(room_a->status) > static_cast<int>(room_b->status);
        });

        for (int i = 0 ; i < all_rooms.size(); ++i) {
            for (int j = i + 1; j < all_rooms.size(); ++j) {
                auto &room_a = all_rooms[i];
                auto &room_b = all_rooms[j];

                int match_level_max = std::max(
                    static_cast<int>(room_a->status),
                    static_cast<int>(room_b->status)
                );

                int rank_distance{};
                if (static_cast<RoomStatus>(match_level_max) == RoomStatus::matching_0)
                    rank_distance = rank_0;
                else if (static_cast<RoomStatus>(match_level_max) == RoomStatus::matching_1)
                    rank_distance = rank_1;
                else if (static_cast<RoomStatus>(match_level_max) == RoomStatus::matching_2)
                    rank_distance = rank_2;

                if (std::abs(room_a->match_rank_ - room_b->match_rank_) > rank_distance)
                    continue;

                matched_rooms.push_back(room_a);
                matched_rooms.push_back(room_b);

                room_a->status = RoomStatus::matched;
                room_b->status = RoomStatus::matched;
                ret.emplace_back(std::make_shared<PendingMatch>(room_a, room_b));
            }
        }

        for (auto &room : matched_rooms)
            remove_matched_rooms(room);

        return ret;
    }
};

class RoomManager {
    // 空闲房间列表
    std::vector<std::shared_ptr<Room>> free_rooms_;
    // 匹配中的房间列表
    MatchTree matching_rooms_;
    // 等待确认的房间
    std::vector<std::shared_ptr<PendingMatch>> pending_matches_;


    std::mutex free_rooms_mutex_;
    std::mutex matching_rooms_mutex_;
    std::mutex pending_matches_mutex_;

    bool try_match(const std::shared_ptr<Room> &room_a, const std::shared_ptr<Room> &room_b, int rank_distance) {
        if (std::abs(room_a->match_rank_ - room_b->match_rank_) < rank_distance)
            return true;
        return false;
    }

public:
    RoomManager() = default;

    void add_free_room(std::shared_ptr<Room> &room) {
        room->status = RoomStatus::free;
        std::lock_guard lock(free_rooms_mutex_);
        free_rooms_.push_back(room);
    }
    void add_matching_room(std::shared_ptr<Room> &room) {
        matching_rooms_.add_matching_room(room);
    }
    void add_pending_room(std::shared_ptr<PendingMatch> &room) {
        auto [room_a, room_b, _] = *room;
        room_a->status = RoomStatus::matched;
        room_b->status = RoomStatus::matched;

        std::lock_guard lock(pending_matches_mutex_);
        pending_matches_.push_back(room);
    }

    // @TODO, 先简易匹配把逻辑整通
    std::vector<std::shared_ptr<PendingMatch>> try_match() {
        std::vector<std::shared_ptr<PendingMatch>> ret;

        return ret;
    }

    void remove_closed_rooms() {
        // @NOTE, 认为只有空闲中的房间才可能是关闭的
        // @TODO, 后续再思考其他可能
        std::lock_guard lock(free_rooms_mutex_);
        std::erase_if(free_rooms_, [](const auto& room) {
            return room->status == RoomStatus::closed;
        });
    }

};