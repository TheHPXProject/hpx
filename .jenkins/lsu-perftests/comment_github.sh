#!/bin/bash -l

# Copyright (c) 2021 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Never trace the token passed in the authorization header.
set -eu

source "$(dirname "$0")/report_common.sh"

# Escape double quotes and slashes for JSON compatibility
json_escape() {
    sed -e 's/"/\\"/g' -e 's/\//\\\//g'
}

pushd "${configuration_name}-reports/reference-comparison"

# The plots are relative to the report, point them to the copies that
# upload_images.sh uploaded (PERFTEST_IMAGES_URL), or leave them out if the
# upload failed
if [[ -n "${PERFTEST_IMAGES_URL:-}" ]]; then
    images="s|<img src=\"|<img src=\"${PERFTEST_IMAGES_URL}/|g"
else
    images='s|<tr><td colspan="4"><details><summary>Plot</summary><img [^>]*></details></td></tr>||g'
fi

# In the order of replacement rule in sed:
# - Remove the image as does not display in github comments (section Details in the report)
# - Point the plots to the uploaded copies, or remove them
report=$(cat index.html | \
    sed -e 's:<section class="grid-section"><h2>Details[-a-z0-9<>/"=\ \.]*</section>::Ig' \
        -e "${images}" | json_escape)
header=$(printf '%s' "${report_table_header}" | json_escape)
legend=$(report_legend | json_escape)
intro=$(report_intro | json_escape)
new_references=$(report_new_references . \
    "${PERFTEST_IMAGES_URL:+${PERFTEST_IMAGES_URL}/references}" | json_escape)
info=""
if [[ -s info.html ]]; then
    info=$(json_escape < info.html)
fi

curl \
  -X POST \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  "https://api.github.com/repos/TheHPXProject/hpx/issues/${ghprbPullId}/comments" \
  -d "{\"body\": \"${intro}${new_references}<details><summary>Performance test report<\/summary>${info}<table>${header}${report}<\/table>${legend}<\/details>\"}"

popd
