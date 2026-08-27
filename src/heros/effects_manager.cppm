/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/26 18:07:11
********************************************************************************/

module;
export module effects_manager;
import std;
import message;
import time;
import timer;

export enum class EffectsType : int {
    reflect,    // 反伤
    channeling, // 引导技能
    // 刺客的下次技能双倍伤害无需计算tick
};
struct EffectsNode {
    EffectsType type{};
    int count{};

    char *serialize(char *buf) {
        buf = message::write(buf, static_cast<int>(type));
        return message::write(buf, count);
    }
    char *deserialize(char *buf) {
        type = static_cast<EffectsType>(message::read(buf));
        count = message::read(buf);
        return buf;
    }
};

export class EffectsManager {
    std::vector<EffectsNode> effects_{};
    CoroutineTimer timer_{};
    int buffer_id_{};
public:
    void remove_effects(EffectsType type) {
        std::erase_if(effects_, [type](EffectsNode &node) {
            return node.type == type;
        });
    }

    void update() {
        timer_.resume();
    }


    TimerTask<void> add_effects(EffectsType type, int s) {
        effects_.emplace_back(EffectsNode{.type = type });
        co_await timer_.sleep_for(std::chrono::seconds{s});
        remove_effects(type);
    }
    bool search_effects(EffectsType type) {
        const auto it = std::ranges::find_if(effects_, [type](const EffectsNode &node) {
            return node.type == type;
        });
        return it != effects_.end();
    }
    char *serialize(char *buf) {
        int size = effects_.size();
        buf = message::write(buf, size);

        for (int i = 0;i < size; ++i)
            buf = effects_[i].serialize(buf);
        return buf;
    }
    char *deserialize(char *buf) {
        int size = message::read(buf);
        effects_.resize(size);
        for (int i = 0;i < size; ++i)
            effects_[i].deserialize(buf);
        return buf;
    }
};