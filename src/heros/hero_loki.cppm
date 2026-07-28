/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:12:25
********************************************************************************/
export module hero.loki;
import hero.base;
import pos;
import std;
import message;

// 刺客、能量
// 技能开启后下次攻击双倍伤害
export class Loki : public Hero {
    bool enable_skill_{};
public:
    Loki() = default;
    char* serialize(char* buf) override {
        buf = message::write(buf, enable_skill_);
        return Hero::serialize(buf);
    }
    char* deserialize(char* buf) override {
        enable_skill_ = message::read(buf);
        return Hero::deserialize(buf);
    }
    bool check_can_cast_skill(const Pos& skill_pos) override {
        return Hero::check_can_cast_skill(skill_pos);
    }
    void receive_damage(std::shared_ptr<Hero> hero, int damage) override {
        Hero::receive_damage(hero, damage);
    }
    void skill(std::shared_ptr<Hero> hero, const Pos &pos) override {
        if (check_can_cast_skill(pos)) {
            enable_skill_ = true;
            mp_ -= skill_need_mp_;
        }
    }
    void attack(std::shared_ptr<Hero> hero) override {
        attack_ *= 2;
        Hero::attack(hero);
        attack_ /= 2;
        enable_skill_ = false;
    }
    virtual ~Loki() = default;
};
