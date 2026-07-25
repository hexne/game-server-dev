/********************************************************************************
* @Author : hexne
* @Date   : 2026/07/25 10:58:41
********************************************************************************/

module;
export module battle.map;
import std;
import pos;


enum class MapType : int {
    road,
    wall,
};

export class BattleMap {
    constexpr static int map_size = 1000;
    MapType data_[map_size * map_size]{};
    std::mdspan<MapType, std::extents<std::size_t, map_size, map_size>> map_{data_};
public:
    BattleMap() {  }

    std::vector<Pos> a_start(const Pos &start, const Pos &goal) {
         return {}; // 无路径
    }
};
