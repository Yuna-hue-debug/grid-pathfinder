# Competition Submission Update - 2026-08-18

This note records the final pre-competition engineering and research workflow after V9 was selected as the champion.

## Final strategy decision

V9 remained the final competition agent. Later challengers (V9.1/V9.2/V9.3 and V10) explored stuck-unit recovery, vision-triggered escape logic, and topology-aware exploration, but they did not consistently outperform V9 across maps. The project therefore kept the empirically strongest stable baseline instead of promoting the newest version.

## Large-scale opponent testing

The testing workflow was extended from a small fixed opponent set to the public stage-4 opponent pool. The browser API was used to retrieve the available opponent model list, and the research runner was upgraded to support resumable all-opponent battle submission.

Key engineering additions:

- discovered and consumed the public opponent-list endpoint;
- expanded the test pool to 234 opponent models at the time of collection;
- added deterministic, server-valid model naming;
- added CSV ledgers mapping map/opponent/repeat to `game_id`;
- added resume support so interrupted tournament runs continue without resubmitting completed games;
- built a fixed 30-opponent benchmark panel for cross-map comparison;
- identified the game-log endpoint and automated NDJSON log collection/packaging.

The research loop therefore evolved into:

```text
Opponent pool
    -> automated battle matrix
    -> game_id ledger
    -> automated log collection
    -> structured failure analysis
    -> challenger hypothesis
    -> cross-map validation
```

## Log-analysis lesson

A preliminary batch of Map 1 logs reinforced an earlier warning from the V9.x experiments: high vision spending and excessive idle/stuck behavior can appear together in difficult states. However, simple back-and-forth movement by itself was not a sufficient explanation for losses. This shifted the analysis away from visually obvious symptoms toward the state transition that causes the agent to enter an inefficient recovery regime.

The main methodological lesson is to avoid treating correlation as a strategy fix. A visible failure should first be tested across many wins and losses before it becomes a code change.

## Official competition build

The final V9 source was frozen and transferred to Ubiquant's official `quant-compiler` machine. The organizer-provided build environment reported GCC 14.3.1 on x86-64 Linux.

The final build was produced with the project Makefile and verified as:

```text
ELF 64-bit LSB shared object, x86-64
exported symbol: moveDecision
```

The exact binary was copied back to the local machine and its SHA-256 hash was checked against the hash produced on the official compiler. The two hashes matched before the `.so` was uploaded to the competition system.

This added an important deployment lesson to the project: reproducibility and artifact provenance matter as much as local correctness when a competition requires source-to-binary verification.

## Updated learning trajectory

```text
BFS fundamentals
    -> Python prototype
    -> C++17 low-latency agent
    -> Linux shared-library deployment
    -> multi-agent target / k / order planning
    -> automated tournaments
    -> win/loss log analysis
    -> champion/challenger testing
    -> cross-map robustness experiments
    -> large opponent-pool benchmarking
    -> automated log acquisition
    -> official reproducible competition build
```

The project is now best described not only as a game-playing agent, but as a compact quantitative research and systems-engineering workflow: build a baseline, instrument it, collect evidence, form a minimal hypothesis, test against a stable champion, reject unsupported complexity, and preserve a reproducible final artifact.
