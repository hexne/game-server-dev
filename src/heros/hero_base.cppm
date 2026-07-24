/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/24 13:43:54
********************************************************************************/

module;
export module hero.base;


export enum class HeroName {
    bjorn,
    loki,
    merlin,
    thorn
};


struct Pos {
    int x;
    int y;
};

export class Hero {
    int hp_{}, hp_max_{};
    int mp_{}, mp_max_{};
    int attack_{};
    int defense_{};
    int attack_speed_{};
    int attack_range_{};
    int move_speed_{};
    int skill_need_mp_{};
    Pos pos_{};

public:

    Hero() = default;
    // 判断能否在 skill_pos 位置释放技能
    virtual bool check_can_cast_skill(const Pos &skill_pos) = 0;
    // 释放技能
    virtual void skill() = 0;

    virtual ~Hero() = default;
};
