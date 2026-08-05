/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/01 17:48:37
********************************************************************************/
module;
#include <sw/redis++/redis++.h>
#include <unistd.h>
#include <sys/eventfd.h>
export module server;
import log;
import net;
import database;
import message;
import online_user_list;
import time;
import room;
import std;
import user;
import user_manager;
import timer;
import battle_manager;
import hero;
import pos;
import config;
import battle;


export class Server {
    std::unique_ptr<TCP> server_listen_;
    UserManager user_manager_;
    RoomManager room_manager_;
    BattleManager battle_manager_;
    int match_timer_fd_;
    int remove_closed_rooms_fd_;
    int pending_match_timeout_fd_;
    int tick_fd_;
    Timer timer_;
    Database db_;
    std::mutex db_mutex;
    Database battle_result_db_;

    // server 的事件分发
    std::map<header::type, void (Server::*)(std::span<char>, TCP *)> events_router {
        { header::type::login,              &Server::login              },
        { header::type::logout,             &Server::logout             },
        { header::type::heart,              &Server::heart              },
        { header::type::room_create,        &Server::room_create        },
        { header::type::room_invite,        &Server::room_invite        },
        { header::type::room_invite_accept, &Server::room_invite_accept },
        { header::type::room_invite_reject, &Server::room_invite_reject },
        { header::type::room_leave,         &Server::room_leave         },
        { header::type::room_chat,          &Server::room_chat          },
        { header::type::match_join,         &Server::match_join         },
        { header::type::match_accept,       &Server::match_accept       },
        { header::type::match_reject,       &Server::match_reject       },
        { header::type::battle_reconnect,   &Server::battle_reconnect   },
        { header::type::battle_pick_hero,   &Server::battle_pick_hero   },
        { header::type::battle_load,        &Server::battle_load        },
        { header::type::battle_move,        &Server::battle_move        },
        { header::type::battle_attack,      &Server::battle_attack      },
        { header::type::battle_cast_skill,  &Server::battle_cast_skill  },

    };

    void logout(std::span<char> msg, TCP *socket) {
        auto p = msg.data();
        int user_id = message::read(p);

        user_manager_.user_offline(user_id);
    }

    // 匹配成功, 向id发送匹配成功信息
    void match_success(int user_id, int pending_match_id) {
        auto user = user_manager_.search_user_by_id(user_id);
        if (!user)
            return;
        auto &tcp = user->tcp();
        if (!tcp)
            return;

        char buf[512]{};
        auto size = message::write(buf, header::type::match_success, pending_match_id);
        tcp->send_now(std::span{buf, size});
    }

    // 尝试匹配
    void try_match() {
        auto res = room_manager_.try_match();
        if (res.empty())
            return;

        // 匹配成功之后，除了告诉用户匹配状态，同时插入匹配超时定时器
        for (auto &pending_match : res) {
            const auto &[pending_match_id, _, room_a, room_b, _] = *pending_match;
            for (auto &user_id : room_a->users())
                match_success(user_id, pending_match_id);
            for (auto &user_id : room_b->users())
                match_success(user_id, pending_match_id);

            timer_.add_task([pending_match, pending_match_id, this] {
                // @FIXME 需要考虑pending_match 后续不存在的问题
                pending_match->is_confirm_timeout = true;
                message::send_signal(pending_match_timeout_fd_, pending_match_id);
            }, PendingMatch::confirm_timeout);
        }
    }

    // 用户同意匹配
    void match_accept(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int user_id = message::read(p);
        int pending_match_id = message::read(p);

        // 插入用户
        auto pending_match = room_manager_.search_pending_match(pending_match_id);
        pending_match->confirmed.insert(user_id);
        int all_user_count = pending_match->room_a->users().size() + pending_match->room_b->users().size();

        // 还有用户没有确认
        if (pending_match->confirmed.size() < all_user_count) {
            ; // 继续等待
        }
        // 所有用户都确认了
        else if (pending_match->confirmed.size() == all_user_count) {
            auto &[_, _, room_a, room_b, _] = *pending_match;

            auto users_a = room_a->users();
            auto users_b = room_b->users();
            // battle type 进入 pick hero状态
            int battle_id = battle_manager_.add_battle(users_a, users_b);

            char buf[512]{};
            auto size = message::write(buf, header::type::battle_pick_hero, battle_id);

            for (const auto cur_user_id : std::views::concat(users_a, users_b)) {
                auto user = user_manager_.search_user_by_id(cur_user_id);
                if (!user)
                    continue;
                user->status(UserStatus::in_battle);
                user->battle_id(battle_id);
                auto &tcp = user->tcp();
                if (!tcp)
                    continue;
                tcp->send_now(std::span{buf, size});
            }
        }
    }

    // 用户拒绝匹配, 所有用户取消匹配
    void match_reject(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int user_id = message::read(p);
        int pending_match_id = message::read(p);
        match_cancel(pending_match_id);
    }

    // 通知某个已经匹配成功的对局取消
    void match_cancel(int pending_match_id) {
        auto pending_match = room_manager_.search_pending_match(pending_match_id);

        auto all_users = pending_match->room_a->users();
        all_users.append_range(pending_match->room_b->users());

        room_manager_.pending_match_cancel(pending_match_id);

        // 对局取消只有可能是有用户拒绝了对局，显示某用户拒绝对局即可，甚至无需该用户id
        char buf[512]{};
        const auto size = message::write(buf, header::type::match_cancel);
        for (auto user_id : all_users) {

            auto user = user_manager_.search_user_by_id(user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }

    void battle_move(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int battle_id = message::read(p);
        int user_id = message::read(p);
        int pos_x = message::read(p);
        int pos_y = message::read(p);

        auto battle = battle_manager_.get_battle(battle_id);
        if (!battle)
            return;
        battle->battle_move(user_id, Pos{.x = pos_x, .y = pos_y});
    }

    void battle_attack(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int battle_id = message::read(p);
        // user1 攻击 user2
        int user_id1 = message::read(p);
        int user_id2 = message::read(p);

        auto battle = battle_manager_.get_battle(battle_id);
        if (!battle)
            return;
        auto hero1 = battle->hero(user_id1);
        auto hero2 = battle->hero(user_id2);
        if (!hero1 || !hero2)
            return;
        hero1->try_attack(hero2, battle->map());
    }

    void battle_cast_skill(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int battle_id = message::read(p);
        int user_id = message::read(p);
        int attack_user_id = message::read(p);
        int pos_x = message::read(p);
        int pos_y = message::read(p);
        auto battle = battle_manager_.get_battle(battle_id);
        if (!battle)
            return;
        auto hero1 = battle->hero(user_id);
        auto hero2 = battle->hero(attack_user_id);
        if (!hero1 || !hero2)
            return;

        auto add_ground_effect = [&battle, user_id] (const Pos &pos, int r, int s){
            battle->add_ground_effects(user_id, pos, r, s);
        };
        hero1->skill(hero2, Pos{.x = pos_x, .y = pos_y}, add_ground_effect);
        // battle->add_ground_effects();
    }

    void tick() {
        auto all_battle_id = battle_manager_.all_battle_id();
        for (auto battle_id : all_battle_id) {
            auto battle = battle_manager_.get_battle(battle_id);
            if (battle->status() != BattleType::battle)
                continue;

            battle->update_all_hero_pos();
            battle->update_all_hero_effects();
            battle->update_all_ground_effects();

            char buf_a[4096]{}, buf_b[4096]{};

            auto pos_a = message::write(buf_a, header::type::battle_snapshot);
            auto end_a = battle->serialize_for_team(true, buf_a + pos_a);
            std::size_t size_a = (end_a - buf_a) / sizeof(char);

            auto pos_b = message::write(buf_b, header::type::battle_snapshot);
            auto end_b = battle->serialize_for_team(false, buf_b + pos_b);
            std::size_t size_b = (end_b - buf_b) / sizeof(char);

            // team_a 的人发 buf_a，team_b 的人发 buf_b
            for (auto user_id : battle->team_a_users()) {  // Battle 需要暴露这个接口
                auto user = user_manager_.search_user_by_id(user_id);
                if (!user || !user->tcp()) continue;
                user->tcp()->send_now(std::span{buf_a, size_a});
            }
            for (auto user_id : battle->team_b_users()) {
                auto user = user_manager_.search_user_by_id(user_id);
                if (!user || !user->tcp()) continue;
                user->tcp()->send_now(std::span{buf_b, size_b});
            }

            if (!battle->need_finish())
                continue;
            battle_settlement(battle_id);

            // 为数据库写入battle result
        }
    }

    void battle_settlement(int battle_id) {
        auto battle = battle_manager_.get_battle(battle_id);
        if (!battle)
            return;
        auto battle_result = battle->battle_finish();
        battle_manager_.battle_finish(battle_id);

        // 对局信息写数据库
        for (auto user_id : battle_result.team_a_users_id) {
            db_.insert_battle_result(battle_result.battle_id,
                                    user_id,
                                    "A",
                                    battle_result.winner_is_team_a
                                    );
        }
        for (auto user_id : battle_result.team_b_users_id) {
            db_.insert_battle_result(battle_result.battle_id,
                                    user_id,
                                    "B",
                                    battle_result.winner_is_team_a
                                    );
        }

        // 用户信息写数据库
        // level, rank, exp
        for (auto user_id : battle->all_users()) {
            auto user = user_manager_.search_user_by_id(user_id);
            if (!user)
                continue;
            bool winner_is_team_a = battle_result.winner_is_team_a;

            // 计算经验和等级
            int exp = user->exp();
            int level = user->level();
            int rank = user->rank();

            if (!user)
                continue;
            exp += 100;
            if (exp % 1000) {
                // 升级
                exp -= 1000;
                level += 1;
            }

            bool user_is_team_a = battle->user_is_team_a(user_id);
            bool user_is_win = user_is_team_a == winner_is_team_a;

            if (user_is_win)
                rank += 10;
            else
                rank -= 10;

            // 更新服务器中的user信息
            user->rank(rank);
            user->level(level);
            user->exp(exp);

            // 更新数据库中的信息
            db_.update_user_progress(user->id(), level, exp, rank);

            // 通知客户端更新信息
            std::string user_info = user->to_string();
            char buf[512]{};
            auto size = message::write(buf, header::type::user_update_info, std::span<char>{user_info.data(), user_info.size()});
            auto &tcp = user->tcp();
            if (!tcp)
                return;
            tcp->send_now(std::span{buf, size});


            // 发送对局结果
            char battle_result_buf[512]{};
            auto pos = message::write(battle_result_buf, header::type::battle_result);
            auto battle_result_buf_end = battle_result.serialize(battle_result_buf + pos);
            tcp->send_now(std::span{battle_result_buf, static_cast<std::size_t>(battle_result_buf_end - battle_result_buf)});
        }

    }
public:

    Server() {
        match_timer_fd_ = eventfd(0, EFD_NONBLOCK);
        remove_closed_rooms_fd_ = eventfd(0, EFD_NONBLOCK);
        pending_match_timeout_fd_ = eventfd(0, EFD_NONBLOCK);
        tick_fd_ = eventfd(0, EFD_NONBLOCK);

        timer_.add_repeat_task([this] {
            message::send_signal(match_timer_fd_);
        }, std::chrono::seconds{10});

        timer_.add_repeat_task([this] {
            message::send_signal(remove_closed_rooms_fd_);
        }, std::chrono::minutes{1});

        timer_.add_repeat_task([this] {
            message::send_signal(tick_fd_);
        }, std::chrono::milliseconds{1000 / tick_hz});

    }

    // login "number"
    void login(std::span<char> msg, TCP *socket) {
        auto pos = std::ranges::find(msg, ':');
        if (pos == msg.end())
            throw std::invalid_argument("invalid login info");
        std::string number(msg.begin(), pos);
        // + 1 跳过 ':'
        std::string password_hash(pos + 1, msg.end());

        std::vector<std::string> user_info;
        {
            std::lock_guard lock(db_mutex);
            user_info = db_.search_user_profile(number);
        }

        auto login_false = [socket] {
            char buf[header::header_size]{};
            auto size = message::write(buf, header::type::login_false);
            socket->send_now(std::span{buf, size});
            Log().push_log("login error");
        };
        if (user_info.empty()) {
            login_false();
            return;
        }

        auto user = std::make_shared<User>(user_info);
        if (user->password_hash() != password_hash) {
            login_false();
            return;
        }
        auto user_info_string = user->to_string();
        char buf[512]{};
        auto size = message::write(buf, header::type::login_true, std::span{user_info_string.data(), user_info_string.size()});
        socket->send_now(std::span{buf, size});

        auto update_user = user_manager_.search_user_by_fd(socket->fd());
        if (!update_user)
            return;
        update_user->update_user_info(user_info);

        auto info = user_manager_.reconnect_info(update_user->id());
        if (!info)
            return;
        // 需要重连
        auto [battle_id, room_id] = info.value();
        auto room = room_manager_.search_room(room_id);
        char reconnect_buf[512]{};
        auto reconnect_size = message::write(reconnect_buf, header::type::battle_need_reconnect, battle_id, room_id, room->master());
        socket->send_now(std::span{reconnect_buf, reconnect_size});
        update_user->status(UserStatus::in_battle);
        update_user->battle_id(battle_id);
        update_user->room_id(room_id);
    }
    // heart <id>
    void heart(std::span<char> msg, TCP *socket) {
        if (msg.size() != sizeof(int))
            throw std::invalid_argument("invalid heart message");

        int user_id = message::read(msg);
        user_manager_.heart_update(user_id);
    }

    //
    void battle_reconnect(std::span<char> msg, TCP *socket) {
        // 把user的状态重置到对局中即可
        // 对局发送的快照是根据user_manager中的id查找的，上线后自动接收快照信息
        // 客户端加载地图之后开始处理接收到的快照即可

        auto p = msg.data();
        int user_id = message::read(p);
        user_manager_.user_reconnect(user_id);
    }

    // room_create <user_id>
    void room_create(std::span<char> msg, TCP *socket) {
        int user_id = message::read(msg);
        // 创建一个room, 将id放到这个room中
        auto room = std::make_shared<Room>(Room::room_create(user_id));
        room_manager_.add_free_room(room);

        auto user = user_manager_.search_user_by_id(user_id);
        if (!user)
            return;
        user->room_id(room->id());

        char buf[1024]{};
        auto size = message::write(buf, header::type::room_create_true, room->id(), room->master());
        socket->send_now(std::span{buf, size});
    }

    // invite <user_id1> <user_id2> <room_id>
    // user1 invite user2 to room_id
    void room_invite(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int user1 = message::read(p);
        int user2 = message::read(p);
        int room_id = message::read(p);

        char buf[1024]{};
        auto size = message::write(buf, header::type::room_invite_message, user1, user2, room_id);
        std::println(std::cout, "{} invite {} to {}", user1, user2, room_id);

        auto user = user_manager_.search_user_by_id(user2);
        if (!user)
            return;
        auto &tcp = user->tcp();
        if (!tcp)
            return;

        tcp->send_now(std::span{buf, size});
    }

    void room_invite_accept(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int user_id = message::read(p);
        int room_id = message::read(p);

        auto room = room_manager_.search_room(room_id);

        room->add_user(user_id);

        // @TODO 房间应该加锁
        auto users = room->users();
        for (auto id : users) {
            auto cur_user = user_manager_.search_user_by_id(id);
            if (!cur_user)
                continue;
            auto &tcp = cur_user->tcp();
            if (!tcp)
                continue;

            char buf[512]{};
            auto size = message::write(buf, header::type::room_join, user_id, room_id, room->master());

            tcp->send_now(std::span{buf, size});
        }
    }

    void room_invite_reject(std::span<char> msg, TCP *socket) {
        // user1 拒绝了 user2
        char *p = msg.data();
        auto user1 = message::read(p);
        auto user2 = message::read(p);

        // 给user2 发送消息
        auto user = user_manager_.search_user_by_id(user2);
        if (!user)
            return;
        auto &tcp = user->tcp();
        if (!tcp)
            return;

        char buf[512]{};
        auto size = message::write(buf, header::type::room_invite_reject, user1);
        tcp->send_now(std::span{buf, size});
    }

    // room_leave <user> <room_id>
    void room_leave(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        auto id = message::read(p);
        auto room_id = message::read(p);

        auto room = room_manager_.search_room(room_id);
        if (!room)
            return;
        auto users = room->users();

        room->remove_user(id);

        auto user = user_manager_.search_user_by_id(id);
        if (!user)
            return;
        user->room_id(std::nullopt);

        for (auto user_id : users) {
            char buf[512]{};
            auto size = message::write(buf, header::type::room_leave, id, room_id);

            auto user = user_manager_.search_user_by_id(user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }

    void room_chat(std::span<char> msg, TCP *socket) {

        char *p = msg.data();
        auto id = message::read(p);
        auto room_id = message::read(p);

        auto room = room_manager_.search_room(room_id);
        if (!room)
            return;

        auto users = room->users();
        for (auto user_id : users) {
            char buf[512]{};
            auto size = message::write(buf, header::type::room_chat, std::span{std::span<char>::iterator(p), msg.end()});

            auto user = user_manager_.search_user_by_id(user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }
    void match_join(std::span<char> msg, TCP *socket) {

        char *p = msg.data();
        int room_id = message::read(p);
        auto room = room_manager_.search_room(room_id);

        // 把房间添加到匹配队列中
        room_manager_.add_matching_room(room);

        try_match();
    }

    // 收到用户选择英雄后，如果所有用户都选择完成，通知客户端开始加载
    void battle_pick_hero(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        auto battle_id = message::read(p);
        auto user_id = message::read(p);
        auto hero_name = static_cast<HeroName>(message::read(p));

        auto battle = battle_manager_.get_battle(battle_id);
        if (!battle)
            return;

        battle->pick_hero(user_id, hero_name);

        if (!battle->all_players_picked())
            return;
        battle->battle_load();

        // 通知所有用户开始加载
        char buf[512]{};
        auto size = message::write(buf, header::type::battle_start_load);
        auto all_user = battle->all_users();
        for (auto cur_user_id : all_user) {
            auto user = user_manager_.search_user_by_id(cur_user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
        }
    }

    void battle_load(std::span<char> msg, TCP *socket) {
        char *p = msg.data();
        int battle_id = message::read(p);
        int user_id = message::read(p);
        int val = message::read(p);

        auto battle = battle_manager_.get_battle(battle_id);
        if (!battle)
            return;

        battle->user_load(user_id, val);

        if (!battle->all_players_picked())
            return;   // 没有全部加载完就不管


        // 全部加载完广播所有用户开始战斗
        char buf[512]{};
        auto size = message::write(buf, header::type::battle_start);
        auto all_user = battle_manager_.get_battle(battle_id)->all_users();
        for (auto cur_user_id : all_user) {
            auto user = user_manager_.search_user_by_id(cur_user_id);
            if (!user)
                continue;
            auto &tcp = user->tcp();
            if (!tcp)
                continue;
            tcp->send_now(std::span{buf, size});
            room_manager_.battle_start(user->room_id().value());
        }


        battle->battle_start();

    }

    void run() {
        Epoll epoll;
        server_listen_ = std::make_unique<TCP>(Address(config().server_listen_ip, config().server_listen_port));
        if (server_listen_->bind()) {
            Log().push_log("bind ret != 0");
            return;
        }
        if (server_listen_->listen()) {
            Log().push_log("listen ret != 0");
            return;
        }

        epoll.add(server_listen_->fd(), epoll_in);
        epoll.add(match_timer_fd_, epoll_in);
        epoll.add(tick_fd_, epoll_in);
        epoll.add(remove_closed_rooms_fd_, epoll_in);
        epoll.add(pending_match_timeout_fd_, epoll_in);

        Log().push_log("epoll ADD");


        epoll_event events[64];
        while (true) {
            // 有几个消息
            int n = epoll.wait(events, 64);

            for (int i = 0;i < n; ++i) {
                int fd = events[i].data.fd;

                if (fd == server_listen_->fd()) {
                    TCP client = server_listen_->accept();
                    int client_fd = client.fd();
                    Log().push_log("Server accept a new connect");
                    user_manager_.add_fd(client_fd, std::make_unique<TCP>(std::move(client)));

                    epoll.add(client_fd, epoll_in | epoll_out | epoll_et);
                    continue;
                }
                if (fd == match_timer_fd_) {
                    message::consume_signal(fd);
                    try_match();
                    continue;
                }
                if (fd == remove_closed_rooms_fd_) {
                    message::consume_signal(fd);
                    room_manager_.remove_closed_rooms();
                    continue;
                }
                if (fd == pending_match_timeout_fd_) {
                    int pending_match_id = message::consume_signal(fd);
                    match_cancel(pending_match_id);
                    continue;
                }
                if (fd == tick_fd_) {
                    message::consume_signal(fd);
                    tick();
                    continue;
                }

                auto user = user_manager_.search_user_by_fd(fd);
                if (!user)
                    continue;
                auto &tcp = user->tcp();
                if (!tcp)
                    continue;
                if (events[i].events & epoll_in)
                    tcp->get_message_impl();

                if (events[i].events & epoll_out)
                    tcp->send_message_impl();

                while (auto msg = tcp->get_message()) {
                    auto span = std::span{msg->data(), msg->size()};
                    auto type = message::read_header(span);
                    if (!events_router.contains(type))
                        throw std::invalid_argument("invalid server type");
                    (this->*events_router[type])(span.subspan(header::header_size), tcp.get());
                }

                if (events[i].events & (epoll_hup | epoll_err)) {
                    auto cur_user = user_manager_.search_user_by_fd(fd);
                    if (!cur_user)
                        continue;
                    int user_id = cur_user->id();
                    user_manager_.user_offline(user_id);
                }

            }
        }

    }
};

export void server_main() {
    Log().push_log("Server start");

    Server server;
    server.run();


}
