/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/26 18:07:11
********************************************************************************/

module;
export module effects_manager;
import std;
import message;
import time;

export enum class EffectsType : int {
    reflect,    // 反伤
    channeling, // 引导技能
    // 刺客的下次技能双倍伤害无需计算tick
};
struct EffectsNode {
    EffectsType type{};
    int count{};

    char *serialize(char *buf) {
        message::write(buf, static_cast<int>(type));
        message::write(buf, count);
    }
    char *deserialize(char *buf) {
        type = static_cast<EffectsType>(message::read(buf));
        count = message::read(buf);
    }
};

export class EffectsManager {
    std::vector<EffectsNode> effects_{};
public:
    void remove_effects(EffectsType type) {
        std::erase_if(effects_, [type](EffectsNode &node) {
            return node.type == type;
        });
    }

    void update() {
        for (auto &[_, count] : effects_) {
            count --;
        }
        std::erase_if(effects_, [](EffectsNode &node) {
            return node.count <= 0;
        });
    }


    void add_effects(EffectsType type, int s) {
        effects_.emplace_back(EffectsNode{.type = type, .count = count_tick(s)});
    }
    char *serialize(char *buf) {
        int size = effects_.size();
        message::write(buf, size);

        for (int i = 0;i < size; ++i)
            effects_[i].serialize(buf);
    }
    char *deserialize(char *buf) {
        int size = message::read(buf);
        effects_.resize(size);
        for (int i = 0;i < size; ++i)
            effects_[i].deserialize(buf);
    }
};