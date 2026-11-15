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
# Right of each graph, a panel shows where the change of each measurement
# since its first runs came from: its lasting steps, changes of the median of
# --runs runs (default 5) against that of the --runs runs before by more than
# --threshold percent, each labeled with its pull request, and the net change.
# Measurements whose level goes up and down are marked unstable, their steps
# aren't attributed. Above the graphs, the overview shows the net changes of
# all measurements since their first runs (overview-start.svg) and since the
# last release (overview-<release>.svg), and the pull requests that changed a
# measurement for good, since the last release and at least the last 20
# (pull-requests.svg, with a table of the changes).
#
# Below each graph, a table lists the commits after which a median moved by
# more than --threshold percent (default 5) against the median of the runs of
# the --window commits before it (default 3), so that a single noisy run
# doesn't count as a change. The window doesn't reach back before the last
# change of the same measurement, so that a step is listed once. On the graph,
# the points that moved are ringed (red slower, green faster), and the
# --labels commits with the largest changes (default 8) are marked with a
# dashed line and the number of their rows in the table. A solid line marks
# each release (see releases below) before the first commit that has it.
#
# The graphs are SVG, so that the repository, which gets new versions of all of
# them with every run, stores only their differences. They are reproducible:
# the same data gives the same files.
#
# Usage: perftests_timeline.py [--threshold <percent>] [--window <runs>]
#            [--runs <runs>] [--labels <commits>] <timeline.csv>
#            <output directory>

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

# The releases marked on the graphs: the name, and the first parent count of
# the master commit that brought the release into master (the commit of the
# tag itself is on the release branch)
releases = (
    ("v2.0.0", 14211),  # 7dbfb3a506, #7560 (release-2.0.X)
)

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


def lasting_steps(medians, threshold, runs):
    # The indices where the level of a series changed for good: the median of
    # the runs points from there on against the median of the runs points
    # before, by more than threshold, and the largest such change within runs
    # points on either side (a step is found once, not at its neighbors too)
    count = len(medians)
    score = [0.0] * count
    for i in range(runs, count - runs + 1):
        before = statistics.median(medians[i - runs:i])
        after = statistics.median(medians[i:i + runs])
        score[i] = math.log(after / before) if before > 0 and after > 0 else 0
    found = [i for i in range(runs, count - runs + 1)
        if abs(score[i]) > math.log1p(threshold) and abs(score[i]) ==
            max(abs(s) for s in score[max(runs, i - runs):i + runs])]
    # A median of runs points still has the new level up to half of them
    # before the step: the step is where the points around it are split best
    # into two levels
    return sorted({split(medians, max(0, i - runs), min(count, i + runs))
        for i in found})


def split(medians, start, end):
    # The index between start and end that splits the points best into two
    # levels: the smallest sum of the distances of the points (on a log scale)
    # from the median of their side
    values = [math.log(m) if m > 0 else 0 for m in medians[start:end]]

    def cost(i):
        return sum(sum(abs(v - statistics.median(side)) for v in side)
            for side in (values[:i], values[i:]))
    return start + min(range(1, len(values)), key=cost)


def analyze(series, position, threshold, runs):
    # The lasting steps of each series of a benchmark: key -> (points ordered
    # by position, step indices, unstable). A series is unstable if its level
    # goes up and down repeatedly, or its steps mostly cancel each other: its
    # steps are then not attributed to pull requests.
    result = {}
    for key, points in series.items():
        points = sorted(points, key=lambda point: position[point[0]])
        steps = lasting_steps([median for _, median, _ in points], threshold,
            runs)
        # measured between the levels of the segments, a step may be smaller
        steps = [i for i in steps if abs(math.log(step_ratio(points, i, runs,
            steps))) > math.log1p(threshold)]
        ratios = [step_ratio(points, i, runs, steps) for i in steps]
        up = [ratio > 1 for ratio in ratios]
        reversals = sum(1 for a, b in zip(up, up[1:]) if a != b)
        gross = sum(abs(math.log(ratio)) for ratio in ratios)
        net = abs(math.log(level(points, len(points) - 1, runs, steps) /
            level(points, 0, runs, steps)))
        unstable = reversals >= 2 or gross > 2 * net + 0.1
        result[key] = (points, [] if unstable else steps, unstable)
    return result


def level(points, index, runs, steps):
    # The level of a series at a point: the median of up to runs points of its
    # segment (between two steps), up to the point, or from the start of the
    # segment if fewer of them come before the point
    start = max([0] + [i for i in steps if i <= index])
    end = min([len(points)] + [i for i in steps if i > index])
    if index + 1 - start >= min(runs, end - start):
        chosen = points[max(start, index + 1 - runs):index + 1]
    else:
        chosen = points[start:min(end, start + runs)]
    return statistics.median(median for _, median, _ in chosen)


def step_ratio(points, index, runs, steps):
    # The change of the level at a step
    return (level(points, index, runs, steps) /
        level(points, index - 1, runs, steps))


def attribution(analyzed, position, runs, since=None):
    # For each series: its steps after the position since (or all of them),
    # as (index, ratio, commit, directory of the run); its net change over
    # that time, the level at the last point against the level at the last
    # point up to since (or at the first point); and whether it is unstable
    # (no steps then, and the net change compares the last runs points with
    # those up to since)
    result = {}
    for key, (points, steps, unstable) in analyzed.items():
        first = 0
        if since is not None:
            before = [i for i, point in enumerate(points)
                if position[point[0]] <= since]
            if not before:
                continue
            first = before[-1]
        net = (level(points, len(points) - 1, runs, steps) /
            level(points, first, runs, steps))
        found = [(i, step_ratio(points, i, runs, steps), points[i][0],
            points[i][2]) for i in steps if i > first]
        result[key] = (found, net, unstable)
    return result


def shared_prefix(names):
    # The length of the part up to the last ' - ' that all names start with
    # (ignoring case, e.g. 'Stream benchmark - '), 0 for a single name. The
    # characters are compared one by one, since lower() may change the length
    if len(set(names)) < 2:
        return 0
    length = 0
    for chars in zip(*names):
        if len({char.lower() for char in chars}) > 1:
            break
        length += 1
    prefix = names[0][:length]
    return prefix.rfind(' - ') + 3 if ' - ' in prefix else 0


def plot(benchmark, series, analyzed, commits, position, args, path):
    nodes = {node for _, _, node in series}
    medians = [median for points in series.values() for _, median, _ in points]
    unit, factor = time_unit(min(medians))

    # A prefix all lines share is said once, as the title of the legend
    names = sorted(name for name, _, _ in series)
    cut = shared_prefix(names)
    title = names[0][:cut - 3] if cut else None

    # The graph, and to its right where the change since the first runs came
    # from
    fig, (ax, steps_ax) = plt.subplots(1, 2, figsize=(graph_width, 4),
        gridspec_kw={'width_ratios': (4.3, 1), 'wspace': 0.08})
    lines = []
    colors = {}
    # ignoring case, the names don't all start alike (e.g. 'stream benchmark')
    for (name, executor, node), points in sorted(series.items(),
            key=lambda item: (item[0][0].lower(), item[0][1:])):
        points = sorted(points, key=lambda point: position[point[0]])
        label = f"{name[cut:]} ({executor})"
        if len(nodes) > 1:
            label += f", {node}"
        lines += ax.plot([position[commit] for commit, _, _ in points],
            [median * factor for _, median, _ in points],
            marker='o', markersize=3, linewidth=1, label=label)
        colors[(name, executor, node)] = lines[-1].get_color()
    plot_steps(steps_ax, attribution(analyzed, position, args.runs),
        colors, commits, args)

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

    # The releases, between the last commit without and the first with them
    ordered = sorted(commits, key=lambda commit: position[commit])
    for name, count in releases:
        first = next((i for i, commit in enumerate(ordered)
            if commits[commit][0] >= count), None)
        if not first:
            continue
        ax.axvline(first - 0.5, color='tab:purple', linewidth=1, zorder=1)
        ax.text(first - 0.5, 0.97, name, transform=top, rotation=90,
            ha='center', va='top', fontsize=7, color='tab:purple', zorder=4,
            bbox={'boxstyle': 'square,pad=0.15', 'fc': 'white', 'ec': 'none'})

    # Measurements of very different sizes share the graph on a log scale
    if max(medians) > 10 * min(medians):
        ax.set_yscale('log')
    # The commits on the x axis, the dates of some of them above the graph
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
    # The legend is centered below the image (the graph and the labels on its
    # left). Two columns for many lines, if they fit into its width: a wider
    # legend would widen the image, and GitHub would scale it down.
    renderer = fig.canvas.get_renderer()
    graph = ax.get_window_extent(renderer)
    left = ax.get_tightbbox(renderer).x0
    center = ((left + graph.x1) / 2 - graph.x0) / graph.width
    for ncol in (1,) if len(series) < 6 else (2, 1):
        # the lines are passed, since a label that starts with '_' (after the
        # prefix is cut) would otherwise be left out of the legend
        legend = ax.legend(lines, [line.get_label() for line in lines],
            fontsize=7, loc='upper center', bbox_to_anchor=(center, -0.28),
            ncol=ncol, frameon=False, title=title, title_fontsize=7)
        if legend.get_window_extent(renderer).width <= graph.x1 - left:
            break
    fig.savefig(path, format='svg', bbox_inches='tight',
        metadata={'Date': None, 'Creator': None})
    plt.close(fig)
    return moved, numbers, len(nodes) > 1


def ratio_axis(ax, ratios, minimum=0.6, vertical=False):
    # A symmetric axis of ratios on a log2 scale (1x in the middle), the x
    # axis, or the y axis if vertical
    limit = max([minimum] + [abs(math.log2(r)) + 0.25 for r in ratios])
    ticks = [t for t in (-2, -1, -0.5, 0, 0.5, 1, 2) if abs(t) < limit]
    labels = [f"{2 ** t:.2g}x" for t in ticks]
    if vertical:
        ax.set_ylim(-limit, limit)
        ax.set_yticks(ticks)
        ax.set_yticklabels(labels, fontsize=label_size)
        ax.axhline(0, color='0.2', linewidth=0.8, zorder=1)
    else:
        ax.set_xlim(-limit, limit)
        ax.set_xticks(ticks)
        ax.set_xticklabels(labels, fontsize=label_size)
        ax.axvline(0, color='0.2', linewidth=0.8, zorder=1)


def plot_steps(ax, attributed, colors, commits, args):
    # Where the change of each series since its first runs came from, as a
    # waterfall: per series (in the color of its line) a column for each
    # lasting step, from the level before it to the level after it, labeled
    # with its pull request, and a framed one for the net change; 'other' is
    # the part of the net change no step explains. The steps of unstable
    # series are not shown, only a cross.
    columns = []    # (label, start, end, color, net), None between series
    for key in sorted(attributed, key=lambda k: (k[0].lower(), k[1:])):
        found, net, unstable = attributed[key]
        if unstable:
            columns.append(("unstable", 1.0, 1.0, colors[key], False))
        elif found:
            at = 1.0
            for _, ratio, commit, _ in found:
                columns.append((commit_label(commit, commits), at, at * ratio,
                    colors[key], False))
                at *= ratio
            if abs(math.log(net / at)) > math.log(1.02):
                columns.append(("other", at, net, colors[key], False))
            columns.append(("net", 1.0, net, colors[key], True))
        else:
            continue
        columns.append(None)
    columns = columns[:-1]
    ax.set_title(f"lasting changes since\nthe first runs (+{args.threshold:g}%"
        f", {args.runs} runs)", fontsize=7)
    ax.yaxis.tick_right()
    if not columns:
        ax.set_yticks([])
        ax.set_xticks([])
        ax.text(0.5, 0.5, "none", transform=ax.transAxes, ha='center',
            va='center', fontsize=label_size, color='0.4')
        return
    # too many columns for their labels: the net changes only
    if len(columns) > 16:
        columns = [column for column in columns if column is None or
            column[4] or column[0] == "unstable"]
        columns = [column for i, column in enumerate(columns)
            if column is not None or (i and columns[i - 1] is not None)]
    labels = []
    for x, column in enumerate(columns):
        if column is None:
            # a line between the columns of two series
            labels.append("")
            ax.axvline(x, color='0.5', linewidth=0.8, zorder=1)
            continue
        label, start, end, color, net = column
        if label == "unstable":
            labels.append(label)
            ax.scatter([x], [0], marker='x', s=14, color=color, zorder=3)
            continue
        labels.append(f"{label} {end / start - 1:+.0%}")
        a, b = math.log2(start), math.log2(end)
        ax.bar(x, b - a, bottom=a, width=0.75, color=color,
            alpha=0.55 if label == "other" else 1,
            edgecolor='0.1' if net else 'none', linewidth=0.8, zorder=2)
    ax.set_xticks(range(len(columns)))
    ax.set_xticklabels(labels, rotation=90, fontsize=6.5)
    for tick, column in zip(ax.get_xticklabels(), columns):
        if column and column[0] != "unstable":
            tick.set_color('tab:red' if column[2] > column[1]
                else 'tab:green')
            if column[4]:
                tick.set_fontweight('bold')
    ax.tick_params(axis='x', length=0)
    ax.set_xlim(-0.6, len(columns) - 0.4)
    ratio_axis(ax, [c[2] / c[1] for c in columns if c] +
        [c[2] for c in columns if c and c[4]], vertical=True)
    ax.grid(axis='x', visible=False)


def series_labels(benchmarks):
    # The labels of all series: name (executor), with the node if there are
    # several, and the benchmark if another benchmark has the same label
    nodes = {key[2] for series in benchmarks.values() for key in series}
    labels = {}
    for benchmark, series in benchmarks.items():
        for name, executor, node in series:
            label = f"{name} ({executor})"
            if len(nodes) > 1:
                label += f", {node}"
            labels[(benchmark, (name, executor, node))] = label
    counts = {}
    for label in labels.values():
        counts[label] = counts.get(label, 0) + 1
    return {key: label if counts[label] == 1 else f"{key[0]}: {label}"
        for key, label in labels.items()}


def label_margin(fig, ax, renderer):
    # Room for the labels left of the graph, so that the image keeps its width
    fig.canvas.draw()
    width = max((label.get_window_extent(renderer).width
        for label in ax.get_yticklabels()), default=0)
    fig.subplots_adjust(left=min(0.7, (width + 12) / fig.bbox.width),
        right=0.9)


def plot_overview(attributed, labels, title, path, threshold):
    # The net change of every stable series that changed by more than the
    # threshold over a time, one dot per series (red slower, green faster),
    # from the largest slowdown to the largest speedup
    shown = sorted(((net, labels[key]) for key, (found, net, unstable)
        in attributed.items() if not unstable and
            abs(math.log(net)) > math.log1p(threshold)), reverse=True)
    stable = sum(1 for _, _, unstable in attributed.values() if not unstable)
    fig, ax = plt.subplots(figsize=(graph_width, 0.24 * len(shown) + 1.1))
    for y, (net, _) in enumerate(shown):
        color = 'tab:red' if net > 1 else 'tab:green'
        ax.plot([0, math.log2(net)], [y, y], color=color, linewidth=1.2)
        ax.scatter([math.log2(net)], [y], color=color, s=18, zorder=3)
        ax.text(math.log2(net) + (0.04 if net > 1 else -0.04), y,
            f"{net - 1:+.0%}", va='center', ha='left' if net > 1 else 'right',
            fontsize=6.5)
    ax.set_yticks(range(len(shown)))
    ax.set_yticklabels([label for _, label in shown], fontsize=6.5)
    ax.set_ylim(len(shown) - 0.5, -0.5)
    ratio_axis(ax, [net for net, _ in shown])
    ax.grid(axis='y', visible=False)
    ax.set_title(f"{title}: {len(shown)} of {stable} measurements changed by "
        f"more than {threshold:.0%} (red slower, green faster)", fontsize=8,
        loc='left')
    if not shown:
        ax.set_yticks([])
        ax.text(0.5, 0.5, "none", transform=ax.transAxes, ha='center',
            va='center', fontsize=label_size, color='0.4')
    label_margin(fig, ax, fig.canvas.get_renderer())
    fig.savefig(path, format='svg', metadata={'Date': None, 'Creator': None})
    plt.close(fig)


def pull_request_steps(attributed, labels, position):
    # The lasting steps of all stable series by commit: position -> (commit,
    # directory, [(label, ratio)])
    by_commit = {}
    for key, (found, _, unstable) in attributed.items():
        for _, ratio, commit, directory in found:
            entry = by_commit.setdefault(position[commit],
                (commit, directory, []))
            entry[2].append((labels[key], ratio))
    return by_commit


def plot_pull_requests(by_commit, commits, path):
    # One row per commit (newest first), a dot for each measurement it changed
    rows = sorted(by_commit, reverse=True)
    fig, ax = plt.subplots(figsize=(graph_width, 0.26 * len(rows) + 1.0))
    ratios = []
    for y, x in enumerate(rows):
        _, _, changed = by_commit[x]
        for _, ratio in changed:
            ratios.append(ratio)
            ax.scatter([math.log2(ratio)], [y], s=20, alpha=0.8, zorder=3,
                color='tab:red' if ratio > 1 else 'tab:green')
        largest = max(changed, key=lambda change: abs(math.log(change[1])))
        note = f"{len(changed)}: {largest[1] - 1:+.0%} {largest[0]}"
        if len(changed) > 1:
            note = f"{len(changed)}, largest {largest[1] - 1:+.0%} {largest[0]}"
        ax.text(1.02, y, note if len(note) <= 72 else note[:71] + "…",
            transform=ax.get_yaxis_transform(), va='center', fontsize=6.5)
    ax.set_yticks(range(len(rows)))
    ax.set_yticklabels([f"{commit_label(by_commit[x][0], commits)}  "
        f"{commits[by_commit[x][0]][1]:%Y-%m-%d}" for x in rows], fontsize=6.5)
    ax.set_ylim(len(rows) - 0.5, -0.5)
    ratio_axis(ax, ratios)
    ax.grid(axis='y', visible=False)
    ax.set_title("lasting changes of the measurements (red slower, green "
        "faster); right: how many, and the largest", fontsize=8, loc='left')
    fig.subplots_adjust(left=0.13, right=0.5)
    fig.savefig(path, format='svg', metadata={'Date': None, 'Creator': None})
    plt.close(fig)


def pull_request_table(by_commit, commits):
    lines = ["| Pull request | Merged | Measurement | Change |",
        "| --- | --- | --- | --- |"]
    for x in sorted(by_commit, reverse=True):
        commit, directory, changed = by_commit[x]
        _, when, pull_request = commits[commit]
        first = (f"[#{pull_request}]({pull_request_url.format(pull_request)})"
            if pull_request else commit[:12])
        run = (f"[{when:%Y-%m-%d}](../{directory}/README.md)" if directory
            else f"{when:%Y-%m-%d}")
        for i, (label, ratio) in enumerate(sorted(changed,
                key=lambda change: -abs(math.log(change[1])))):
            cells = [first, run] if i == 0 else ["", ""]
            lines.append(f"| {' | '.join(cells)} | {label} | "
                f"{ratio - 1:+.0%} |")
    return '\n'.join(lines) + '\n'


def overview(benchmarks, analyzed, commits, ordered, position, args):
    # The part of the README above the graphs: the net changes since the first
    # runs and since the last release, the unstable measurements, and the
    # pull requests that changed a measurement for good
    labels = series_labels(benchmarks)

    def merged(since=None):
        return {(benchmark, key): value for benchmark in analyzed
            for key, value in attribution(analyzed[benchmark], position,
                args.runs, since).items()}

    threshold = args.threshold / 100
    sections = ["### Overview", "",
        f"The lasting changes: a step of more than {args.threshold:g}% between "
        f"the median of {args.runs} runs and that of the {args.runs} runs "
        "before it, for the measurements whose level doesn't go up and down. "
        "The net change compares the median of the last runs of a "
        "measurement with that of its first runs (the graph on the right of "
        "each benchmark shows where it came from).", ""]
    start = merged()
    plot_overview(start, labels, "since the first runs",
        os.path.join(args.output_dir, "overview-start.svg"), threshold)
    sections += ["![since the first runs](overview-start.svg)", ""]
    release = None
    for name, count in releases:
        first = next((i for i, commit in enumerate(ordered)
            if commits[commit][0] >= count), None)
        if first:
            release = (name, first)
    if release:
        name, first = release
        since = merged(first)
        plot_overview(since, labels, f"since {name}",
            os.path.join(args.output_dir, f"overview-{name}.svg"), threshold)
        sections += [f"![since {name}](overview-{name}.svg)", ""]
    unstable = sorted(labels[key] for key, (_, _, flag) in start.items()
        if flag)
    if unstable:
        sections += ["Measurements whose level goes up and down, so that "
            "their changes are not attributed to pull requests: " +
            ", ".join(unstable) + ".", ""]

    # the pull requests since the last release, at least the last 20 of them
    by_commit = pull_request_steps(start, labels, position)
    recent = sorted(by_commit, reverse=True)
    keep = set(recent[:20]) | {x for x in recent
        if release and x >= release[1]}
    by_commit = {x: by_commit[x] for x in keep}
    sections += ["### Pull requests that changed performance", ""]
    if by_commit:
        plot_pull_requests(by_commit, commits,
            os.path.join(args.output_dir, "pull-requests.svg"))
        sections += ["The commits after which a measurement changed for good"
            + (f" since {release[0]}, and at least the last 20" if release
                else ", the last 20") + ".", "",
            "![pull requests](pull-requests.svg)", "",
            pull_request_table(by_commit, commits)]
    else:
        sections += ["None.", ""]
    return '\n'.join(sections)


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
    parser.add_argument('--runs', type=int, default=5,
        help="runs on each side of a lasting change (default 5)")
    parser.add_argument('timeline')
    parser.add_argument('output_dir')
    args = parser.parse_args()
    if args.window < 1:
        parser.error('--window must be at least 1')
    if args.runs < 1:
        parser.error('--runs must be at least 1')
    if args.labels < 0:
        parser.error('--labels must not be negative')
    if not math.isfinite(args.threshold) or args.threshold < 0:
        parser.error('--threshold must be a finite number of at least 0')

    benchmarks, commits = read_timeline(args.timeline)
    os.makedirs(args.output_dir, exist_ok=True)
    # The commits in the order of the master branch, evenly spaced
    ordered = sorted(commits, key=lambda commit: commits[commit][:2])
    position = {commit: i for i, commit in enumerate(ordered)}

    analyzed = {benchmark: analyze(series, position, args.threshold / 100,
        args.runs) for benchmark, series in benchmarks.items()}
    sections = [overview(benchmarks, analyzed, commits, ordered, position,
        args)]
    for benchmark in sorted(benchmarks):
        moved, numbers, several_nodes = plot(benchmark,
            benchmarks[benchmark], analyzed[benchmark], commits, position, args,
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
