/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/04 22:55:06
********************************************************************************/

module;
#include <sys/eventfd.h>
#include <unistd.h>
export module message;
import std;
import net;

using header_type = std::uint32_t;
export namespace header {
    enum class type : header_type {
        // 登录相关
        login,
        login_true,
        login_false,

        // 心跳
        heart,

        // 房间
        room_create,
        room_create_true,
        room_invite,
        room_invite_message,
        room_invite_accept,
        room_invite_reject,
        room_join,
        room_leave,
        room_chat,

        // 匹配
        match_join,
        match_cancel,
        match_success,


        // 系统/错误
        error
    };

    constexpr std::size_t header_size = sizeof(type);

}


export namespace message {

    std::size_t write(char *buf, header::type type) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        return sizeof(v);
    }

    std::size_t write(char *buf, header::type type, std::span<char> msg) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        std::memcpy(buf + sizeof(v), msg.data(), msg.size());
        return sizeof(v) + msg.size();
    }

    std::size_t write(char *buf, header::type type, int number) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        std::memcpy(buf + sizeof(v), &number, sizeof(number));
        return sizeof(v) + sizeof(number);
    }
    std::size_t write(char *buf, header::type type, int number1, int number2) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        std::memcpy(buf + sizeof(v), &number1, sizeof(number1));
        std::memcpy(buf + sizeof(v) + sizeof(number1), &number2, sizeof(number2));
        return sizeof(v) + sizeof(number1) + sizeof(number2);
    }
    std::size_t write(char *buf, header::type type, int number1, int number2, int number3) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        std::memcpy(buf + sizeof(v), &number1, sizeof(number1));
        std::memcpy(buf + sizeof(v) + sizeof(number1), &number2, sizeof(number2));
        std::memcpy(buf + sizeof(v) + sizeof(number1) + sizeof(number2), &number3, sizeof(number3));
        return sizeof(v) + sizeof(number1) + sizeof(number2) + sizeof(number3);
    }

    std::size_t write(char *buf, header::type type, int number, std::span<char> msg) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        std::memcpy(buf + sizeof(v), &number, sizeof(number));
        std::memcpy(buf + sizeof(v) + sizeof(number), &msg[0], msg.size());
        return sizeof(v) + sizeof(number) + msg.size();
    }
    std::size_t write(char *buf, header::type type, int number1, int number2, std::span<char> msg) {
        std::uint32_t v = static_cast<std::uint32_t>(type);
        std::memcpy(buf, &v, sizeof(v));
        std::memcpy(buf + sizeof(v), &number1, sizeof(number1));
        std::memcpy(buf + sizeof(v) + sizeof(number1), &number2, sizeof(number2));
        std::memcpy(buf + sizeof(v) + sizeof(number1) + sizeof(number2), &msg[0], msg.size());
        return sizeof(v) + sizeof(number1) + sizeof(number2) + msg.size();
    }

    char *write(char *buf, int number) {
        std::memcpy(buf, &number, sizeof(number));
        return buf + sizeof(number);
    }
    int read(char *buf) {
        int number;
        std::memcpy(&number, buf, sizeof(number));
        return number;
    }
    header::type read_header(std::span<char> buf) {
        header::type v;
        std::memcpy(&v, buf.data(), sizeof(v));
        return v;
    }
    int read(std::span<char> span) {
        return read(span.data());
    }

    void send_signal(int fd) {
        std::uint64_t val = 1;
        ::write(fd, &val, sizeof(val));
    }

    void consume_signal(int fd) {
        std::uint64_t val;
        ::read(fd, &val, sizeof(val));  // 消费事件
    }

}

