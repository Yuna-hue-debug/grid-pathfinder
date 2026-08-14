# Log-driven V3 optimization

This iteration is based on the downloaded competition logs rather than on a new heuristic chosen in isolation.

## Repeated failure mode: NPC congestion / trampling

The strongest repeated signal in the current logs is that the agent keeps entering cells where several NPCs converge. Examples include repeated trample penalties around the central corridor such as `(7,8)`, `(8,8)`, `(8,9)`, `(9,9)` and nearby cells. In several late-game snapshots the agent is already behind in gold while continuing to take these congestion penalties.

The current V2 code only uses NPC positions to discount a gold target. It does **not** prevent a route to another target from passing through an NPC cluster. This is the key mismatch addressed by V3.

## V3 change

Add an `npcDangerCells` layer before route planning:

1. Count visible NPCs per cell.
2. If two or more visible NPCs occupy the same cell, mark that cell as blocked for route planning.
3. If three or more visible NPCs occupy the same cell, also mark its four orthogonal neighbors as dangerous, except the agent's current cells.
4. Merge these danger cells with visible enemy cells before BFS / target planning.

This is deliberately conservative only when there is direct evidence of clustering. A single NPC remains a competitor rather than a hard obstacle.

## Why not simply avoid every NPC?

NPCs can also indicate active gold-producing regions, so globally treating every NPC as an obstacle would throw away too much expected value. The logs specifically show losses when **multiple** NPCs converge on the same small area. V3 therefore distinguishes ordinary competition from congestion risk.

## Secondary signal: rich regions left unused

Snapshots occasionally show a region with substantial `gold_remaining` while the current local region is crowded. This suggests a future V3.1 improvement: use snapshot region statistics to bias exploration away from exhausted/crowded regions and toward high remaining-gold regions. This requires a reliable mapping from region IDs to board coordinates, so it is intentionally not guessed in this patch.

## Evaluation

Compare V2 and V3 on the same map/opponent matrix. Track not only win rate and gold difference, but also:

- total `burned` gold;
- number of `trample_events` affecting our units;
- gold collected per 100 rounds;
- vision spent;
- late-game gold deficit.

The V3 hypothesis is successful if trample loss falls materially without reducing gold collection enough to offset the benefit.
