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
# executor) for every run, over the date of the commit. The graphs are written
# as <output directory>/<benchmark>.svg, and the part of the README.md of the
# repository that shows them to <output directory>/README-timeline.md. Its
# links are relative to the output directory, so that it shows the graphs on
# its own as well; upload_history.sh adjusts them for the README.md.
#
# The graphs are SVG, so that the repository, which gets new versions of all of
# them with every run, stores only their differences. They are reproducible:
# the same data gives the same files.
#
# Usage: perftests_timeline.py <timeline.csv> <output directory>

import csv
import datetime
import os
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import seaborn as sns

sns.set_style("ticks", {'axes.grid': True})
# No random ids and no fonts embedded as paths, see the comment above
plt.rcParams['svg.hashsalt'] = 'hpx-performance'
plt.rcParams['svg.fonttype'] = 'none'


def time_unit(seconds):
    # the unit that shows the given time with one to three integer digits, as
    # in perftests_plot.py
    for unit, factor in (("s", 1), ("ms", 1e3), ("µs", 1e6)):
        if seconds * factor >= 1:
            return unit, factor
    return "ns", 1e9


def read_timeline(path):
    benchmarks = {}
    with open(path, newline='') as f:
        for row in csv.DictReader(f):
            try:
                median = float(row['median'])
                when = datetime.datetime.fromisoformat(row['commit_date'])
            except (KeyError, ValueError):
                continue
            series = benchmarks.setdefault(row['benchmark'], {})
            # Several nodes only get a line of their own if there are several
            key = (row['name'], row['executor'], row['node'])
            series.setdefault(key, []).append((when, median))
    return benchmarks


def plot(benchmark, series, path):
    nodes = {node for _, _, node in series}
    medians = [median for points in series.values() for _, median in points]
    unit, factor = time_unit(min(medians))

    fig, ax = plt.subplots(figsize=(8, 4))
    for (name, executor, node), points in sorted(series.items()):
        points.sort()
        label = f"{name} ({executor})"
        if len(nodes) > 1:
            label += f", {node}"
        ax.plot([when for when, _ in points],
            [median * factor for _, median in points],
            marker='o', markersize=3, linewidth=1, label=label)

    # Measurements of very different sizes share the graph on a log scale
    if max(medians) > 10 * min(medians):
        ax.set_yscale('log')
    locator = mdates.AutoDateLocator()
    ax.xaxis.set_major_locator(locator)
    ax.xaxis.set_major_formatter(mdates.ConciseDateFormatter(locator))
    ax.set_xlabel("commit date")
    ax.set_ylabel(f"median time [{unit}]")
    ax.set_title(benchmark, fontsize=10)
    ax.legend(fontsize=7, loc='upper center', bbox_to_anchor=(0.5, -0.18),
        ncol=1 if len(series) < 6 else 2, frameon=False)
    fig.savefig(path, format='svg', bbox_inches='tight',
        metadata={'Date': None, 'Creator': None})
    plt.close(fig)


def main():
    if len(sys.argv) != 3:
        print("Usage: perftests_timeline.py <timeline.csv> <output directory>")
        return 1
    timeline, output_dir = sys.argv[1:]
    benchmarks = read_timeline(timeline)
    os.makedirs(output_dir, exist_ok=True)

    sections = []
    for benchmark in sorted(benchmarks):
        plot(benchmark, benchmarks[benchmark],
            os.path.join(output_dir, f"{benchmark}.svg"))
        sections.append(f"### {benchmark}\n\n"
            f"![{benchmark}]({benchmark}.svg)\n")
    with open(os.path.join(output_dir, 'README-timeline.md'), 'w',
            newline='') as f:
        f.write('\n'.join(sections))
    return 0


if __name__ == '__main__':
    sys.exit(main())
