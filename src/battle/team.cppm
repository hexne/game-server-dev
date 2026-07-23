/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 23:23:47
********************************************************************************/

module;
export module team;
import std;

export class Team {
    std::vector<int> users_;
    std::vector<int> confirmed_;    // 0 - 100
public:
    explicit Team(std::vector<int> users) {
        users_ = users;
    }
    std::vector<int> users() {
        return users_;
    }
};