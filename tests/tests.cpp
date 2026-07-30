/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/28 18:45:49
********************************************************************************/
import args;
import client;
import client_manager;
import std;
import message;

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
    client_manager.add(args.indexs.front());
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

    // 所有指定的用户都已经开始匹配
    for (auto &index : indexs) {
        auto &client = client_manager.client(index);
        auto res = wait(client, header::type::match_success);
        if (!res.contains(header::type::match_success)) {
            return TestResult{std::string("error header info")};
        }
    }

    // 全部都收到了匹配成功的通知
    // 如果是拒绝，就挑一个用户拒绝然后检查所有是否为match_cancel
    // 如果是接受，就全部接受然后检查是否开始
    // 如果是超时，就先挑一个不发消息，sleep,后面检查是否为 match_cancel

    // 接受
    if (type == 0) {
        for (auto index : indexs) {
            auto &client = client_manager.client(index);
            auto res = wait(client, header::type::battle_pick_hero);
            if (!res.contains(header::type::battle_pick_hero))
                return TestResult {std::string("error header info")};
        }
        return TestResult{true};
    }
    // 拒绝
    if (type == 1) {

    }
    // 超时
    if (type == 2) {

    }




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
        { "match_timeout", match_timeout },
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
            return 0;
        }
    }
}
