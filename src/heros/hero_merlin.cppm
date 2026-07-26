/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:12:11
********************************************************************************/
export module hero.merlin;
import hero.base;
import pos;
import std;

// 法师、远程
export class Merlin : public Hero {
public:
    Merlin() = default;
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
    void skill() override {  }
    virtual ~Merlin() = default;
};