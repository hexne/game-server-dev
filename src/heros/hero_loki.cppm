/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:12:25
********************************************************************************/
export module hero.loki;
import hero.base;
import pos;
import std;

// 刺客、能量
// 技能开启后下次攻击双倍伤害
export class Loki : public Hero {
    bool enable_skill_{};
public:
    Loki() = default;
    char* serialize(char* buf) override {
        return Hero::serialize(buf);
    }
    char* deserialize(char* buf) override {
        return Hero::deserialize(buf);
    }
    bool check_can_cast_skill(const Pos& skill_pos) override {
        return false;
    }
    void receive_damage(std::shared_ptr<Hero> hero, int damage) override {
        Hero::receive_damage(hero, damage);
    }
    void skill(std::shared_ptr<Hero> hero, const Pos &pos) override {
        enable_skill_ = true;
    }
    virtual ~Loki() = default;
};
