/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/20 16:16:35
********************************************************************************/

module;
export module id_generator;

import std;

export template<typename Tag>
class ID {
    std::atomic<int> id{1};
public:
    int next() {
        return id.fetch_add(1, std::memory_order_relaxed);
    }
};

struct RoomTag  {};
struct MatchTag {};
struct PendingMatchTag {};
struct BattleTag  {};

export using RoomIDGenerator = ID<RoomTag>;
export using MatchIDGenerator = ID<MatchTag>;
export using PendingMatchIDGenerator = ID<PendingMatchTag>;
export using BattleIDGenerator = ID<BattleTag>;

