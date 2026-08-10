# Development Log

---

## 2026-08-06

### Version

v0.2

### Goal

Implement the first move validation function.

### What I learned

- Represent the game map using a 2D grid.
- Understand row and column coordinates.
- Learn how to check whether a move is valid.
- Learn to use Python functions.

### Progress

- [x] Grid representation
- [x] Coordinate system
- [ ] Move validation
- [ ] Breadth-First Search (BFS)

### Reflection

Today I learned that before an AI can find a path, it must first determine whether a move is legal. This validation function will become the foundation of the pathfinding algorithm.
## 2026-08-07

### Topic

Breadth-First Search (BFS)

### What I learned

- A grid can be modeled as a graph.
- BFS explores nodes level by level.
- Queue is required for shortest path search.

### Progress

- [x] Grid representation
- [x] Move validation
- [x] BFS implementation
## 2026-08-08

### Topic: BFS and Distance Tracking

Today I implemented a basic BFS pathfinding algorithm for a grid-based environment.

### What I learned

- BFS explores a graph level by level.
- A queue stores nodes waiting to be explored.
- A visited set prevents repeated exploration.
- Distance records the shortest number of steps from the starting point.
- BFS can find the shortest path when each movement has the same cost.

### Implementation

I implemented BFS using Python's `deque` and added distance tracking.

The agent can now:
1. Explore a grid.
2. Avoid obstacles.
3. Avoid revisiting cells.
4. Detect the target.
5. Calculate the shortest distance to the target.

### Next Step

Implement path reconstruction so that the agent can determine not only the distance to the target, but also the actual route it should take.
## 2026-08-08 — Path Reconstruction

### What I learned

- A parent dictionary can record how each node was reached.
- By following parent pointers backwards, the shortest path can be reconstructed.
- The reconstructed path needs to be reversed to obtain the route from start to goal.
- The path can be converted into actions such as UP, DOWN, LEFT, and RIGHT.

### Implementation

Added:
- Parent tracking
- Path reconstruction
- Action generation from consecutive positions

### Next Step

Extend the agent to handle multiple targets and make decisions based on both reward and distance.

---

## 2026-08-10 — Multi-Target Decision Making

### Topic

Using BFS results to choose which target the agent should pursue next.

### What I learned

- BFS is not only useful for finding one shortest path; it can also measure the distance from the agent to multiple reachable targets.
- When several targets exist on the grid, the agent needs a decision rule instead of simply moving toward the first target it finds.
- A simple scoring idea is to compare each target's reward with its distance, so closer or more valuable targets can be prioritized.
- Unreachable targets should be ignored because no valid path exists through walls or blocked cells.
- Separating pathfinding from decision making makes the agent easier to improve: BFS answers "how far and by which route," while the decision layer answers "which goal is best."

### Implementation Notes

The next version of the agent should:

1. Scan the grid for all possible targets.
2. Run BFS or reuse BFS distance data to evaluate each target.
3. Filter out unreachable targets.
4. Choose the best target using a clear score, such as `reward / distance`.
5. Reconstruct the path and convert it into movement actions.

### Progress

- [x] Grid representation
- [x] Move validation
- [x] BFS implementation
- [x] Shortest path recovery
- [ ] Multi-target decision making
- [ ] Opponent prediction
- [ ] Performance optimization

### Reflection

Today I learned that a pathfinding agent should not only ask whether it can reach a goal, but also whether that goal is the best choice. This changes the project from simple shortest-path search into the beginning of strategic decision making.
