/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 19:21:38
********************************************************************************/

module;
export module battle;
import std;
import team;
import battle_result;
import hero;

export class Battle {
    Team team_a_, team_b_;
    int random_seed_{};
    std::default_random_engine random_engine_;
public:
    Battle(int random_seed, Team team_a, Team team_b)
        : random_seed_(random_seed), team_a_(std::move(team_a)), team_b_(std::move(team_b)), random_engine_(random_seed_) {  }

    char* serialize(char* buf) {
        buf = team_a_.serialize(buf);
        return team_b_.serialize(buf);
    }
    char* deserialize(char *buf) {
        buf = team_a_.deserialize(buf);
        return team_b_.deserialize(buf);
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

    // 用户加载完毕
    void user_loaded(int user_id) {
        if (team_a_.have_user(user_id))
            team_a_.user_loaded(user_id);
        else if (team_b_.have_user(user_id))
            team_b_.user_loaded(user_id);
    }

    bool all_players_picked() {
        return team_a_.all_players_picked() && team_b_.all_players_picked();
    }
    bool all_players_loaded() {
        return team_a_.all_players_loaded() && team_b_.all_players_loaded();
    }

    // 结束战斗，生成战斗结算
    BattleResult finish_battle() {
        return {};
    }

    // 获取该对局所有用户id
    std::vector<int> all_users() {
        auto ret = team_a_.all_users();
        ret.append_range(team_b_.all_users());
        return ret;
    }

    void user_load(int user_id, int val) {
        if (team_a_.have_user(user_id))
            team_a_.user_load(user_id, val);
        else if (team_b_.have_user(user_id))
            team_b_.user_load(user_id, val);
    }

    void update_all_hero_pos() {
        team_a_.update_all_hero_pos();
        team_b_.update_all_hero_pos();
    }
    std::size_t serialize_size() const {
        const std::size_t size = team_a_.serialize_size() + team_b_.serialize_size();
        return std::bit_ceil(size);
    }

    std::shared_ptr<Hero> hero(int user_id) {
        if (team_a_.have_user(user_id))
            return team_a_.hero(user_id);
        if (team_b_.have_user(user_id))
            return team_b_.hero(user_id);
        return {};
    }
};

