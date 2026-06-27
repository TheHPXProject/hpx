//  Copyright (c) 2025 Jiakun Yan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)) ||      \
    defined(HPX_HAVE_MODULE_LCI_BASE)

#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/synchronization.hpp>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include <hpx/config/warnings_prefix.hpp>

#include "lci.hpp"
#include "lct.h"

namespace hpx::util {

    HPX_CXX_CORE_EXPORT struct HPX_EXPORT lci_environment
    {
        static bool check_lci_environment(runtime_configuration& cfg);

        static void init(int* argc, char*** argv, runtime_configuration& cfg);
        static void finalize();

        static bool enabled();

        static int rank();
        static int size();

        static std::string get_processor_name();

        static int get_max_tag();

        // progress
        static bool do_progress(::lci::device_t device);

        // log
        enum class log_level_t
        {
            none,
            profile,
            debug,
        };

        static log_level_t log_level;
#ifdef HPX_HAVE_PARCELPORT_LCI_LOG
        static LCT_log_ctx_t log_ctx;
#endif
        static void log(
            log_level_t level, char const* tag, char const* format, ...);

        // performance counter
        // clang-format off
#define HPX_LCI_PCOUNTER_NONE_FOR_EACH(_macro)

#define HPX_LCI_PCOUNTER_TREND_FOR_EACH(_macro)     \
        _macro(send_conn_start)                     \
        _macro(send_conn_end)                       \
        _macro(recv_conn_start)                     \
        _macro(recv_conn_end)

#define HPX_LCI_PCOUNTER_TIMER_FOR_EACH(_macro)     \
        _macro(send_conn_timer)                     \
        _macro(recv_conn_timer)                     \
        _macro(async_write_timer)                   \
        _macro(send_timer)                          \
        _macro(handle_parcels)                      \
        _macro(poll_comp)                           \
        _macro(useful_bg_work)
        // clang-format on

#define HPX_LCI_PCOUNTER_HANDLE_DECL(name) static LCT_pcounter_handle_t name;

        HPX_LCI_PCOUNTER_NONE_FOR_EACH(HPX_LCI_PCOUNTER_HANDLE_DECL)
        HPX_LCI_PCOUNTER_TREND_FOR_EACH(HPX_LCI_PCOUNTER_HANDLE_DECL)
        HPX_LCI_PCOUNTER_TIMER_FOR_EACH(HPX_LCI_PCOUNTER_HANDLE_DECL)

        static LCT_pcounter_ctx_t pcounter_ctx;
        static int64_t pcounter_now();
        static int64_t pcounter_since(int64_t then);
        static void pcounter_add(LCT_pcounter_handle_t handle, int64_t val);
        static void pcounter_start(LCT_pcounter_handle_t handle);
        static void pcounter_end(LCT_pcounter_handle_t handle);

    private:
        static bool enabled_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>

#else

#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT struct HPX_EXPORT lci_environment
    {
        static bool check_lci_environment(runtime_configuration& cfg);
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>

#endif
