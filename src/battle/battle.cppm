/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 19:21:38
********************************************************************************/

module;
export module battle;
import std;
import team;
export class Battle {
    int id_;
    Team team_a_, team_b_;
public:
    Battle(const int id, Team team_a, Team team_b)
        : id_(id), team_a_(std::move(team_a)), team_b_(std::move(team_b)) {
    }

};

