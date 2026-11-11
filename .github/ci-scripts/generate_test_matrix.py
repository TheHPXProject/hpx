#!/usr/bin/env python3
# Copyright (c) 2026 The STE||AR Group
# Copyright (c) 2026 Surya Teja
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import argparse
import sys
import json
import re
import math
import os

DEFAULT_EXCLUDE_FILE = os.path.abspath(os.path.join(
    os.path.dirname(__file__), "..", "workflows", "exclude.targets"
))

def get_excluded_tests(exclude_files):
    excluded = set()
    for exclude_file in exclude_files:
        if not os.path.exists(exclude_file):
            raise FileNotFoundError(f"exclude file not found: {exclude_file}")
        with open(exclude_file, "r") as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    excluded.add(line)
    return excluded

def generate_matrix(ctest_output, num_buckets, exclude_files):
    tests = []
    pattern = re.compile(r"^\s*Test\s*#\d+:\s*(.+)$")

    excluded = get_excluded_tests(exclude_files)

    for line in ctest_output.splitlines():
        match = pattern.match(line)
        if match:
            test_name = match.group(1).strip()
            if test_name not in excluded and not test_name.endswith('_perftest'):
                tests.append(test_name)

    tests.sort()

    if not tests:
        return {"include": []}

    total_tests = len(tests)
    bucket_size = math.ceil(total_tests / num_buckets)

    matrix = {"include": []}

    for i in range(num_buckets):
        start = i * bucket_size
        end = min((i + 1) * bucket_size, total_tests)

        if start >= total_tests:
            break

        bucket_tests = tests[start:end]

        test_regex = "^(" + "|".join(re.escape(t) for t in bucket_tests) + ")$"

        # distributed tests have .distributed.<parcelport>. in their name which
        # is not in the target
        bucket_targets = []
        for t in bucket_tests:
            target = re.sub(r'\.distributed\.(tcp|mpi|lci|lcw|gasnet)\.', '.', t)
            bucket_targets.append(target)

        unique_targets = sorted(list(set(bucket_targets)))
        if len(unique_targets) != 0:
            first_name_parts = unique_targets[0].split('.')
            last_name_parts = unique_targets[-1].split('.')

            first_name_index = -2 if len(first_name_parts) > 1 else -1
            last_name_index = -2 if len(last_name_parts) > 1 else -1

            first_name = first_name_parts[first_name_index]
            last_name = last_name_parts[last_name_index]

            targets_str = " ".join(unique_targets)
            matrix["include"].append({
                "id": i + 1,
                "name": f"{i+1:02d}-{first_name}-{last_name}",
                "tests": test_regex,
                "targets": targets_str,
                "count": len(bucket_tests)
            })

    return matrix

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Split the tests listed by 'ctest -N' (read from stdin) "
        "into a GitHub Actions job matrix")
    parser.add_argument("num_jobs", nargs="?", type=int, default=10,
        help="number of matrix entries (default: 10)")
    parser.add_argument("--exclude", action="append", default=None,
        metavar="FILE",
        help="file listing tests to leave out, one per line ('#' starts a "
        "comment); may be given more than once (default: "
        ".github/workflows/exclude.targets)")
    args = parser.parse_args()

    try:
        input_data = sys.stdin.read()
        exclude_files = args.exclude if args.exclude else [DEFAULT_EXCLUDE_FILE]

        matrix = generate_matrix(input_data, args.num_jobs, exclude_files)
        print(json.dumps(matrix))
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
