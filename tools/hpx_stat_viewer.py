#!/usr/bin/env python3
#
# Copyright (c) 2026 Arpit Khandelwal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import sys
import collections
import time
import argparse
import re

BLOCKS = [' ', '▏', '▎', '▍', '▌', '▋', '▊', '▉', '█']
SPARKS = [' ', '▂', '▃', '▄', '▅', '▆', '▇', '█']

def clear_screen():
    print("\033[2J\033[H", end="")

def extract_locality(name):
    match = re.search(r'\{locality#(\d+)', name)
    return int(match.group(1)) if match else 0

def draw_bar(val, max_val, max_width=35):
    if max_val == 0:
        return ""
    scaled = (val / max_val) * max_width
    full_blocks = int(scaled)
    remainder = scaled - full_blocks

    bar = BLOCKS[-1] * full_blocks
    if remainder > 0.05:
        bar += BLOCKS[int(remainder * len(BLOCKS))]

    return bar

def draw_sparkline(history):
    if not history:
        return ""
    if len(history) == 1:
        return SPARKS[0]

    min_val, max_val = min(history), max(history)
    span = max_val - min_val
    if span == 0:
        return SPARKS[0] * len(history)

    sparkline = ""
    for v in history:
        idx = int((v - min_val) / span * (len(SPARKS) - 1))
        sparkline += SPARKS[idx]

    return sparkline.ljust(10)

def detect_anomaly(val, ema):
    """ Detects if the current value deviates wildly from the Exponential Moving Average """
    if ema == 0.0:
        return ""
    ratio = val / ema

    # Ignore anomalies for extremely tiny absolute values to prevent noise
    if ema < 0.5 and val < 0.5:
        return ""

    if ratio > 1.6:
        # Flashing Red Alert High Spike
        return f"\033[5;91m [⚡ SPIKE: +{int((ratio-1)*100)}%] \033[0m"
    elif ratio < 0.4:
        # Flashing Yellow Alert Drop
        return f"\033[5;93m [⚠️ DROP: -{int((1-ratio)*100)}%] \033[0m"
    return ""

def render_dashboard(history_dict, ema_dict, sequence_num):
    clear_screen()
    print(f"\033[93m{'='*20} HPX ADVANCED DASHBOARD (Seq: {sequence_num}) {'='*20}\033[0m\n")

    if not history_dict:
        print("Waiting for HPX counter data that matches filters...")
        return

    latest_values = {k: v[-1] for k, v in history_dict.items() if v}
    if not latest_values:
        return

    all_values = list(latest_values.values())
    global_max = max(all_values) if all_values else 1.0

    # Sort counter names primarily by Locality, then by name
    sorted_names = sorted(latest_values.keys(), key=lambda n: (extract_locality(n), n))

    current_locality = -1

    for name in sorted_names:
        loc = extract_locality(name)
        if loc != current_locality:
            print(f"\n\033[1;36m>> Locality #{loc} \033[0m")
            current_locality = loc

        val = latest_values[name]
        hist = list(history_dict[name])
        ema = ema_dict.get(name, val)

        # Color coding
        color = '\033[92m'  # Green
        if val > global_max * 0.8:
            color = '\033[91m'  # Red
        elif val > global_max * 0.4:
            color = '\033[93m'  # Yellow

        reset = '\033[0m'

        # Shorten extreme long counter names nicely
        short_name = name if len(name) < 45 else "..." + name[-42:]

        bar = draw_bar(val, global_max)
        spark = draw_sparkline(hist)
        anomaly_tag = detect_anomaly(val, ema)

        print(f"{short_name[:45]:<45} | \033[36m{spark}\033[0m | {color}{val:>12.3f}{reset} | {color}{bar}{reset} {anomaly_tag}")

    print("\n\033[90m(Press Ctrl+C to exit)\033[0m")

def main():
    parser = argparse.ArgumentParser(description="HPX Advanced Dashboard CLI")
    parser.add_argument('--filter', type=str, default=".*", help="Regex to filter counter names (e.g. 'idle-rate|utilization')")
    parser.add_argument('--replay', type=str, help="Path to an offline .csv HPX performance log to replay")
    parser.add_argument('--speed', type=float, default=0.2, help="Seconds to sleep between sequence updates in replay mode")
    args = parser.parse_args()

    filter_regex = re.compile(args.filter)
    history_dict = collections.defaultdict(lambda: collections.deque(maxlen=10))
    ema_dict = {}
    current_sequence = -1

    ALPHA = 0.4 # Smoothing factor for EMA

    stream = open(args.replay, 'r') if args.replay else sys.stdin

    print("====== HPX ADVANCED DASHBOARD ======")
    if args.replay:
        print(f"Replaying offline log: {args.replay} at {args.speed}s per sequence update...")
    else:
        print("Waiting for live HPX counter output on stdin...\n")

    time.sleep(1)

    try:
        for line in stream:
            line = line.strip()
            if not line or not line.startswith('/'):
                continue

            parts = line.split(',')
            if len(parts) >= 4:
                counter_name = parts[0]
                if not filter_regex.search(counter_name):
                    continue

                try:
                    seq_num = int(parts[1])
                    value = float(parts[3])

                    if seq_num > current_sequence and current_sequence != -1:
                        render_dashboard(history_dict, ema_dict, current_sequence)
                        if args.replay:
                            time.sleep(args.speed)

                    current_sequence = seq_num
                    history_dict[counter_name].append(value)

                    # Update the Exponential Moving Average (EMA)
                    if counter_name not in ema_dict:
                        ema_dict[counter_name] = value
                    else:
                        ema_dict[counter_name] = (value * ALPHA) + (ema_dict[counter_name] * (1 - ALPHA))

                except ValueError:
                    continue

        # Final render when stream closes
        if current_sequence != -1:
            render_dashboard(history_dict, ema_dict, current_sequence)
            print("\n\033[92m[DONE] Replay or Live Stream completed.\033[0m")

    except KeyboardInterrupt:
        print("\n\033[93m[EXIT] Closed by user.\033[0m")
    finally:
        if args.replay:
            stream.close()

if __name__ == '__main__':
    main()
