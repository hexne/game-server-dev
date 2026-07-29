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
    client_manager.add(args.args_to_int().front());
    return TestResult{true};
}

// 登录测试
// login                        全部登录
// login index1 index2          指定的index登录
// login index1-index2          [index1, index2]登录
// index 的范围和内容是可推测的
TestResult login(const Args &args) {
    std::vector<int> indexs{};
    if (args.args.empty()) {
        indexs = client_manager.all_index();
    }
    else if (auto pos = args.args.front().find("-"); pos != std::string::npos) {
        auto string = args.args.front();
        auto all_index = client_manager.all_index();
        std::ranges::sort(all_index);
        auto it = string.begin();
        int begin = std::stoi(std::string(it, it+pos));
        int end = std::stoi(std::string(it + pos + 1, string.end()));

        for (auto index : all_index) {
            if (index >= begin && index <= end)
                indexs.push_back(index);
        }
    }
    else {
        for (auto index : args.args_to_int())
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
        { "login", login }
    };

    // in 已经完成统一
    Args args;
    while (*in >> args) {
        if (!rounter.contains(args.cmd))
            throw std::runtime_error(std::format("error command : {}", args.cmd));
        auto result = rounter[args.cmd](args);
        static int line{1};

        std::println("{} : {}", line++, result.string());
        std::flush(std::cout);
        if (!result.is_pass) {
            return 0;
        }
    }
}
