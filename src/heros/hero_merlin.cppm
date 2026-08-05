/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:12:11
********************************************************************************/
export module hero.merlin;
import hero.base;
import pos;
import std;
import effects_manager;
import message;

// 法师、远程
// 技能开启后，引导时间内， pos位置，范围rang, 不断掉血
export class Merlin : public Hero {
    int skill_need_distance_{};

    int skill_r_ {10};
    int sikll_s_ {5};
public:
    Merlin() = default;
    char* serialize(char* buf) override {
        buf = message::write(buf, skill_need_distance_);
        return Hero::serialize(buf);
    }
    char* deserialize(char* buf) override {
        skill_need_distance_ = message::read(buf);
        return Hero::deserialize(buf);
    }
    bool check_can_cast_skill(const Pos& skill_pos) override {
        int dx = std::abs(pos_.x - skill_pos.x);
        int dy = std::abs(pos_.y - skill_pos.y);
        bool check_distance = dx * dx + dy * dy <= skill_need_distance_ * skill_need_distance_;

        return check_distance && Hero::check_can_cast_skill(skill_pos);
    }
    void receive_damage(std::shared_ptr<Hero> hero, int damage) override {
        Hero::receive_damage(hero, damage);
    }
    void skill(std::shared_ptr<Hero> hero, const Pos &pos, const GroundEffectCallback &add_ground_effect) override {
        if (check_can_cast_skill(pos)) {
            effects_manager_.add_effects(EffectsType::channeling, 5);
            mp_ -= skill_need_mp_;
            add_ground_effect(pos, skill_r_, sikll_s_);
        }
    }
    virtual ~Merlin() = default;
};