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
import pos;
import time;
import message;
import ground_effects;

export class Battle {
    Team team_a_, team_b_;
    int random_seed_{};
    std::default_random_engine random_engine_;

    std::vector<GroundEffects> ground_effects_;

    bool in_range(const Pos &pos, int r, std::shared_ptr<Hero> hero) {
        auto hero_pos = hero->pos();
        int dx = std::abs(hero_pos.x - pos.x);
        int dy = std::abs(hero_pos.y - pos.y);
        return dx * dx + dy * dy <= r;
    }

public:
    Battle(int random_seed, Team team_a, Team team_b)
        : random_seed_(random_seed), team_a_(std::move(team_a)), team_b_(std::move(team_b)), random_engine_(random_seed_) {  }

    char* serialize(char* buf) {
        int size = ground_effects_.size();
        buf = message::write(buf, size);
        for (auto ground_effect: ground_effects_)
            buf = ground_effect.serialize(buf);

        buf = team_a_.serialize(buf);
        return team_b_.serialize(buf);
    }
    char* deserialize(char *buf) {
        int size = message::read(buf);
        ground_effects_.resize(size);
        for (int i = 0;i < size; ++i)
            buf = ground_effects_[i].deserialize(buf);

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
    void update_all_hero_effects() {
        team_a_.update_all_hero_effects();
        team_b_.update_all_hero_effects();
    }

    void update_all_ground_effects() {
        update_ground_effects();
        for (auto &ground_effect: ground_effects_) {
            auto user_id = ground_effect.user_id;
            auto pos = ground_effect.pos;
            auto r = ground_effect.radius;
            auto hero = this->hero(user_id);

            // 当前技能是team_a放的
            if (team_a_.have_user(user_id)) {
                for (auto team_b_user : team_b_.all_users()) {
                    auto team_b_hero = this->hero(team_b_user);
                    if (in_range(pos, r, team_b_hero)) {
                        hero->attack(team_b_hero);
                    }
                }
            }
            // 当前技能是team_b放的
            else if (team_b_.have_user(user_id)) {
                for (auto team_a_user : team_b_.all_users()) {
                    auto team_a_hero = this->hero(team_a_user);
                    if (in_range(pos, r, team_a_hero)) {
                        hero->attack(team_a_hero);
                    }
                }
            }

        }

        // 遍历所有ground_effects
        // 遍历对team_b的伤害
        // 遍历对team_a的伤害
    }

    // 释放影响地形技能的英雄、技能的中心位置、技能的半径、技能的持续时间
    void add_ground_effects(int user_id, const Pos &pos, int r, int s) {
        ground_effects_.emplace_back(GroundEffects{
            .pos = pos,
            .radius = r,
            .count = count_tick(s),
            .user_id = user_id,
        });
    }

    void update_ground_effects() {
        for (auto &ground_effect: ground_effects_)
            ground_effect.count --;

        std::erase_if(ground_effects_, [](const GroundEffects &ground_effect) {
            return ground_effect.count <= 0;
        });
    }

    std::shared_ptr<Hero> hero(int user_id) {
        if (team_a_.have_user(user_id))
            return team_a_.hero(user_id);
        if (team_b_.have_user(user_id))
            return team_b_.hero(user_id);
        return {};
    }
};

