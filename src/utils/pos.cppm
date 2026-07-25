/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/25 11:10:38
********************************************************************************/

module;
export module pos;
import message;

export struct Pos {
    int x;
    int y;

    bool operator == (const Pos& r) const {
        return x == r.x && y == r.y;
    }

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