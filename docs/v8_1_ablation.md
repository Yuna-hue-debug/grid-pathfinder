# V8.1 Ablation: When More Complexity Made the Agent Worse

## Why this iteration exists

A direct head-to-head match between the previously strong V8 agent and the newer, more complex agent produced an unexpected result: the newer version lost decisively. The important lesson was not simply that one version won; it was that several individually reasonable risk controls had been added at once, making it impossible to know which change actually helped or hurt.

This changed the development strategy from **heuristic accumulation** to **ablation-based research**.

## Observation

The newer agent added stronger rival-distance estimation, NPC-density awareness, route-level danger handling and more conservative exploration. These changes were intended to reduce local failures such as losing contested piles or entering NPC-heavy areas.

However, the head-to-head result showed that reducing local risk did not necessarily increase total reward. The older V8 policy remained more aggressive around high-value opportunities and accumulated substantially more gold.

## Research lesson

The optimization objective is not:

```text
minimize every visible failure mode
```

It is:

```text
maximize total expected match reward
```

Avoiding a possible 10-20 gold loss can be harmful if the same rule repeatedly prevents entry into a productive region. A more complicated model is not automatically a better model.

## V8.1 hypothesis

Return to V8 as the baseline and add only two isolated changes:

1. **Cross-unit target de-duplication**
   - When the first unit is already planned to collect a pile, the second unit does not independently count that same pile as another reward opportunity.
   - This fixes a planning-accounting issue without changing V8's exploration style.

2. **Soft NPC-cluster target penalty**
   - NPC congestion reduces the score of a pile only when multiple visible NPCs are directly on or immediately around it.
   - NPCs do not become hard obstacles and their neighboring cells are not globally blocked.
   - This preserves V8's aggressive access to productive regions.

Everything else is intentionally kept close to V8.

## What V8.1 deliberately does NOT include

- no hard NPC danger-zone blocking;
- no heavy route-level risk aversion;
- no additional BFS rival-distance field;
- no broad exploration-separation rule;
- no large bundle of simultaneous parameter changes.

This makes the experiment interpretable.

## Evaluation design

V8 remains the champion/baseline. V8.1 is the challenger.

Run both versions on the same map/opponent conditions and compare:

- win rate;
- final gold;
- gold difference;
- number of idle/STAY actions;
- major trample losses;
- performance by map.

If V8.1 fails to improve on V8, reject the change rather than adding another heuristic on top of it.

## Updated research loop

```text
Strong baseline (V8)
      ↓
One small hypothesis
      ↓
V8.1 challenger
      ↓
Controlled head-to-head test
      ↓
Keep only measured improvements
      ↓
Next isolated hypothesis
```

## Reflection

This iteration became one of the most useful parts of the project. Earlier development often assumed that a newly identified failure mode should immediately produce a new rule. The V8-versus-latest experiment showed why systematic strategy research needs baselines, ablations and falsifiable hypotheses. The project therefore evolved from "keep adding smarter logic" toward evidence-driven model selection.
