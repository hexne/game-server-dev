/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/24 13:43:54
********************************************************************************/

module;
export module hero.base;
import std;
import message;
import pos;
import battle.map;
import effects_manager;


export enum class HeroName {
    bjorn,
    loki,
    merlin,
    thorn
};




export class Hero : public std::enable_shared_from_this<Hero> {
protected:
    int hp_{}, hp_max_{};
    int mp_{}, mp_max_{};
    int attack_{};
    int defense_{};
    int attack_speed_{};
    int attack_range_{};
    int move_speed_{};  // 单位时间走过的位置数量
    int skill_need_mp_{};
    Pos pos_{};

    // path_无需序列化, 用于server设置hero当前位置
    std::queue<Pos> path_;
    EffectsManager effects_manager_;


public:

    Hero() = default;

    // 参数是被攻击者的位置
    bool can_cast_attack(const Pos &other) {
        int dx = std::abs(pos_.x - other.x);
        int dy = std::abs(pos_.y - other.y);
        return attack_range_ * attack_range_ >= dx * dx + dy * dy;
    }
    Pos pos() const {
        return pos_;
    }

    // 计算被攻击后受到的伤害是多少
    int calculate_damage(int damage) {
        const int real = damage * (100 - defense_) / 100;
        return real > 0 ? real : 1;
    }

    // 被攻击后的扣血
    virtual void receive_damage(std::shared_ptr<Hero> hero, int damage) {
        int cur_hp = hp_ - damage;
        if (cur_hp < 0)
            cur_hp = 0;
        hp_ = cur_hp;
    }

    // 一个单位时间之后在哪里
    void update_pos() {
        int count = move_speed_;
        while (!path_.empty() && count --) {
            pos_ = path_.front();
            path_.pop();
        }
    }
    void update_effects() {
        effects_manager_.update();
    }

    // 只给目的坐标，从当前位置开始移动
    void move(const Pos &pos) {
        path_ = BattleMap::a_start(pos_, pos);
    }

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
        pos_.serialize(p);
        return effects_manager_.serialize(p);
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
        pos_.deserialize(buf);
        return effects_manager_.deserialize(buf);
    }

    // 判断能否在 skill_pos 位置释放技能
    virtual bool check_can_cast_skill(const Pos &skill_pos) = 0;

    // 释放技能
    virtual void skill(std::shared_ptr<Hero>, const Pos &) = 0;

    virtual void attack(std::shared_ptr<Hero> hero) {
        // 如果不能攻击到
        if (!can_cast_attack(hero->pos())) {
            move(hero->pos()); // 也许还得记录对方位置，实现跟踪？
        }
        // 能攻击到就直接攻击
        else {
            // 当前角色攻击hero
            // @TODO, 这里攻击没有计算攻速
            int damage = hero->calculate_damage(attack_);
            hero->receive_damage(shared_from_this(), damage);
        }
    }

    virtual ~Hero() = default;
};
