# GoldRush: A Low-Latency Multi-Agent Decision Engine

A C++ decision engine for competitive grid-based resource collection under partial observability. The project began as a BFS pathfinding exercise and evolved into a low-latency multi-agent planner, an experimental workflow, and a submission project for Polymer Tech Expo 2026.

## Project Summary

The agent controls two units sharing six actions per turn. It must jointly decide action allocation (`k`), execution order, resource targets, exploration behavior, and whether additional vision is worth its cost. The environment combines fog-of-war, stochastic resources, obstacles, bombs, competing players/NPCs, execution-order effects, and strict latency requirements.

The project therefore has three layers:

```text
Algorithms        Engineering           Research
BFS / scoring  -> C++ / Docker / .so -> logs / experiments / iteration
```

## Final Competition Version - V9

**V9 is the selected competition version.** It is a log-driven refinement of the V8 baseline rather than a full strategy rewrite.

Winning and losing V8 logs motivated three targeted changes:

- **exploration separation** - light unit-specific frontier preferences reduce duplicated exploration;
- **execution-order urgency** - contested first targets can make execution order strategically meaningful;
- **more conservative vision purchasing** - information is purchased only when visible opportunity is weak and substantial fog remains.

V9 preserved the core V8 expected-value planner so the changes remained interpretable.

### Champion / Challenger Evidence

V8 was retained as the champion while V9 was tested as a challenger. V9 defeated V8 on all three public-test maps used in the initial direct validation and was promoted.

A later V10 experiment attempted to improve robustness on a newly introduced maze/corridor-style map using topology-aware frontier scoring, central accessibility, and stronger idle-action penalties. V9 still won the direct tests. V10 was therefore **rejected rather than promoted**.

This negative result is part of the project: a theoretically more sophisticated heuristic can reduce actual reward. The final choice is deliberately V9 rather than the numerically newest version.

## System Architecture

```text
Game State
    ↓
Environment Parsing
    ↓
Candidate Gold Detection
    ↓
Competition-Aware Target Scoring
    ↓
Multi-Target Route Planning
    ↓
Joint k / Execution-Order Optimization
    ↓
Exploration / Selective Vision
    ↓
Six Final Actions
```

BFS answers **how to reach a target**; the scoring layer decides **which target is worth pursuing**; the joint planner decides **how the shared six-action budget should be used**.

## Learning Trajectory

### Stage 1 - BFS fundamentals
- represented the map as a grid;
- learned queue vs. visited-state logic;
- implemented shortest-path distance;
- reconstructed paths with parent pointers;
- converted paths into UP / DOWN / LEFT / RIGHT actions.

### Stage 2 - From Python to a competition artifact
- migrated the agent to C++17;
- learned the organizer's `moveDecision` interface;
- compiled a Linux x86-64 shared library with Docker;
- validated `player.so` with `file` and `nm`.

### Stage 3 - From pathfinding to decision making
- dynamic `k` allocation across two units;
- value-versus-distance target scoring;
- opponent/NPC race-risk estimation;
- multi-target chaining;
- joint execution-order search;
- fog-of-war exploration;
- selective vision purchasing.

### Stage 4 - From heuristics to experiments
- automated repeated public-test submissions;
- collected match logs;
- compared winning and losing behavior;
- used V8 as a stable champion instead of overwriting every version;
- promoted V9 only after direct head-to-head validation;
- rejected V10 when extra complexity failed to beat V9.

This stage changed the core workflow from `I think this heuristic is better` to `form a hypothesis, test it, measure it, keep or reject it`.

## Experimental Method

```text
Collect match logs
        ↓
Compare wins and losses
        ↓
Identify repeated failure modes
        ↓
Form a testable hypothesis
        ↓
Make minimal targeted changes
        ↓
Champion vs. challenger test
        ↓
Promote or reject from evidence
```

Important lessons included:

- latency stopped being the main bottleneck once the C++ agent was sufficiently fast;
- visible failures such as NPC trampling were not automatically the causal performance bottleneck;
- `STAY` frequency is useful diagnostically, but forcing movement can reward unproductive actions;
- map topology can change which heuristics work, so robustness matters more than optimizing one public map;
- more complex code is not necessarily a stronger agent.

## Cross-Map Robustness

A newly introduced fifth map exposed stronger corridor, bottleneck, and connectivity effects. This motivated V10's topology-aware experiment. The experiment was useful even though V10 was rejected: it showed that topology information should be introduced cautiously and that exploration value should not overwhelm immediate expected reward.

For future work, map behavior should be characterized through continuous structural features such as obstacle density, connectivity, corridor width, central accessibility, fog exposure, and competition density - not hard-coded `if (map_id == ...)` rules.

## Performance Engineering

The production agent is implemented in C++17 and compiled into a Linux x86-64 `.so` file using Docker.

```bash
docker run --rm -it --platform linux/amd64 \
  -v "$PWD":/goldrush \
  -w /goldrush \
  gcc:latest bash

make clean
make
file player.so
nm -D player.so | grep moveDecision
```

## Repository Structure

```text
.
├── BFS.py                # Early Python BFS learning/prototype
├── player.cpp            # Competition build path
├── player_v9.cpp         # Selected final competition agent
├── Development_Log.md    # Chronological experiments and lessons
├── README.md             # Project overview and methodology
└── .gitignore
```

The repository is intentionally more than a code dump: `BFS.py` records the starting point, the C++ agent records the engineering transition, and `Development_Log.md` records how strategy decisions became evidence-driven experiments.

## AI-Assisted Development

AI was used as a research and engineering copilot outside the latency-critical runtime loop. Typical uses included:

- explaining BFS and C++ concepts;
- reviewing game logs and grouping failure modes;
- generating hypotheses for target selection, `k`, order, exploration, and vision;
- reviewing code changes;
- designing champion/challenger experiments;
- documenting results and rejected ideas.

The runtime agent itself remains a deterministic C++ decision engine. This separation is intentional: AI accelerates the research loop while the competition path stays fast and reproducible.

## Final Status

- [x] BFS shortest-path search and path reconstruction
- [x] Python-to-C++ migration
- [x] Dynamic `k` allocation
- [x] Competition-aware target scoring
- [x] Joint two-unit planning
- [x] Multi-target collection
- [x] Fog-of-war exploration
- [x] Selective vision purchasing
- [x] Automated repeated match submission
- [x] Win/loss log analysis
- [x] Champion/challenger versioning
- [x] V9 promoted after beating V8 across the initial public-map validation
- [x] V10 topology experiment evaluated and rejected after losing to V9
- [x] V9 frozen as the final competition version

## Future Work

If development resumes, the priority is not to add more heuristics immediately. Useful next steps would be automated log metric extraction, larger cross-map samples, opponent behavior profiling, and systematic robustness evaluation on unseen map topologies.

## Reflection

The project started with a question about how BFS works. It ended with a broader lesson about quantitative engineering: algorithms matter, but so do deployment constraints, experimental design, baselines, negative results, and the discipline to reject a more complicated model when the data does not support it.
