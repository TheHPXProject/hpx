#!/bin/bash -l

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Uploads the run of a master commit to the performance data repository,
# TheHPXProject/hpx-performance (PERFTEST_HISTORY_REPO), so that the
# performance of every benchmark over time can be reconstructed from it. The
# repository holds
#
#   README.md        the performance of every benchmark over time (the graphs
#                    in timeline/), and links to the runs of every year
#   LICENSE_1_0.txt  the Boost Software License
#   runs.tsv         the index of all runs, one line per run
#   timeline.csv     the medians of all measurements of all runs (see
#                    tools/perftests_ci/perftests_history.py)
#   timeline/        the graphs of the README.md, one per benchmark (see
#                    tools/perftests_ci/perftests_timeline.py)
#   <year>/README.md the runs of the year, the newest commit first
#   <year>/<node>/<commit date>-<commit>-b<build>/
#                    a run: its report (README.md), the commit, its dates and
#                    pull request, when and where the benchmarks ran, the
#                    Jenkins build, the configuration and the status
#                    (metadata.json, see perftests_record.py), the medians of
#                    all measurements (summary.json), the results of all
#                    benchmarks as measured (results.tar.gz) and the
#                    comparison with the references of the node (index.html,
#                    info.html, changed.txt). The plots of the comparison
#                    are left out, perftests_plot.py can draw them again from
#                    the results and the references of the node at the
#                    commit.
#
# Runs are only ever added. The graphs are drawn with the Python environment of
# the performance tests (PERFTEST_PYTHON_ENV); if that fails, the run is
# uploaded with the graphs left as they were.
#
# Usage: upload_history.sh <reports directory>
#
# The reports directory is perftests-reports, as written by batch.sh, with
# the record of the run in its history subdirectory.

# Never trace the token the credential helper passes to git.
set -eu

source "$(dirname "$0")/report_common.sh"

reports_dir="$(cd "$1" && pwd)"
record_dir="${reports_dir}/history"
report_dir="${reports_dir}/reference-comparison"
history_repo="${PERFTEST_HISTORY_REPO:-https://github.com/TheHPXProject/hpx-performance.git}"
python_env="${PERFTEST_PYTHON_ENV:-/home/pansysk75/virtual_envs/perftests_env}"
src_dir="$(cd "$(dirname "$0")/../.." && pwd)"
tools_dir="${src_dir}/tools/perftests_ci"

target="$(tr -d '\r\n' < "${record_dir}/path.txt")"
year="${target%%/*}"
node="${target#*/}"
node="${node%%/*}"

work_dir="$(mktemp -d)"
trap 'rm -rf "${work_dir}"' EXIT

# The helper reads the token from the environment when git asks for it, so the
# token doesn't show up on a command line or in the clone's configuration.
# Commits are made as the STE||AR-Bot.
git_history() {
    git -C "${work_dir}" \
        -c credential.helper= \
        -c credential.helper='!f() { echo username=x-access-token; echo "password=${GITHUB_TOKEN}"; }; f' \
        -c user.name="STE||AR-Bot" \
        -c user.email="7449129+StellarBot@users.noreply.github.com" \
        "$@"
}

# Check out only the directory of this run, the graphs, and the files of the
# directories above them (the index, the timeline and the READMEs), the
# repository grows with every run. A new, empty repository gets its first
# commit from this run.
perftest_log "Cloning ${history_repo} (only the files this run changes)"
git_history clone --quiet --depth 1 --filter=blob:none --sparse \
    "${history_repo}" data 2> "${work_dir}/clone.err" ||
    { cat "${work_dir}/clone.err" >&2; exit 1; }
git_history -C data sparse-checkout init --cone
git_history -C data sparse-checkout set "${target}" timeline
if git_history -C data rev-parse --verify --quiet HEAD > /dev/null; then
    perftest_log "The repository is at $(git_history -C data log -1 \
        --format='%h (%cI)'), $(($(wc -l < "${work_dir}/data/runs.tsv" \
        2> /dev/null || echo 1) - 1)) runs so far"
else
    perftest_log "The repository is empty, this run starts it"
fi

# Prints the README.md of the repository
main_readme() {
    local data="$1"
    echo "# HPX performance history"
    echo
    echo "The results of the performance tests of" \
        "[HPX](https://github.com/TheHPXProject/hpx) for the commits of its" \
        "master branch, measured by its Jenkins CI on the Rostam cluster at" \
        "LSU (see \`.jenkins/lsu-perftests\` and \`tools/perftests_ci\` in" \
        "the HPX repository). The graphs below show the median time of every" \
        "measurement over the date of the commit; lower is faster."
    echo
    echo "## Layout"
    echo
    echo "- \`<year>/<node>/<commit date>-<commit>-b<build>/\`: one run, with" \
        "its report (\`README.md\`), the commit, when and where the" \
        "benchmarks ran and the configuration (\`metadata.json\`), the" \
        "medians of all measurements (\`summary.json\`), the results of all" \
        "benchmarks as measured (\`results.tar.gz\`), and the comparison with" \
        "the references of the node."
    echo "- \`runs.tsv\`: all runs. \`timeline.csv\`: the medians of all" \
        "measurements of all runs, as written by" \
        "\`tools/perftests_ci/perftests_history.py\`, which rebuilds it from" \
        "the run directories. \`timeline/\`: the graphs below, drawn by" \
        "\`tools/perftests_ci/perftests_timeline.py\`."
    echo
    echo "## Runs"
    echo
    tail -n +2 "${data}/runs.tsv" | cut -f7 | cut -d/ -f1 | sort -ru |
        while read -r run_year; do
            echo "- [${run_year}](${run_year}/README.md)"
        done
    echo
    echo "## Performance over time"
    echo
    # The links of the graphs are relative to timeline/
    if [[ -s "${data}/timeline/README-timeline.md" ]]; then
        sed 's|](\([^/)]*\.svg\))|](timeline/\1)|' \
            "${data}/timeline/README-timeline.md"
    else
        echo "The graphs couldn't be drawn yet."
    fi
    echo
    echo "## License"
    echo
    echo "Distributed under the Boost Software License, Version 1.0, see" \
        "[LICENSE_1_0.txt](LICENSE_1_0.txt)."
}

# Prints the README.md of the runs of a year, the newest commit first. The
# builds link to Jenkins: runs.tsv has only their numbers, but all runs are
# uploaded by builds of this job (JOB_URL).
year_readme() {
    local data="$1" run_year="$2" build_link
    echo "# Performance test runs of ${run_year}"
    echo
    echo "[Performance history](../README.md)"
    echo
    echo "| Commit date | Commit | Pull request | Build | Node | Status |"
    echo "| --- | --- | --- | --- | --- | --- |"
    # read would merge consecutive tabs (tab is IFS whitespace) and shift the
    # fields of a run without pull request, so split at \037 instead
    tail -n +2 "${data}/runs.tsv" | sort -r | tr '\t' '\037' |
        while IFS=$'\037' read -r date commit pull_request build status \
            run_node directory; do
        if [[ "${directory%%/*}" != "${run_year}" ]]; then
            continue
        fi
        case "${status}" in
            0) status="passed" ;;
            "") status="unknown" ;;
            *) status="failed (${status})" ;;
        esac
        build_link="${build}"
        if [[ -n "${build}" && -n "${JOB_URL:-}" ]]; then
            build_link="[${build}](${JOB_URL%/}/${build}/)"
        fi
        printf '| %s | [%s](%s/README.md) | %s | %s | %s | %s |\n' \
            "${date}" "${commit:0:12}" "${directory#*/}" \
            "${pull_request:+[#${pull_request}](https://github.com/TheHPXProject/hpx/pull/${pull_request})}" \
            "${build_link}" "${run_node}" "${status}"
    done
}

# Adds the run, its lines in the index and the timeline, the graphs and the
# READMEs to the clone and commits them
add_run() {
    local data="${work_dir}/data" run="${work_dir}/data/${target}" file
    if [[ -e "${run}" ]]; then
        echo "${target} is already in the performance history" >&2
        return 1
    fi
    if [[ ! -e "${data}/LICENSE_1_0.txt" ]]; then
        cp "${src_dir}/LICENSE_1_0.txt" "${data}"
    fi

    perftest_log "Adding the run as ${target}"
    mkdir -p "${run}"
    cp "${record_dir}/metadata.json" "${record_dir}/summary.json" "${run}"
    if compgen -G "${reports_dir}/*.json" > /dev/null; then
        "${tools_dir}/pack_references.sh" "${reports_dir}" results "${run}" >&2
    else
        perftest_log "The run has no results, only its record is added"
    fi
    for file in index.html info.html changed.txt; do
        if [[ -e "${report_dir}/${file}" ]]; then
            cp "${report_dir}/${file}" "${run}"
        fi
    done

    {
        echo "# Performance test results of ${target##*/}"
        echo
        echo "[Runs of ${year}](../../README.md)," \
            "[performance history](../../../README.md)"
        echo
        cat "${record_dir}/intro.md"
        echo
        if [[ -s "${run}/info.html" ]]; then
            cat "${run}/info.html"
            printf '\n\n'
        fi
        # The table without the rows of the plots, which aren't uploaded
        if [[ -s "${run}/index.html" ]]; then
            echo "<table>${report_table_header}$(sed -e \
                's|<tr><td colspan="4"><details><summary>Plot</summary><img [^>]*></details></td></tr>||g' \
                "${run}/index.html")</table>"
            echo
        fi
        PERFTEST_NODE="${node}" report_legend
        echo
    } > "${run}/README.md"

    # The index of all runs, one line per run, in the order they were uploaded
    if [[ ! -e "${data}/runs.tsv" ]]; then
        printf 'commit_date\tcommit\tpull_request\tbuild\tstatus\tnode\tdirectory\n' \
            > "${data}/runs.tsv"
    fi
    tr -d '\r' < "${record_dir}/run.tsv" >> "${data}/runs.tsv"

    # The medians of all measurements of all runs, and their graphs
    local rows_before=0
    if [[ -e "${data}/timeline.csv" ]]; then
        rows_before=$(($(wc -l < "${data}/timeline.csv") - 1))
        python3 "${tools_dir}/perftests_history.py" "${data}" \
            --run "${target}" --no-header >> "${data}/timeline.csv"
    else
        python3 "${tools_dir}/perftests_history.py" "${data}" \
            --run "${target}" > "${data}/timeline.csv"
    fi
    perftest_log "Added $(($(wc -l < "${data}/timeline.csv") - 1 - \
        rows_before)) measurements to timeline.csv," \
        "$(($(wc -l < "${data}/timeline.csv") - 1)) in all"
    perftest_log "Drawing the graphs of all benchmarks over time" \
        "(Python environment ${python_env})"
    if ! (
        # The environment was set up for the compute nodes, libraries of
        # modules loaded by the login shell (e.g. a newer libgcc_s on the
        # LD_LIBRARY_PATH) can break it on the Jenkins agent
        set +u
        if type module > /dev/null 2>&1; then
            module purge > /dev/null 2>&1 || true
        fi
        unset LD_LIBRARY_PATH
        if [[ -e "${python_env}/bin/activate" ]]; then
            source "${python_env}/bin/activate"
        fi
        set -u
        python3 "${tools_dir}/perftests_timeline.py" "${data}/timeline.csv" \
            "${data}/timeline" >&2
    ); then
        echo "Warning: could not draw the graphs of the performance" \
            "history, they are left as they were" >&2
        rm -rf "${data}/timeline"
        git_history -C data checkout --quiet HEAD -- timeline 2> /dev/null ||
            true
    fi

    year_readme "${data}" "${year}" > "${data}/${year}/README.md"
    main_readme "${data}" > "${data}/README.md"

    git_history -C data add -A .
    git_history -C data commit --quiet \
        -m "Add the performance test results of ${target##*/}"
}

add_run

# Other builds may push at the same time. Add the run again on top of their
# commits, the index, the timeline and the READMEs may have changed.
for attempt in 1 2 3; do
    perftest_log "Pushing to ${history_repo} (attempt ${attempt})"
    if git_history -C data push --quiet origin HEAD:main; then
        perftest_log "Uploaded the run to" \
            "${history_repo%.git}/tree/main/${target}"
        exit 0
    fi
    perftest_log "The push failed, adding the run again on top of the" \
        "current repository"
    sleep "${attempt}"
    git_history -C data fetch --quiet --depth 1 --filter=blob:none origin main
    git_history -C data reset --quiet --hard FETCH_HEAD
    git_history -C data clean --quiet -fdx
    add_run
done

perftest_log "Could not upload the run to ${history_repo}"
exit 1
