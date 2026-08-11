#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Grid pathfinding utilities and a compact GoldRush-style decision agent.
// This C++ version mirrors the Python prototype while keeping the original
// beginner demo runnable from main().

namespace pathfinder {

constexpr int FOG = -5;
constexpr int BOMB = -3;
constexpr int OBSTACLE = -1;
constexpr int EMPTY = 0;
constexpr int STEPS = 6;

// Action encoding: 0 = UP, 1 = DOWN, 2 = LEFT, 3 = RIGHT, 4 = STAY.
enum Action { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, STAY = 4 };

constexpr std::array<int, 4> DR{-1, 1, 0, 0};
constexpr std::array<int, 4> DC{0, 0, -1, 1};
constexpr std::array<int, 4> ACTIONS{UP, DOWN, LEFT, RIGHT};

struct Position {
    int row = 0;
    int col = 0;

    bool operator==(const Position& other) const noexcept {
        return row == other.row && col == other.col;
    }

    bool operator!=(const Position& other) const noexcept {
        return !(*this == other);
    }
};

struct PositionHash {
    std::size_t operator()(const Position& pos) const noexcept {
        return (static_cast<std::size_t>(pos.row) << 32) ^
               static_cast<std::size_t>(pos.col);
    }
};

using Grid = std::vector<std::vector<int>>;
using BlockedSet = std::unordered_set<Position, PositionHash>;

struct SearchResult {
    std::vector<Position> path;
    std::vector<int> actions;
    int distance = 0;
    int value = 0;
    std::optional<Position> target;
};

struct Decision {
    std::vector<int> actions;
    int k = STEPS / 2;
    int order = 0;
    std::array<std::vector<int>, 2> paths;
    std::array<int, 2> values{0, 0};
};

struct BfsResult {
    std::unordered_map<Position, int, PositionHash> distance;
    std::unordered_map<Position, Position, PositionHash> parent;
};

bool inBounds(const Grid& grid, Position pos) {
    return !grid.empty() && !grid.front().empty() && pos.row >= 0 &&
           pos.row < static_cast<int>(grid.size()) && pos.col >= 0 &&
           pos.col < static_cast<int>(grid.front().size());
}

bool isWalkable(const Grid& grid,
                Position pos,
                const BlockedSet& blocked = {},
                bool avoidBombs = true,
                bool allowFog = false) {
    if (!inBounds(grid, pos)) return false;

    const int cell = grid[pos.row][pos.col];
    if (cell == OBSTACLE) return false;
    if (!allowFog && cell == FOG) return false;
    if (avoidBombs && cell == BOMB) return false;
    return blocked.find(pos) == blocked.end();
}

int actionBetween(Position current, Position next) {
    const int dr = next.row - current.row;
    const int dc = next.col - current.col;
    for (int i = 0; i < 4; ++i) {
        if (dr == DR[i] && dc == DC[i]) return ACTIONS[i];
    }
    return STAY;
}

std::vector<int> pathToActions(const std::vector<Position>& path) {
    std::vector<int> actions;
    if (path.size() < 2) return actions;

    actions.reserve(path.size() - 1);
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        actions.push_back(actionBetween(path[i], path[i + 1]));
    }
    return actions;
}

std::vector<Position> reconstructPath(
    const std::unordered_map<Position, Position, PositionHash>& parent,
    Position start,
    Position target) {
    std::vector<Position> path;
    Position current = target;
    path.push_back(current);

    while (current != start) {
        auto it = parent.find(current);
        if (it == parent.end()) return {};
        current = it->second;
        path.push_back(current);
    }

    std::reverse(path.begin(), path.end());
    return path;
}

BfsResult bfsAll(const Grid& grid,
                 Position start,
                 const BlockedSet& blocked = {},
                 int maxSteps = -1,
                 bool avoidBombs = true,
                 bool allowFog = false) {
    BfsResult result;
    if (!inBounds(grid, start)) return result;

    std::deque<Position> queue;
    queue.push_back(start);
    result.distance[start] = 0;
    result.parent[start] = start;

    while (!queue.empty()) {
        const Position current = queue.front();
        queue.pop_front();
        const int currentDistance = result.distance[current];

        if (maxSteps >= 0 && currentDistance >= maxSteps) continue;

        for (int action = 0; action < 4; ++action) {
            const Position next{current.row + DR[action], current.col + DC[action]};
            if (result.distance.find(next) != result.distance.end()) continue;
            if (!isWalkable(grid, next, blocked, avoidBombs, allowFog)) continue;

            result.distance[next] = currentDistance + 1;
            result.parent[next] = current;
            queue.push_back(next);
        }
    }

    return result;
}

int cellValue(int cell) {
    return cell > 0 ? cell : 0;
}

std::optional<SearchResult> findTargetPath(const Grid& grid,
                                           Position start,
                                           Position target,
                                           const BlockedSet& blocked = {},
                                           int maxSteps = -1) {
    BfsResult bfs = bfsAll(grid, start, blocked, maxSteps);
    auto distIt = bfs.distance.find(target);
    if (distIt == bfs.distance.end()) return std::nullopt;

    SearchResult result;
    result.path = reconstructPath(bfs.parent, start, target);
    result.actions = pathToActions(result.path);
    result.distance = distIt->second;
    result.value = cellValue(grid[target.row][target.col]);
    result.target = target;
    return result;
}

std::optional<SearchResult> bestGoldPath(const Grid& grid,
                                         Position start,
                                         const BlockedSet& blocked = {},
                                         int maxSteps = STEPS) {
    BfsResult bfs = bfsAll(grid, start, blocked, maxSteps);

    bool found = false;
    double bestScore = -1.0;
    int bestValue = 0;
    int bestDistance = std::numeric_limits<int>::max();
    Position bestTarget{};

    for (const auto& [position, distance] : bfs.distance) {
        if (distance == 0) continue;

        const int value = cellValue(grid[position.row][position.col]);
        if (value <= 0) continue;

        const double score = static_cast<double>(value) / (distance + 1);
        const bool better = !found || score > bestScore + 1e-12 ||
                            (std::abs(score - bestScore) <= 1e-12 &&
                             (value > bestValue ||
                              (value == bestValue && distance < bestDistance)));
        if (better) {
            found = true;
            bestScore = score;
            bestValue = value;
            bestDistance = distance;
            bestTarget = position;
        }
    }

    if (!found) return std::nullopt;

    SearchResult result;
    result.path = reconstructPath(bfs.parent, start, bestTarget);
    result.actions = pathToActions(result.path);
    result.distance = bestDistance;
    result.value = bestValue;
    result.target = bestTarget;
    return result;
}

std::optional<SearchResult> frontierPath(const Grid& grid,
                                         Position start,
                                         const BlockedSet& blocked = {},
                                         int maxSteps = STEPS) {
    BfsResult bfs = bfsAll(grid, start, blocked, maxSteps);

    bool found = false;
    double bestScore = -std::numeric_limits<double>::infinity();
    int bestDistance = std::numeric_limits<int>::max();
    Position bestTarget{};

    for (const auto& [position, distance] : bfs.distance) {
        if (distance == 0) continue;

        int fogNeighbors = 0;
        for (int action = 0; action < 4; ++action) {
            const Position next{position.row + DR[action], position.col + DC[action]};
            if (inBounds(grid, next) && grid[next.row][next.col] == FOG) {
                ++fogNeighbors;
            }
        }
        if (fogNeighbors <= 0) continue;

        const double score = 3.0 * fogNeighbors - 0.25 * distance;
        if (!found || score > bestScore + 1e-12 ||
            (std::abs(score - bestScore) <= 1e-12 && distance < bestDistance)) {
            found = true;
            bestScore = score;
            bestDistance = distance;
            bestTarget = position;
        }
    }

    if (!found) return std::nullopt;

    SearchResult result;
    result.path = reconstructPath(bfs.parent, start, bestTarget);
    result.actions = pathToActions(result.path);
    result.distance = bestDistance;
    result.value = 0;
    result.target = bestTarget;
    return result;
}

std::vector<Position> positionsAfterActions(Position start, const std::vector<int>& actions) {
    std::vector<Position> positions;
    positions.reserve(actions.size());

    Position current = start;
    for (int action : actions) {
        if (action == STAY) continue;
        current.row += DR[action];
        current.col += DC[action];
        positions.push_back(current);
    }
    return positions;
}

SearchResult planUnit(const Grid& grid,
                      Position start,
                      const BlockedSet& blocked = {},
                      int maxSteps = STEPS) {
    if (auto gold = bestGoldPath(grid, start, blocked, maxSteps)) return *gold;
    if (auto frontier = frontierPath(grid, start, blocked, maxSteps)) return *frontier;
    return SearchResult{{start}, {}, 0, 0, std::nullopt};
}

int chooseAllocation(const std::array<std::vector<int>, 2>& paths,
                     const std::array<int, 2>& values,
                     int steps = STEPS) {
    auto allocationValue = [&](int index, int allocatedSteps) -> double {
        if (allocatedSteps <= 0 || paths[index].empty()) return 0.0;

        const int pathLength = static_cast<int>(paths[index].size());
        const int executed = std::min(pathLength, allocatedSteps);
        if (values[index] > 0) {
            return static_cast<double>(values[index]) * executed / pathLength;
        }
        return 0.20 * executed;
    };

    int bestK = steps / 2;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (int k = 0; k <= steps; ++k) {
        const double score = allocationValue(0, k) + allocationValue(1, steps - k);
        const int imbalance = std::abs(k - (steps - k));
        const int bestImbalance = std::abs(bestK - (steps - bestK));

        if (score > bestScore + 1e-12 ||
            (std::abs(score - bestScore) <= 1e-12 && imbalance < bestImbalance)) {
            bestScore = score;
            bestK = k;
        }
    }

    return bestK;
}

Decision decideTurn(const Grid& grid,
                    const std::array<Position, 2>& starts,
                    const BlockedSet& enemies = {},
                    int steps = STEPS) {
    BlockedSet enemyCells;
    for (const Position& enemy : enemies) {
        if (inBounds(grid, enemy)) enemyCells.insert(enemy);
    }

    std::array<SearchResult, 2> plans{
        planUnit(grid, starts[0], enemyCells, steps),
        planUnit(grid, starts[1], enemyCells, steps),
    };

    auto opportunity = [](const SearchResult& result) -> double {
        if (result.value <= 0 || result.actions.empty()) return 0.0;
        return static_cast<double>(result.value) / result.actions.size();
    };

    const int first = opportunity(plans[0]) >= opportunity(plans[1]) ? 0 : 1;
    const int second = 1 - first;

    BlockedSet blocked = enemyCells;
    for (const Position& pos : positionsAfterActions(starts[first], plans[first].actions)) {
        blocked.insert(pos);
    }
    plans[second] = planUnit(grid, starts[second], blocked, steps);

    Decision decision;
    decision.paths = {plans[0].actions, plans[1].actions};
    decision.values = {plans[0].value, plans[1].value};
    decision.k = chooseAllocation(decision.paths, decision.values, steps);
    decision.order = decision.values[0] >= decision.values[1] ? 0 : 1;
    decision.actions.assign(steps, STAY);

    for (int i = 0; i < decision.k && i < static_cast<int>(decision.paths[0].size()); ++i) {
        decision.actions[i] = decision.paths[0][i];
    }

    const int unitOneSteps = steps - decision.k;
    for (int j = 0; j < unitOneSteps && j < static_cast<int>(decision.paths[1].size()); ++j) {
        decision.actions[decision.k + j] = decision.paths[1][j];
    }

    return decision;
}

std::string actionName(int action) {
    switch (action) {
        case UP:
            return "UP";
        case DOWN:
            return "DOWN";
        case LEFT:
            return "LEFT";
        case RIGHT:
            return "RIGHT";
        default:
            return "STAY";
    }
}

void printActions(const std::vector<int>& actions) {
    std::cout << '[';
    for (std::size_t i = 0; i < actions.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << actionName(actions[i]);
    }
    std::cout << ']';
}

}  // namespace pathfinder

int main() {
    using namespace pathfinder;

    Grid grid = {
        {0, 0, 0, OBSTACLE, 0},
        {0, OBSTACLE, 0, OBSTACLE, 0},
        {0, 0, 0, 0, 0},
        {OBSTACLE, 0, OBSTACLE, 1, 0},
        {0, 0, 0, 0, 0},
    };

    const auto result = findTargetPath(grid, Position{0, 0}, Position{3, 3});
    if (!result) {
        std::cout << "未找到金币\n";
        return 0;
    }

    std::cout << "找到金币\n";
    std::cout << "距离：" << result->distance << '\n';
    std::cout << "路径：";
    for (const Position& pos : result->path) {
        std::cout << " (" << pos.row << "," << pos.col << ")";
    }
    std::cout << "\n操作：";
    printActions(result->actions);
    std::cout << '\n';

    return 0;
}
