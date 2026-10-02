#!/usr/bin/env bash
# Copyright (c) 2026 the-ivii
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Mimic Compiler Explorer: compile wrap-main smoke with a raw compiler
# invocation, reusing the archives and linker flags already discovered by
# the CMake hello_wrap target.
#
# Usage:
#   run_raw_gxx_smoke.sh --prefix <install-prefix> \
#       --cmake-build-dir <hello_wrap cmake build dir>

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PREFIX=
CMAKE_BUILD_DIR=
OUT_DIR=build/smoke

while [[ $# -gt 0 ]]; do
    case "$1" in
    --prefix)
        PREFIX=$2
        shift 2
        ;;
    --cmake-build-dir)
        CMAKE_BUILD_DIR=$2
        shift 2
        ;;
    --out-dir)
        OUT_DIR=$2
        shift 2
        ;;
    *)
        echo "unknown argument: $1" >&2
        exit 2
        ;;
    esac
done

if [[ -z "${PREFIX}" || -z "${CMAKE_BUILD_DIR}" ]]; then
    echo "error: --prefix and --cmake-build-dir are required" >&2
    exit 2
fi

mkdir -p "${OUT_DIR}"

# Compiler Explorer invokes the compiler directly. Reuse the CMake
# compiler and -std= flag: default g++ in some images is too old for
# -std=c++20 (it only accepts -std=c++2a).
#
# HPX static builds install one archive per module
# (libhpx_include_local.a, libhpx_logging.a, ...). Those objects are
# not merged into libhpx_core.a, so -lhpx_wrap -lhpx_init -lhpx
# -lhpx_core is not enough. Reuse the archives and linker flags
# CMake already used for hello_wrap (full-path .a/.so, Boost, hwloc).
COMMANDS=$(ninja -C "${CMAKE_BUILD_DIR}" -t commands hello_wrap)
CXX=$(grep '^CMAKE_CXX_COMPILER:' \
    "${CMAKE_BUILD_DIR}/CMakeCache.txt" | head -n1 | cut -d= -f2)
STDFLAG=$(printf '%s\n' "${COMMANDS}" | tr ' ' '\n' |
    grep -E '^-std=' | head -n1)
LINK_LINE=$(printf '%s\n' "${COMMANDS}" | tail -n1)
EXTRA=$(printf '%s\n' "${LINK_LINE}" | tr ' ' '\n' |
    grep -E '^-l|^-L|^-Wl,|^-pthread|\.a$|\.so(\.[0-9]+)*$' |
    grep -v 'hello_wrap' |
    tr '\n' ' ')
: "${CXX:=g++}"
: "${STDFLAG:=-std=c++20}"

# shellcheck disable=SC2086
"${CXX}" ${STDFLAG} -O2 -o "${OUT_DIR}/raw_wrap" \
    "${SCRIPT_DIR}/raw_wrap.cpp" \
    -isystem "${PREFIX}/include" \
    -DHPX_APPLICATION_EXPORTS \
    ${EXTRA}

"${OUT_DIR}/raw_wrap" --hpx:threads=2
