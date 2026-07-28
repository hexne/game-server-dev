/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:11:52
********************************************************************************/
export module hero.bjorn;
import hero.base;
import pos;
import std;
import effects_manager;

// 战士、近战
// 技能开启后6s内反伤30%
export class Bjorn : public Hero {

public:
    Bjorn() = default;
    char* serialize(char* buf) override {
        return Hero::serialize(buf);
    }
    char* deserialize(char* buf) override {
        return Hero::deserialize(buf);
    }
    bool check_can_cast_skill(const Pos& skill_pos) override {
        return Hero::check_can_cast_skill(skill_pos);
    }
    // 应用伤害
    void receive_damage(std::shared_ptr<Hero> hero, int damage) override {
        Hero::receive_damage(hero, damage);
        if (!effects_manager_.search_effects(EffectsType::reflect))
            return;

        // 反伤 30%
        int val_30 = damage * 0.3;
        int val = Hero::calculate_damage(val_30);

        // hero 攻击this
        hero->receive_damage(shared_from_this(), val);
    }

    void skill(std::shared_ptr<Hero> hero, const Pos &pos) override {
        if (check_can_cast_skill(pos)) {
            effects_manager_.add_effects(EffectsType::reflect, 6);
            mp_ -= skill_need_mp_;
        }
    }

    virtual ~Bjorn() = default;

};
