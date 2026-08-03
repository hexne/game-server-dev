/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 19:23:11
********************************************************************************/

module;
export module battle_result;
import std;
import user;

export struct BattleResult {
    int battle_id{};
    bool winner_is_team_a{};
    std::vector<int> team_a_users_id;
    std::vector<int> team_b_users_id;
};