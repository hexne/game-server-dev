/********************************************************************************
* @Author : hexne
* @Date   : 2026/05/15 16:32:04
********************************************************************************/

module;
export module game;
import std;


export class Game {
    int game_id_{};
    std::vector<int> users_id_{};
public:
    explicit Game(const int id) : game_id_(id) {  }

    // 对局开始
    void start() {  }

    // 对局中
    void update() {  }

    // 对局结束，结算
    void end() {  }

    ~Game() {

    }
};

export class GameList {
    std::unordered_map<int, Game> games_;
public:


};