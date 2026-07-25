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
    void skill() override {  }

    virtual ~Bjorn() = default;

};
