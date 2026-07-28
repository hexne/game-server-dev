/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/23 23:23:47
********************************************************************************/

module;
export module team;
import std;
import hero;
import hero_factory;
import message;


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

    // 用户加载完成
    void user_loaded(int user_id) {
        if (!confirmed_.contains(user_id))
            return;
        confirmed_[user_id] = true;
    }

    bool all_players_picked() {
        for (auto &[user_id, hero] : users_) {
            if (!hero)
                return false;
        }
        return true;
    }

    bool all_players_loaded() {
        for (auto &[user_id, flag] : confirmed_) {
            if (flag != 100)
                return false;
        }
        return true;
    }

    std::vector<int> all_users() {
        std::vector<int> ret(users_.size());
        for (const auto &[user_id, _] : users_) {
            ret.push_back(user_id);
        }
        return ret;
    }

    void user_load(int user_id, int val) {
        if (!confirmed_.contains(user_id))
            return;
        confirmed_[user_id] = val;
    }

    // team.cppm
    char* serialize_filtered(char *buf, const std::unordered_set<int>& visible_ids) {
        int visible_count = 0;
        for (auto &[user_id, hero] : users_)
            if (hero && visible_ids.contains(user_id))
                visible_count++;

        buf = message::write(buf, visible_count); // 先写可见数量，客户端按这个数量循环读取
        for (auto &[user_id, hero] : users_) {
            if (!hero || !visible_ids.contains(user_id))
                continue;
            buf = message::write(buf, user_id);
            buf = hero->serialize(buf);
        }
        return buf;
    }

    char* serialize(char *buf) {
        char *new_pos = buf;
        for (auto &[user_id, hero] : users_) {
            new_pos = message::write(new_pos, user_id);
            new_pos = hero->serialize(new_pos);
        }
        return new_pos;
    }

    char* deserialize(char *buf) {
        char* new_pos = buf;
        for (int i = 0;i < users_.size(); i++) {
            int user_id = message::read(new_pos);
            auto hero = users_[user_id];
            new_pos = hero->deserialize(new_pos);
        }
        return new_pos;
    }

    // 移动所有hero的位置
    void update_all_hero_pos() {
        for (auto &[user_id, hero] : users_) {
            hero->update_pos();
        }
    }
    void update_all_hero_effects() {
        for (auto &[user_id, hero] : users_) {
            hero->update_effects();
        }
    }

    std::size_t serialize_size() const {
        constexpr int size = sizeof(int) + sizeof(Hero);
        return size * users_.size();
    }

    std::shared_ptr<Hero> hero(int user_id) {
        return users_[user_id];
    }
};
