#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set -eux

# Computes the status of the job and store the artifacts
status_computation_and_artifacts_storage() {
    ctest_exit_code=$?

    # ctest.cmake lists the benchmarks that failed to build or to run and a
    # failed comparison, ctest itself doesn't fail for them
    errors_file="jenkins-hpx-${configuration_name}-perftest-errors.txt"
    if [[ -s "${errors_file}" ]]; then
        echo "Failed performance test steps:" >&2
        cat "${errors_file}" >&2
        test_errors=$(grep -c . "${errors_file}")
    fi

    ctest_status=$(( ctest_exit_code + configure_build_errors + test_errors + plot_errors ))
    perftest_log "Performance tests finished with status ${ctest_status}" \
        "(ctest ${ctest_exit_code}, failed steps ${test_errors})"

    # Write status file first
    echo "${ctest_status}" > "jenkins-hpx-${configuration_name}-ctest-status.txt"

    # Copy the testing directory for saving as an artifact
    cp -r "${build_dir}/Testing" "${src_dir}/${configuration_name}-Testing"
    mkdir -p "${src_dir}/${configuration_name}-reports"
    for result in "${perftests_dir}"/results/*.json; do
        if [[ -e "${result}" ]]; then
            cp "${result}" "${src_dir}/${configuration_name}-reports"
        fi
    done

    # The results of this run packed as references of this node (see
    # tools/perftests_ci/pack_references.sh), ready to replace the current ones
    results=$(compgen -G "${perftests_dir}/results/*.json" | wc -l)
    perftest_log "${results} benchmarks have results:" \
        $(cd "${perftests_dir}/results" 2> /dev/null && ls *.json 2> /dev/null)
    if [[ "${results}" -gt 0 ]]; then
        perftest_log "Packing the results as references of $(hostname -s)" \
            "(${configuration_name}-reports/references in the artifacts)"
        mkdir -p "${references_artifact_dir}"
        "${src_dir}/tools/perftests_ci/pack_references.sh" \
            "${perftests_dir}/results" "$(hostname -s)" \
            "${references_artifact_dir}" ||
            echo "Warning: could not pack the results as references" >&2
    fi

    # entry.sh comments on the pull request with this report if a measurement
    # got more than 5% faster or slower (listed in changed.txt)
    if [[ -s "${perftests_dir}/index.html" ]]; then
        mkdir -p "${src_dir}/${configuration_name}-reports/reference-comparison"
        cp "${perftests_dir}/index.html" \
            "${src_dir}/${configuration_name}-reports/reference-comparison"
        for file in changed.txt info.html; do
            if [[ -e "${perftests_dir}/${file}" ]]; then
                cp "${perftests_dir}/${file}" \
                    "${src_dir}/${configuration_name}-reports/reference-comparison"
            fi
        done
        for plot in "${perftests_dir}"/*.png; do
            if [[ -e "${plot}" ]]; then
                cp "${plot}" \
                    "${src_dir}/${configuration_name}-reports/reference-comparison"
            fi
        done
    fi

    # If a benchmark has no reference (listed in no_reference.txt), because it
    # is new or its measurements changed, all references of this node are made
    # again from this run, so that they are measured together. The existing
    # references are kept only for the benchmarks without a result.
    # entry.sh uploads them in any case (see upload_images.sh).
    if [[ -s "${perftests_dir}/no_reference.txt" ]]; then
        node="$(hostname -s)"
        perftest_log "No references on ${node} for:" \
            $(cat "${perftests_dir}/no_reference.txt") \
            "- making new references of all benchmarks from this run"
        new_references_dir="${new_references_root}/${node}"
        mkdir -p "${new_references_dir}"
        for reference in "${perftests_dir}/references/${node}"/*.json \
            "${perftests_dir}"/results/*.json; do
            if [[ -e "${reference}" ]]; then
                cp "${reference}" "${new_references_dir}"
            fi
        done
    fi

    # The record of a run of a master commit (HPX_PERFTESTS_HISTORY, set by
    # entry.sh), which entry.sh uploads to the performance data repository
    # with upload_history.sh, also if some of the benchmarks failed
    if [[ -n "${HPX_PERFTESTS_HISTORY:-}" ]]; then
        perftest_log "Recording the run for the performance history"
        python3 "${src_dir}/tools/perftests_ci/perftests_record.py" \
            --source-dir "${src_dir}" \
            --results-dir "${perftests_dir}/results" \
            --output-dir "${history_dir}" \
            --commit "${GIT_COMMIT:-HEAD}" \
            --branch "${git_local_branch:-}" \
            --node "$(hostname -s)" \
            --build-number "${BUILD_NUMBER:-}" \
            --build-url "${BUILD_URL:-}" \
            --started "${run_started}" \
            --finished "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
            --status "${ctest_status}" \
            --errors-file "${errors_file}" \
            --configure-options="${configure_extra_options:-}" ||
            echo "Warning: could not record the run" >&2
    fi

    exit $ctest_status
}

src_dir="$(pwd)"
source "${src_dir}/.jenkins/lsu-perftests/report_common.sh"

trap "status_computation_and_artifacts_storage" EXIT

run_started="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
build_dir="${src_dir}/build/${configuration_name}"
perftest_log "Slurm job ${SLURM_JOB_ID:-unknown} on $(hostname -s)," \
    "commit $(git -C "${src_dir}" rev-parse HEAD 2> /dev/null || echo unknown)," \
    "build directory ${build_dir}"
if [[ -n "${HPX_PERFTESTS_HISTORY:-}" ]]; then
    perftest_log "The run is recorded for the performance history"
fi

mkdir -p "${build_dir}/tools"
cp -r "${src_dir}/tools/perftests_ci" "${build_dir}/tools"

# Variables
perftests_dir=${build_dir}/tools/perftests_ci
envfile=${src_dir}/.jenkins/lsu-perftests/env-${configuration_name}.sh
mkdir -p ${src_dir}/${configuration_name}-reports
logfile=${src_dir}/${configuration_name}-reports/jenkins-hpx-${configuration_name}.log

# Don't keep the report, its plots or the results of a previous run, so that a
# benchmark that fails this time is not reported with its old results
rm -f "${perftests_dir}/index.html" "${perftests_dir}/changed.txt" \
    "${perftests_dir}/info.html" "${perftests_dir}/no_reference.txt" \
    "${perftests_dir}"/*.png \
    "${perftests_dir}"/results/*.json \
    "jenkins-hpx-${configuration_name}-perftest-errors.txt"
new_references_root="${src_dir}/${configuration_name}-reports/reference-comparison/new-references"
references_artifact_dir="${src_dir}/${configuration_name}-reports/references"
history_dir="${src_dir}/${configuration_name}-reports/history"
rm -rf "${new_references_root}" "${references_artifact_dir}" "${history_dir}"
rm -f "${src_dir}/${configuration_name}-reports"/*.json

# Load python packages
source /home/pansysk75/virtual_envs/perftests_env/bin/activate

# Things went alright by default
configure_build_errors=0
test_errors=0
plot_errors=0

# Synchronize after the asynchronous copy from the source dir
wait

# Build and Run the perftests
source "${src_dir}/.jenkins/lsu-perftests/env-${configuration_name}.sh"

# CTest to upload the results to CDash
perftest_log "Configuring HPX, building the benchmarks, running and comparing" \
    "them (see ctest.cmake)"
ctest \
    --verbose --output-on-failure \
    -S "${src_dir}/.jenkins/lsu-perftests/ctest.cmake" \
    -DCTEST_BUILD_CONFIGURATION_NAME="${configuration_name}" \
    -DCTEST_CONFIGURE_EXTRA_OPTIONS="${configure_extra_options}" \
    -DCTEST_SOURCE_DIRECTORY="${src_dir}" \
    -DCTEST_BINARY_DIRECTORY="${build_dir}"
