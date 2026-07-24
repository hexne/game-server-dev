/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:12:25
********************************************************************************/
export module hero.loki;
import hero.base;

// 刺客、能量
export class Loki : public Hero {
public:
    Loki() = default;
    char* serialize(char* buf) override {
        return Hero::serialize(buf);
    }
    char* deserialize(char* buf) override {
        return Hero::deserialize(buf);
    }
    bool check_can_cast_skill(const ::Pos& skill_pos) override {
        return false;
    }
    void skill() override {  }
    virtual ~Loki() = default;
};
