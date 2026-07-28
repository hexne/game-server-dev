/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/28 18:34:42
********************************************************************************/

module;
export module args;
import std;

export struct Args {
    std::string cmd;
    std::vector<std::string> args;

    friend std::istream& operator>>(std::istream& is, Args& out) {
        out.cmd.clear();
        out.args.clear();

        std::string line;
        if (!std::getline(is, line)) {
            return is;
        }

        std::istringstream iss(line);
        iss >> out.cmd;

        std::string arg;
        while (iss >> arg) {
            out.args.push_back(arg);
        }

        return is;
    }
};

