# GoldRush: A Low-Latency Multi-Agent Decision Engine

A C++ decision engine for competitive grid-based resource collection under partial observability. The project began as a BFS pathfinding exercise and evolved into a low-latency multi-agent planner for the GoldRush programming competition and Polymer Tech Expo 2026.

## Overview

The agent controls two units sharing six actions per turn. It must jointly decide action allocation, execution order, resource targets, exploration behavior, and whether information from additional vision is worth its cost.

The environment combines fog-of-war, stochastic gold generation, obstacles and bombs, competing players/NPCs, execution-order effects, and strict decision-latency requirements.

## Current Version — V9

V9 is a log-driven refinement of the V8 baseline. Instead of a large strategy rewrite, winning and losing V8 logs were compared and three targeted changes were introduced:

- **exploration separation** — light unit-specific frontier preferences reduce duplicated exploration;
- **execution-order urgency** — contested first targets can make execution order strategically meaningful rather than a near-constant tie-break;
- **more conservative vision purchasing** — information is purchased only under weaker visible opportunity and substantial remaining fog.

V9 intentionally preserves the core V8 expected-value planner so the effect of these changes remains interpretable.

### Validation

V8 was kept as the champion and V9 was tested directly as a challenger. In the current simulation validation, **V9 defeated V8 on all three public-test maps**.

This result supports promotion of V9 as the current candidate, while V8 remains the benchmark for larger-sample and unseen-map testing.

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

BFS answers **how to reach a target**; the scoring layer decides **which target is worth pursuing**; the joint planner decides **how the shared action budget should be used**.

## Strategy Evolution

### V1 — Grid Pathfinder
- grid representation and move validation;
- BFS shortest paths;
- distance tracking and parent-based path reconstruction;
- action conversion.

### V2 — C++ Fast Agent
- migration from Python to C++17;
- competition `moveDecision` interface;
- Linux/x86-64 shared-library deployment through Docker.

### V2.x — Strategic Planner
- dynamic `k` allocation across two units;
- resource value / travel-distance trade-off;
- opponent and NPC race-risk estimation;
- chained multi-target collection;
- joint execution-order search;
- exploration under fog-of-war;
- selective vision purchasing.

### V8 — Champion Baseline
V8 provided a stable baseline for log collection and controlled comparison. Analysis of winning and losing games revealed that some apparent problems, such as NPC/trample activity, were less discriminative than action efficiency, vision spending, and execution-order behavior.

### V9 — Log-Driven Challenger
V9 converted those observations into minimal testable changes and then beat V8 across all three public-test maps in the current simulation validation.

## Experimental Method

Development now follows a champion/challenger workflow:

```text
Collect logs
    ↓
Compare wins vs losses
    ↓
Identify repeated behavioral differences
    ↓
Form one testable hypothesis
    ↓
Make minimal changes
    ↓
Run V_new vs V_baseline
    ↓
Promote only after empirical improvement
```

This helps avoid overfitting individual matches or accumulating heuristics without evidence.

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
├── BFS.py                # Early Python BFS prototype
├── player.cpp            # Competition agent / baseline path
├── player_v9.cpp         # Current V9 log-driven challenger
├── Development_Log.md    # Iteration history and research notes
├── README.md
└── .gitignore
```

## Next Questions

1. Does V9 retain its advantage over a larger number of matches and opponents?
2. Does its improvement persist on hidden or structurally different maps?
3. How much do `STAY` ratio, vision spending, and execution-order distribution change relative to V8?
4. Can match-log metrics be extracted automatically after every tournament?
5. Which single additional mechanism is best justified for V10?

## Motivation

The project is both an algorithmic engineering exercise and an applied decision-making experiment. It combines graph algorithms, low-latency C++, multi-agent planning, empirical evaluation, and iterative strategy design in one system.
