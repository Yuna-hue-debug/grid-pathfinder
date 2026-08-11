#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>
#include "game_api.h"

// ============================================================
// GoldRush 2.0 - V2 Fast Agent (C++)
//
// This file is the C++ translation of the current Python V2 agent.
// It assumes the organizer's GameInput / GameOutput / Position /
// NpcInfo / Snapshot / RegionStat definitions are available from
// the competition template/header.
//
// If the template already declares moveDecision(), keep the body
// below and adapt only the exact function signature required by the
// organizer.
// ============================================================

namespace {

constexpr int H = 17;
constexpr int W = 17;
constexpr int STEPS = 6;
constexpr int FOG = -5;
constexpr int BOMB = -3;
constexpr int OBSTACLE = -1;

// 0 = UP, 1 = DOWN, 2 = LEFT, 3 = RIGHT, 4 = STAY
constexpr int DR[4] = {-1, 1, 0, 0};
constexpr int DC[4] = {0, 0, -1, 1};

struct Pos {
    int r;
    int c;

    bool operator==(const Pos& other) const {
        return r == other.r && c == other.c;
    }
};

struct PosHash {
    std::size_t operator()(const Pos& p) const noexcept {
        return static_cast<std::size_t>(p.r * W + p.c);
    }
};

inline bool inBounds(int r, int c) {
    return r >= 0 && r < H && c >= 0 && c < W;
}

inline bool samePos(const Pos& a, const Pos& b) {
    return a.r == b.r && a.c == b.c;
}

// The organizer's grid uses -5 for fog and -1 for obstacle.
// V2 deliberately plans only through currently visible cells.
template <typename Grid>
bool knownWalkable(const Grid& grid, int r, int c,
                   const std::unordered_set<Pos, PosHash>& blocked,
                   bool avoidBomb = true) {
    if (!inBounds(r, c)) return false;
    if (grid[r][c] == FOG || grid[r][c] == OBSTACLE) return false;
    if (avoidBomb && grid[r][c] == BOMB) return false;
    if (blocked.find(Pos{r, c}) != blocked.end()) return false;
    return true;
}

struct SearchResult {
    std::vector<int> actions;
    int value = 0;
    Pos target{-1, -1};
};

// Encode a position as an integer for tiny fixed-size BFS arrays.
inline int id(int r, int c) {
    return r * W + c;
}

inline Pos fromId(int x) {
    return Pos{x / W, x % W};
}

// Reconstruct actions from parent arrays.
std::vector<int> reconstruct(const std::array<int, H * W>& parent,
                             const std::array<int, H * W>& parentAction,
                             int startId, int targetId) {
    std::vector<int> actions;
    int cur = targetId;

    while (cur != startId && cur >= 0) {
        actions.push_back(parentAction[cur]);
        cur = parent[cur];
    }

    std::reverse(actions.begin(), actions.end());
    return actions;
}

template <typename Grid>
SearchResult goldPath(
    const Grid& grid,
    Pos start,
    const std::unordered_set<Pos, PosHash>& blocked,
    int maxSteps = 6) {

    std::array<int, H * W> parent;
    std::array<int, H * W> parentAction;
    std::array<int, H * W> distance;
    parent.fill(-1);
    parentAction.fill(-1);
    distance.fill(-1);

    std::queue<int> q;
    const int startId = id(start.r, start.c);
    q.push(startId);
    distance[startId] = 0;
    parent[startId] = startId;

    bool found = false;
    int bestValue = 0;
    int bestDist = 1;
    int bestId = -1;
    double bestScore = -1.0;

    while (!q.empty()) {
        const int curId = q.front();
        q.pop();

        const Pos cur = fromId(curId);
        const int d = distance[curId];

        if (d > 0 && grid[cur.r][cur.c] >= 1) {
            const int value = grid[cur.r][cur.c];
            const double score = static_cast<double>(value) / (d+1);

            if (!found || score > bestScore ||
                (std::abs(score - bestScore) < 1e-12 && value > bestValue)) {
                found = true;
                bestScore = score;
                bestValue = value;
                bestDist = d;
                bestId = curId;
            }
        }

        if (d == maxSteps) continue;

        for (int action = 0; action < 4; ++action) {
            const int nr = cur.r + DR[action];
            const int nc = cur.c + DC[action];
            if (!inBounds(nr, nc)) continue;

            const int nxtId = id(nr, nc);
            if (distance[nxtId] != -1) continue;
            if (!knownWalkable(grid, nr, nc, blocked)) continue;

            distance[nxtId] = d + 1;
            parent[nxtId] = curId;
            parentAction[nxtId] = action;
            q.push(nxtId);
        }
    }

    if (!found) return {};

    SearchResult result;
    result.actions = reconstruct(parent, parentAction, startId, bestId);
    result.value = bestValue;
    result.target = fromId(bestId);
    (void)bestDist;
    return result;
}

template <typename Grid>
std::vector<int> frontierPath(
    const Grid& grid,
    Pos start,
    const std::unordered_set<Pos, PosHash>& blocked,
    int maxSteps = 6) {

    std::array<int, H * W> parent;
    std::array<int, H * W> parentAction;
    std::array<int, H * W> distance;
    parent.fill(-1);
    parentAction.fill(-1);
    distance.fill(-1);

    std::queue<int> q;
    const int startId = id(start.r, start.c);
    q.push(startId);
    distance[startId] = 0;
    parent[startId] = startId;

    bool found = false;
    double bestScore = -1e100;
    int bestId = -1;

    while (!q.empty()) {
        const int curId = q.front();
        q.pop();
        const Pos cur = fromId(curId);
        const int d = distance[curId];

        if (d > 0) {
            int fogNeighbors = 0;
            for (int a = 0; a < 4; ++a) {
                const int nr = cur.r + DR[a];
                const int nc = cur.c + DC[a];
                if (inBounds(nr, nc) && grid[nr][nc] == FOG) {
                    ++fogNeighbors;
                }
            }

            if (fogNeighbors > 0) {
                const double score = 3.0 * fogNeighbors - 0.25 * d;
                if (!found || score > bestScore) {
                    found = true;
                    bestScore = score;
                    bestId = curId;
                }
            }
        }

        if (d == maxSteps) continue;

        for (int action = 0; action < 4; ++action) {
            const int nr = cur.r + DR[action];
            const int nc = cur.c + DC[action];
            if (!inBounds(nr, nc)) continue;

            const int nxtId = id(nr, nc);
            if (distance[nxtId] != -1) continue;
            if (!knownWalkable(grid, nr, nc, blocked)) continue;

            distance[nxtId] = d + 1;
            parent[nxtId] = curId;
            parentAction[nxtId] = action;
            q.push(nxtId);
        }
    }

    if (!found) return {};
    return reconstruct(parent, parentAction, startId, bestId);
}

std::vector<Pos> pathPositions(Pos start, const std::vector<int>& actions) {
    std::vector<Pos> result;
    result.reserve(actions.size());

    Pos cur = start;
    for (int action : actions) {
        if (action >= 4) continue;
        cur.r += DR[action];
        cur.c += DC[action];
        result.push_back(cur);
    }
    return result;
}

} // namespace

// ============================================================
// Main strategy function
// ============================================================
// IMPORTANT:
// The exact declaration below should match the organizer's C++ template.
// Based on the published interface shown in the competition rules, it is:
//
// extern "C" GameOutput moveDecision(const GameInput* input)
//
// If the provided starter code already contains this declaration, replace
// only the function body with the implementation below.
// ============================================================

extern "C" GameOutput moveDecision(const GameInput* input) {
    GameOutput output{};

    const GameInput& game = *input;

    Pos starts[2] = {
        {game.my_units[0].row, game.my_units[0].col},
        {game.my_units[1].row, game.my_units[1].col}
    };

    std::unordered_set<Pos, PosHash> enemyCells;
    for (int i = 0; i < 2; ++i) {
        const int r = game.visible_enemies[i].row;
        const int c = game.visible_enemies[i].col;
        if (r >= 0 && c >= 0) {
            enemyCells.insert(Pos{r, c});
        }
    }

    std::vector<int> tempPaths[2];
    int tempValues[2] = {0, 0};

    for (int i = 0; i < 2; ++i) {
        SearchResult result = goldPath(game.grid, starts[i], enemyCells, 6);
        tempPaths[i] = std::move(result.actions);
        tempValues[i] = result.value;

        if (tempPaths[i].empty()) {
            tempPaths[i] = frontierPath(game.grid, starts[i], enemyCells, 6);
            tempValues[i] = 0;
        }
    }

    auto opportunityScore = [&](int idx) -> double {
        if (tempValues[idx] <= 0 || tempPaths[idx].empty()) return 0.0;
        return static_cast<double>(tempValues[idx]) /
               static_cast<double>(tempPaths[idx].size());
    };

    const int first = (opportunityScore(0) >= opportunityScore(1)) ? 0 : 1;
    const int second = 1 - first;

    std::vector<int> paths[2];
    int values[2] = {0, 0};

    paths[first] = tempPaths[first];
    values[first] = tempValues[first];

    std::unordered_set<Pos, PosHash> blocked = enemyCells;
    for (const Pos& p : pathPositions(starts[first], paths[first])) {
        blocked.insert(p);
    }

    SearchResult secondResult = goldPath(game.grid, starts[second], blocked, 6);
    paths[second] = std::move(secondResult.actions);
    values[second] = secondResult.value;

    if (paths[second].empty()) {
        paths[second] = frontierPath(game.grid, starts[second], blocked, 6);
        values[second] = 0;
    }

    // --------------------------------------------------------
    // V2.1: dynamic k allocation
    //
    // k = number of actions allocated to unit 0.
    // Unit 1 receives 6-k actions.
    //
    // Instead of using fixed heuristic weights (0.65/0.03/etc.),
    // estimate the value captured by each unit according to how
    // much of its planned path can actually be executed.
    // --------------------------------------------------------
    auto allocationValue = [&](int idx, int allocatedSteps) -> double {
        if (allocatedSteps <= 0 || paths[idx].empty()) {
            return 0.0;
        }

        const int pathLength = static_cast<int>(paths[idx].size());
        const int executed = std::min(pathLength, allocatedSteps);

        if (values[idx] > 0) {
            // If the whole path fits, assume the target gold is collected.
            // Otherwise, estimate partial value linearly by path progress.
            return static_cast<double>(values[idx]) *
                   static_cast<double>(executed) /
                   static_cast<double>(pathLength);
        }

        // No visible gold target: give a small exploration value to
        // actually executable frontier movement.
        return 0.20 * static_cast<double>(executed);
    };

    int bestK = 3;
    double bestScore = -1e100;

    for (int k = 0; k <= 6; ++k) {
        const int n0 = k;
        const int n1 = 6 - k;

        const double score =
            allocationValue(0, n0) +
            allocationValue(1, n1);

        std::cout << "k=" << k
          << " | value0=" << values[0]
          << " path0=" << paths[0].size()
          << " | value1=" << values[1]
          << " path1=" << paths[1].size()
          << " | score=" << score
          << std::endl;
        // Deterministic tie-break: prefer the more balanced allocation.
        const int imbalance = std::abs(n0 - n1);
        const int bestImbalance = std::abs(bestK - (6 - bestK));

        if (score > bestScore + 1e-12 ||
            (std::abs(score - bestScore) <= 1e-12 &&
             imbalance < bestImbalance)) {
            bestScore = score;
            bestK = k;
        }
    }
    std::cout << "BEST K="
          << bestK
          << " | BEST SCORE="
          << bestScore
          << std::endl;
    for (int i = 0; i < STEPS; ++i) output.actions[i] = 4; // STAY

    for (int i = 0; i < bestK && i < static_cast<int>(paths[0].size()); ++i) {
        output.actions[i] = paths[0][i];
    }

    for (int j = 0; j < (6 - bestK) &&
                     j < static_cast<int>(paths[1].size()); ++j) {
        output.actions[bestK + j] = paths[1][j];
    }

    output.k = bestK;
    output.order = (values[0] >= values[1]) ? 0 : 1;

    const int totalGold = game.my_units_gold[0] + game.my_units_gold[1];
    output.vp = (game.snapshot_valid && totalGold >= 2) ? 1 : 0;

    return output;
}
