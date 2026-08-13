# Experiments

This directory documents the empirical evaluation of the GoldRush agent.

## Evaluation Goal

Decision latency alone does not determine match performance. The experimental framework therefore measures strategy robustness across different maps and opponents.

## Current Controlled Tournament

The current benchmark uses a balanced factorial design:

| Dimension | Values |
|---|---:|
| Maps | 3 |
| Opponents | 6 |
| Repetitions per map/opponent pair | 3 |
| Total matches | 54 |

The six opponent model IDs used by the tournament runner are:

```text
104077
395768
338303
72594
306066
386591
```

The experiment can be reproduced with the root-level `goldrush_auto_battle_v2.py` script after setting a valid local authentication token and compiling `player.so`.

Example:

```bash
python3 goldrush_auto_battle_v2.py --map 1 --repeats 3
python3 goldrush_auto_battle_v2.py --map 2 --repeats 3
python3 goldrush_auto_battle_v2.py --map 3 --repeats 3
```

Authentication credentials are intentionally not stored in the repository.

## Results Schema

Raw match outcomes should be recorded in `results.csv` using the following fields:

```text
game_id,map_id,opponent_id,repeat,result,my_gold,opp_gold,gold_diff
```

One row represents one match.

## Planned Analysis

### 1. Overall performance

- total games;
- wins and losses;
- overall win rate;
- average gold difference.

### 2. Map sensitivity

For each map:

- games;
- win rate;
- average own gold;
- average gold difference.

This tests whether a single global strategy generalizes across map topology.

### 3. Opponent sensitivity

For each opponent:

- games;
- win rate;
- average gold difference.

This identifies opponent styles against which the current planner is systematically weak.

### 4. Map × Opponent matrix

The most useful controlled comparison is a 3 × 6 matrix where every cell contains three repeated matches.

This separates effects such as:

```text
map-specific weakness
```

from:

```text
opponent-specific weakness
```

## Results

The 54-match tournament has been executed. Final statistics will be populated after the downloaded match logs/results are consolidated into `results.csv`.

| Map | Games | Wins | Win Rate | Avg Gold Diff |
|---|---:|---:|---:|---:|
| Map 1 | 18 | TBD | TBD | TBD |
| Map 2 | 18 | TBD | TBD | TBD |
| Map 3 | 18 | TBD | TBD | TBD |
| **Overall** | **54** | **TBD** | **TBD** | **TBD** |

## Hypotheses to Test

1. Performance differs materially across map topology.
2. Manhattan-distance competition estimates are less reliable on obstacle-heavy maps.
3. Exploration parameters that work well on one map may over-explore or under-explore on another.
4. Certain opponent policies may systematically exploit contested-target behavior.
5. Map-adaptive parameters may improve robustness, but should be tested against the same 54-match design to avoid judging changes from anecdotal matches.

## Experimental Discipline

Future strategy versions should be evaluated against a frozen baseline. A change should ideally modify one strategic hypothesis at a time and use the same map/opponent matrix.

```text
Baseline
   ↓
One strategy change
   ↓
Same controlled tournament
   ↓
Compare map/opponent breakdown
   ↓
Accept, reject, or revise hypothesis
```

This makes the project an iterative research workflow rather than a sequence of untracked heuristic changes.
