#!/bin/bash
# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Packs a directory of performance test reference results (*.json) into
# tools/perftests_ci/perftest/references/<node>.tar.gz, which is unpacked into
# the build directory when CMake configures the performance tests.
#
# Usage: pack_references.sh <directory with the *.json files> [<node>
#            [<output directory>]]
#
# The timings differ between nodes, so each node the performance tests run on
# has its own references, named after the node (the host name without the
# domain). <node> defaults to the name of this host, the output directory to
# tools/perftests_ci/perftest/references. To add the references of a node,
# pack the results of a run on that node (the CI keeps them in the
# perftests-reports directory of its artifacts, and already packed as
# perftests-reports/references/<node>.tar.gz; on a node without references it
# also uploads them packed next to the plots). To update a reference,
# unpack the current archive, replace the json files that changed, and pack
# the directory again:
#
#   tar -xzf tools/perftests_ci/perftest/references/medusa08.tar.gz -C /tmp
#   cp <build>/tools/perftests_ci/results/stream_report.json /tmp/medusa08
#   tools/perftests_ci/pack_references.sh /tmp/medusa08 medusa08
#
# The archive is reproducible: packing the same files again produces the
# same archive, so unchanged references don't show up as changes.

set -eu

if [[ $# -lt 1 || $# -gt 3 ]]; then
    echo "Usage: $0 <directory with the *.json files> [<node> [<output directory>]]" >&2
    exit 1
fi

input_dir="$(cd "$1" && pwd)"
name="${2:-$(hostname -s)}"
references_dir="${3:-$(cd "$(dirname "$0")" && pwd)/perftest/references}"
archive="${references_dir}/${name}.tar.gz"

shopt -s nullglob
files=("${input_dir}"/*.json)
if [[ ${#files[@]} -eq 0 ]]; then
    echo "No *.json files found in ${input_dir}" >&2
    exit 1
fi

# Stage the files under <name>/ so the archive always unpacks into a directory
# with the expected name, whatever the input directory is called. Strip
# carriage returns, so the archive is the same when packed on Windows.
staging_dir="$(mktemp -d)"
trap 'rm -rf "${staging_dir}"' EXIT
mkdir "${staging_dir}/${name}"
for file in "${files[@]}"; do
    tr -d '\r' < "${file}" > "${staging_dir}/${name}/$(basename "${file}")"
done

tar --sort=name --owner=0 --group=0 --numeric-owner \
    --mtime='1970-01-01 00:00:00 UTC' \
    -C "${staging_dir}" -cf - "${name}" | gzip -9 -n > "${archive}"

echo "Packed ${#files[@]} reference files into ${archive}"
