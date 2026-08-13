# Development Log

This log documents the evolution of the project from a basic Python grid pathfinder into a low-latency C++ multi-agent decision engine for GoldRush.

---

## 2026-08-06 — Grid Representation and Move Validation

### Goal

Build the minimum environment representation needed before implementing search.

### What I learned

- A game map can be represented as a 2D grid.
- Row/column coordinates must be handled consistently.
- Pathfinding requires a reliable legality check before expanding neighboring states.

### Progress

- [x] Grid representation
- [x] Coordinate system
- [x] Move validation

---

## 2026-08-07 — Breadth-First Search

### Goal

Find shortest paths on an unweighted grid.

### Implementation

Implemented BFS using a queue and visited-state tracking.

### Key idea

BFS explores the grid level by level, so the first time a reachable cell is discovered gives its shortest distance from the start when every movement has equal cost.

### Progress

- [x] BFS implementation
- [x] Obstacle avoidance
- [x] Reachability testing

---

## 2026-08-08 — Distance Tracking and Path Reconstruction

### Goal

Move from answering "can I reach the target?" to producing the actual action sequence.

### Implementation

Added:

- shortest-distance tracking;
- parent pointers;
- path reconstruction;
- conversion from consecutive positions to `UP`, `DOWN`, `LEFT`, and `RIGHT` actions.

### Lesson

Pathfinding and action generation should be separate components: BFS determines the route, while the action layer translates that route into game commands.

---

## 2026-08-10 — Multi-Target Decision Making

### Problem

A shortest-path algorithm is insufficient when several resource targets are available. The closest target is not necessarily the best target.

### Approach

The decision layer began evaluating targets using both reward and travel cost. Unreachable targets are filtered out before scoring.

Conceptually:

```text
target utility = f(resource value, path distance)
```

### Design lesson

This was the transition from a pure pathfinding project to a strategic decision-making project:

- BFS answers **how far and by which route**;
- the decision layer answers **which goal should be pursued**.

---

## 2026-08-10 — Migration from Python to C++

### Motivation

Competition performance depends not only on strategy but also on decision latency. The agent was therefore migrated from the Python learning prototype to C++17.

### Engineering work

- Reimplemented core planning logic in C++.
- Built the competition `moveDecision` interface.
- Created a Linux/x86-64 shared-object build workflow.
- Used Docker on macOS to reproduce the submission environment.
- Validated the resulting binary using `file` and `nm`.

### Build validation

```bash
make clean
make
file player.so
nm -D player.so | grep moveDecision
```

This ensured that `player.so` was an ELF x86-64 shared object and exported the required decision function.

---

## 2026-08-10 — V2.1 Dynamic `k` Allocation

### Problem

The two controlled units share six actions per turn. A fixed 4/2 split can waste actions when one unit has a much better opportunity than the other.

### Approach

Evaluate candidate split points:

```text
k ∈ {0, 1, 2, 3, 4, 5, 6}
```

where unit 0 receives the first `k` actions and unit 1 receives the remaining `6-k` actions.

### Tests

Constructed behavior tests covering:

- unit 0 having the stronger route;
- unit 1 having the stronger route;
- balanced opportunities.

The tests exposed an important issue: a scoring function can produce the expected `k` while still being too insensitive to differences in target value and route length.

### Improvement

Refined the allocation objective to account for both marginal movement value and path requirements rather than using a fixed heuristic split.

---

## 2026-08-10 — Value Sensitivity and Distance/Value Trade-off

### Research question

Should the agent spend more actions reaching a distant high-value target or collect a smaller nearby target?

### Improvement

Target evaluation was changed from a simple nearest-target rule toward a utility model balancing:

- visible gold value;
- number of actions required;
- whether the allocated action budget can actually reach/collect the target.

This made `k` allocation depend on the opportunity each unit could realistically exploit during the current turn.

---

## 2026-08-11 — Competition-Aware Target Selection

### Problem

A target can look attractive to our agent but have almost zero practical value if an opponent or NPC will collect it first.

### Approach

Extended target scoring to consider competitive pressure.

Conceptually:

```text
expected target value
    = resource value
    - travel cost
    - opponent race risk
    - NPC race risk
```

### Strategy change

The agent became more conservative about contested gold and more willing to pursue uncontested alternatives instead of blindly following the largest visible reward.

---

## 2026-08-11 — Multi-Target and Joint Planning

### Problem

Optimizing each unit independently can create globally poor decisions. A unit may finish one target early and have unused actions, or both units may interfere with the same opportunity.

### Improvement

The planner was extended toward joint evaluation of:

- route for unit 0;
- route for unit 1;
- `k` allocation;
- execution order;
- chained collection opportunities.

Candidate plans are evaluated using the actions that would actually be executed rather than scoring target choices in isolation.

### Result

The agent moved from a single-target BFS policy toward a small combinatorial planner over the six-action turn budget.

---

## 2026-08-11 — Exploration and Vision

### Problem

Under fog-of-war, there are turns where no strong visible resource target exists.

### Approach

Added fallback exploration behavior with a preference for useful map coverage rather than random movement. Vision purchasing remains selective because information has a cost.

This introduces an exploration/exploitation trade-off:

```text
known profitable target → exploit
weak/no visible target  → explore
```

---

## 2026-08-11 to 2026-08-13 — Automated Tournament Testing

### Motivation

A low decision latency does not guarantee a high win rate. Strategy needs empirical evaluation against different opponents and map structures.

### Tooling

Developed a Python automation script for repeated public-test submissions.

### Current experiment design

- 3 maps
- 6 opponents
- 3 repetitions for each map/opponent pair
- 54 total matches

### Why this matters

The experiment allows comparisons such as:

```text
same agent + same opponent + different map
```

This helps distinguish general strategy weaknesses from map-specific weaknesses.

### Current observation

Performance appears to vary materially across maps. This suggests that one global set of target-scoring and exploration parameters may not be optimal for every topology.

---

## Current Agent Architecture

```text
GameInput
   ↓
Parse visible grid / units / enemies / NPCs
   ↓
Generate resource candidates
   ↓
Estimate routes and competitive pressure
   ↓
Build candidate plans for both units
   ↓
Evaluate k + execution order
   ↓
Exploration fallback / vision decision
   ↓
GameOutput
```

---

## Current Status

- [x] Grid representation
- [x] Move validation
- [x] BFS shortest-path search
- [x] Path reconstruction
- [x] Python-to-C++ migration
- [x] Multi-target decision making
- [x] Dynamic `k` allocation
- [x] Value/distance trade-off
- [x] Competition-aware scoring
- [x] Joint two-unit planning
- [x] Low-latency shared-library deployment
- [x] Automated repeated match submission
- [ ] Automated match-log download
- [ ] Map-specific performance analysis
- [ ] Opponent behavior profiling
- [ ] Systematic parameter optimization

---

## Next Research Iteration

The next version will focus less on adding isolated heuristics and more on using match data to identify failure modes.

Priority workflow:

```text
Collect match logs
      ↓
Measure win rate by map/opponent
      ↓
Identify repeated strategic failures
      ↓
Form one testable hypothesis
      ↓
Modify planner/scoring
      ↓
Run controlled tournament
      ↓
Compare against baseline
```

Potential improvements include map-adaptive parameters, true BFS-based opponent race distances, opponent behavior profiling, and automated log analysis.

---

## Reflection

The biggest change in this project was conceptual. It started as an exercise in learning BFS, but competitive play revealed that shortest-path search is only one component of a decision engine. Good performance requires combining routing, resource valuation, competition modeling, action-budget allocation, execution order, exploration, latency engineering, and empirical testing.
