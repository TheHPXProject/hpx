#!/bin/bash -l

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Uploads the plots a report shows to TheHPXProject/hpx-perftest-images, so
# that the pull request comment can show them, together with a README.md that
# shows the report with a link back to the pull request, and a README.md of the
# pull request that links to it and to the reports of its other builds. The new
# references of a node without references for some benchmarks (see batch.sh)
# are uploaded packed next to the plots, as references/<node>.tar.gz. A build
# of a branch is uploaded to branch/<branch>/<build number> instead of
# pr/<pull request>/<build number>.
#
# Usage: upload_images.sh <report directory> <pull request or branch>
#            <build number>
#
# Prints the URL of the directory the plots were uploaded to, or nothing if the
# report doesn't show any plots and there are no new references.

# Never trace the token the credential helper passes to git.
set -eu

source "$(dirname "$0")/report_common.sh"

report_dir="$1"
build_number="$3"
if [[ "$2" =~ ^[0-9]+$ ]]; then
    source_dir="pr/$2"
    source_name="pull request #$2"
    source_link="Pull request [#$2](https://github.com/TheHPXProject/hpx/pull/$2)"
else
    source_dir="branch/$2"
    source_name="branch $2"
    source_link="Branch [$2](https://github.com/TheHPXProject/hpx/tree/$2)"
fi
images_repo="${PERFTEST_IMAGES_REPO:-https://github.com/TheHPXProject/hpx-perftest-images.git}"
images_url="https://raw.githubusercontent.com/TheHPXProject/hpx-perftest-images"

# Upload only the plots that the report shows
plots=$(grep -o '<img src="[^"/]*\.png"' "${report_dir}/index.html" \
    2> /dev/null | sed 's/<img src="\(.*\)"/\1/' | sort -u || true)
new_references=$(ls -d "${report_dir}"/new-references/*/ 2>/dev/null || true)
if [[ -z "${plots}" && -z "${new_references}" ]]; then
    perftest_log "The report shows no plots and there are no new" \
        "references, nothing to upload"
    exit 0
fi
perftest_log "Uploading $(wc -w <<< "${plots}") plots" \
    "${new_references:+and the new references }of ${source_name}," \
    "build ${build_number}, to ${images_repo}"

work_dir="$(mktemp -d)"
trap 'rm -rf "${work_dir}"' EXIT

# The helper reads the token from the environment when git asks for it, so the
# token doesn't show up on a command line or in the clone's configuration.
git_images() {
    git -C "${work_dir}" \
        -c credential.helper= \
        -c credential.helper='!f() { echo username=x-access-token; echo "password=${GITHUB_TOKEN}"; }; f' \
        -c user.name="HPX performance tests" \
        -c user.email="hpx-perftests@users.noreply.github.com" \
        "$@"
}

git_images clone --quiet --depth 1 "${images_repo}" images
target="${source_dir}/${build_number}"

# Adds the plots and the README.md of this build, and the README.md of the pull
# request (or branch) that lists the reports of all its builds, to the clone
# and commits them
add_report() {
    local images="${work_dir}/images" build_dir build
    mkdir -p "${images}/${target}"
    for plot in ${plots}; do
        cp "${report_dir}/${plot}" "${images}/${target}"
    done
    for node_dir in ${new_references}; do
        mkdir -p "${images}/${target}/references"
        "$(dirname "$0")/../../tools/perftests_ci/pack_references.sh" \
            "${node_dir}" "$(basename "${node_dir}")" \
            "${images}/${target}/references" >&2
    done

    # The same report as in the pull request comment, the plots are next to it
    {
        echo "# Performance test report"
        echo
        echo "${source_link}, [all reports of the ${source_name% #*}](../README.md)"
        echo
        BUILD_NUMBER="${build_number}" report_intro
        echo
        report_new_references "${report_dir}" references
        echo
        if [[ -s "${report_dir}/info.html" ]]; then
            cat "${report_dir}/info.html"
            printf '\n\n'
        fi
        if [[ -s "${report_dir}/index.html" ]]; then
            echo "<table>${report_table_header}$(cat "${report_dir}/index.html")</table>"
            echo
            report_legend
            echo
        fi
    } > "${images}/${target}/README.md"

    # The reports of all builds of the pull request, the newest first. Builds
    # uploaded before the reports had a README.md link to their plots.
    {
        echo "# Performance test reports of ${source_name}"
        echo
        echo "${source_link}"
        echo
        for build_dir in "${images}/${source_dir}"/*/; do
            basename "${build_dir}"
        done | grep -x '[0-9][0-9]*' | sort -rn | while read -r build; do
            if [[ -e "${images}/${source_dir}/${build}/README.md" ]]; then
                echo "- [Build ${build}](${build}/README.md)"
            else
                echo "- [Build ${build}](${build}) (plots only)"
            fi
        done
    } > "${images}/${source_dir}/README.md"

    git_images -C images add "${source_dir}"
    git_images -C images commit --quiet \
        -m "Add the plots of ${source_name}, build ${build_number}"
}

add_report

# Other builds may push at the same time. Add the report again on top of their
# commits, the README.md of the pull request may have changed.
for attempt in 1 2 3; do
    perftest_log "Pushing to ${images_repo} (attempt ${attempt})"
    if git_images -C images push --quiet origin HEAD:main; then
        perftest_log "Uploaded the plots to ${target}"
        echo "${images_url}/$(git_images -C images rev-parse HEAD)/${target}"
        exit 0
    fi
    perftest_log "The push failed, adding the plots again on top of the" \
        "current repository"
    sleep "${attempt}"
    git_images -C images fetch --quiet --depth 1 origin main
    git_images -C images reset --quiet --hard FETCH_HEAD
    add_report
done

echo "Could not upload the plots to ${images_repo}" >&2
exit 1
