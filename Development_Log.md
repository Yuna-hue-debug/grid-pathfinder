# Development Log

This log documents the evolution of the project from a basic Python grid pathfinder into a low-latency C++ multi-agent decision engine for GoldRush.

---

## 2026-08-06 - Grid Representation and Move Validation

### Goal
Build the minimum environment representation needed before implementing search.

### What I learned
- A game map can be represented as a 2D grid.
- Row/column coordinates must be handled consistently.
- Pathfinding requires a reliable legality check before expanding neighboring states.

---

## 2026-08-07 to 2026-08-11 - From BFS to a Competitive Planner

The project progressed through BFS shortest-path search, distance tracking, parent-based path reconstruction, action generation, C++17 migration, dynamic `k` allocation, value/distance trade-offs, competition-aware target scoring, multi-target routing, joint two-unit planning, exploration, and selective vision purchasing.

A central architectural lesson was to separate routing from strategy:

```text
BFS: how can I reach a target?
Decision layer: which target is worth pursuing?
Joint planner: how should six actions be allocated between two units?
```

The C++ version is built as a Linux/x86-64 shared library and validated with `file` and `nm` before submission.

---

## 2026-08-11 to 2026-08-13 - Automated Tournament Testing

Repeated public-test submissions were automated across multiple maps and opponents. This shifted development from intuition-driven heuristic changes toward controlled empirical comparison.

The key lesson was that low latency alone is not enough: strategy must be evaluated across different map topologies and opponents.

---

## 2026-08-15 - V8 Failure Analysis

V8 was treated as the champion rather than being overwritten by every new heuristic. Match logs from both wins and losses were compared to identify behavioral variables correlated with poor outcomes.

### Main observations

1. **Inefficient action usage.** Losing games showed more `STAY` behavior.
2. **Over-spending on vision.** Losing logs tended to spend more on vision than winning logs.
3. **Execution-order degeneracy.** Although V8 searched both execution orders, the selected order was overwhelmingly `order = 0`.
4. **NPC activity was not sufficiently discriminative.** Visible trample events did not justify another large NPC-risk heuristic.

### Research lesson

Do not add complexity merely because a failure is visible in a log. Compare winning and losing samples first and ask whether the feature actually separates them.

---

## 2026-08-15 - V9: Minimal Log-Driven Challenger

V9 deliberately remained close to V8. It introduced three small changes corresponding to the observed failure modes.

### Exploration separation

The two units receive a light directional/lane preference when otherwise similar frontier cells are available. This is a tie-breaker rather than a hard map partition.

### Execution-order urgency

A small urgency bonus is added when the first-executing unit is in a genuine race for its first target. This makes `order` strategically relevant without dominating expected resource value.

### More conservative vision purchasing

Vision remains available, but V9 requires weaker visible opportunity and substantial remaining fog before paying for additional information.

### What V9 intentionally did not add

- no large NPC cluster penalty;
- no hard partition between units;
- no complete scoring rewrite;
- no replacement of V8's expected-value framework.

---

## 2026-08-15 - V9 Champion/Challenger Validation

V8 remained the champion while V9 was tested as a challenger.

**V9 defeated V8 on all three public-test maps in the direct simulation validation and was promoted.**

The important workflow became:

```text
Winning + losing logs
        ↓
Measure behavioral differences
        ↓
Identify repeated failure modes
        ↓
Make minimal targeted changes
        ↓
Keep old version as champion
        ↓
Run direct challenger comparison
        ↓
Promote only after empirical improvement
```

---

## 2026-08-17 - New Map and Cross-Map Robustness

A newly introduced fifth map contained stronger corridor, wall, bottleneck, and connectivity effects. This exposed an important generalization question: a strategy that performs well on one topology may not transfer cleanly to another.

Rather than hard-coding a `map_id` strategy, the analysis reframed maps through structural properties such as:

- obstacle density;
- connectivity;
- corridor width;
- central accessibility;
- fog exposure;
- competition density.

This is important for hidden-map robustness because map-specific constants can overfit public tests.

---

## 2026-08-17 - V10 Topology Experiment

### Hypothesis

V9's frontier exploration might undervalue narrow entrances leading to large unseen regions. A topology-aware planner might improve performance on corridor/maze maps.

### Changes tested

V10 introduced three ideas:

1. larger-radius fog/frontier potential;
2. accessibility toward the central resource region rather than relying only on geometric center distance;
3. a stronger penalty for unused action budget.

### Result

**V9 won the direct V9-vs-V10 tests. V10 was rejected.**

### Why the negative result matters

The V10 experiment revealed two important mistakes in the initial hypothesis implementation:

- topology/exploration value can become too influential relative to immediate expected gold;
- `STAY` is a useful diagnostic signal, but penalizing idle actions directly can reward movement that is active but not productive.

In other words:

```text
moving more != earning more
more sophisticated heuristic != stronger agent
```

This was an important correction from correlation toward causal thinking.

---

## 2026-08-17 - Competition Freeze

Further heuristic optimization was stopped before the preliminary competition.

**V9 is frozen as the final competition version.**

The decision is evidence-based rather than version-number-based:

```text
V8 baseline
    ↓
V9 beats V8 -> PROMOTE
    ↓
V10 attempts topology improvement
    ↓
V9 beats V10 -> REJECT V10
    ↓
V9 FINAL
```

The project therefore preserves negative results rather than assuming the newest version must be the best version.

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
Exploration fallback / selective vision
   ↓
GameOutput
```

---

## Current Status

- [x] BFS shortest-path search and path reconstruction
- [x] Python BFS prototype
- [x] Python-to-C++17 migration
- [x] Linux/x86-64 `.so` deployment through Docker
- [x] Dynamic `k` allocation
- [x] Competition-aware target scoring
- [x] Joint two-unit planning
- [x] Multi-target collection
- [x] Exploration under fog-of-war
- [x] Selective vision purchasing
- [x] Automated repeated match submission
- [x] Win/loss log analysis
- [x] Champion/challenger versioning
- [x] V9 promoted after beating V8
- [x] Cross-map/topology analysis after the fifth map appeared
- [x] V10 tested and rejected after losing to V9
- [x] V9 frozen as final competition version

---

## Learning Trajectory

The technical learning path can be summarized as:

```text
Grid representation
      ↓
BFS queue + visited
      ↓
Distance + parent
      ↓
Path reconstruction
      ↓
Action generation
      ↓
Python prototype
      ↓
C++17 implementation
      ↓
Docker + Linux shared library
      ↓
Target scoring
      ↓
Two-unit joint planning
      ↓
k + order optimization
      ↓
Fog / vision / opponent reasoning
      ↓
Automated matches
      ↓
Log analysis
      ↓
Champion/challenger experiments
      ↓
Cross-map robustness
      ↓
Rejecting unsupported complexity
```

The largest shift was methodological. Early development focused on making an algorithm work. Later development focused on determining whether a proposed improvement actually works.

---

## AI-Assisted Research Loop

AI was used outside the latency-critical runtime loop to accelerate learning and experimentation:

```text
Game / match logs
       ↓
AI-assisted diagnosis
       ↓
Human-reviewed hypothesis
       ↓
Code modification
       ↓
Compilation / deployment
       ↓
Battle test
       ↓
Evidence
       ↓
Keep or reject
```

AI helped explain algorithms, inspect logs, generate hypotheses, review code, design comparisons, and document findings. The runtime competition agent remains deterministic C++.

---

## Reflection

The project began as a BFS learning exercise and became a small quantitative research workflow. The final lesson was not simply how to write a faster agent. It was how to separate routing from strategy, build reproducible artifacts, use logs rather than anecdotes, preserve a baseline, test challengers, learn from negative results, and stop optimizing when additional complexity is no longer supported by evidence.
