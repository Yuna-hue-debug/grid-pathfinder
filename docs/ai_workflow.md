# AI-Assisted Development Workflow

This project used AI as a research and engineering copilot rather than as a replacement for the decision engine itself. The production agent remains a deterministic C++ planner, while AI supported diagnosis, hypothesis generation, implementation review, debugging, experimentation, and documentation.

## Workflow

```text
Observed failure
      ↓
AI-assisted diagnosis
      ↓
Strategy hypothesis
      ↓
Controlled test
      ↓
Human review
      ↓
C++ implementation
      ↓
Automated tournament
      ↓
New evidence
      └──────────────→ repeat
```

## Example Iteration 1 — From Fixed Allocation to Dynamic `k`

### Observation

A fixed 4/2 action split often wasted movement capacity because one unit could have a strong nearby opportunity while the other had little useful work.

### AI-assisted step

AI helped decompose the decision into a measurable allocation problem and suggested testing all legal split points:

```text
k ∈ {0,1,2,3,4,5,6}
```

### Human validation

Behavior tests were created for three cases:

- unit 0 has the stronger opportunity;
- unit 1 has the stronger opportunity;
- both units have balanced opportunities.

### Result

The agent moved from a hard-coded split toward state-dependent action allocation.

---

## Example Iteration 2 — From Nearest Gold to Competition-Aware Scoring

### Observation

The shortest or highest-value target was not always profitable because an opponent or NPC could arrive first.

### AI-assisted step

AI helped formulate target selection as an expected-value problem combining:

- gold value;
- route length;
- enemy distance;
- NPC pressure;
- execution-order risk.

### Human validation

Match logs were inspected to verify whether apparently good targets were repeatedly lost to earlier-moving competitors.

### Result

Contested targets are now discounted, allowing the agent to switch toward less obvious but more achievable resources.

---

## Example Iteration 3 — From Independent Planning to Joint Planning

### Observation

Optimizing each unit separately could create conflicts, duplicated effort, or unused actions.

### AI-assisted step

AI helped identify a mismatch between target selection, `k` selection, and the routes actually executed.

### Result

The current planner evaluates candidate `k` values and both execution orders using the routes that would actually be executed. It also supports chained multi-target collection within the six-action turn budget.

---

## Example Iteration 4 — From Manual Testing to Automated Tournamenting

### Observation

Single-match testing was too noisy to distinguish real improvements from map/opponent variance.

### AI-assisted step

AI helped design a controlled evaluation matrix and automate repeated submissions.

### Current experiment

```text
3 maps × 6 opponents × 3 repetitions = 54 matches
```

This enables comparisons such as:

```text
same agent + same opponent + different map
```

and

```text
same map + same agent + different opponent
```

### Result

The project shifted from ad-hoc strategy tuning toward evidence-driven iteration.

---

## Role of Human Judgment

AI suggestions were not accepted automatically. The development loop required manual validation through:

- unit tests and behavior tests;
- compilation and ABI checks;
- competition-server matches;
- log inspection;
- latency checks;
- controlled comparisons across maps and opponents.

The main value of AI was accelerating the cycle from failure observation to a testable hypothesis.

## Tools and Platforms

- ChatGPT — strategy reasoning, debugging, code review, experimentation design, documentation
- Python — early BFS prototype and tournament automation
- C++17 — production decision engine
- Docker — Linux/x86-64 build environment on macOS
- GitHub — source control and project documentation
- GoldRush public test server — empirical evaluation environment

## Example Prompt Patterns

Representative prompt styles used during development included:

```text
Analyze these match logs and identify repeated strategic failure modes.
```

```text
Given two units sharing six actions, design a scoring function for k that balances target value and route length.
```

```text
Compare the old and new strategy and propose a controlled experiment that can isolate whether the change improves win rate.
```

```text
Inspect this C++ planner for wasted actions, target conflicts, and latency-heavy logic.
```

These prompts were used to generate hypotheses and implementation ideas that were then tested rather than treated as ground truth.

## Why This Matters

The AI component of this project is not an LLM embedded inside the runtime agent. Instead, AI is integrated into the software-development and research loop:

```text
AI-assisted analysis + deterministic low-latency execution
```

This structure is appropriate for a latency-sensitive environment where runtime calls to a large model would be impractical, but AI can still accelerate strategy research and engineering outside the critical execution path.
