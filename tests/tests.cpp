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

auto wait(std::unique_ptr<Client> &client) {
    std::unique_lock lock(test_mutex);
    test_cv.wait(lock, [] { return test_ready; });
    auto info = client->test_info();
    test_ready = false;
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
        auto res = wait(client);
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
        auto res = wait(client);
        if (res.contains(header::type::login_true))
            continue;
        return TestResult{std::string("room create false")};
    }
    return TestResult{true};
}


// room_invite index1 index2, index1 邀请index2加入房间
TestResult room_invite(bool is_accept, const Args& args) {

}

TestResult room_invite_accept(const Args& args) {
    return room_invite(true, args);
}
TestResult room_invite_reject(const Args& args) {
    return room_invite(false, args);
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
