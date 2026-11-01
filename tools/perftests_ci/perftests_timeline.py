#!/usr/bin/env python3
#
# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Plots the performance of every benchmark over time from timeline.csv of the
# performance data repository (see perftests_history.py and
# .jenkins/lsu-perftests/upload_history.sh): one graph per benchmark, with
# the median time of each of its measurements (one line per measurement and
# executor) for every run, over the master commits in their order (evenly
# spaced, labeled with their pull request or commit). The graphs are written
# as <output directory>/<benchmark>.svg, and the part of the README.md of the
# repository that shows them to <output directory>/README-timeline.md. Its
# links are relative to the output directory, so that it shows the graphs on
# its own as well; upload_history.sh adjusts them for the README.md.
#
# Below each graph, a table lists the commits after which a median moved by
# more than --threshold percent (default 5) against the median of the runs of
# the --window commits before it (default 3), so that a single noisy run
# doesn't count as a change. The window doesn't reach back before the last
# change of the same measurement, so that a step is listed once. On the graph,
# the points that moved are ringed (red slower, green faster), and the
# --labels commits with the largest changes (default 8) are marked with a
# dashed line and the number of their rows in the table.
#
# The graphs are SVG, so that the repository, which gets new versions of all of
# them with every run, stores only their differences. They are reproducible:
# the same data gives the same files.
#
# Usage: perftests_timeline.py [--threshold <percent>] [--window <runs>]
#            [--labels <commits>] <timeline.csv> <output directory>

import argparse
import csv
import datetime
import math
import os
import statistics
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.transforms as transforms
import seaborn as sns

sns.set_style("ticks", {'axes.grid': True})
# No random ids and no fonts embedded as paths, see the comment above
plt.rcParams['svg.hashsalt'] = 'hpx-performance'
plt.rcParams['svg.fonttype'] = 'none'

pull_request_url = "https://github.com/TheHPXProject/hpx/pull/{}"

# At most this many labels on the x axis, and dates on the axis above it
max_commit_labels = 40
max_date_labels = 8
# The size of their text
label_size = 7.5

# GitHub shows a README at most about 830 pixels wide (on the start page of the
# repository, next to its sidebar); a graph of this width (in inches) fills it,
# together with the labels around it. A wider one would be scaled down, and its
# text with it.
graph_width = 10.2


def time_unit(seconds):
    # the unit that shows the given time with one to three integer digits, as
    # in perftests_plot.py
    for unit, factor in (("s", 1), ("ms", 1e3), ("µs", 1e6)):
        if seconds * factor >= 1:
            return unit, factor
    return "ns", 1e9


def read_timeline(path):
    # Returns the benchmarks, each with its series of points (commit, median,
    # directory of the run), and the commits of all runs: commit -> (first
    # parent count, date, pull request)
    benchmarks = {}
    commits = {}
    with open(path, newline='') as f:
        for row in csv.DictReader(f):
            try:
                median = float(row['median'])
                when = datetime.datetime.fromisoformat(row['commit_date'])
                count = int(row['first_parent_count'])
            except (KeyError, ValueError):
                continue
            commit = row['commit']
            commits.setdefault(commit, (count, when,
                row.get('pull_request', '')))
            series = benchmarks.setdefault(row['benchmark'], {})
            # Several nodes only get a line of their own if there are several
            key = (row['name'], row['executor'], row['node'])
            series.setdefault(key, []).append(
                (commit, median, row.get('directory', '')))
    return benchmarks, commits


def commit_label(commit, commits):
    pull_request = commits[commit][2]
    return f"#{pull_request}" if pull_request else commit[:7]


def changes(series, position, threshold, window):
    # The points whose median moved by more than threshold against the median
    # of the window points before it: (position, key, median, change,
    # directory of the run). After a change, the points are compared only
    # with those from the change on, so that a step is listed once, and not
    # again for the points after it whose window still reaches back before
    # it.
    found = []
    for key, points in series.items():
        points = sorted(points, key=lambda point: position[point[0]])
        start = 0
        for i in range(1, len(points)):
            before = statistics.median(
                median for _, median, _ in points[max(start, i - window):i])
            commit, median, directory = points[i]
            change = median / before - 1 if before > 0 else 0
            if abs(change) > threshold:
                found.append((position[commit], key, median, change, directory))
                start = i
    return found


def plot(benchmark, series, commits, position, args, path):
    nodes = {node for _, _, node in series}
    medians = [median for points in series.values() for _, median, _ in points]
    unit, factor = time_unit(min(medians))

    fig, ax = plt.subplots(figsize=(graph_width, 4))
    for (name, executor, node), points in sorted(series.items()):
        points = sorted(points, key=lambda point: position[point[0]])
        label = f"{name} ({executor})"
        if len(nodes) > 1:
            label += f", {node}"
        ax.plot([position[commit] for commit, _, _ in points],
            [median * factor for _, median, _ in points],
            marker='o', markersize=3, linewidth=1, label=label)

    # The points that moved, and the commits with the largest changes, numbered
    # from left to right
    moved = changes(series, position, args.threshold / 100, args.window)
    for x, _, median, change, _ in moved:
        ax.scatter([x], [median * factor], s=60, facecolors='none',
            edgecolors='tab:red' if change > 0 else 'tab:green',
            linewidths=1.2, zorder=3)
    largest = {}
    for x, _, _, change, _ in moved:
        largest[x] = max(largest.get(x, 0), abs(change))
    labeled = sorted(sorted(largest, key=lambda x: (-largest[x], x))
        [:args.labels])
    numbers = {x: i + 1 for i, x in enumerate(labeled)}
    top = transforms.blended_transform_factory(ax.transData, ax.transAxes)
    for x, number in numbers.items():
        ax.axvline(x, color='0.4', linestyle='--', linewidth=0.8, zorder=1)
        ax.text(x, 1.0, str(number), transform=top, ha='center', va='center',
            fontsize=7, bbox={'boxstyle': 'circle,pad=0.2', 'fc': 'white',
                'ec': '0.4', 'lw': 0.8})

    # Measurements of very different sizes share the graph on a log scale
    if max(medians) > 10 * min(medians):
        ax.set_yscale('log')
    # The commits on the x axis, the dates of some of them above the graph
    ordered = sorted(commits, key=lambda commit: position[commit])
    step = math.ceil(len(ordered) / max_commit_labels)
    shown = ordered[::step]
    ax.set_xticks([position[commit] for commit in shown])
    ax.set_xticklabels([commit_label(commit, commits) for commit in shown],
        rotation=90, fontsize=label_size)
    ax.set_xlim(-0.5, len(ordered) - 0.5)
    dates = ax.secondary_xaxis('top')
    # the first commit of each day
    dated = [commit for i, commit in enumerate(ordered) if i == 0 or
        commits[commit][1].date() != commits[ordered[i - 1]][1].date()]
    dated = dated[::math.ceil(len(dated) / max_date_labels)]
    dates.set_xticks([position[commit] for commit in dated])
    dates.set_xticklabels(
        [f"{commits[commit][1]:%Y-%m-%d}" for commit in dated],
        fontsize=label_size)
    dates.tick_params(pad=8)
    ax.set_xlabel("master commit (pull request, or the commit without one)")
    ax.set_ylabel(f"median time [{unit}]")
    ax.set_title(benchmark, fontsize=10, pad=18)
    ax.legend(fontsize=7, loc='upper center', bbox_to_anchor=(0.5, -0.28),
        ncol=1 if len(series) < 6 else 2, frameon=False)
    fig.savefig(path, format='svg', bbox_inches='tight',
        metadata={'Date': None, 'Creator': None})
    plt.close(fig)
    return moved, numbers, len(nodes) > 1


def changes_table(moved, numbers, commits, by_position, several_nodes, args):
    # The table of the changes below a graph, one row per measurement that
    # moved, grouped by commit from the newest to the oldest, and by run for
    # several runs of a commit (several nodes). The links to the runs are
    # relative to the output directory (timeline/).
    window = (f"the median of up to {args.window} runs before it, not "
        "reaching back past the last change")
    if not moved:
        return (f"No median moved by more than {args.threshold:g}% against "
            f"{window}.\n")
    lines = [f"Medians that moved by more than {args.threshold:g}% against "
        f"{window} (+ is slower); the numbers are those on the graph.", "",
        "| # | Commit | Pull request | Measurement | Change |",
        "| --- | --- | --- | --- | --- |"]
    last = (None, None)
    for x, (name, executor, node), _, change, directory in sorted(moved,
            key=lambda m: (-m[0], m[4], m[1])):
        commit = by_position[x]
        case = f"{name} ({executor})"
        if several_nodes:
            case += f", {node}"
        cells = ["", "", ""]
        if (x, directory) != last:
            _, _, pull_request = commits[commit]
            # the number once per commit, the commit once per run of it
            cells = [str(numbers.get(x, "")) if x != last[0] else "",
                f"[{commit[:12]}](../{directory}/README.md)" if directory
                    else commit[:12],
                f"[#{pull_request}]({pull_request_url.format(pull_request)})"
                    if pull_request else ""]
            last = (x, directory)
        lines.append(f"| {' | '.join(cells)} | {case} | {change:+.0%} |")
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(
        description="Plots the performance of every benchmark over time")
    parser.add_argument('--threshold', type=float, default=5,
        help="change of a median that is listed, in percent (default 5)")
    parser.add_argument('--window', type=int, default=3,
        help="runs before a commit it is compared with (default 3)")
    parser.add_argument('--labels', type=int, default=8,
        help="commits numbered on a graph (default 8)")
    parser.add_argument('timeline')
    parser.add_argument('output_dir')
    args = parser.parse_args()
    if args.window < 1:
        parser.error('--window must be at least 1')
    if args.labels < 0:
        parser.error('--labels must not be negative')
    if not math.isfinite(args.threshold) or args.threshold < 0:
        parser.error('--threshold must be a finite number of at least 0')

    benchmarks, commits = read_timeline(args.timeline)
    os.makedirs(args.output_dir, exist_ok=True)
    # The commits in the order of the master branch, evenly spaced
    ordered = sorted(commits, key=lambda commit: commits[commit][:2])
    position = {commit: i for i, commit in enumerate(ordered)}

    sections = []
    for benchmark in sorted(benchmarks):
        moved, numbers, several_nodes = plot(benchmark,
            benchmarks[benchmark], commits, position, args,
            os.path.join(args.output_dir, f"{benchmark}.svg"))
        sections.append(f"### {benchmark}\n\n"
            f"![{benchmark}]({benchmark}.svg)\n\n" + changes_table(moved,
                numbers, commits, ordered, several_nodes, args))
    with open(os.path.join(args.output_dir, 'README-timeline.md'), 'w',
            newline='') as f:
        f.write('\n'.join(sections))
    return 0


if __name__ == '__main__':
    sys.exit(main())
