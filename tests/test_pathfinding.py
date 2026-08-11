import unittest

from BFS import (
    BOMB,
    DOWN,
    FOG,
    LEFT,
    OBSTACLE,
    RIGHT,
    STAY,
    UP,
    action_names,
    best_gold_path,
    choose_allocation,
    decide_turn,
    find_target_path,
    frontier_path,
)


class PathfinderTests(unittest.TestCase):
    def test_bfs_distance_parent_and_actions(self):
        grid = [
            [0, 0, 0, OBSTACLE, 0],
            [0, OBSTACLE, 0, OBSTACLE, 0],
            [0, 0, 0, 0, 0],
            [OBSTACLE, 0, OBSTACLE, 1, 0],
            [0, 0, 0, 0, 0],
        ]
        result = find_target_path(grid, (0, 0), (3, 3))
        self.assertIsNotNone(result)
        self.assertEqual(result.distance, 6)
        self.assertEqual(result.path[0], (0, 0))
        self.assertEqual(result.path[-1], (3, 3))
        self.assertEqual(
            action_names(result.actions),
            ["DOWN", "DOWN", "RIGHT", "RIGHT", "RIGHT", "DOWN"],
        )

    def test_target_selection_prefers_value_distance_score(self):
        grid = [
            [0, 1, 0, 0, 9],
            [0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0],
        ]
        result = best_gold_path(grid, (0, 0), max_steps=6)
        self.assertEqual(result.target, (0, 4))
        self.assertEqual(result.value, 9)

    def test_k_value_sensitivity_allocates_more_to_high_value_unit(self):
        k = choose_allocation(
            ([RIGHT, RIGHT, RIGHT, RIGHT], [LEFT, LEFT]), (20, 5), steps=6
        )
        self.assertEqual(k, 4)

    def test_k_distance_value_tradeoff_far_20_vs_near_5(self):
        k = choose_allocation(([RIGHT] * 6, [LEFT]), (20, 5), steps=6)
        self.assertEqual(k, 5)

    def test_collision_uses_first_units_path_as_blocked(self):
        grid = [
            [0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0],
            [9, 0, 0, 0, 0],
        ]
        decision = decide_turn(grid, [(0, 0), (2, 0)], steps=6)
        self.assertNotIn((1, 0), _positions_after((2, 0), decision.paths[1]))

    def test_enemy_cells_are_blocked(self):
        grid = [
            [0, 0, 1],
            [0, 0, 0],
            [0, 0, 0],
        ]
        result = best_gold_path(grid, (0, 0), blocked={(0, 1)}, max_steps=6)
        self.assertEqual(result.actions[0], DOWN)

    def test_frontier_stops_next_to_fog_but_not_inside_fog(self):
        grid = [
            [0, 0, FOG],
            [0, 0, FOG],
            [0, 0, 0],
        ]
        result = frontier_path(grid, (0, 0), max_steps=6)
        self.assertIsNotNone(result)
        self.assertNotEqual(grid[result.target[0]][result.target[1]], FOG)
        self.assertIn(result.target, {(0, 1), (1, 1), (2, 2)})

    def test_bomb_is_avoided(self):
        grid = [
            [0, BOMB, 1],
            [0, 0, 0],
            [0, 0, 0],
        ]
        result = best_gold_path(grid, (0, 0), max_steps=6)
        self.assertEqual(result.actions[0], DOWN)

    def test_edge_case_no_gold_no_frontier_stays(self):
        grid = [
            [0, OBSTACLE],
            [OBSTACLE, 0],
        ]
        decision = decide_turn(grid, [(0, 0), (1, 1)], steps=6)
        self.assertEqual(decision.actions, [STAY] * 6)


def _positions_after(start, actions):
    deltas = {
        UP: (-1, 0),
        DOWN: (1, 0),
        LEFT: (0, -1),
        RIGHT: (0, 1),
        STAY: (0, 0),
    }
    r, c = start
    out = []
    for action in actions:
        dr, dc = deltas[action]
        r += dr
        c += dc
        if action != STAY:
            out.append((r, c))
    return out


if __name__ == "__main__":
    unittest.main()
