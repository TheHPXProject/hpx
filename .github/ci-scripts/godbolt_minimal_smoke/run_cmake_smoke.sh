#!/usr/bin/env bash
# Copyright (c) 2026 the-ivii
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Build and run the godbolt-minimal CMake smoke tests against an installed
# HPX prefix. Usage:
#
#   run_cmake_smoke.sh --prefix <install-prefix> [options]
#
# Options:
#   --generator <name>     CMake -G value (default: Ninja)
#   --toolchain <file>     Optional CMAKE_TOOLCHAIN_FILE
#   --config <cfg>         Multi-config build type (e.g. Release on VS)
#   --copy-dlls            Copy hwloc/install DLLs next to Windows binaries
#   --build-dir <path>     Out-of-source build directory (default: build/smoke)

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SOURCE_DIR=${SCRIPT_DIR}
PREFIX=
GENERATOR=Ninja
TOOLCHAIN=
CONFIG=
COPY_DLLS=0
BUILD_DIR=build/smoke

while [[ $# -gt 0 ]]; do
    case "$1" in
    --prefix)
        PREFIX=$2
        shift 2
        ;;
    --generator)
        GENERATOR=$2
        shift 2
        ;;
    --toolchain)
        TOOLCHAIN=$2
        shift 2
        ;;
    --config)
        CONFIG=$2
        shift 2
        ;;
    --copy-dlls)
        COPY_DLLS=1
        shift
        ;;
    --build-dir)
        BUILD_DIR=$2
        shift 2
        ;;
    *)
        echo "unknown argument: $1" >&2
        exit 2
        ;;
    esac
done

if [[ -z "${PREFIX}" ]]; then
    echo "error: --prefix is required" >&2
    exit 2
fi

CMAKE_ARGS=(
    -S"${SOURCE_DIR}"
    -B"${BUILD_DIR}"
    -G"${GENERATOR}"
    "-DCMAKE_PREFIX_PATH=${PREFIX}"
)

if [[ -n "${TOOLCHAIN}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN}")
fi

if [[ -z "${CONFIG}" ]]; then
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Release)
fi

cmake "${CMAKE_ARGS[@]}"

BUILD_ARGS=(--build "${BUILD_DIR}")
if [[ -n "${CONFIG}" ]]; then
    BUILD_ARGS+=(--config "${CONFIG}" -- -maxcpucount:2 -verbosity:minimal -nologo)
fi
cmake "${BUILD_ARGS[@]}"

if [[ -n "${CONFIG}" ]]; then
    BIN_DIR=${BUILD_DIR}/${CONFIG}
    HELLO_WORLD=${BIN_DIR}/hello_world.exe
    HELLO_WRAP=${BIN_DIR}/hello_wrap.exe
else
    BIN_DIR=${BUILD_DIR}
    HELLO_WORLD=${BIN_DIR}/hello_world
    HELLO_WRAP=${BIN_DIR}/hello_wrap
fi

if [[ "${COPY_DLLS}" -eq 1 ]]; then
    # Fetched hwloc is a DLL. Windows will not start the smoke binaries
    # unless that DLL is on PATH or sits next to the .exe. Git bash
    # reports a missing DLL as exit 127.
    for dll_dir in \
        "${PREFIX}/bin" \
        "${PREFIX}/hwloc_installed/bin" \
        "$(dirname "${PREFIX}")/_deps/hwloc-src/bin" \
        "$(dirname "$(dirname "${PREFIX}")")/_deps/hwloc-src/bin"
    do
        if [[ -d "${dll_dir}" ]]; then
            cp -f "${dll_dir}"/*.dll "${BIN_DIR}/" 2>/dev/null || true
            export PATH="${dll_dir}:${PATH}"
        fi
    done
fi

"${HELLO_WORLD}"
"${HELLO_WRAP}" --hpx:threads=2
