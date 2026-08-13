#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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
    int rivalDist = 99;
    double expectedGold = 0.0;
    double objective = 0.0;
};

inline int id(int r, int c) {
    return r * W + c;
}

inline Pos fromId(int x) {
    return Pos{x / W, x % W};
}

inline int manhattan(const Pos& a, const Pos& b) {
    return std::abs(a.r - b.r) + std::abs(a.c - b.c);
}

inline int nearestCompetitorDistance(
    const Pos& target,
    const std::array<Pos, 9>& competitors,
    int begin,
    int end) {

    int best = 99;
    for (int i = begin; i < end; ++i) {
        best = std::min(best, manhattan(target, competitors[i]));
    }
    return best;
}

inline int nearestCompetitorDistance(
    const Pos& target,
    const std::array<Pos, 9>& competitors,
    int competitorCount) {
    return nearestCompetitorDistance(target, competitors, 0, competitorCount);
}

inline double targetUtility(int value) {
    const double v = static_cast<double>(value);
    return v * (1.0 + 0.015 * std::min(v, 30.0));
}

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
    const std::array<Pos, 9>& competitors,
    int competitorCount,
    int enemyCount,
    int maxSteps = 6,
    const std::unordered_set<Pos, PosHash>* ignoredTargets = nullptr) {

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
    int bestRivalDist = 99;
    double bestScore = -1.0;

    while (!q.empty()) {
        const int curId = q.front();
        q.pop();

        const Pos cur = fromId(curId);
        const int d = distance[curId];

        if (d > 0 && grid[cur.r][cur.c] >= 1 &&
            (ignoredTargets == nullptr ||
             ignoredTargets->find(cur) == ignoredTargets->end())) {
            const int value = grid[cur.r][cur.c];
            const int enemyDist =
                nearestCompetitorDistance(
                    cur, competitors, 0, enemyCount);
            const int npcDist =
                nearestCompetitorDistance(
                    cur, competitors, enemyCount, competitorCount);

            double competitionFactor = 1.0;

            if (enemyDist < d) {
                competitionFactor *= 0.15;
            } else if (enemyDist == d) {
                competitionFactor *= 0.28;
            } else if (enemyDist == d + 1) {
                competitionFactor *= 0.58;
            } else if (enemyDist == d + 2) {
                competitionFactor *= 0.82;
            }

            if (npcDist + 1 < d) {
                competitionFactor *= 0.38;
            } else if (npcDist < d) {
                competitionFactor *= 0.50;
            } else if (npcDist == d) {
                competitionFactor *= 0.66;
            } else if (npcDist == d + 1) {
                competitionFactor *= 0.84;
            }

            const int rivalDist = std::min(enemyDist, npcDist);
            const double utility = targetUtility(value);
            const double score =
                (utility / static_cast<double>(d + 1)) * competitionFactor
                + 0.02 * utility;

            if (!found || score > bestScore + 1e-12 ||
                (std::abs(score - bestScore) < 1e-12 &&
                 (value > bestValue ||
                  (value == bestValue && d < bestDist)))) {
                found = true;
                bestScore = score;
                bestValue = value;
                bestDist = d;
                bestId = curId;
                bestRivalDist = rivalDist;
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
    result.rivalDist = bestRivalDist;
    result.objective = bestScore;

    double captureFactor = 1.0;
    if (bestRivalDist < bestDist) captureFactor = 0.35;
    else if (bestRivalDist == bestDist) captureFactor = 0.55;
    else if (bestRivalDist == bestDist + 1) captureFactor = 0.80;
    result.expectedGold =
        0.65 * static_cast<double>(bestValue) * captureFactor;

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
                const int centerDist =
                    std::abs(cur.r - 8) + std::abs(cur.c - 8);
                const bool inCentral9 =
                    (cur.r >= 4 && cur.r <= 12 &&
                     cur.c >= 4 && cur.c <= 12);

                const double score =
                    3.0 * fogNeighbors
                    - 0.22 * d
                    - 0.10 * centerDist
                    + (inCentral9 ? 1.2 : 0.0);

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

struct RoutePlan {
    std::vector<int> actions;
    int collectedValue = 0;
    double expectedGold = 0.0;
    double objective = 0.0;
};

template <typename Grid>
RoutePlan buildRoute(
    const Grid& grid,
    Pos start,
    int budget,
    const std::unordered_set<Pos, PosHash>& baseBlocked,
    const std::array<Pos, 9>& competitors,
    int competitorCount,
    int enemyCount) {

    RoutePlan plan;
    if (budget <= 0) return plan;

    std::unordered_set<Pos, PosHash> blocked = baseBlocked;
    std::unordered_set<Pos, PosHash> claimedTargets;
    Pos cur = start;
    int remaining = budget;

    for (int chain = 0; chain < 4 && remaining > 0; ++chain) {
        SearchResult next =
            goldPath(
                grid, cur, blocked, competitors,
                competitorCount, enemyCount, remaining, &claimedTargets);

        if (next.actions.empty() || next.value <= 0) break;

        plan.actions.insert(
            plan.actions.end(), next.actions.begin(), next.actions.end());
        plan.collectedValue += next.value;
        plan.expectedGold += next.expectedGold;
        plan.objective += next.objective;
        remaining -= static_cast<int>(next.actions.size());
        cur = next.target;

        claimedTargets.insert(next.target);
    }

    if (remaining > 0) {
        std::vector<int> explore =
            frontierPath(grid, cur, blocked, remaining);
        if (static_cast<int>(explore.size()) > remaining) {
            explore.resize(remaining);
        }
        plan.actions.insert(
            plan.actions.end(), explore.begin(), explore.end());
    }

    if (static_cast<int>(plan.actions.size()) > budget) {
        plan.actions.resize(budget);
    }

    return plan;
}

inline int visibleGoldSum(const GameInput& game) {
    int total = 0;
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            if (game.grid[r][c] > 0) total += game.grid[r][c];
        }
    }
    return total;
}

inline int snapshotGoldRemaining(const GameInput& game) {
    if (!game.snapshot_valid) return 0;
    int total = 0;
    for (int i = 0; i < REGION_COUNT; ++i) {
        total += std::max(0, game.snapshot.regions[i].gold_remaining);
    }
    return total;
}

inline int chooseVision(const GameInput& game) {
    if (!game.snapshot_valid) return 0;

    const int carried =
        game.my_units_gold[0] + game.my_units_gold[1];
    const int visible = visibleGoldSum(game);
    const int remaining = snapshotGoldRemaining(game);
    const int deficit = game.gold_opp - carried;

    if (carried < 20) return 0;

    if (visible == 0 && remaining >= 120) {
        return 1;
    }

    if (visible <= 2 && remaining >= 90 && deficit >= 80) {
        return 1;
    }

    return 0;
}

} // namespace

extern "C" GameOutput moveDecision(const GameInput* input) {
    GameOutput output{};

    const GameInput& game = *input;

    Pos starts[2] = {
        {game.my_units[0].row, game.my_units[0].col},
        {game.my_units[1].row, game.my_units[1].col}
    };

    std::unordered_set<Pos, PosHash> enemyCells;
    std::array<Pos, 9> competitors{};
    int competitorCount = 0;

    for (int i = 0; i < 2; ++i) {
        const int r = game.visible_enemies[i].row;
        const int c = game.visible_enemies[i].col;
        if (r >= 0 && c >= 0) {
            const Pos p{r, c};
            enemyCells.insert(p);
            competitors[competitorCount++] = p;
        }
    }

    const int enemyCount = competitorCount;

    for (int i = 0;
         i < game.num_visible_npcs &&
         i < MAX_NPCS &&
         competitorCount < static_cast<int>(competitors.size());
         ++i) {
        const int r = game.visible_npcs[i].pos.row;
        const int c = game.visible_npcs[i].pos.col;
        if (game.visible_npcs[i].id != 0 && r >= 0 && c >= 0) {
            competitors[competitorCount++] = Pos{r, c};
        }
    }

    struct Candidate {
        RoutePlan plans[2];
        int k = 3;
        int order = 0;
        double score = -1e100;
    };

    Candidate best;

    for (int k = 0; k <= STEPS; ++k) {
        const int budget[2] = {k, STEPS - k};

        for (int order = 0; order < 2; ++order) {
            const int firstUnit = order;
            const int secondUnit = 1 - order;

            Candidate cand;
            cand.k = k;
            cand.order = order;

            std::unordered_set<Pos, PosHash> firstBlocked = enemyCells;
            firstBlocked.insert(starts[secondUnit]);

            cand.plans[firstUnit] =
                buildRoute(
                    game.grid,
                    starts[firstUnit],
                    budget[firstUnit],
                    firstBlocked,
                    competitors,
                    competitorCount,
                    enemyCount);

            std::unordered_set<Pos, PosHash> secondBlocked = enemyCells;

            const std::vector<Pos> firstPath =
                pathPositions(
                    starts[firstUnit],
                    cand.plans[firstUnit].actions);

            if (!firstPath.empty()) {
                secondBlocked.insert(firstPath.back());
            } else {
                secondBlocked.insert(starts[firstUnit]);
            }

            cand.plans[secondUnit] =
                buildRoute(
                    game.grid,
                    starts[secondUnit],
                    budget[secondUnit],
                    secondBlocked,
                    competitors,
                    competitorCount,
                    enemyCount);

            const double expected =
                cand.plans[0].expectedGold +
                cand.plans[1].expectedGold;

            const double targetQuality =
                cand.plans[0].objective +
                cand.plans[1].objective;

            const int used =
                static_cast<int>(cand.plans[0].actions.size()) +
                static_cast<int>(cand.plans[1].actions.size());

            cand.score =
                expected
                + 0.08 * targetQuality
                + 0.015 * static_cast<double>(used);

            const int candImbalance =
                std::abs(k - (STEPS - k));
            const int bestImbalance =
                std::abs(best.k - (STEPS - best.k));

            if (cand.score > best.score + 1e-12 ||
                (std::abs(cand.score - best.score) <= 1e-12 &&
                 candImbalance < bestImbalance)) {
                best = std::move(cand);
            }
        }
    }

    const int bestK = best.k;
    RoutePlan finalPlans[2] = {
        std::move(best.plans[0]),
        std::move(best.plans[1])
    };
    const int execFirst = best.order;

    for (int i = 0; i < STEPS; ++i) {
        output.actions[i] = 4;
    }

    for (int i = 0;
         i < bestK &&
         i < static_cast<int>(finalPlans[0].actions.size());
         ++i) {
        output.actions[i] = finalPlans[0].actions[i];
    }

    for (int j = 0;
         j < STEPS - bestK &&
         j < static_cast<int>(finalPlans[1].actions.size());
         ++j) {
        output.actions[bestK + j] = finalPlans[1].actions[j];
    }

    output.k = bestK;
    output.order = execFirst;
    output.vp = chooseVision(game);

    return output;
}
