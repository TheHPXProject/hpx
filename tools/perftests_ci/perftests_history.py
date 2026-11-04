#!/usr/bin/env python3
#
# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Turns the performance history in a clone of the performance data repository
# (see .jenkins/lsu-perftests/upload_history.sh) into a time line of every
# measurement: one CSV row per run and measurement, ordered by the date of the
# commit (and the Jenkins build, for several runs of one commit). The runs are
# read from their own directories (metadata.json and summary.json), not from
# the index, so that the time line can be rebuilt from any revision of the
# repository.
#
# The runs are in <year>/<node>/<commit date>-<commit>-b<build>. The CSV has
# the same columns as timeline.csv in the repository, which upload_history.sh
# extends with the rows of every new run (using --run and --no-header), and
# which perftests_timeline.py plots.
#
# Usage: perftests_history.py <clone of the data repository>
#            [--node <node>] [--benchmark <name>] [--since <date>]
#            [--until <date>] [--run <directory>] [--no-header]
#            [--output <file.csv>]

import argparse
import csv
import datetime
import glob
import json
import os
import sys


def parse_date(text):
    when = datetime.datetime.fromisoformat(text.replace('Z', '+00:00'))
    if when.tzinfo is None:
        when = when.replace(tzinfo=datetime.timezone.utc)
    return when.astimezone(datetime.timezone.utc)


def runs(repository, node, run):
    # <year>/<node>/<run>/metadata.json, or the one run given
    if run:
        pattern = os.path.join(repository, run, 'metadata.json')
    else:
        pattern = os.path.join(repository, '[0-9]' * 4, node or '*', '*',
            'metadata.json')
    for metadata_path in glob.glob(pattern):
        run_dir = os.path.dirname(metadata_path)
        try:
            with open(metadata_path) as f:
                metadata = json.load(f)
            with open(os.path.join(run_dir, 'summary.json')) as f:
                summary = json.load(f)
        except (OSError, ValueError) as e:
            print(f"Skipping {run_dir}: {e}", file=sys.stderr)
            continue
        yield run_dir, metadata, summary


def main():
    parser = argparse.ArgumentParser(
        description='time line of the performance history')
    parser.add_argument('repository')
    parser.add_argument('--node', default=None)
    parser.add_argument('--benchmark', default=None)
    parser.add_argument('--since', type=parse_date, default=None)
    parser.add_argument('--until', type=parse_date, default=None)
    parser.add_argument('--run', default=None,
        help='only the run in this directory (relative to the repository)')
    parser.add_argument('--no-header', action='store_true')
    parser.add_argument('--output', default=None)
    args = parser.parse_args()

    rows = []
    for run_dir, metadata, summary in runs(args.repository, args.node,
            args.run):
        commit = metadata.get('commit', {})
        run = metadata.get('run', {})
        commit_date = commit.get('committer_date')
        when = parse_date(commit_date) if commit_date else None
        if when is not None and (
                (args.since and when < args.since) or
                (args.until and when > args.until)):
            continue
        for benchmark, data in summary.get('benchmarks', {}).items():
            if args.benchmark and benchmark != args.benchmark:
                continue
            for measurement in data.get('measurements', []):
                medians = measurement.get('execution_medians') or []
                rows.append({
                    'node': run.get('node'),
                    'commit_date': when.isoformat() if when else '',
                    'first_parent_count': commit.get('first_parent_count'),
                    'commit': commit.get('hash'),
                    'pull_request': commit.get('pull_request') or '',
                    'build': run.get('jenkins_build') or '',
                    'run_started': run.get('started') or '',
                    'status': run.get('status'),
                    'benchmark': benchmark,
                    'name': measurement.get('name'),
                    'executor': measurement.get('executor'),
                    'median': measurement.get('median'),
                    'executions': len(medians),
                    'execution_median_min': min(medians) if medians else '',
                    'execution_median_max': max(medians) if medians else '',
                    'directory': os.path.relpath(run_dir,
                        args.repository).replace(os.sep, '/'),
                })

    def build_number(row):
        try:
            return int(row['build'])
        except ValueError:
            return 0

    rows.sort(key=lambda row: (row['node'] or '', row['commit_date'],
        build_number(row), row['benchmark'], row['name'] or '',
        row['executor'] or ''))

    fields = ['node', 'commit_date', 'first_parent_count', 'commit',
        'pull_request', 'build', 'run_started', 'status', 'benchmark', 'name',
        'executor', 'median', 'executions', 'execution_median_min',
        'execution_median_max', 'directory']
    output = open(args.output, 'w', newline='') if args.output else sys.stdout
    try:
        writer = csv.DictWriter(output, fieldnames=fields, lineterminator='\n')
        if not args.no_header:
            writer.writeheader()
        writer.writerows(rows)
    finally:
        if args.output:
            output.close()


if __name__ == '__main__':
    main()
