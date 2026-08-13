# Agent Architecture

## Objective

GoldRush is treated as a constrained online planning problem. At each turn, two controlled units share a budget of six actions. The planner must transform a partially observed state into a joint action sequence while balancing immediate resource collection, competition risk, exploration, and computation time.

## Decision Pipeline

```text
┌───────────────────────────┐
│        GameInput          │
│ grid / units / enemies /  │
│ NPCs / snapshot / gold    │
└─────────────┬─────────────┘
              ↓
┌───────────────────────────┐
│     State Processing      │
│ walkability / visibility  │
│ blocked and risky cells   │
└─────────────┬─────────────┘
              ↓
┌───────────────────────────┐
│   Candidate Generation    │
│ visible gold + fallback   │
│ exploration opportunities │
└─────────────┬─────────────┘
              ↓
┌───────────────────────────┐
│       Path Planning       │
│ BFS distance and actions  │
└─────────────┬─────────────┘
              ↓
┌───────────────────────────┐
│   Competition Adjustment  │
│ enemy / NPC race pressure │
└─────────────┬─────────────┘
              ↓
┌───────────────────────────┐
│    Joint Plan Search      │
│ route0, route1, k, order  │
└─────────────┬─────────────┘
              ↓
┌───────────────────────────┐
│ Exploration / Vision Rule │
└─────────────┬─────────────┘
              ↓
┌───────────────────────────┐
│        GameOutput         │
│ actions[6], k, order, vp  │
└───────────────────────────┘
```

## 1. State Processing

The game grid contains known terrain, fog, obstacles, bombs, and visible gold. The planner builds a local representation suitable for fast search and treats clearly unsafe/unwalkable cells as blocked.

The state also contains two friendly units, visible enemies, visible NPCs, held gold, and occasional global snapshot statistics.

## 2. Path Planning

The project started with standard BFS because movement on the grid has unit cost. BFS provides both shortest distance and a parent structure for reconstructing the action sequence.

The important engineering separation is:

```text
Pathfinder: Where can I go and how?
Decision layer: Which destination is worth going to?
```

This separation made it possible to improve strategy without replacing the core routing primitive.

## 3. Target Utility

A resource target should not be evaluated only by its face value. A useful conceptual model is:

```text
utility(target, unit)
    ≈ collectible value
      - movement cost
      - competition risk
      - execution risk
```

The exact implementation is intentionally lightweight because the function is evaluated repeatedly under a strict latency budget.

## 4. Dynamic Action Allocation (`k`)

There are six total actions. `k` defines the split:

```text
unit 0: actions [0, k)
unit 1: actions [k, 6)
```

A fixed split is inefficient because opportunity quality changes every turn. The planner therefore compares candidate allocations based on the value each unit can actually realize with its allocated number of actions.

## 5. Execution Order

The output also specifies which friendly unit executes first. Order can matter when both units interact with nearby resources or when one unit's movement changes what remains useful for the second unit.

The current architecture therefore treats `k` and execution order as parts of the joint plan rather than unrelated constants.

## 6. Competitive Pressure

Visible opponents and NPCs affect expected target value. If another actor is likely to arrive first, the target should be discounted or abandoned.

A major future improvement is replacing approximate race estimates with true obstacle-aware BFS distances for all relevant competitors when the latency budget permits.

## 7. Exploration

Partial observability creates turns where exploitation is not clearly profitable. In these cases the agent uses fallback exploration rather than remaining idle or moving randomly.

This produces a simple policy hierarchy:

```text
strong visible opportunity → collect
contested/weak opportunity → compare alternatives
no useful visible target   → explore
```

## 8. Performance Constraints

The planner is implemented in C++17 and deployed as a Linux x86-64 shared object. The design deliberately favors small fixed-size data structures and bounded searches over heavyweight optimization methods.

This project therefore has two simultaneous objectives:

1. improve strategic expected value;
2. keep per-turn computation extremely small.

## 9. Experimental Loop

Strategy changes are evaluated empirically rather than accepted solely because they appear reasonable.

```text
Hypothesis
   ↓
Code change
   ↓
Compile / validate
   ↓
Controlled matches
   ↓
Logs and win-rate breakdown
   ↓
Failure analysis
   ↓
Next hypothesis
```

The current tournament design spans multiple maps and opponents so that improvements can be separated from map-specific or opponent-specific effects.
