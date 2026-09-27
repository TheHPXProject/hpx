//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)

#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/synchronization.hpp>

#include <cstdlib>
#include <string>

#include <shmem.h>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT openshmem_environment
    {
        static bool check_openshmem_environment(
            runtime_configuration const& cfg);

        static void init(int* argc, char*** argv, runtime_configuration& cfg);

        static void finalize() noexcept;

        static bool enabled() noexcept;

        static bool has_called_init() noexcept;

        static int rank() noexcept;

        static int size() noexcept;

        static std::string get_processor_name();

        // Thread level negotiated by shmem_init_thread() (see init()); the
        // parcelport uses it to decide whether multiple threads may use it.
        static int provided_thread_level() noexcept;

        // True if the runtime provided SHMEM_THREAD_MULTIPLE, i.e. multiple
        // threads may call shmem_* concurrently.
        static bool thread_multiple() noexcept;

        using mutex_type = hpx::spinlock;
        using scoped_lock = std::unique_lock<mutex_type>;

    private:
        static mutex_type mtx_;

        static bool enabled_;
        static bool has_called_init_;
        static int provided_thread_level_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>

#endif
