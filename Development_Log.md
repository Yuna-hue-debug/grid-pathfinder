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

---

## 2026-08-07 to 2026-08-11 — From BFS to a Competitive Planner

The project progressed through BFS shortest-path search, distance tracking, parent-based path reconstruction, action generation, C++17 migration, dynamic `k` allocation, value/distance trade-offs, competition-aware target scoring, multi-target routing, joint two-unit planning, exploration, and selective vision purchasing.

A central architectural lesson was to separate routing from strategy:

```text
BFS: how can I reach a target?
Decision layer: which target is worth pursuing?
Joint planner: how should six actions be allocated between two units?
```

The C++ version is built as a Linux/x86-64 shared library and validated with `file` and `nm` before submission.

---

## 2026-08-11 to 2026-08-13 — Automated Tournament Testing

Repeated public-test submissions were automated across multiple maps and opponents. This shifted development from intuition-driven heuristic changes toward controlled empirical comparison.

The key lesson was that low latency alone is not enough: strategy must be evaluated across different map topologies and opponents.

---

## 2026-08-15 — V8 Failure Analysis

### Baseline

V8 was treated as the champion rather than being overwritten by every new heuristic. Match logs from both wins and losses were compared to identify behavioral variables correlated with poor outcomes.

### Main observations

The win/loss comparison suggested three recurring issues:

1. **Inefficient action usage.** Losing games showed more `STAY` behavior, indicating that the six-action budget was not always converted into productive movement or collection.
2. **Over-spending on vision.** Losing logs tended to spend materially more on vision than winning logs. This suggested a possible negative feedback loop: weak visible opportunities caused more information spending, which reduced retained gold without guaranteeing better collection.
3. **Execution-order degeneracy.** Although V8 nominally searched both execution orders, the selected order was overwhelmingly `order = 0`. The objective often did not distinguish the two orders strongly enough, so execution-order search was close to a tie-breaking artifact rather than a meaningful strategic variable.

An important negative result was also retained: NPC/trample activity alone did not explain the performance gap well enough to justify another large NPC-risk heuristic.

### Research lesson

Do not add complexity merely because a failure is visible in a log. First compare winning and losing samples and ask whether the feature actually separates them.

---

## 2026-08-15 — V9: Minimal Log-Driven Challenger

### Design principle

V9 deliberately remained close to V8. Instead of rewriting the planner, it introduced three small changes corresponding directly to the observed V8 failure modes.

### 1. Exploration separation

V8's center-biased frontier exploration was preserved, but the two units receive a light directional/lane preference when otherwise similar frontier cells are available.

The purpose is not to hard-partition the map. It is a tie-breaking signal intended to reduce duplicated exploration and increase information gained per movement action.

### 2. Execution-order urgency

V9 adds a small urgency bonus when the unit executing first is in a genuine race with an opponent or NPC for its first target.

This makes `order` strategically relevant without allowing execution priority to dominate expected resource value.

### 3. More conservative vision purchasing

Vision remains available, but V9 requires a weaker visible opportunity and a sufficiently incomplete board before purchasing additional information.

This directly addresses the observation that losing V8 games tended to spend more on vision.

### What V9 intentionally did NOT add

- no large NPC cluster penalty;
- no aggressive hard partition between the two units;
- no complete scoring rewrite;
- no replacement of the existing V8 expected-value framework.

This keeps the experiment interpretable: if V9 improves, the cause is more likely to be attributable to the three targeted changes rather than a large bundle of unrelated heuristics.

---

## 2026-08-15 — Champion/Challenger Validation

V8 was retained as the champion and V9 was tested as a challenger on all three public-test maps.

### Result

**V9 defeated V8 on all three public-test maps in the simulation validation.**

This is evidence that the log-driven changes improved the agent within the tested public-map environment. It is not treated as proof that V9 dominates every opponent or unseen map, so V8 remains useful as a historical benchmark.

### Development lesson

The most successful iteration in this stage followed this workflow:

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

This is a stronger development process than repeatedly adding heuristics based on individual matches.

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
- [x] Python-to-C++ migration
- [x] Dynamic `k` allocation
- [x] Competition-aware target scoring
- [x] Joint two-unit planning
- [x] Multi-target collection
- [x] Exploration under fog-of-war
- [x] Automated repeated match submission
- [x] Win/loss log analysis
- [x] Champion/challenger versioning
- [x] V9 validated against V8 on all three public maps
- [ ] Larger V9 sample against diverse opponents
- [ ] Automated match-log download and metric extraction
- [ ] Opponent behavior profiling
- [ ] Hidden-map robustness testing
- [ ] Systematic parameter optimization

---

## Next Research Iteration

The next iteration should resist unnecessary architectural expansion. The priority is to test whether V9's improvement survives larger samples and different opponents.

Useful measurements include:

- win rate by map and opponent;
- `STAY` ratio;
- vision spending;
- `order = 1` selection frequency;
- gold trajectory over the match;
- collection efficiency per movement action.

Only after these metrics are stable should V10 introduce another strategic mechanism.

---

## Reflection

The project has moved beyond learning BFS. The important lesson from V8 → V9 is methodological: competitive-agent development benefits from treating strategy changes as experiments. Logs provide evidence, the baseline provides a control, and a challenger should change as little as necessary to test a hypothesis.
