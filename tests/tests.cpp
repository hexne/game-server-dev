/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/28 18:45:49
********************************************************************************/
import args;
import client_manager;
import std;

ClientManager client_manager;

// 登录测试
void login(const Args &args) {

}

int main(int argc, char* argv[]) {
    std::istream *in = &std::cin;
    std::ifstream in_file;
    if (argc != 1
        && std::filesystem::exists(argv[1])) {
        in_file = std::ifstream(argv[1]);
        in = &in_file;
    }

    std::map<std::string, std::function<void (const Args&)>> rounter {
        { "login", login }
    };

    // in 已经完成统一
    Args args;
    while (*in >> args) {
        if (!rounter.contains(args.cmd))
            throw std::runtime_error("error test command");
        rounter[args.cmd](args);

        // 小睡一会
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}
