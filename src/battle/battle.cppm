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

export enum class BattleType {
    pick_hero,
    battle_load,
    battle,
    finish
};

export class Battle {
    int battle_id_;
    BattleType type_;
    Team team_a_, team_b_;
    int random_seed_{};
    std::default_random_engine random_engine_;
    std::bernoulli_distribution dist_;
    std::vector<GroundEffects> ground_effects_;

    bool in_range(const Pos &pos, int r, std::shared_ptr<Hero> hero) {
        auto hero_pos = hero->pos();
        int dx = std::abs(hero_pos.x - pos.x);
        int dy = std::abs(hero_pos.y - pos.y);
        return dx * dx + dy * dy <= r * r;
    }
    bool check_winner_is_team_a() {
        return dist_(random_engine_);
    }
public:
    Battle(int battle_id, int random_seed, Team team_a, Team team_b, BattleType type)
        : battle_id_(battle_id), random_seed_(random_seed), team_a_(std::move(team_a)), team_b_(std::move(team_b)),
            random_engine_(random_seed_), dist_(std::bernoulli_distribution(0.5f)), type_(type) {  }

    // battle.cppm
    char* serialize_for_team(bool viewer_is_team_a, char* buf) {
        auto visible = visible_users(viewer_is_team_a);

        // 地面效果（技能范围伤害区域）也要按视野过滤，
        // 否则敌方放的技能范围会暴露对方的走位意图
        std::vector<GroundEffects> visible_effects;
        for (auto &e : ground_effects_) {
            bool is_ally_effect = viewer_is_team_a
                ? team_a_.have_user(e.user_id)
                : team_b_.have_user(e.user_id);
            if (is_ally_effect || visible.contains(e.user_id))
                visible_effects.push_back(e);
        }
        int size = visible_effects.size();
        buf = message::write(buf, size);
        for (auto &e : visible_effects)
            buf = e.serialize(buf);

        // 己方队伍：全量序列化，不做任何隐藏
        // 敌方队伍：只序列化可见的英雄，其余跳过
        auto &own_team = viewer_is_team_a ? team_a_ : team_b_;
        auto &enemy_team = viewer_is_team_a ? team_b_ : team_a_;

        buf = own_team.serialize(buf);
        buf = enemy_team.serialize_filtered(buf, visible); // 见下方
        return buf;
    }

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

    void battle_start() {
        type_ = BattleType::battle;
        // 直接随机数决出获胜队伍
        bool winner_is_a_team = dist_(random_engine_);
    }

    void battle_load() {
        type_ = BattleType::battle_load;
    }

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


    bool need_finish() {
        return true;
    }
    // 结束战斗，生成战斗结算
    BattleResult battle_finish() {
        type_ = BattleType::finish;
        return BattleResult{
                            .battle_id = battle_id_,
                            .winner_is_team_a = check_winner_is_team_a(),
                            .team_a_users_id = team_a_users(),
                            .team_b_users_id = team_b_users(),
        };


        return {};
    }

    // 获取该对局所有用户id
    std::vector<int> all_users() {
        auto ret = team_a_.all_users();
        ret.append_range(team_b_.all_users());
        return ret;
    }
    std::vector<int> team_a_users() {
        return team_a_.all_users();
    }
    std::vector<int> team_b_users() {
        return team_b_.all_users();
    }
    bool user_is_team_a(int user_id) {
        return team_a_.have_user(user_id);
    }
    bool user_is_team_b(int user_id) {
        return team_b_.have_user(user_id);
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
            if (!hero)
                continue;

            // 当前技能是team_a放的
            if (team_a_.have_user(user_id)) {
                for (auto team_b_user : team_b_.all_users()) {
                    auto team_b_hero = this->hero(team_b_user);
                    if (!team_b_hero)
                        continue;
                    if (in_range(pos, r, team_b_hero)) {
                        hero->attack(team_b_hero);
                    }
                }
            }
            // 当前技能是team_b放的
            else if (team_b_.have_user(user_id)) {
                for (auto team_a_user : team_a_.all_users()) {
                    auto team_a_hero = this->hero(team_a_user);
                    if (!team_a_hero)
                        continue;
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

    std::unordered_set<int> visible_users(bool viewer_is_team_a) {
        auto &viwers = viewer_is_team_a ? team_a_ : team_b_;
        auto &enemies = viewer_is_team_a ? team_b_ : team_a_;

        std::unordered_set<int> visible;
        for (auto viewer_id : viwers.all_users()) {
            auto viewer_hero = this->hero(viewer_id);
            if (!viewer_hero)
                continue;

            for (auto enemy_id : enemies.all_users()) {
                auto enemy_hero = enemies.hero(enemy_id);
                if (!enemy_hero)
                    continue;
                if (visible.contains(enemy_id))
                    continue;

                if (in_range(viewer_hero->pos(), Hero::vision_range, enemy_hero))
                    visible.insert(enemy_id);
            }
        }
        return visible;
    }
};

