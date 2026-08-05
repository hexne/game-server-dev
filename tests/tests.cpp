/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/28 18:45:49
********************************************************************************/
import args;
import client;
import client_manager;
import std;
import message;
import hero;

ClientManager client_manager;

auto wait(std::unique_ptr<Client> &client, header::type type, header::type type2 = header::type::error) {
    auto &cv = client->test_cv;
    std::unique_lock lock(client->test_mutex);
    cv.wait(lock, [&client, type, type2] { return client->has_msg(type, type2); });
    auto info = client->test_info();
    client->consume_msg(type, type2);
    return info;
}



struct TestResult {
    bool is_pass{};
    std::string error_msg{};

    explicit TestResult(bool pass) : is_pass{pass} {  }
    explicit TestResult(const std::string &message) : error_msg{message} { }

    std::string string() {
        if (is_pass)
            return "[ PASS ]";
        return std::format("[FAILED] {}", error_msg);
    }
};

TestResult add(const Args& args) {
    if (!client_manager.add(args.indexs.front()))
        return TestResult{std::string("client connect failed")};
    return TestResult{true};
}

// 登录测试
// login                        全部登录
// login index1 index2          指定的index登录
// login index1-index2          [index1, index2]登录
// index 的范围和内容是可推测的
TestResult login(const Args &args) {
    std::vector<int> indexs{};
    if (args.args_type == Args::ArgsType::none) {
        indexs = client_manager.all_index();
    }
    else if (args.args_type == Args::ArgsType::range) {
        auto [begin, end] = args.range;
        auto all_index = client_manager.all_index();
        for (auto index : all_index) {
            if (index >= begin && index <= end)
                indexs.push_back(index);
        }
    }
    else {
        for (auto index : args.indexs)
            indexs.push_back(index);
    }

    client_manager.login(indexs);


    // 拿到了所有的index, 检查获取的内容
    for (auto index : indexs) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::login_true);
        if (res.contains(header::type::login_true))
            continue;

        return TestResult{std::string("login false")};
    }
    return TestResult{true};
}


// room index ..., 每个被指定的index创建一个房间
TestResult room_create(const Args& args) {
    std::vector<int> indexs{};
    if (args.args_type == Args::ArgsType::none) {
        indexs = client_manager.all_index();
    }
    else if (args.args_type == Args::ArgsType::range) {
        auto all_index = client_manager.all_index();
        auto [begin, end] = args.range;
        for (auto index : all_index) {
            if (index >= begin && index <= end)
                indexs.push_back(index);
        }
    }
    else {
        for (auto index : args.indexs)
            indexs.push_back(index);
    }
    client_manager.room_create(indexs);

    // 开始检查client的消息
    for (auto index : indexs) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::room_create_true);
        if (res.contains(header::type::room_create_true))
            continue;
        return TestResult{std::string("room create false")};
    }
    return TestResult{true};
}


// room_invite index1 index2, index1 邀请index2加入房间
TestResult room_invite(bool is_accept, const Args& args) {
    if (args.args_type == Args::ArgsType::none)
        return TestResult{std::string("error command format")};
    if (args.args_type == Args::ArgsType::range)
        return TestResult{std::string("error command format")};
    if (args.indexs.size() != 2)
        return TestResult{std::string("error command format")};
    auto user1 = args.indexs[0];
    auto user2 = args.indexs[1];

    // user1 邀请user2
    client_manager.room_invite(user1, user2);
    auto &client1 = client_manager.client(user1);
    auto &client2 = client_manager.client(user2);
    auto res2 = wait(client2, header::type::room_invite_message);

    if (!res2.contains(header::type::room_invite_message))
        return TestResult{std::string("haven't get room invite")};

    if (is_accept)
        client2->room_invite_accept(client1->user_id());
    else
        client2->room_invite_reject(client1->user_id());

    auto res1 = wait(client1, header::type::room_join, header::type::room_invite_reject);
    if (is_accept && res1.contains(header::type::room_join))
        return TestResult {true};
    if (!is_accept && res1.contains(header::type::room_invite_reject))
        return TestResult {true};

    return TestResult{ std::string("error info") };
}

TestResult room_invite_accept(const Args& args) {
    return room_invite(true, args);
}
TestResult room_invite_reject(const Args& args) {
    return room_invite(false, args);
}


TestResult room_chat(const Args& args) {
    if (args.args_type == Args::ArgsType::none) {
        return TestResult{std::string("error command format")};
    }
    if (args.args_type == Args::ArgsType::range) {
        return TestResult{std::string("error command format")};
    }

    static constexpr std::string msg = "hello room";
    auto user1 = args.indexs.front();
    auto &send_msg_client = client_manager.client(user1);
    send_msg_client->room_chat(msg);

    auto get_all_user_index = [&args] {
        std::vector<int> ret{};
        for (int i = 1; i < args.indexs.size(); ++i)
            ret.push_back(args.indexs[i]);
        return ret;
    };
    auto all_user_index = get_all_user_index();

    for (auto index : all_user_index) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::room_chat);
        if (!res.contains(header::type::room_chat)) {
            return TestResult{std::string("haven't get chat msg")};
        }

        auto get_msg = res[header::type::room_chat];
        if (get_msg != msg) {
            return TestResult{std::string("get error chat msg")};
        }
    }
    return TestResult{true};
}


TestResult match(int type, const Args& args) {
    // 0 -> 确认对局
    // 1 -> 拒绝对局
    // 2 -> 超时
    std::vector<int> indexs{};
    if (args.args_type == Args::ArgsType::none) {
        indexs = client_manager.all_index();
    }
    else if (args.args_type == Args::ArgsType::range) {
        auto all_index = client_manager.all_index();
        auto [begin, end] = args.range;
        for (auto index : all_index) {
            if (index >= begin && index <= end)
                indexs.push_back(index);
        }
    }
    else {
        for (auto index : args.indexs)
            indexs.push_back(index);
    }

    for (auto index : indexs) {
        auto &client = client_manager.client(index);
        client->match_join();
    }

    // 所有指定的用户都已经开始匹配，同时记录 pending_match_id
    int pending_match_id{};
    for (auto &index : indexs) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::match_success);
        if (!res.contains(header::type::match_success)) {
            return TestResult{std::string("error header info")};
        }
        auto &payload = res[header::type::match_success];
        pending_match_id = message::read(std::span{payload.data(), payload.size()});
    }

    // 接受：全部用户发送 match_accept，然后应收到 battle_pick_hero
    if (type == 0) {
        for (auto index : indexs) {
            auto &client = client_manager.client(index);
            client->match_accept(pending_match_id);
        }
        for (auto index : indexs) {
            auto &client = client_manager.client(index);
            auto res = wait(client, header::type::battle_pick_hero);
            if (!res.contains(header::type::battle_pick_hero))
                return TestResult{std::string("error header info")};
        }
        return TestResult{true};
    }

    // 拒绝：挑一个用户拒绝，检查所有人都收到 match_cancel
    if (type == 1) {
        auto &reject_client = client_manager.client(indexs.front());
        reject_client->match_reject(pending_match_id);

        for (auto index : indexs) {
            auto &client = client_manager.client(index);
            auto res = wait(client, header::type::match_cancel);
            if (!res.contains(header::type::match_cancel))
                return TestResult{std::string("error header info")};
        }
        return TestResult{true};
    }

    // 超时：留一个用户不发任何消息，其余照常 accept，
    // 等服务端 30s 超时定时器触发后广播 match_cancel
    if (type == 2) {
        for (std::size_t i = 0; i + 1 < indexs.size(); ++i) {
            auto &client = client_manager.client(indexs[i]);
            client->match_accept(pending_match_id);
        }
        // indexs.back() 故意什么都不发

        for (auto index : indexs) {
            auto &client = client_manager.client(index);
            auto res = wait(client, header::type::match_cancel);
            if (!res.contains(header::type::match_cancel))
                return TestResult{std::string("error header info")};
        }
        return TestResult{true};
    }

    return TestResult{std::format("unknown match test type: {}", type)};
}
TestResult match_accept(const Args& args) {
    return match(0, args);
}
TestResult match_reject(const Args& args) {
    return match(1, args);
}
TestResult match_timeout(const Args &args) {
    return match(2, args);
}

TestResult battle_pick(const Args &args) {
    if (args.args_type != Args::ArgsType::indexs)
        return TestResult{std::string("error args")};

    for (auto index : args.indexs) {
        auto& client = client_manager.client(index);
        client->battle_pick_hero(HeroName::merlin);
    }

    // 全部用户选择之后，等待收到的battle_load指令
    for (auto index : args.indexs) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::battle_start_load);
        if (!res.contains(header::type::battle_start_load))
            return TestResult{std::string("error header info")};
    }
    return TestResult{true};
}

TestResult disconnect(const Args &args) {
    if (args.args_type != Args::ArgsType::indexs)
        return TestResult{std::string("error args")};

    for (auto index : args.indexs) {
        client_manager.logout(index);
    }
    return TestResult{true};
}

TestResult reconnect(const Args &args) {
    if (args.args_type != Args::ArgsType::indexs)
        return TestResult{std::string("error args")};

    client_manager.login(args.indexs);

    for (auto index : args.indexs) {
        auto& client = client_manager.client(index);
        auto res = wait(client, header::type::login_true);
        if (!res.contains(header::type::login_true))
            return TestResult{std::string("error header info")};
    }
    // 登录成功之后还会有need_connect

    for (auto index : args.indexs) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::battle_need_reconnect);
        if (!res.contains(header::type::battle_need_reconnect))
            return TestResult{std::string("error header info")};
        client->battle_reconnect();
    }

    for (auto index : args.indexs) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::battle_snapshot);
        if (!res.contains(header::type::battle_snapshot))
            return TestResult{std::string("error header info")};
    }
    return TestResult{true};
}



int main(int argc, char* argv[]) {
    std::istream *in = &std::cin;
    std::ifstream in_file;
    if (argc != 1
        && std::filesystem::exists(argv[1])) {
        in_file = std::ifstream(argv[1]);
        in = &in_file;
    }

    std::map<std::string, std::function<TestResult (const Args&)>> rounter {
        { "add", add },
        { "login", login },
        { "room_create", room_create },
        { "room_invite.accept", room_invite_accept },
        { "room_invite.reject", room_invite_reject },
        { "room_chat", room_chat },
        { "match.accept", match_accept },
        { "match.reject", match_reject },
        { "match.timeout", match_timeout },
        { "battle.pick", battle_pick },
        { "disconnect", disconnect },
        { "reconnect", reconnect },
    };

    // in 已经完成统一
    Args args;
    while (*in >> args) {
        if (!rounter.contains(args.cmd))
            throw std::runtime_error(std::format("error command : {}", args.cmd));
        auto result = rounter[args.cmd](args);
        static int line{1};

        std::println("{} : {} {}", line++, result.string(), args.cmd);
        std::flush(std::cout);
        if (!result.is_pass) {
            return -1;
        }
    }
    return 0;
}
