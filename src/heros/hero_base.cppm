/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/24 13:43:54
********************************************************************************/

module;
export module hero.base;
import std;
import message;


export enum class HeroName {
    bjorn,
    loki,
    merlin,
    thorn
};


export struct Pos {
    int x;
    int y;

    char* serialize(char *buf) {
        message::write(buf, x);
        return message::write(buf + sizeof(int), y);
    }
    char * deserialize(char *buf) {
        x = message::read(buf);
        y = message::read(buf);
        return buf;
    }
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

    virtual char* serialize(char* buf) {
        char* p = buf;
        p = message::write(p, hp_);
        p = message::write(p, hp_max_);
        p = message::write(p, mp_);
        p = message::write(p, mp_max_);
        p = message::write(p, attack_);
        p = message::write(p, defense_);
        p = message::write(p, attack_speed_);
        p = message::write(p, attack_range_);
        p = message::write(p, move_speed_);
        p = message::write(p, skill_need_mp_);
        return pos_.serialize(p);
    }
    virtual char* deserialize(char *buf) {
        hp_           = message::read(buf);
        hp_max_       = message::read(buf);
        mp_           = message::read(buf);
        mp_max_       = message::read(buf);
        attack_       = message::read(buf);
        defense_      = message::read(buf);
        attack_speed_ = message::read(buf);
        attack_range_ = message::read(buf);
        move_speed_   = message::read(buf);
        skill_need_mp_= message::read(buf);
        return pos_.deserialize(buf);
    }

    // 判断能否在 skill_pos 位置释放技能
    virtual bool check_can_cast_skill(const Pos &skill_pos) = 0;
    // 释放技能
    virtual void skill() = 0;

    virtual ~Hero() = default;
};
