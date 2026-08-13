# GoldRush: A Low-Latency Multi-Agent Decision Engine

A C++ decision engine for competitive grid-based resource collection under partial observability. The project began as a BFS pathfinding exercise and evolved into a low-latency multi-agent planner for the GoldRush programming competition and Polymer Tech Expo 2026.

## Overview

The agent controls two units that share six actions each turn. It must decide how to allocate those actions, which unit should move first, which visible resource targets are worth pursuing, and when to explore instead of chasing a contested target.

The environment is challenging because of:

- partial observability and fog-of-war;
- stochastic gold generation;
- obstacles and bombs;
- competing players and NPCs;
- execution-order effects when multiple agents race for the same resource;
- a strict latency requirement for every decision.

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
Six Final Actions
```

The current planner separates pathfinding from strategic decision making. BFS answers **how to reach a target**, while the scoring layer decides **which target is worth pursuing**.

## Strategy Evolution

### V1 — Grid Pathfinder

- Represented the game as a 2D grid.
- Implemented move validation.
- Built BFS shortest-path search.
- Added distance tracking, parent pointers, path reconstruction, and action conversion.

### V2 — C++ Fast Agent

The Python prototype was migrated to C++17 to reduce decision latency and support the competition's Linux shared-object submission format.

Key additions:

- visible-cell BFS;
- dynamic target selection;
- C++ shared-library deployment;
- Docker-based Linux/x86-64 compilation.

### V2.1 — Dynamic Action Allocation

The two units share six actions per turn. Instead of using a fixed split, the agent evaluates different values of `k`, where:

```text
unit 0 receives actions [0, k)
unit 1 receives actions [k, 6)
```

This allows movement capacity to be allocated to the unit with the stronger opportunity.

### V2.2 — Competition-Aware Planning

Target quality is no longer based only on raw gold value and distance. The agent also considers whether an opponent or NPC is likely to reach the target first.

This turns the target-selection problem into an expected-value trade-off between:

- resource value;
- path distance;
- opponent distance;
- NPC pressure;
- execution priority.

### V2.3 — Joint Planner

The current design evaluates action allocation and execution order together. Candidate plans compare multiple `k` values and both unit execution orders using the routes that would actually be executed.

The planner also supports:

- chained multi-target collection;
- conservative handling of contested gold;
- center-biased exploration when no strong visible target exists;
- selective vision purchasing.

## Performance Engineering

The production agent is implemented in C++17 and compiled into a Linux x86-64 `.so` file using Docker.

Typical workflow:

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

Public-test decision latency has been around the low-microsecond range in recent iterations.

## Experimental Framework

The project now includes an automated tournament workflow for repeated public-test submissions against multiple opponents and maps.

One recent benchmark used:

- 3 maps;
- 6 opponents;
- 3 repetitions per opponent/map pair;
- 54 total matches.

This experimental setup is intended to measure not only overall win rate, but also map-dependent performance and opponent-specific weaknesses.

## Current Research Questions

1. Why does win rate vary across different map topologies?
2. When should Manhattan distance be replaced by true BFS race distance for opponent prediction?
3. How should the two units divide exploration versus exploitation roles?
4. Can opponent behavior be profiled from match logs and used to adapt target scoring?
5. Can map-specific parameters outperform one global parameter set without overfitting?

## Repository Structure

```text
.
├── BFS.py                # Early Python BFS prototype
├── player.cpp            # Current C++ competition agent
├── Development_Log.md    # Iteration history and research notes
├── README.md
└── .gitignore
```

The repository intentionally keeps the early BFS prototype to show the progression from basic shortest-path search to competitive multi-agent decision making.

## Future Work

- map-adaptive strategy parameters;
- automated log collection and analysis;
- opponent strategy profiling;
- stronger race-distance estimation;
- local baseline opponents and agent tournaments;
- systematic parameter search using match-level data.

## Motivation

The project is designed as both an algorithmic engineering exercise and an applied decision-making research project. It combines graph algorithms, game strategy, low-latency C++, experimental evaluation, and iterative model design in a single system.
