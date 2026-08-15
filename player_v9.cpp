#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <queue>
#include <unordered_set>
#include <vector>
#include "game_api.h"

// ============================================================
// GoldRush 2.0 - V9 Challenger (V8 baseline + log-driven fixes)
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
    std::size_t operator()(const Pos& p) const {
        return static_cast<std::size_t>(p.r * 31 + p.c);
    }
};

bool inBounds(int r, int c) {
    return r >= 0 && r < H && c >= 0 && c < W;
}

int manhattan(const Pos& a, const Pos& b) {
    return std::abs(a.r - b.r) + std::abs(a.c - b.c);
}

int dirFromTo(const Pos& a, const Pos& b) {
    if (b.r == a.r - 1 && b.c == a.c) return 0;
    if (b.r == a.r + 1 && b.c == a.c) return 1;
    if (b.r == a.r && b.c == a.c - 1) return 2;
    if (b.r == a.r && b.c == a.c + 1) return 3;
    return 4;
}

bool isWalkable(int cell) {
    return cell != OBSTACLE && cell != BOMB && cell != FOG;
}

std::vector<Pos> bfsPath(
    const int grid[H][W],
    const Pos& start,
    const Pos& target,
    const std::unordered_set<Pos, PosHash>& blocked) {

    if (start == target) return {start};

    std::array<std::array<bool, W>, H> visited{};
    std::array<std::array<Pos, W>, H> parent{};
    std::queue<Pos> q;

    visited[start.r][start.c] = true;
    q.push(start);

    bool found = false;
    while (!q.empty() && !found) {
        Pos cur = q.front();
        q.pop();

        for (int d = 0; d < 4; ++d) {
            Pos nxt{cur.r + DR[d], cur.c + DC[d]};
            if (!inBounds(nxt.r, nxt.c)) continue;
            if (visited[nxt.r][nxt.c]) continue;
            if (!isWalkable(grid[nxt.r][nxt.c])) continue;
            if (!(nxt == target) && blocked.find(nxt) != blocked.end()) continue;

            visited[nxt.r][nxt.c] = true;
            parent[nxt.r][nxt.c] = cur;
            q.push(nxt);

            if (nxt == target) {
                found = true;
                break;
            }
        }
    }

    if (!found) return {};

    std::vector<Pos> rev;
    Pos cur = target;
    rev.push_back(cur);
    while (!(cur == start)) {
        cur = parent[cur.r][cur.c];
        rev.push_back(cur);
    }

    std::reverse(rev.begin(), rev.end());
    return rev;
}

struct TargetChoice {
    Pos target{-1, -1};
    std::vector<Pos> path;
    int rawValue = 0;
    int distance = 0;
    double expectedValue = 0.0;
    double score = -1e18;
    int rivalDistance = 999;
};

TargetChoice bestTarget(
    const int grid[H][W],
    const Pos& start,
    int remainingBudget,
    const std::unordered_set<Pos, PosHash>& blocked,
    const std::array<Pos, 9>& competitors,
    int competitorCount,
    int enemyCount,
    const std::unordered_set<Pos, PosHash>& claimedTargets) {

    TargetChoice best;

    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            if (grid[r][c] <= 0) continue;
            Pos target{r, c};
            if (claimedTargets.find(target) != claimedTargets.end()) continue;

            auto path = bfsPath(grid, start, target, blocked);
            if (path.empty()) continue;

            int d = static_cast<int>(path.size()) - 1;
            if (d <= 0 || d > remainingBudget) continue;

            int rawValue = grid[r][c];
            int enemyDist = 999;
            int npcDist = 999;

            for (int i = 0; i < competitorCount; ++i) {
                int md = manhattan(competitors[i], target);
                if (i < enemyCount) enemyDist = std::min(enemyDist, md);
                else npcDist = std::min(npcDist, md);
            }

            double competitionFactor = 1.0;
            if (enemyDist + 1 < d) competitionFactor *= 0.30;
            else if (enemyDist < d) competitionFactor *= 0.45;
            else if (enemyDist == d) competitionFactor *= 0.62;
            else if (enemyDist == d + 1) competitionFactor *= 0.82;

            if (npcDist + 1 < d) competitionFactor *= 0.38;
            else if (npcDist < d) competitionFactor *= 0.50;
            else if (npcDist == d) competitionFactor *= 0.66;
            else if (npcDist == d + 1) competitionFactor *= 0.84;

            const int rivalDist = std::min(enemyDist, npcDist);
            const double expected = rawValue * competitionFactor;
            const double distancePenalty = 1.0 + 0.80 * d;
            const double score = expected / distancePenalty + 0.05 * rawValue;

            if (score > best.score) {
                best.target = target;
                best.path = std::move(path);
                best.rawValue = rawValue;
                best.distance = d;
                best.expectedValue = expected;
                best.score = score;
                best.rivalDistance = rivalDist;
            }
        }
    }

    return best;
}

std::vector<Pos> frontierPath(
    const int grid[H][W],
    const Pos& start,
    int maxSteps,
    const std::unordered_set<Pos, PosHash>& blocked,
    int unitIndex) {

    if (maxSteps <= 0) return {};

    std::array<std::array<bool, W>, H> visited{};
    std::array<std::array<int, W>, H> dist{};
    std::array<std::array<Pos, W>, H> parent{};
    std::queue<Pos> q;

    for (auto& row : dist) row.fill(-1);
    visited[start.r][start.c] = true;
    dist[start.r][start.c] = 0;
    q.push(start);

    Pos best = start;
    double bestScore = -1e18;

    while (!q.empty()) {
        Pos cur = q.front();
        q.pop();
        int cd = dist[cur.r][cur.c];
        if (cd > maxSteps) continue;

        int fogNeighbors = 0;
        for (int d = 0; d < 4; ++d) {
            int nr = cur.r + DR[d], nc = cur.c + DC[d];
            if (inBounds(nr, nc) && grid[nr][nc] == FOG) ++fogNeighbors;
        }

        if (cd > 0) {
            double score = fogNeighbors * 3.0 - cd * 0.55;

            // Keep V8's preference for productive central regions.
            if (cur.r >= 4 && cur.r <= 12 && cur.c >= 4 && cur.c <= 12) score += 1.25;
            score -= 0.06 * (std::abs(cur.r - 8) + std::abs(cur.c - 8));

            // V9: light exploration separation.  Unit 0 prefers the upper/right
            // side of an otherwise similar frontier; unit 1 prefers lower/left.
            // This remains only a tie-breaking signal, not a hard partition.
            if (unitIndex == 0) {
                score += 0.035 * ((8 - cur.r) + (cur.c - 8));
            } else {
                score += 0.035 * ((cur.r - 8) + (8 - cur.c));
            }

            if (score > bestScore) {
                bestScore = score;
                best = cur;
            }
        }

        if (cd == maxSteps) continue;

        for (int d = 0; d < 4; ++d) {
            Pos nxt{cur.r + DR[d], cur.c + DC[d]};
            if (!inBounds(nxt.r, nxt.c)) continue;
            if (visited[nxt.r][nxt.c]) continue;
            if (!isWalkable(grid[nxt.r][nxt.c])) continue;
            if (blocked.find(nxt) != blocked.end()) continue;

            visited[nxt.r][nxt.c] = true;
            dist[nxt.r][nxt.c] = cd + 1;
            parent[nxt.r][nxt.c] = cur;
            q.push(nxt);
        }
    }

    if (best == start) return {};

    std::vector<Pos> rev;
    Pos cur = best;
    rev.push_back(cur);
    while (!(cur == start)) {
        cur = parent[cur.r][cur.c];
        rev.push_back(cur);
    }
    std::reverse(rev.begin(), rev.end());
    return rev;
}

struct RoutePlan {
    std::vector<int> actions;
    int collectedValue = 0;
    double expectedGold = 0.0;
    double objective = 0.0;
    double firstTargetUrgency = 0.0;
};

RoutePlan buildRoute(
    const int grid[H][W],
    const Pos& start,
    int budget,
    const std::unordered_set<Pos, PosHash>& baseBlocked,
    const std::array<Pos, 9>& competitors,
    int competitorCount,
    int enemyCount,
    int unitIndex) {

    RoutePlan plan;
    if (budget <= 0) return plan;

    std::unordered_set<Pos, PosHash> blocked = baseBlocked;
    std::unordered_set<Pos, PosHash> claimedTargets;

    Pos cur = start;
    int remaining = budget;
    bool firstTarget = true;

    while (remaining > 0) {
        TargetChoice next = bestTarget(
            grid,
            cur,
            remaining,
            blocked,
            competitors,
            competitorCount,
            enemyCount,
            claimedTargets);

        if (next.target.r < 0) break;

        if (firstTarget && next.rivalDistance < 999) {
            // V9: if this unit is in a genuine race, prefer plans that allow
            // the urgent unit to execute first.  Small enough not to dominate EV.
            int raceMargin = next.rivalDistance - next.distance;
            if (raceMargin <= 0) plan.firstTargetUrgency = 1.8;
            else if (raceMargin == 1) plan.firstTargetUrgency = 1.0;
            else if (raceMargin == 2) plan.firstTargetUrgency = 0.4;
        }
        firstTarget = false;

        int steps = std::min(remaining, next.distance);
        for (int i = 0; i < steps; ++i) {
            plan.actions.push_back(dirFromTo(next.path[i], next.path[i + 1]));
        }

        remaining -= steps;
        cur = next.path[steps];

        if (steps < next.distance) break;

        plan.collectedValue += next.rawValue;
        plan.expectedGold += next.expectedValue;
        plan.objective += next.score;
        claimedTargets.insert(next.target);
    }

    if (remaining > 0) {
        auto exploration = frontierPath(grid, cur, remaining, blocked, unitIndex);
        if (!exploration.empty()) {
            int steps = std::min(remaining, static_cast<int>(exploration.size()) - 1);
            for (int i = 0; i < steps; ++i) {
                plan.actions.push_back(dirFromTo(exploration[i], exploration[i + 1]));
            }
            remaining -= steps;
        }
    }

    while (remaining-- > 0) plan.actions.push_back(4);
    return plan;
}

struct JointCandidate {
    int k = 3;
    int order = 0;
    RoutePlan plans[2];
    double score = -1e18;
};

}  // namespace

extern "C" GameOutput moveDecision(const GameInput& game) {
    GameOutput out{};

    const Pos starts[2] = {
        {game.players[0].units[0].position.x, game.players[0].units[0].position.y},
        {game.players[0].units[1].position.x, game.players[0].units[1].position.y}
    };

    std::array<Pos, 9> competitors{};
    int competitorCount = 0;
    int enemyCount = 0;

    if (game.players[1].units[0].position.x >= 0) {
        competitors[competitorCount++] = {
            game.players[1].units[0].position.x,
            game.players[1].units[0].position.y
        };
        ++enemyCount;
    }
    if (game.players[1].units[1].position.x >= 0) {
        competitors[competitorCount++] = {
            game.players[1].units[1].position.x,
            game.players[1].units[1].position.y
        };
        ++enemyCount;
    }

    for (int i = 0; i < game.npc_count && competitorCount < 9; ++i) {
        competitors[competitorCount++] = {
            game.npcs[i].position.x,
            game.npcs[i].position.y
        };
    }

    std::unordered_set<Pos, PosHash> baseBlocked;
    for (int i = 0; i < competitorCount; ++i) {
        baseBlocked.insert(competitors[i]);
    }

    JointCandidate best;

    for (int k = 0; k <= STEPS; ++k) {
        int budget[2] = {k, STEPS - k};

        for (int order = 0; order < 2; ++order) {
            JointCandidate cand;
            cand.k = k;
            cand.order = order;

            int firstUnit = order;
            int secondUnit = 1 - order;

            cand.plans[firstUnit] = buildRoute(
                game.grid,
                starts[firstUnit],
                budget[firstUnit],
                baseBlocked,
                competitors,
                competitorCount,
                enemyCount,
                firstUnit);

            std::unordered_set<Pos, PosHash> secondBlocked = baseBlocked;

            Pos simulated = starts[firstUnit];
            for (int action : cand.plans[firstUnit].actions) {
                if (action >= 0 && action < 4) {
                    simulated.r += DR[action];
                    simulated.c += DC[action];
                }
                secondBlocked.insert(simulated);
            }

            cand.plans[secondUnit] = buildRoute(
                game.grid,
                starts[secondUnit],
                budget[secondUnit],
                secondBlocked,
                competitors,
                competitorCount,
                enemyCount,
                secondUnit);

            int usedActions = 0;
            for (int u = 0; u < 2; ++u) {
                for (int a : cand.plans[u].actions) {
                    if (a != 4) ++usedActions;
                }
            }

            double expected = cand.plans[0].expectedGold + cand.plans[1].expectedGold;
            double quality = cand.plans[0].objective + cand.plans[1].objective;

            // V9: execution order now receives a small urgency bonus when the
            // first unit is racing a rival for its first target.
            double urgencyBonus = cand.plans[firstUnit].firstTargetUrgency;

            cand.score = expected * 2.5 + quality * 4.0 + usedActions * 0.18 + urgencyBonus;

            if (cand.score > best.score) best = std::move(cand);
        }
    }

    out.k = best.k;
    out.order = best.order;

    for (int u = 0; u < 2; ++u) {
        for (int i = 0; i < STEPS; ++i) out.actions[u][i] = 4;
        int n = std::min(STEPS, static_cast<int>(best.plans[u].actions.size()));
        for (int i = 0; i < n; ++i) out.actions[u][i] = best.plans[u].actions[i];
    }

    // V9: losing logs spent materially more on vision than winning logs.
    // Keep vision available, but require both low visible opportunity and a
    // sufficiently incomplete board before buying more information.
    int visibleGold = 0;
    int fogCells = 0;
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            if (game.grid[r][c] > 0) visibleGold += game.grid[r][c];
            else if (game.grid[r][c] == FOG) ++fogCells;
        }
    }

    out.vision = 0;
    if (visibleGold <= 2 && fogCells > 135 && game.players[0].gold >= 80) {
        out.vision = 1;
    }

    return out;
}
