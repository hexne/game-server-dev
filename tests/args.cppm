/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/28 18:34:42
********************************************************************************/

module;
export module args;
import std;

export struct Args {
    std::string cmd;
    enum class ArgsType {
        none, indexs, range
    } args_type;

    std::vector<int> indexs{};
    std::tuple<int, int> range{};

    friend std::istream& operator>>(std::istream& is, Args& out) {
        out.cmd.clear();
        out.indexs.clear();
        out.range = std::make_tuple(-1, -1);

        std::string line;
        if (!std::getline(is, line)) {
            return is;
        }

        std::istringstream iss(line);
        iss >> out.cmd;

        std::string arg;
        iss >> arg;
        // 空的
        if (arg.empty()) {
            out.args_type = ArgsType::none;
        }
        // range
        else if (arg.find('-') != std::string::npos) {
            out.args_type = ArgsType::range;

            auto it = arg.begin();
            auto pos = arg.find('-');

            int begin = std::stoi(std::string(it, it + pos));
            int end = std::stoi(std::string(it + pos + 1, arg.end()));
            out.range = std::make_tuple(begin, end);
        }
        else {
            out.args_type = ArgsType::indexs;
            out.indexs.push_back(std::stoi(arg));
            int index{};
            while (iss >> index) {
                out.indexs.push_back(index);
            }
        }

        return is;
    }
};

