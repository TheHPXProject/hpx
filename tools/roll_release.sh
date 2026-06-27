#!/usr/bin/env bash
#
# Copyright (c) 2019-2021 ETH Zurich
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)

# This script tags a release locally and creates a release on GitHub. It relies
# on the hub command line tool (https://hub.github.com/).

set -o errexit

VERSION_MAJOR=$(sed -n 's/set(HPX_VERSION_MAJOR \(.*\))/\1/p' CMakeLists.txt)
VERSION_MINOR=$(sed -n 's/set(HPX_VERSION_MINOR \(.*\))/\1/p' CMakeLists.txt)
VERSION_SUBMINOR=$(sed -n 's/set(HPX_VERSION_SUBMINOR \(.*\))/\1/p' CMakeLists.txt)
VERSION_TAG=$(sed -n 's/set(HPX_VERSION_TAG "\(.*\)")/\1/p' CMakeLists.txt)
VERSION_FULL_NOTAG=${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_SUBMINOR}
VERSION_FULL_TAG=v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_SUBMINOR}${VERSION_TAG}
VERSION_FULL_NOTAG_UNDERSCORE=${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_SUBMINOR}
VERSION_TITLE="HPX V${VERSION_FULL_NOTAG}: The C++ Standards Library for Parallelism and Concurrency"
VERSION_RELEASE_NOTES_URL="https://doc.hpx.dev/tags/${VERSION_FULL_TAG}/html/releases/whats_new_${VERSION_FULL_NOTAG_UNDERSCORE}.html"
VERSION_DESCRIPTION="[Release notes](${VERSION_RELEASE_NOTES_URL})"
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

if ! which hub > /dev/null 2>&1; then
    echo "Hub not installed on this system (see https://hub.github.com/). Exiting."
    exit 1
fi

if ! [[ "$CURRENT_BRANCH" =~ ^release-[0-9]+\.[0-9]+\.X$ ]]; then
    echo "Not on release branch (current branch is \"${CURRENT_BRANCH}\"). Not continuing to make release."
    exit 1
fi

if [ -z "${VERSION_TAG}" ]; then
    echo "You are about to tag and create a final release on GitHub."

    echo ""
    echo "Sanity checking release"

    sanity_errors=0

    whats_new_file_nosuffix="whats_new_${VERSION_FULL_NOTAG_UNDERSCORE}"
    whats_new_path="docs/sphinx/releases/${whats_new_file_nosuffix}.rst"
    printf "Checking that %s exists... " "${whats_new_path}"
    if [[ -f "${whats_new_path}" ]]; then
        echo "OK"
    else
        echo "Missing"
        sanity_errors=$((sanity_errors+1))
    fi


    printf "Checking that %s.rst is included in the docs/sphinx/releases.rst table of contents... " "${whats_new_file_nosuffix}"
    if [[ $(grep "${whats_new_file_nosuffix}" docs/sphinx/releases.rst) ]]; then
        echo "OK"
    else
        echo "Missing"
        sanity_errors=$((sanity_errors+1))
    fi

    if [[ ${sanity_errors} -gt 0 ]]; then
        echo "Found ${sanity_errors} error(s). Fix it/them and try again."
        exit 1
    fi
else
    echo "You are about to tag and create a pre-release on GitHub."
    echo "If you intended to make a final release, remove the tag in the main CMakeLists.txt first."
fi

echo ""
echo "The version is \"${VERSION_FULL_TAG}\"."
echo "The version title is:"
echo "\"${VERSION_TITLE}\"."
echo "The version description is:"
echo "\"${VERSION_DESCRIPTION}\"."
echo ""

echo "Do you want to continue?"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) break;;
        No ) exit;;
    esac
done

if [ -z "${VERSION_TAG}" ]; then
    PRERELEASE_FLAG=""
else
    PRERELEASE_FLAG="--prerelease"
fi

echo ""
echo "Setting the signing key for signing the release. It is up to you to change it back to your own afterwards."
git config user.signingkey E18AE35E86BB194F 
git config user.email contact@stellar-group.org
git config user.name "STE||AR Group"

echo ""
echo "Tagging release."
git tag --sign --annotate "${VERSION_FULL_TAG}" --message="${VERSION_TITLE}"
git push origin "${VERSION_FULL_TAG}"

echo ""
echo "Creating release."
hub release create \
    ${PRERELEASE_FLAG} \
    --message "${VERSION_TITLE}" \
    --message "${VERSION_DESCRIPTION}" \
    "${VERSION_FULL_TAG}"

echo ""
echo "Now add the above URL to the downloads pages on hpx.dev."
