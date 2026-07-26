/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:11:52
********************************************************************************/
export module hero.bjorn;
import hero.base;
import pos;
import std;

// 战士、近战
export class Bjorn : public Hero {
    bool enable_skill_{};   // 6s内反伤
public:
    Bjorn() = default;
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
        if (!enable_skill_)
            return;

        // 反伤 30%
        int val_30 = damage * 0.3;
        int val = Hero::calculate_damage(damage);

        // hero 攻击this
        hero->receive_damage(shared_from_this(), val);
    }

    void skill() override {
        enable_skill_ = true;
    }

    virtual ~Bjorn() = default;

};
