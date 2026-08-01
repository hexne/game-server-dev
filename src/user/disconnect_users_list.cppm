/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/31 15:13:16
********************************************************************************/

module;
export module disconnect_users_list;
import std;



export class DisconnectUsersList {
    // user_id, battle_id, room_id
    std::unordered_map<int, std::tuple<int, int>> users_;
public:
    void add_disconnect_user(int user_id, int battle_id, int room_id) {
        users_.insert(std::make_pair(user_id, std::make_tuple(battle_id, room_id)));
    }
    void remove_disconnect_user(int user_id) {
        if (!users_.contains(user_id))
            return;
        users_.erase(user_id);
    }

    std::optional<std::tuple<int, int>> get_battle_id_and_room_id(int user_id) {
        if (!users_.contains(user_id))
            return std::nullopt;
        return users_.at(user_id);
    }
};