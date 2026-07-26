/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:12:44
********************************************************************************/
export module hero.thorn;
import hero.base;
import pos;
import std;

// 坦克、反伤
export class Thorn : public Hero {
public:
    Thorn() = default;
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
    virtual ~Thorn() = default;
};