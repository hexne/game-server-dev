/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/27 19:42:22
********************************************************************************/

module;
export module ground_effects_manager;
import hero;
import pos;
import std;
import time;
import message;

export struct GroundEffects {
    Pos pos{};
    int radius{};
    int count{};
    int user_id{};
    char *serialize(char *buf) {
        buf = pos.serialize(buf);
        buf = message::write(buf, radius);
        buf = message::write(buf, count);
        return message::write(buf, user_id);
    }
    char *deserialize(char *buf) {
        buf = pos.deserialize(buf);
        radius = message::read(buf);
        count = message::read(buf);
        user_id = message::read(buf);
        return buf;
    }
};

