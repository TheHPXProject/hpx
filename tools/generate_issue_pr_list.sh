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
# GitHub CLI (https://cli.github.com/), git, jq, and sed. Run from the repo root:
#   tools/generate_issue_pr_list.sh BASE_REF [RELEASE_REF] [--limit]
# BASE_REF is the previous release/maintenance revision; RELEASE_REF defaults
# to HEAD. Pin both revisions when regenerating an audited release inventory.
# --limit prints only the newest 50 entries in each reconciled list.
# Set GH_REPO (e.g. GH_REPO=TheHPXProject/hpx) when working from a fork.
#
# Milestone issues describe closure, not delivery. PRs are selected by merged
# status AND local ancestry, including PRs found outside the milestone. Fetch
# the complete comparison history first. Cherry-picks/rebases whose GitHub
# merge commit is absent require a separate, manual patch-provenance check.
set -e -o pipefail

LIMIT=2147483647
REFS=()
for ARG in "$@"; do
    if [[ "$ARG" = "--limit" ]]; then
        LIMIT=50
    elif [[ "$ARG" = -* ]]; then
        echo "Unknown option: $ARG" >&2
        exit 1
    else
        REFS+=("$ARG")
    fi
done
if (( ${#REFS[@]} < 1 || ${#REFS[@]} > 2 )); then
    echo "Usage: $0 BASE_REF [RELEASE_REF] [--limit]" >&2
    exit 1
fi
BASE=$(git rev-parse --verify "${REFS[0]}^{commit}")
RELEASE=$(git rev-parse --verify "${REFS[1]:-HEAD}^{commit}")
if [[ $(git rev-parse --is-shallow-repository) = true ]]; then
    echo "Fetch full history before reconciling release notes." >&2
    exit 1
fi

VERSION_MAJOR=$(git show "${RELEASE}:CMakeLists.txt" |
    sed -n 's/set(HPX_VERSION_MAJOR \(.*\))/\1/p')
VERSION_MINOR=$(git show "${RELEASE}:CMakeLists.txt" |
    sed -n 's/set(HPX_VERSION_MINOR \(.*\))/\1/p')
VERSION_SUBMINOR=$(git show "${RELEASE}:CMakeLists.txt" |
    sed -n 's/set(HPX_VERSION_SUBMINOR \(.*\))/\1/p')
VERSION_FULL_NOTAG=${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_SUBMINOR}

for TOOL in gh jq; do
    if ! command -v "$TOOL" > /dev/null 2>&1; then
        echo "Required tool not installed: $TOOL" >&2
        exit 1
    fi
done
WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT

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

# GitHub titles are not plain ASCII: they carry typographic punctuation and the
# occasional emoji, while the release notes are kept ASCII. Map what has an
# ASCII equivalent and drop the rest.
#
# Separately, an underscore that ends a word makes reStructuredText treat that
# word as a hyperlink reference, so a title such as "The test
# partitioned_vector_ ..." renders as a broken link. reStructuredText
# recognises the reference when the underscore is followed by whitespace, by
# one of its closing punctuation characters ("foo_." and "foo_," among them) or
# by the end of the title, so escape it in exactly those positions and leave
# underscores inside words be.
# shellcheck disable=SC2016
JQ_DEFS='def ascii_clean: gsub("\u2026"; "...") | gsub("[\u2018\u2019]"; "'"'"'")
    | gsub("[\u201c\u201d]"; "\"") | gsub("[\u2013\u2014]"; "-")
    | gsub("[^\\x00-\\x7f]"; "") | gsub("\\s{2,}"; " ")
    | sub("^\\s+"; "") | sub("\\s+$"; "");
def rst_escape: gsub("_(?<t>[\\s\\-.,:;!?/)\\]}>])"; "\\_\(.t)") | gsub("_$"; "\\_");'

# Query complete candidate lists even with --limit: applying the limit before
# ancestry filtering can hide eligible PRs. Fail at the search cap rather than
# silently emit a truncated inventory.
gh issue list --state closed --milestone "${VERSION_FULL_NOTAG}" \
    --limit 1000 --json number,title > "$WORK_DIR/issues.json"
gh pr list --state merged --search "milestone:\"${VERSION_FULL_NOTAG}\"" \
    --limit 1000 --json number,title,mergeCommit > "$WORK_DIR/milestone.json"
for LIST in issues milestone; do
    if [[ $(jq length "$WORK_DIR/$LIST.json") -ge 1000 ]]; then
        echo "The $LIST query reached its cap; partition the query first." >&2
        exit 1
    fi
done
cp "$WORK_DIR/milestone.json" "$WORK_DIR/candidates.json"
jq -r '.[].number' "$WORK_DIR/milestone.json" > "$WORK_DIR/known"

# Include nested merges and squash subjects. A subject is only a candidate:
# the commits API identifies PRs in this repository, avoiding PR numbers from
# contributors' fork histories and issue references in ordinary subjects.
git log --format='%H %s' "$BASE..$RELEASE" | sed -n \
    -e 's/^\([0-9a-f]*\) Merge pull request #\([0-9]*\) .*/\1 \2/p' \
    -e 's/^\([0-9a-f]*\) .* (#\([0-9]*\))$/\1 \2/p' \
    > "$WORK_DIR/history"
while read -r COMMIT NUMBER; do
    if ! grep -qxF "$NUMBER" "$WORK_DIR/known"; then
        gh api --paginate "repos/{owner}/{repo}/commits/$COMMIT/pulls" \
            --jq '[.[] | select(.merged_at != null) |
                {number, title, mergeCommit: {oid: .merge_commit_sha}}]' \
            >> "$WORK_DIR/candidates.json"
    fi
done < "$WORK_DIR/history"
jq -s 'add | unique_by(.number)' "$WORK_DIR/candidates.json" \
    > "$WORK_DIR/unique.json"
jq -r '.[] | [.number, .mergeCommit.oid] | @tsv' "$WORK_DIR/unique.json" \
    > "$WORK_DIR/commits"
: > "$WORK_DIR/included"
while read -r NUMBER COMMIT; do
    if ! git cat-file -e "$COMMIT^{commit}" 2>/dev/null; then
        echo "PR #$NUMBER: merge commit unavailable locally; omitted." >&2
        continue
    fi
    if git merge-base --is-ancestor "$COMMIT" "$RELEASE"; then
        if git merge-base --is-ancestor "$COMMIT" "$BASE"; then
            continue
        else
            STATUS=$?
            if [[ $STATUS -ne 1 ]]; then exit "$STATUS"; fi
        fi
        echo "$NUMBER" >> "$WORK_DIR/included"
    else
        STATUS=$?
        if [[ $STATUS -ne 1 ]]; then exit "$STATUS"; fi
        echo "PR #$NUMBER: merge commit outside release; omitted." >&2
    fi
done < "$WORK_DIR/commits"

# Buffer both lists so a failed API query never produces a partial inventory.
echo "Closed issues"
echo "============="
echo ""
# shellcheck disable=SC2016
jq -r --argjson limit "$LIMIT" "${JQ_DEFS}"'
    sort_by(.number) | reverse | .[:$limit][] |
    "* :hpx-issue:`\(.number)` - \(.title | ascii_clean | rst_escape)"' \
    "$WORK_DIR/issues.json"
echo ""
echo "Merged pull requests"
echo "===================="
echo ""
# shellcheck disable=SC2016
jq -r --argjson limit "$LIMIT" --slurpfile included "$WORK_DIR/included" \
    "${JQ_DEFS}"'
    map(select(.number as $n | $included | index($n))) |
    sort_by(.number) | reverse | .[:$limit][] |
    "* :hpx-pr:`\(.number)` - \(.title | ascii_clean | rst_escape)"' \
    "$WORK_DIR/unique.json"
echo "Check cherry-picked/rebased PRs separately before publishing." >&2
