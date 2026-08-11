"""Grid pathfinding utilities and a compact GoldRush-style decision agent.

The module keeps the beginner-friendly BFS example runnable, but exposes small,
testable functions for the next test areas: target selection, value/distance
trade-offs, collision avoidance, enemies, fog, bombs, and edge cases.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Iterable, Sequence

# Cell values used by the GoldRush grid.
FOG = -5
BOMB = -3
OBSTACLE = -1
EMPTY = 0

# Action encoding compatible with the C++ agent shown in the prompt.
UP = 0
DOWN = 1
LEFT = 2
RIGHT = 3
STAY = 4
ACTION_NAMES = {
    UP: "UP",
    DOWN: "DOWN",
    LEFT: "LEFT",
    RIGHT: "RIGHT",
    STAY: "STAY",
}
DIRECTIONS = (
    (-1, 0, UP),
    (1, 0, DOWN),
    (0, -1, LEFT),
    (0, 1, RIGHT),
)

Position = tuple[int, int]
Grid = Sequence[Sequence[int | str]]


@dataclass(frozen=True)
class SearchResult:
    """Result returned by BFS helpers."""

    path: list[Position]
    actions: list[int]
    distance: int
    value: int = 0
    target: Position | None = None


@dataclass(frozen=True)
class Decision:
    """Two-unit six-step action allocation."""

    actions: list[int]
    k: int
    order: int
    paths: tuple[list[int], list[int]]
    values: tuple[int, int]


def in_bounds(grid: Grid, position: Position) -> bool:
    """Return whether a position is inside a rectangular grid."""

    r, c = position
    return bool(grid) and bool(grid[0]) and 0 <= r < len(grid) and 0 <= c < len(grid[0])


def is_walkable(
    grid: Grid,
    position: Position,
    blocked: Iterable[Position] = (),
    *,
    avoid_bombs: bool = True,
    allow_fog: bool = False,
) -> bool:
    """Return whether BFS may step onto a cell.

    By default the agent is conservative: walls, fog, bombs, and dynamic blocked
    cells such as enemies or another unit's planned path are not walkable.
    """

    if not in_bounds(grid, position):
        return False

    cell = grid[position[0]][position[1]]
    if cell in {"#", OBSTACLE}:
        return False
    if not allow_fog and cell == FOG:
        return False
    if avoid_bombs and cell == BOMB:
        return False
    return position not in blocked


def action_between(current: Position, next_position: Position) -> int:
    """Convert two adjacent positions to an encoded action."""

    dr = next_position[0] - current[0]
    dc = next_position[1] - current[1]
    for dir_r, dir_c, action in DIRECTIONS:
        if (dr, dc) == (dir_r, dir_c):
            return action
    return STAY


def path_to_actions(path: Sequence[Position]) -> list[int]:
    """Convert a path of positions into movement actions."""

    return [action_between(path[i], path[i + 1]) for i in range(len(path) - 1)]


def reconstruct_path(
    parent: dict[Position, Position | None], target: Position
) -> list[Position]:
    """Rebuild a path from a BFS parent map."""

    path: list[Position] = []
    current: Position | None = target
    while current is not None:
        path.append(current)
        current = parent[current]
    path.reverse()
    return path


def bfs_all(
    grid: Grid,
    start: Position,
    blocked: Iterable[Position] = (),
    *,
    max_steps: int | None = None,
    avoid_bombs: bool = True,
    allow_fog: bool = False,
) -> tuple[dict[Position, int], dict[Position, Position | None]]:
    """Run BFS once and return distance and parent maps."""

    if not in_bounds(grid, start):
        return {}, {}

    blocked_set = set(blocked)
    queue: deque[Position] = deque([start])
    distance = {start: 0}
    parent: dict[Position, Position | None] = {start: None}

    while queue:
        current = queue.popleft()
        current_distance = distance[current]
        if max_steps is not None and current_distance >= max_steps:
            continue

        for dr, dc, _action in DIRECTIONS:
            nxt = (current[0] + dr, current[1] + dc)
            if nxt in distance:
                continue
            if not is_walkable(
                grid,
                nxt,
                blocked_set,
                avoid_bombs=avoid_bombs,
                allow_fog=allow_fog,
            ):
                continue
            distance[nxt] = current_distance + 1
            parent[nxt] = current
            queue.append(nxt)

    return distance, parent


def find_target_path(
    grid: Grid,
    start: Position,
    target: Position,
    blocked: Iterable[Position] = (),
    *,
    max_steps: int | None = None,
) -> SearchResult | None:
    """Find the shortest path to a specific target."""

    distance, parent = bfs_all(grid, start, blocked, max_steps=max_steps)
    if target not in distance:
        return None
    path = reconstruct_path(parent, target)
    value = cell_value(grid[target[0]][target[1]])
    return SearchResult(path, path_to_actions(path), distance[target], value, target)


def cell_value(cell: int | str) -> int:
    """Return target value; supports integer gold and the legacy 'G' demo cell."""

    if cell == "G":
        return 1
    return cell if isinstance(cell, int) and cell > 0 else 0


def best_gold_path(
    grid: Grid,
    start: Position,
    blocked: Iterable[Position] = (),
    *,
    max_steps: int = 6,
) -> SearchResult | None:
    """Choose reachable gold using value/(distance+1), then value, then distance."""

    distance, parent = bfs_all(grid, start, blocked, max_steps=max_steps)
    best: tuple[float, int, int, Position] | None = None

    for position, dist in distance.items():
        if dist == 0:
            continue
        value = cell_value(grid[position[0]][position[1]])
        if value <= 0:
            continue
        score = value / (dist + 1)
        candidate = (score, value, -dist, position)
        if best is None or candidate > best:
            best = candidate

    if best is None:
        return None

    _score, value, neg_dist, target = best
    path = reconstruct_path(parent, target)
    return SearchResult(path, path_to_actions(path), -neg_dist, value, target)


def frontier_path(
    grid: Grid,
    start: Position,
    blocked: Iterable[Position] = (),
    *,
    max_steps: int = 6,
) -> SearchResult | None:
    """Move toward visible cells adjacent to fog when no gold is reachable."""

    distance, parent = bfs_all(grid, start, blocked, max_steps=max_steps)
    best: tuple[float, int, Position] | None = None
    for position, dist in distance.items():
        if dist == 0:
            continue
        fog_neighbors = sum(
            1
            for dr, dc, _action in DIRECTIONS
            if in_bounds(grid, (position[0] + dr, position[1] + dc))
            and grid[position[0] + dr][position[1] + dc] == FOG
        )
        if fog_neighbors <= 0:
            continue
        score = 3.0 * fog_neighbors - 0.25 * dist
        candidate = (score, -dist, position)
        if best is None or candidate > best:
            best = candidate

    if best is None:
        return None

    _score, neg_dist, target = best
    path = reconstruct_path(parent, target)
    return SearchResult(path, path_to_actions(path), -neg_dist, 0, target)


def positions_after_actions(start: Position, actions: Sequence[int]) -> list[Position]:
    """Return cells occupied by following actions, ignoring STAY."""

    current = start
    positions: list[Position] = []
    for action in actions:
        if action == STAY:
            continue
        dr, dc, _ = DIRECTIONS[action]
        current = (current[0] + dr, current[1] + dc)
        positions.append(current)
    return positions


def plan_unit(
    grid: Grid,
    start: Position,
    blocked: Iterable[Position] = (),
    *,
    max_steps: int = 6,
) -> SearchResult:
    """Plan for gold first, otherwise exploration, otherwise stay."""

    return (
        best_gold_path(grid, start, blocked, max_steps=max_steps)
        or frontier_path(grid, start, blocked, max_steps=max_steps)
        or SearchResult([start], [], 0, 0, None)
    )


def choose_allocation(
    paths: Sequence[Sequence[int]], values: Sequence[int], *, steps: int = 6
) -> int:
    """Pick k actions for unit 0 based on executable value and balanced ties."""

    def allocation_value(index: int, allocated_steps: int) -> float:
        if allocated_steps <= 0 or not paths[index]:
            return 0.0
        path_length = len(paths[index])
        executed = min(path_length, allocated_steps)
        if values[index] > 0:
            return values[index] * executed / path_length
        return 0.20 * executed

    best_k = steps // 2
    best_score = float("-inf")
    for k in range(steps + 1):
        score = allocation_value(0, k) + allocation_value(1, steps - k)
        imbalance = abs(k - (steps - k))
        best_imbalance = abs(best_k - (steps - best_k))
        if score > best_score + 1e-12 or (
            abs(score - best_score) <= 1e-12 and imbalance < best_imbalance
        ):
            best_score = score
            best_k = k
    return best_k


def decide_turn(
    grid: Grid,
    starts: Sequence[Position],
    enemies: Iterable[Position] = (),
    *,
    steps: int = 6,
) -> Decision:
    """Plan a two-unit turn with enemy avoidance and path-collision blocking."""

    enemy_cells = {enemy for enemy in enemies if in_bounds(grid, enemy)}
    first_plans = [plan_unit(grid, start, enemy_cells, max_steps=steps) for start in starts]

    def opportunity(result: SearchResult) -> float:
        return result.value / len(result.actions) if result.value > 0 and result.actions else 0.0

    first = 0 if opportunity(first_plans[0]) >= opportunity(first_plans[1]) else 1
    second = 1 - first

    plans = [first_plans[0], first_plans[1]]
    blocked = enemy_cells | set(positions_after_actions(starts[first], plans[first].actions))
    plans[second] = plan_unit(grid, starts[second], blocked, max_steps=steps)

    paths = (plans[0].actions, plans[1].actions)
    values = (plans[0].value, plans[1].value)
    k = choose_allocation(paths, values, steps=steps)

    actions = [STAY] * steps
    actions[: min(k, len(paths[0]))] = paths[0][:k]
    unit1_count = min(steps - k, len(paths[1]))
    actions[k : k + unit1_count] = paths[1][:unit1_count]

    return Decision(actions, k, 0 if values[0] >= values[1] else 1, paths, values)


def action_names(actions: Sequence[int]) -> list[str]:
    """Convert encoded actions to readable labels."""

    return [ACTION_NAMES[action] for action in actions]


def demo() -> None:
    """Run the original small BFS demo."""

    demo_grid: list[list[int | str]] = [
        ["A", 0, 0, "#", 0],
        [0, "#", 0, "#", 0],
        [0, 0, 0, 0, 0],
        ["#", 0, "#", "G", 0],
        [0, 0, 0, 0, 0],
    ]
    result = find_target_path(demo_grid, (0, 0), (3, 3))
    if result is None:
        print("未找到金币")
        return
    print("找到金币")
    print("距离：", result.distance)
    print("路径：", result.path)
    print("操作:", action_names(result.actions))


if __name__ == "__main__":
    demo()
