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

struct Node {
    Pos pos{};
    int route_cost{};              // 路程：起点到当前节点
    int estimated_cost_to_goal{};  // 预计：当前节点到目标的估价
    bool operator<(const Node &r) const {
        return (route_cost + estimated_cost_to_goal) >
               (r.route_cost + r.estimated_cost_to_goal);
    }
};

export class BattleMap {
    constexpr static int map_size = 1000;
    MapType data_[map_size * map_size]{};
    std::mdspan<MapType, std::extents<std::size_t, map_size, map_size>> map_{data_};


    static int distance(const Pos &start, const Pos &goal) {
        return std::abs(start.x - goal.x) + std::abs(start.y - goal.y);
    }
public:
    BattleMap() {  }

    std::vector<Pos> a_start(const Pos &start, const Pos &goal) {
        std::priority_queue<Node> queue_;
        queue_.push(Node{start, 0, distance(start, goal)});

        bool visited[map_size][map_size]{false};
        Pos came_from[map_size][map_size]; // 用来回溯路径
        for (int i = 0; i < map_size; ++i)
            for (int j = 0; j < map_size; ++j)
                came_from[i][j] = {-1, -1};

        const int dx[] = {1, -1, 0, 0};
        const int dy[] = {0, 0, 1, -1};

        while (!queue_.empty()) {
            Node cur = queue_.top();
            queue_.pop();

            if (visited[cur.pos.x][cur.pos.y])
                continue;
            visited[cur.pos.x][cur.pos.y] = true;

            if (cur.pos.x == goal.x && cur.pos.y == goal.y) {
                // 回溯路径
                std::vector<Pos> path;
                for (Pos p = goal; p.x != -1; p = came_from[p.x][p.y])
                    path.push_back(p);
                std::reverse(path.begin(), path.end());
                return path;
            }

            // 扩展邻居
            for (int i = 0; i < 4; ++i) {
                Pos next{cur.pos.x + dx[i], cur.pos.y + dy[i]};
                if (next.x < 0 || next.y < 0 || next.x >= map_size || next.y >= map_size)
                    continue;
                if (map_[next.x, next.y] == MapType::wall)
                    continue;
                if (visited[next.x][next.y])
                    continue;

                came_from[next.x][next.y] = cur.pos;
                int new_route_cost = cur.route_cost + 1;
                int new_estimated_cost = distance(next, goal);
                queue_.push(Node{next, new_route_cost, new_estimated_cost});
            }
        }

        return {}; // 无路径
    }

};
