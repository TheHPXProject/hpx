#  Copyright (c) 2026 Priyanshi Sharma
#
#  SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#!/usr/bin/env python3
"""Compile-time benchmark: HPX macro actions vs reflect_action.

Generates N action definitions using both the old macro approach and
the new C++26 reflection approach, measures compile time of each,
and reports the comparison.

Usage: python3 compile_time_benchmark.py [N1 N2 N3 ...]
Default N values: 10 50 100
"""
import subprocess
import sys
import time
import os
import tempfile

HPX_INCLUDE = os.environ.get("HPX_INCLUDE", "/hpx/build_repro")
HPX_SRC     = os.environ.get("HPX_SRC",     "/hpx")
CXX         = os.environ.get("CXX",         "g++")
# Old macro-style approach: reflection enabled (required by HPX headers)
# but using manual struct expansion instead of reflect_action
CXXFLAGS_MACRO = (
    "-freflection -std=c++26"
    " -DHPX_HAVE_CXX26_REFLECTION"
    " -DHPX_MODULE_STATIC_LINKING"
    " -DHPX_DEBUG"
)
# New reflect_action approach: C++26 with reflection
CXXFLAGS_REFLECT = (
    "-freflection -std=c++26"
    " -DHPX_HAVE_CXX26_REFLECTION"
    " -DHPX_MODULE_STATIC_LINKING"
    " -DHPX_DEBUG"
)
CXXFLAGS = CXXFLAGS_REFLECT  # default, overridden per call


def include_flags():
    dirs = [
        "/hpx",
        "/hpx/build_repro",
        "/hpx/build_repro/libs/core/affinity/include",
        "/hpx/build_repro/libs/core/algorithms/include",
        "/hpx/build_repro/libs/core/allocator_support/include",
        "/hpx/build_repro/libs/core/asio/include",
        "/hpx/build_repro/libs/core/assertion/include",
        "/hpx/build_repro/libs/core/async_base/include",
        "/hpx/build_repro/libs/core/async_combinators/include",
        "/hpx/build_repro/libs/core/async_local/include",
        "/hpx/build_repro/libs/core/batch_environments/include",
        "/hpx/build_repro/libs/core/cache/include",
        "/hpx/build_repro/libs/core/checkpoint_base/include",
        "/hpx/build_repro/libs/core/command_line_handling_local/include",
        "/hpx/build_repro/libs/core/compute_local/include",
        "/hpx/build_repro/libs/core/concepts/include",
        "/hpx/build_repro/libs/core/concurrency/include",
        "/hpx/build_repro/libs/core/config/include",
        "/hpx/build_repro/libs/core/config_registry/include",
        "/hpx/build_repro/libs/core/contracts/include",
        "/hpx/build_repro/libs/core/coroutines/include",
        "/hpx/build_repro/libs/core/datastructures/include",
        "/hpx/build_repro/libs/core/debugging/include",
        "/hpx/build_repro/libs/core/errors/include",
        "/hpx/build_repro/libs/core/execution/include",
        "/hpx/build_repro/libs/core/execution_base/include",
        "/hpx/build_repro/libs/core/executors/include",
        "/hpx/build_repro/libs/core/filesystem/include",
        "/hpx/build_repro/libs/core/format/include",
        "/hpx/build_repro/libs/core/functional/include",
        "/hpx/build_repro/libs/core/futures/include",
        "/hpx/build_repro/libs/core/hardware/include",
        "/hpx/build_repro/libs/core/hashing/include",
        "/hpx/build_repro/libs/core/include_local/include",
        "/hpx/build_repro/libs/core/ini/include",
        "/hpx/build_repro/libs/core/init_runtime_local/include",
        "/hpx/build_repro/libs/core/io_service/include",
        "/hpx/build_repro/libs/core/iostream/include",
        "/hpx/build_repro/libs/core/iterator_support/include",
        "/hpx/build_repro/libs/core/itt_notify/include",
        "/hpx/build_repro/libs/core/lcos_local/include",
        "/hpx/build_repro/libs/core/lock_registration/include",
        "/hpx/build_repro/libs/core/logging/include",
        "/hpx/build_repro/libs/core/memory/include",
        "/hpx/build_repro/libs/core/pack_traversal/include",
        "/hpx/build_repro/libs/core/plugin/include",
        "/hpx/build_repro/libs/core/prefix/include",
        "/hpx/build_repro/libs/core/preprocessor/include",
        "/hpx/build_repro/libs/core/program_options/include",
        "/hpx/build_repro/libs/core/properties/include",
        "/hpx/build_repro/libs/core/resiliency/include",
        "/hpx/build_repro/libs/core/resource_partitioner/include",
        "/hpx/build_repro/libs/core/runtime_configuration/include",
        "/hpx/build_repro/libs/core/runtime_local/include",
        "/hpx/build_repro/libs/core/schedulers/include",
        "/hpx/build_repro/libs/core/serialization/include",
        "/hpx/build_repro/libs/core/static_reinit/include",
        "/hpx/build_repro/libs/core/string_util/include",
        "/hpx/build_repro/libs/core/synchronization/include",
        "/hpx/build_repro/libs/core/tag_invoke/include",
        "/hpx/build_repro/libs/core/testing/include",
        "/hpx/build_repro/libs/core/thread_pool_util/include",
        "/hpx/build_repro/libs/core/thread_pools/include",
        "/hpx/build_repro/libs/core/thread_support/include",
        "/hpx/build_repro/libs/core/threading/include",
        "/hpx/build_repro/libs/core/threading_base/include",
        "/hpx/build_repro/libs/core/threadmanager/include",
        "/hpx/build_repro/libs/core/timed_execution/include",
        "/hpx/build_repro/libs/core/timing/include",
        "/hpx/build_repro/libs/core/topology/include",
        "/hpx/build_repro/libs/core/tracing/include",
        "/hpx/build_repro/libs/core/type_support/include",
        "/hpx/build_repro/libs/core/util/include",
        "/hpx/build_repro/libs/core/version/include",
        "/hpx/build_repro/libs/full/actions_base/include",
        "/hpx/build_repro/libs/full/components_base/include",
        "/hpx/build_repro/libs/full/naming_base/include",
        "/hpx/build_repro/libs/full/parcelset_base/include",
        "/hpx/libs/core/affinity/include",
        "/hpx/libs/core/algorithms/include",
        "/hpx/libs/core/allocator_support/include",
        "/hpx/libs/core/asio/include",
        "/hpx/libs/core/assertion/include",
        "/hpx/libs/core/async_base/include",
        "/hpx/libs/core/async_combinators/include",
        "/hpx/libs/core/async_local/include",
        "/hpx/libs/core/batch_environments/include",
        "/hpx/libs/core/cache/include",
        "/hpx/libs/core/checkpoint_base/include",
        "/hpx/libs/core/command_line_handling_local/include",
        "/hpx/libs/core/compute_local/include",
        "/hpx/libs/core/concepts/include",
        "/hpx/libs/core/concurrency/include",
        "/hpx/libs/core/config/include",
        "/hpx/libs/core/config_registry/include",
        "/hpx/libs/core/contracts/include",
        "/hpx/libs/core/coroutines/include",
        "/hpx/libs/core/datastructures/include",
        "/hpx/libs/core/debugging/include",
        "/hpx/libs/core/errors/include",
        "/hpx/libs/core/execution/include",
        "/hpx/libs/core/execution_base/include",
        "/hpx/libs/core/executors/include",
        "/hpx/libs/core/filesystem/include",
        "/hpx/libs/core/format/include",
        "/hpx/libs/core/functional/include",
        "/hpx/libs/core/futures/include",
        "/hpx/libs/core/hardware/include",
        "/hpx/libs/core/hashing/include",
        "/hpx/libs/core/include_local/include",
        "/hpx/libs/core/ini/include",
        "/hpx/libs/core/init_runtime_local/include",
        "/hpx/libs/core/io_service/include",
        "/hpx/libs/core/iostream/include",
        "/hpx/libs/core/iterator_support/include",
        "/hpx/libs/core/itt_notify/include",
        "/hpx/libs/core/lcos_local/include",
        "/hpx/libs/core/lock_registration/include",
        "/hpx/libs/core/logging/include",
        "/hpx/libs/core/memory/include",
        "/hpx/libs/core/pack_traversal/include",
        "/hpx/libs/core/plugin/include",
        "/hpx/libs/core/prefix/include",
        "/hpx/libs/core/preprocessor/include",
        "/hpx/libs/core/program_options/include",
        "/hpx/libs/core/properties/include",
        "/hpx/libs/core/resiliency/include",
        "/hpx/libs/core/resource_partitioner/include",
        "/hpx/libs/core/runtime_configuration/include",
        "/hpx/libs/core/runtime_local/include",
        "/hpx/libs/core/schedulers/include",
        "/hpx/libs/core/serialization/include",
        "/hpx/libs/core/static_reinit/include",
        "/hpx/libs/core/string_util/include",
        "/hpx/libs/core/synchronization/include",
        "/hpx/libs/core/tag_invoke/include",
        "/hpx/libs/core/testing/include",
        "/hpx/libs/core/thread_pool_util/include",
        "/hpx/libs/core/thread_pools/include",
        "/hpx/libs/core/thread_support/include",
        "/hpx/libs/core/threading/include",
        "/hpx/libs/core/threading_base/include",
        "/hpx/libs/core/threadmanager/include",
        "/hpx/libs/core/timed_execution/include",
        "/hpx/libs/core/timing/include",
        "/hpx/libs/core/topology/include",
        "/hpx/libs/core/tracing/include",
        "/hpx/libs/core/type_support/include",
        "/hpx/libs/core/util/include",
        "/hpx/libs/core/version/include",
        "/hpx/libs/full/actions_base/include",
        "/hpx/libs/full/components_base/include",
        "/hpx/libs/full/naming_base/include",
        "/hpx/libs/full/parcelset_base/include",
    ]
    isystem = [
        "/hpx/build_repro/_deps/stdexec-src/include",
        "/hpx/build_repro/_deps/asio-src/asio/include",
    ]
    return (
        " ".join(f"-I{d}" for d in dirs)
        + " "
        + " ".join(f"-isystem {d}" for d in isystem)
    )


def gen_macro_file(n):
    """Old-style: manual struct action (equivalent to HPX_PLAIN_ACTION expansion)."""
    lines = [
        "#include <hpx/config.hpp>",
        "#include <hpx/actions_base/plain_action.hpp>",
        "#include <cstdint>",
        "",
    ]
    for i in range(n):
        lines.append(
            f"std::int32_t action_func_{i}(std::int32_t x) {{ return x + {i}; }}"
        )
    lines.append("")
    for i in range(n):
        # Manual struct equivalent of HPX_PLAIN_ACTION without reflection path
        lines.append(
            f"struct action_func_{i}_action"
            f" : hpx::actions::make_action_t<"
            f"decltype(&action_func_{i}), &action_func_{i},"
            f" action_func_{i}_action> {{}};"
        )
    lines.append("")
    return "\n".join(lines)


def gen_reflect_file(n):
    lines = [
        "#include <hpx/config.hpp>",
        "#include <hpx/actions_base/reflect_action.hpp>",
        "#include <cstdint>",
        "",
    ]
    for i in range(n):
        lines.append(
            f"std::int32_t action_func_{i}(std::int32_t x) {{ return x + {i}; }}"
        )
    lines.append("")
    for i in range(n):
        lines.append(
            f"using action_func_{i}_action ="
            f" hpx::actions::reflect_action<^^action_func_{i}>;"
        )
    lines.append("")
    return "\n".join(lines)


def measure_compile(source, label, runs=3, cxxflags=None):
    with tempfile.NamedTemporaryFile(
        suffix=".cpp", mode="w", delete=False
    ) as f:
        f.write(source)
        fname = f.name
    try:
        flags = cxxflags if cxxflags is not None else CXXFLAGS
        cmd = f"{CXX} {flags} {include_flags()} -c {fname} -o /dev/null"
        times = []
        for _ in range(runs):
            start = time.perf_counter()
            result = subprocess.run(
                cmd, shell=True, capture_output=True, text=True
            )
            elapsed = time.perf_counter() - start
            if result.returncode != 0:
                print(f"  ERROR compiling {label}:")
                print(result.stderr[:3000])
                return None
            times.append(elapsed)
        return min(times)   # best of N runs
    finally:
        os.unlink(fname)


def main():
    ns = (
        [int(x) for x in sys.argv[1:]] if len(sys.argv) > 1 else [10, 50, 100]
    )
    print()
    print("Compile-time benchmark: macro actions vs reflect_action")
    print(f"Compiler: {CXX}  (best of 3 runs each)")
    print()
    print(f"{'N':>5}  {'Macro (s)':>12}  {'Reflect (s)':>12}  {'Speedup':>10}")
    print("-" * 50)
    for n in ns:
        t_macro   = measure_compile(gen_macro_file(n),   f"macro   N={n}", cxxflags=CXXFLAGS_MACRO)
        t_reflect = measure_compile(gen_reflect_file(n), f"reflect N={n}", cxxflags=CXXFLAGS_REFLECT)
        if t_macro is not None and t_reflect is not None:
            speedup = t_macro / t_reflect if t_reflect > 0 else float("inf")
            print(
                f"{n:>5}  {t_macro:>12.3f}  {t_reflect:>12.3f}"
                f"  {speedup:>9.2f}x"
            )
        else:
            print(
                f"{n:>5}  {'ERROR':>12}  {'ERROR':>12}  {'N/A':>10}"
            )
    print()


if __name__ == "__main__":
    main()
