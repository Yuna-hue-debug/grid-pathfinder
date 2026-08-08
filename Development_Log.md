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
