/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 19:21:38
********************************************************************************/

module;
export module battle;
import std;
import team;
import battle_result;

export class Battle {
    Team team_a_, team_b_;
public:
    Battle(Team team_a, Team team_b)
        : team_a_(std::move(team_a)), team_b_(std::move(team_b)) {
    }

    // 先不实现ban英雄功能
    void ban() {  }

    void pick_hero(int user_id, HeroName hero_name) {
        if (team_a_.have_user(user_id))
            team_a_.user_pick_hero(user_id, hero_name);
        else if (team_b_.have_user(user_id))
            team_b_.user_pick_hero(user_id, hero_name);
    }

    void start_battle() {  }

    // 结束战斗，生成战斗结算
    BattleResult finish_battle() {
        return {};
    }
};

