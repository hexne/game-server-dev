/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 19:23:11
********************************************************************************/

module;
export module battle_result;
import std;
import user;
import message;

export struct BattleResult {
    int battle_id{};
    bool winner_is_team_a{};
    std::vector<int> team_a_users_id;
    std::vector<int> team_b_users_id;

    char *serialize(char *p) {
        p = message::write(p, battle_id);
        p = message::write(p, winner_is_team_a);

        std::size_t size = team_a_users_id.size();
        p = message::write(p, size);
        for (auto id : team_a_users_id)
            p = message::write(p, id);

        size = team_b_users_id.size();
        p = message::write(p, size);
        for (auto id : team_b_users_id)
            p = message::write(p, id);
        return p;
    }
    char *deserialize(char *p) {
        battle_id = message::read(p);
        winner_is_team_a = message::read(p);

        std::size_t size = message::read(p);
        team_a_users_id.resize(size);
        for (int i = 0;i < size; ++i)
            team_a_users_id[i] = message::read(p);

        size = message::read(p);
        team_b_users_id.resize(size);
        for (int i = 0;i < size; ++i)
            team_b_users_id[i] = message::read(p);
        return p;
    }
};