# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The parts of the performance report that comment_github.sh (the pull request
# comment) and upload_images.sh (the README.md next to the uploaded plots)
# share. perftests_plot.py writes the rows of the table into index.html, and
# what the results and the references were measured with into info.html.

# Writes a message about what the performance test scripts do to stderr,
# prefixed with the time (UTC) and the name of the script, so that it stands
# out in the traced output of the Jenkins log
perftest_log() {
    { local tracing="${-//[^x]/}"; set +x; } 2> /dev/null
    echo "[perftests $(date -u +%H:%M:%S) ${0##*/}] $*" >&2
    if [[ -n "${tracing}" ]]; then
        set -x
    fi
}

# Prints the commit the report belongs to and the Jenkins build that measured
# it, from the variables Jenkins sets
report_intro() {
    local commit="${ghprbActualCommit:-${GIT_COMMIT:-}}"
    local text="" build=""
    if [[ -n "${commit}" ]]; then
        text="Commit <a href=\"https://github.com/TheHPXProject/hpx/commit/${commit}\">${commit}</a>"
    fi
    if [[ -n "${BUILD_NUMBER:-}" ]]; then
        build="${BUILD_NUMBER}"
        if [[ -n "${BUILD_URL:-}" ]]; then
            build="<a href=\"${BUILD_URL}\">${BUILD_NUMBER}</a>"
        fi
        text="${text:+${text}, }Jenkins build ${build}"
    fi
    # Requested by "retest lsu perf full" (see entry.sh)
    if [[ -n "${HPX_PERFTESTS_REPORT_ALL:-}" ]]; then
        text="${text:+${text}, }full report (requested)"
    fi
    if [[ -n "${text}" ]]; then
        printf '<p>%s</p>' "${text}"
    fi
}

# Prints where the new references of a node without references for some
# benchmarks are, which batch.sh keeps in <report directory>/new-references/
# <node> and upload_images.sh uploads packed as <node>.tar.gz to
# <references URL>.
#
# Usage: report_new_references <report directory> [<references URL>]
report_new_references() {
    local node_dir node
    for node_dir in "$1"/new-references/*/; do
        if [[ ! -d "${node_dir}" ]]; then
            continue
        fi
        node="$(basename "${node_dir}")"
        if [[ -n "${2:-}" ]]; then
            printf '<p>%s has no references for some of the benchmarks (new ones, or ones whose measurements changed). New references of all its benchmarks, measured by this run, are uploaded as <a href="%s">%s</a>, copy it to tools/perftests_ci/perftest/references to use them.</p>' \
                "${node}" "$2/${node}.tar.gz" "${node}.tar.gz"
        else
            printf '<p>%s has no references for some of the benchmarks (new ones, or ones whose measurements changed). New references of all its benchmarks, measured by this run, are in the Jenkins artifacts (perftests-reports/reference-comparison/new-references/%s), pack them with tools/perftests_ci/pack_references.sh to use them.</p>' \
                "${node}" "${node}"
        fi
    done
}

report_table_header='<tr><th>Test name</th><th>Executor</th><th>Percentage Diff.</th><th>Classification</th></tr>'

# Prints the legend of the report, with the node the benchmarks ran on (from
# PERFTEST_NODE, set by entry.sh). The classification is the one of classify()
# in perftests_plot.py.
report_legend() {
    printf '%s' '<h3>Explanation of Symbols</h3>'
    if [[ -n "${PERFTEST_NODE:-}" ]]; then
        printf '<p>The benchmarks ran on %s, the results are compared with the references of that node.</p>' "${PERFTEST_NODE}"
    fi
    printf '%s' '<p>Percentage Diff. is the change of the median time relative to the reference, positive values mean faster.</p><table><tr><th>Symbol</th><th>Meaning</th></tr><tr><td>=</td><td>No performance change (confidence interval within &plusmn;1%)</td></tr><tr><td>(=)</td><td>Probably no performance change (confidence interval within &plusmn;2%)</td></tr><tr><td>(+)/(-)</td><td>Very small performance improvement/degradation (&le;1%)</td></tr><tr><td>+/-</td><td>Small performance improvement/degradation (&le;5%)</td></tr><tr><td>++/--</td><td>Large performance improvement/degradation (&le;10%)</td></tr><tr><td>+++/---</td><td>Very large performance improvement/degradation (&gt;10%)</td></tr><tr><td>?</td><td>Probably no change, but quite large uncertainty (confidence interval within &plusmn;5%)</td></tr><tr><td>??</td><td>Unclear result, very large uncertainty (&plusmn;10%)</td></tr><tr><td>???</td><td>Something unexpected</td></tr><tr><td>no reference</td><td>No reference results for this node yet, the plot shows only the current results</td></tr></table>'
}
