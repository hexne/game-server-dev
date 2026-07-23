/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 21:01:00
********************************************************************************/

module;
export module hero;

export class Hero {
    int hp_{}, hp_max_{};
    int mp_{}, mp_max_{};
    int attack_{};
    int defense_{};
    int attack_speed_{};
    int attack_range_{};
    int move_speed_{};
    int skill_need_mp_{};
public:
    Hero() = default;
    bool check_can_cast_skill() {
        return skill_need_mp_;
    }

    virtual void skill() {  }
    virtual ~Hero() = default;
};
