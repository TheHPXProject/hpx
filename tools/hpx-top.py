#!/usr/bin/env python3
#
# Copyright (c) 2026 Arpit Khandelwal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import sys
import subprocess  # nosec
import threading
import queue
import re
import time
import argparse
from datetime import datetime

try:
    from rich.console import Console
    from rich.live import Live
    from rich.table import Table
    from rich.panel import Panel
    from rich.layout import Layout
    from rich.text import Text
except ImportError:
    print("Error: 'rich' library is required for hpx-top.")
    print("Please install it using: pip install rich")
    sys.exit(1)

class HPXTop:
    def __init__(self, cmd_args):
        self.cmd_args = cmd_args
        self.console = Console()
        self.metrics = {
            "localities": {},
            "global": {
                "uptime": "00:00:00",
                "tasks_per_sec": 0,
                "parcels_sent": 0
            }
        }
        self.start_time = time.time()
        self.running = True
        self.show_help = False
        self.data_queue = queue.Queue()

    def parse_hpx_output(self, line):
        """
        Parses HPX counter output in csv-short format:
        locality#0/threads/count/instantaneous/all,count,1.000,123
        """
        # Example line: "locality#0/threads/count/instantaneous/all,1,1.001614[s],123"
        # or from csv-short: "locality#0/threads/count/instantaneous/all,count,1.001614,123"
        parts = line.strip().split(',')
        if len(parts) < 4:
            return

        counter_name = parts[0]
        # Skip header if any
        if "locality" not in counter_name:
            return

        try:
            value = float(parts[3])

            # Extract locality ID
            loc_match = re.search(r"locality#(\d+)", counter_name)
            if not loc_match:
                return
            loc_id = f"Locality #{loc_match.group(1)}"

            if loc_id not in self.metrics["localities"]:
                self.metrics["localities"][loc_id] = {
                    "threads": 0,
                    "threads_history": [0.0] * 50,
                    "tasks": 0,
                    "parcels_sent": 0,
                    "parcels_received": 0,
                    "parcels_history": [0.0] * 50,
                    "agas_hits": 0,
                    "agas_misses": 0,
                    "mem": 0
                }

            # Map common counters to metrics
            if "threads/count/instantaneous/all" in counter_name:
                self.metrics["localities"][loc_id]["threads"] = value
                self.metrics["localities"][loc_id]["threads_history"].append(value)
                self.metrics["localities"][loc_id]["threads_history"] = self.metrics["localities"][loc_id]["threads_history"][-50:]
            elif "threads/count/cumulative" in counter_name:
                self.metrics["localities"][loc_id]["tasks"] = value
            elif "parcels/count/sent" in counter_name:
                self.metrics["localities"][loc_id]["parcels_sent"] = value
                self.metrics["localities"][loc_id]["parcels_history"].append(value)
                self.metrics["localities"][loc_id]["parcels_history"] = self.metrics["localities"][loc_id]["parcels_history"][-50:]
            elif "parcels/count/received" in counter_name:
                self.metrics["localities"][loc_id]["parcels_received"] = value
            elif "agas/count/cache-hit" in counter_name:
                self.metrics["localities"][loc_id]["agas_hits"] = value
            elif "agas/count/cache-miss" in counter_name:
                self.metrics["localities"][loc_id]["agas_misses"] = value

        except (ValueError, IndexError):
            pass

    def run_application(self):
        # Inject required HPX flags for counter output
        # We use a 500ms interval for relatively smooth updates
        interval = "500"
        hpx_flags = [
            f"--hpx:print-counter-interval={interval}",
            "--hpx:print-counter-format=csv-short",
            "--hpx:print-counter=/threads/count/instantaneous/all",
            "--hpx:print-counter=/threads/count/cumulative",
            "--hpx:print-counter=/parcels/count/sent/total",
            "--hpx:print-counter=/parcels/count/received/total",
            "--hpx:print-counter=/agas/count/cache-hit",
            "--hpx:print-counter=/agas/count/cache-miss"
        ]

        full_cmd = self.cmd_args + hpx_flags

        try:
            process = subprocess.Popen(
                full_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
                shell=False  # nosec
            )

            for line in process.stdout:
                if not self.running:
                    process.terminate()
                    break
                self.parse_hpx_output(line)

            process.wait()
        except Exception as e:
            self.data_queue.put(f"Error running HPX app: {e}")
        finally:
            self.running = False

    def make_layout(self) -> Layout:
        layout = Layout()
        layout.split(
            Layout(name="header", size=3),
            Layout(name="main", ratio=1),
            Layout(name="footer", size=3)
        )
        layout["main"].split_row(
            Layout(name="left", ratio=2),
            Layout(name="right", ratio=1)
        )
        return layout

    def generate_sparkline(self, data_list, width=20) -> Text:
        """Generates a simple ASCII sparkline."""
        if not data_list:
            return Text(" " * width)

        # Sparkline characters:  ▂▃▄▅▆▇█
        chars = " ▂▃▄▅▆▇█"
        max_val = max(data_list) if max(data_list) > 0 else 1
        normalized = [int((v / max_val) * (len(chars) - 1)) for v in data_list[-width:]]

        line = "".join(chars[i] for i in normalized)
        # Pad with leading spaces if too short
        line = " " * (width - len(line)) + line
        return Text(line, style="bold cyan")

    def generate_table(self) -> Table:
        table = Table(title="Locality Performance", expand=True)
        table.add_column("Locality", style="cyan", no_wrap=True)
        table.add_column("Threads", style="magenta", justify="right")
        table.add_column("Tasks", style="green", justify="right")
        table.add_column("S-Parcels", style="yellow", justify="right")
        table.add_column("R-Parcels", style="yellow", justify="right")

        for loc, data in sorted(self.metrics["localities"].items()):
            table.add_row(
                loc,
                f"{int(data['threads'])}",
                f"{int(data['tasks'])}",
                f"{int(data['parcels_sent'])}",
                f"{int(data['parcels_received'])}"
            )
        return table

    def generate_stats_panel(self) -> Panel:
        if self.metrics["localities"]:
            # Aggregate stats
            total_threads = sum(l["threads"] for l in self.metrics["localities"].values())
            first_loc = list(self.metrics["localities"].values())[0]
            t_history = first_loc.get("threads_history", [0])
            p_history = first_loc.get("parcels_history", [0])

            t_spark = self.generate_sparkline(t_history)
            p_spark = self.generate_sparkline(p_history)

            hits = sum(l.get("agas_hits", 0) for l in self.metrics["localities"].values())
            miss = sum(l.get("agas_misses", 0) for l in self.metrics["localities"].values())
            total_agas = hits + miss
            hit_rate = (hits / total_agas * 100) if total_agas > 0 else 100.0

            stats = Text.assemble(
                ("\nThread History (Loc#0):\n", "bold"),
                t_spark,
                ("\n\nParcel Flow (Loc#0):\n", "bold"),
                p_spark,
                ("\n\nTotal Threads: ", "bold"), (f"{int(total_threads)}", "magenta"),
                ("\nAGAS Hit Rate: ", "bold"), (f"{hit_rate:.1f}%", "green" if hit_rate > 95 else "yellow"),
                ("\nLocalities: ", "bold"), (str(len(self.metrics["localities"])), "cyan")
            )
        else:
            stats = Text("Waiting for data...", justify="center")

        return Panel(stats, title="System Stats")

    def generate_header(self) -> Panel:
        uptime = str(datetime.now() - datetime.fromtimestamp(self.start_time)).split('.')[0]
        grid = Table.grid(expand=True)
        grid.add_column(justify="left", ratio=1)
        grid.add_column(justify="center", ratio=1)
        grid.add_column(justify="right", ratio=1)
        grid.add_row(
            Text(" HPX-TOP v0.1 ", style="bold white on blue"),
            Text("Distributed Runtime Monitor", style="bold"),
            Text(f"Uptime: {uptime} ", style="bold green")
        )
        return Panel(grid, style="white on black")

    def run_tui(self):
        layout = self.make_layout()

        with Live(layout, refresh_per_second=4, screen=True):
            while self.running:
                layout["header"].update(self.generate_header())
                layout["left"].update(Panel(self.generate_table(), title="Utilization"))
                layout["right"].update(self.generate_stats_panel())
                layout["footer"].update(Panel(
                    Text("Press Ctrl+C to stop monitoring and exit application", justify="center", style="dim")
                ))
                time.sleep(0.25)

    def run_mock_data(self):
        """Injects mock data into the parser for testing without an HPX app."""
        localities = 4
        while self.running:
            for i in range(localities):
                loc = f"locality#{i}"
                # Simulated counter output
                self.parse_hpx_output(f"{loc}/threads/count/instantaneous/all,count,0.5,{10 + (i*2)}")
                self.parse_hpx_output(f"{loc}/threads/count/cumulative,count,0.5,{1000 * (i+1) + time.time()%100}")
                self.parse_hpx_output(f"{loc}/parcels/count/sent/total,count,0.5,{500 * (i+1) + time.time()%50}")
                self.parse_hpx_output(f"{loc}/parcels/count/received/total,count,0.5,{400 * (i+1) + time.time()%40}")
                self.parse_hpx_output(f"{loc}/agas/count/cache-hit,count,0.5,{10000}")
                self.parse_hpx_output(f"{loc}/agas/count/cache-miss,count,0.5,{10 + i}")
            time.sleep(0.5)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="HPX-Top: Terminal UI for HPX Monitoring")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="The HPX application and its arguments")
    parser.add_argument("--mock", action="store_true", help="Run with mock data for TUI testing")
    args = parser.parse_args()

    if not args.command and not args.mock:
        print("Usage: hpx-top.py <hpx_application> [args...]")
        print("   or: hpx-top.py --mock")
        sys.exit(1)

    top = HPXTop(args.command)

    if args.mock:
        app_thread = threading.Thread(target=top.run_mock_data)
    else:
        # Start HPX app in background thread
        app_thread = threading.Thread(target=top.run_application)

    app_thread.daemon = True
    app_thread.start()

    try:
        top.run_tui()
    except KeyboardInterrupt:
        top.running = False
        print("\nStopping monitor...")
