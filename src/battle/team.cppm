/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 23:23:47
********************************************************************************/

module;
export module team;
import std;
import hero;
import hero_factory;


export class Team {
    // <user_id, hero>
    std::unordered_map<int, std::shared_ptr<Hero>> users_{};
    std::unordered_map<int, int> confirmed_{};

public:
    // 一开始没有pick角色
    explicit Team(const std::vector<int>& users) {
        for (auto user_id : users) {
            users_[user_id] = nullptr;
            confirmed_[user_id] = 0;
        }
    }
    bool have_user(int user_id) {
        return users_.contains(user_id);
    }

    // 获取用户选择的英雄
    std::shared_ptr<Hero> get_player_hero(int user_id) {
        if (!users_.contains(user_id))
            return nullptr;
        return users_[user_id];
    }
    void user_pick_hero(int user_id, HeroName hero_name) {
        if (!users_.contains(user_id))
            return;

        users_[user_id] = HeroFactory::create_hero(hero_name);
    }

};
