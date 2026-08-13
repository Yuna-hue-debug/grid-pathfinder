#!/usr/bin/env python3
from pathlib import Path
import csv
from collections import defaultdict

RESULTS = Path(__file__).with_name("results.csv")


def load_rows():
    with RESULTS.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    cleaned = []
    for row in rows:
        if not row.get("game_id"):
            continue
        row["map_id"] = int(row["map_id"])
        row["opponent_id"] = int(row["opponent_id"])
        row["repeat"] = int(row["repeat"])
        row["my_gold"] = int(row["my_gold"])
        row["opp_gold"] = int(row["opp_gold"])
        row["gold_diff"] = int(row["gold_diff"])
        row["result"] = row["result"].strip().upper()
        cleaned.append(row)
    return cleaned


def summarize(rows, key_fn):
    groups = defaultdict(list)
    for row in rows:
        groups[key_fn(row)].append(row)

    output = []
    for key in sorted(groups, key=lambda x: str(x)):
        group = groups[key]
        wins = sum(r["result"] == "WIN" for r in group)
        games = len(group)
        avg_diff = sum(r["gold_diff"] for r in group) / games
        output.append((key, games, wins, 100.0 * wins / games, avg_diff))
    return output


def print_table(title, summary):
    print(f"\n{title}")
    print("-" * len(title))
    print(f"{'Group':<20} {'Games':>6} {'Wins':>6} {'WinRate':>9} {'AvgDiff':>10}")
    for key, games, wins, win_rate, avg_diff in summary:
        print(f"{str(key):<20} {games:>6} {wins:>6} {win_rate:>8.1f}% {avg_diff:>10.1f}")


def main():
    rows = load_rows()
    if not rows:
        print("results.csv contains no match rows yet.")
        return

    games = len(rows)
    wins = sum(r["result"] == "WIN" for r in rows)
    avg_diff = sum(r["gold_diff"] for r in rows) / games

    print("GoldRush Tournament Summary")
    print("===========================")
    print(f"Games: {games}")
    print(f"Wins: {wins}")
    print(f"Overall win rate: {100.0 * wins / games:.1f}%")
    print(f"Average gold difference: {avg_diff:.1f}")

    print_table("By Map", summarize(rows, lambda r: r["map_id"]))
    print_table("By Opponent", summarize(rows, lambda r: r["opponent_id"]))
    print_table(
        "Map x Opponent",
        summarize(rows, lambda r: f"M{r['map_id']} / {r['opponent_id']}")
    )


if __name__ == "__main__":
    main()
