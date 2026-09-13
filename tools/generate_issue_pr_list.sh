#!/usr/bin/env bash
#
# Copyright (c) 2019 Mikael Simberg
# Copyright (c) 2026 Abhishek Kumar
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)

# This script generates issue and PR lists for the current release. The output
# is meant to be used in the release notes for each release. It relies on the
# GitHub CLI (https://cli.github.com/) and sed.
# To only list the last ones in order to just update the list in the release
# notes, specify --limit as an argument of this script, this makes the request
# quicker.
# The repository is the one inferred from the current checkout; set GH_REPO
# (for example GH_REPO=STEllAR-GROUP/hpx) to override that when working from a
# fork.

set -o pipefail

VERSION_MAJOR=$(sed -n 's/set(HPX_VERSION_MAJOR \(.*\))/\1/p' CMakeLists.txt)
VERSION_MINOR=$(sed -n 's/set(HPX_VERSION_MINOR \(.*\))/\1/p' CMakeLists.txt)
VERSION_SUBMINOR=$(sed -n 's/set(HPX_VERSION_SUBMINOR \(.*\))/\1/p' CMakeLists.txt)
VERSION_FULL_NOTAG=$VERSION_MAJOR.$VERSION_MINOR.$VERSION_SUBMINOR

if ! command -v gh > /dev/null 2>&1; then
    echo "The GitHub CLI (gh) is not installed on this system. Exiting.." >&2
    exit 1
fi

# Unlike the milestone lookup this replaces, closed milestones are considered
# as well, so the lists can still be regenerated after a release is out. The
# titles are collected first rather than piped into grep, so that grep exiting
# early cannot make gh fail with SIGPIPE under pipefail.
MILESTONES=$(gh api --paginate \
    "repos/{owner}/{repo}/milestones?state=all&per_page=100" --jq '.[].title')

if ! grep -qxF "${VERSION_FULL_NOTAG}" <<< "${MILESTONES}"; then
    echo "No milestone named '${VERSION_FULL_NOTAG}' found in this repository." >&2
    echo "Check HPX_VERSION_* in CMakeLists.txt, or set GH_REPO." >&2
    exit 1
fi

if [[ "$1" = "--limit" ]]; then
    LIMIT=50
else
    LIMIT=1000
fi

# A trailing underscore makes reStructuredText treat the preceding word as a
# hyperlink reference, so titles such as "The test partitioned_vector_ ..."
# would render as a broken link. Escape those underscores.
# shellcheck disable=SC2016
RST_ESCAPE='def rst_escape: gsub("_(?<t>\\s)"; "\\_\(.t)") | gsub("_$"; "\\_");'

echo "Closed issues"
echo "============="
echo ""

# \(...) below is jq interpolation, not shell expansion
# shellcheck disable=SC2016
gh issue list --state closed --milestone "${VERSION_FULL_NOTAG}" \
    --limit "${LIMIT}" --json number,title \
    --jq "${RST_ESCAPE}"'.[] | "* :hpx-issue:`\(.number)` - \(.title | rst_escape)"'

echo ""
echo "Closed pull requests"
echo "===================="
echo ""

# gh pr list has no --milestone option, but the milestone can be given as a
# search qualifier instead.
# shellcheck disable=SC2016
gh pr list --search "milestone:\"${VERSION_FULL_NOTAG}\" is:closed" \
    --limit "${LIMIT}" --json number,title \
    --jq "${RST_ESCAPE}"'.[] | "* :hpx-pr:`\(.number)` - \(.title | rst_escape)"'
