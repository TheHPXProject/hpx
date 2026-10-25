#!/usr/bin/env python3
#
# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Writes the record of a performance test run of a master commit, which
# .jenkins/lsu-perftests/upload_history.sh uploads to the performance data
# repository, so that the performance of every benchmark over time can be
# reconstructed from the repository (see perftests_history.py):
#
#   metadata.json  the commit (hash, parents, author and committer dates, pull
#                  request), when and where the benchmarks ran, the Jenkins
#                  build, the configuration and the status of the run
#   summary.json   the median of every measurement, overall and per execution
#   run.tsv        one line for the index of all runs (runs.tsv)
#   path.txt       the directory of the run in the repository,
#                  <year>/<node>/<commit date>-<commit>-b<build>
#   intro.md       the beginning of the README.md of the run
#
# Only the standard library is used, so that a run whose Python environment
# couldn't be set up is still recorded.

import argparse
import datetime
import glob
import json
import os
import re
import statistics
import subprocess


def git(source_dir, *args):
    try:
        return subprocess.run(['git', '-C', source_dir, *args], check=True,
            capture_output=True, text=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def commit_info(source_dir, commit):
    fields = git(source_dir, 'log', '-1', '--format=%H%n%P%n%an%n%aI%n%cI%n%s',
        commit)
    if fields is None:
        return {'hash': commit}
    sha, parents, author, author_date, committer_date, subject = \
        (fields.split('\n') + [''] * 6)[:6]
    info = {
        'hash': sha,
        'parents': parents.split(),
        'subject': subject,
        'author': author,
        'author_date': author_date,
        'committer_date': committer_date,
        'pull_request': None,
        # The position of the commit on the first parent history of the
        # branch, which orders the runs independently of the dates. A
        # shallow clone doesn't have the whole history.
        'first_parent_count': None,
    }
    match = re.match(r'Merge pull request #(\d+)', subject)
    if match:
        info['pull_request'] = int(match.group(1))
    if git(source_dir, 'rev-parse', '--is-shallow-repository') == 'false':
        count = git(source_dir, 'rev-list', '--count', '--first-parent', sha)
        if count is not None:
            info['first_parent_count'] = int(count)
    return info


def execution_medians(output):
    # The median of each execution, see execution_medians in perftests_plot.py
    series = output.get('series', [])
    sizes = output.get('executions')
    if not sizes or sum(sizes) != len(series):
        return []
    medians, begin = [], 0
    for size in sizes:
        if size > 0:
            medians.append(statistics.median(series[begin:begin + size]))
        begin += size
    return medians


def summarize(results_dir):
    benchmarks = {}
    for path in sorted(glob.glob(os.path.join(results_dir, '*.json'))):
        try:
            with open(path) as f:
                data = json.load(f)
        except (OSError, ValueError):
            continue
        measurements = []
        for output in data.get('outputs', []):
            series = output.get('series', [])
            measurements.append({
                'name': output.get('name'),
                'executor': output.get('executor'),
                'timings': len(series),
                'median': statistics.median(series) if series else None,
                'execution_medians': execution_medians(output),
            })
        benchmarks[os.path.splitext(os.path.basename(path))[0]] = {
            'datetime': data.get('environment', {}).get('datetime'),
            'measurements': measurements,
        }
    return benchmarks


def main():
    parser = argparse.ArgumentParser(
        description='record a performance test run of a master commit')
    parser.add_argument('--source-dir', required=True)
    parser.add_argument('--results-dir', required=True)
    parser.add_argument('--output-dir', required=True)
    parser.add_argument('--commit', default='HEAD')
    parser.add_argument('--branch', default=None)
    parser.add_argument('--node', required=True)
    parser.add_argument('--build-number', default=None)
    parser.add_argument('--build-url', default=None)
    parser.add_argument('--started', default=None)
    parser.add_argument('--finished', default=None)
    parser.add_argument('--status', type=int, default=None)
    parser.add_argument('--errors-file', default=None)
    parser.add_argument('--compiler', default=None)
    parser.add_argument('--configure-options', default='')
    args = parser.parse_args()

    commit = commit_info(args.source_dir, args.commit)
    benchmarks = summarize(args.results_dir)

    failed_steps = []
    if args.errors_file and os.path.exists(args.errors_file):
        with open(args.errors_file) as f:
            failed_steps = [line.strip() for line in f if line.strip()]

    # The compiler the results were measured with (added by the driver)
    compiler = args.compiler
    if compiler is None:
        for path in glob.glob(os.path.join(args.results_dir, '*.json')):
            try:
                with open(path) as f:
                    compiler = json.load(f).get('environment', {}).get(
                        'compiler')
                break
            except (OSError, ValueError):
                continue

    metadata = {
        'format': 1,
        'commit': commit,
        'branch': args.branch,
        'run': {
            'node': args.node,
            'started': args.started,
            'finished': args.finished,
            'recorded': datetime.datetime.now(
                datetime.timezone.utc).isoformat(timespec='seconds'),
            'jenkins_build': args.build_number,
            'jenkins_build_url': args.build_url,
            'status': args.status,
            'failed_steps': failed_steps,
            'benchmarks': sorted(benchmarks),
        },
        'configuration': {
            'compiler': compiler,
            'configure_options': args.configure_options.split(),
        },
    }

    # The directory sorts by the commit date, the build number keeps several
    # runs of the same commit apart
    date = commit.get('committer_date') or args.started or \
        metadata['run']['recorded']
    when = datetime.datetime.fromisoformat(date.replace('Z', '+00:00'))
    when = when.astimezone(datetime.timezone.utc)
    name = '{}-{}-b{}'.format(when.strftime('%Y%m%dT%H%M%SZ'),
        commit['hash'][:12], args.build_number or 0)
    path = '{}/{}/{}'.format(when.strftime('%Y'), args.node, name)

    # Always with \n line endings, the files are uploaded as they are
    def output(name):
        return open(os.path.join(args.output_dir, name), 'w', newline='')

    os.makedirs(args.output_dir, exist_ok=True)
    with output('metadata.json') as f:
        json.dump(metadata, f, indent=2)
        f.write('\n')
    with output('summary.json') as f:
        json.dump({'format': 1, 'benchmarks': benchmarks}, f, indent=2)
        f.write('\n')
    with output('path.txt') as f:
        f.write(path + '\n')

    # The beginning of the README.md of the run
    commit_url = 'https://github.com/TheHPXProject/hpx/commit/' + \
        commit['hash']
    lines = ['Commit [{}]({}) "{}", committed {}'.format(commit['hash'],
        commit_url, commit.get('subject', ''), commit.get('committer_date'))]
    if commit.get('pull_request'):
        lines[0] += ', pull request [#{0}]({1}/pull/{0})'.format(
            commit['pull_request'], 'https://github.com/TheHPXProject/hpx')
    build = args.build_number or 'unknown'
    if args.build_url:
        build = '[{}]({})'.format(build, args.build_url)
    lines.append('Jenkins build {}, ran on {} from {} to {}, status {}'.format(
        build, args.node, args.started, args.finished, args.status))
    if failed_steps:
        lines.append('Failed steps: ' + ', '.join(failed_steps))
    lines.append('[metadata.json](metadata.json), [summary.json]'
        '(summary.json), [results.tar.gz](results.tar.gz) (the results of '
        'all benchmarks)')
    with output('intro.md') as f:
        f.write('\n\n'.join(lines) + '\n')
    # committer date, commit, pull request, build, status, node, directory
    with output('run.tsv') as f:
        f.write('\t'.join(str(value) for value in (
            when.strftime('%Y-%m-%dT%H:%M:%SZ'), commit['hash'],
            commit.get('pull_request') or '', args.build_number or '',
            '' if args.status is None else args.status, args.node,
            path)) + '\n')


if __name__ == '__main__':
    main()
