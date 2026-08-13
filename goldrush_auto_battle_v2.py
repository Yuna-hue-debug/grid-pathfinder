#!/usr/bin/env python3
import argparse
import os
import time
from datetime import datetime
from pathlib import Path
import requests

BASE_URL = "http://47.103.127.219"

DEFAULT_OPPONENTS = [
    104077,
    395768,
    338303,
    72594,
    306066,
    386591,
]

def auth_headers(token):
    return {
        "Authorization": token,
        "Accept": "application/json, text/plain, */*",
        "Origin": BASE_URL,
        "Referer": BASE_URL + "/dashboard/",
    }

def submit_match(session, token, so_path, map_id, opponent_id, model_name):
    with open(so_path, "rb") as f:
        r = session.post(
            BASE_URL + "/api/user/add_model_1",
            headers=auth_headers(token),
            data={
                "map_id": str(map_id),
                "model_id": str(opponent_id),
                "model_langs": "2",
                "model_names": model_name,
            },
            files={"model_files": ("player.so", f, "application/octet-stream")},
            timeout=60,
        )
    r.raise_for_status()
    payload = r.json()
    if payload.get("code") != 0:
        raise RuntimeError(payload)
    return payload["data"]["game_id"]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--so", default="player.so")
    parser.add_argument("--map", type=int, default=2)
    parser.add_argument("--opponents", type=int, nargs="*", default=DEFAULT_OPPONENTS)
    parser.add_argument("--repeats", type=int, default=1)
    parser.add_argument("--delay", type=float, default=3.0)
    args = parser.parse_args()

    token = os.environ.get("GOLDRUSH_TOKEN")
    if not token:
        raise SystemExit("Missing GOLDRUSH_TOKEN.")

    so_path = Path(args.so)
    if not so_path.exists():
        raise SystemExit(f"Cannot find {so_path}")

    session = requests.Session()
    total = len(args.opponents) * args.repeats
    n = 0

    for opponent_id in args.opponents:
        for rep in range(1, args.repeats + 1):
            n += 1
            model_name = "auto" + datetime.now().strftime("%H%M%S")
            try:
                game_id = submit_match(
                    session, token, so_path, args.map, opponent_id, model_name
                )
                print(f"[{n}/{total}] OK map={args.map} opponent={opponent_id} game_id={game_id}")
            except Exception as e:
                print(f"[{n}/{total}] ERROR map={args.map} opponent={opponent_id}: {e}")
            if n < total:
                time.sleep(args.delay)

if __name__ == "__main__":
    main()
