//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <cstdint>

namespace hpx::threads::detail {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
    HPX_CXX_CORE_EXPORT struct scheduling_counters
    {
        scheduling_counters(std::int64_t& executed_threads,
            std::int64_t& executed_thread_phases, std::int64_t& tfunc_time,
            std::int64_t& exec_time, std::int64_t& idle_loop_count,
            std::int64_t& busy_loop_count, bool& is_active,
            std::int64_t& background_work_duration,
            std::int64_t& background_send_duration,
            std::int64_t& background_receive_duration) noexcept
          : executed_threads_(executed_threads)
          , executed_thread_phases_(executed_thread_phases)
          , tfunc_time_(tfunc_time)
          , exec_time_(exec_time)
          , idle_loop_count_(idle_loop_count)
          , busy_loop_count_(busy_loop_count)
          , background_work_duration_(background_work_duration)
          , background_send_duration_(background_send_duration)
          , background_receive_duration_(background_receive_duration)
          , is_active_(is_active)
        {
        }

        std::int64_t& executed_threads_;
        std::int64_t& executed_thread_phases_;
        std::int64_t& tfunc_time_;
        std::int64_t& exec_time_;
        std::int64_t& idle_loop_count_;
        std::int64_t& busy_loop_count_;
        std::int64_t& background_work_duration_;
        std::int64_t& background_send_duration_;
        std::int64_t& background_receive_duration_;
        bool& is_active_;
    };
#else
    HPX_CXX_CORE_EXPORT struct scheduling_counters
    {
        scheduling_counters(std::int64_t& executed_threads,
            std::int64_t& executed_thread_phases, std::int64_t& tfunc_time,
            std::int64_t& exec_time, std::int64_t& idle_loop_count,
            std::int64_t& busy_loop_count, bool& is_active) noexcept
          : executed_threads_(executed_threads)
          , executed_thread_phases_(executed_thread_phases)
          , tfunc_time_(tfunc_time)
          , exec_time_(exec_time)
          , idle_loop_count_(idle_loop_count)
          , busy_loop_count_(busy_loop_count)
          , is_active_(is_active)
        {
        }

        std::int64_t& executed_threads_;
        std::int64_t& executed_thread_phases_;
        std::int64_t& tfunc_time_;
        std::int64_t& exec_time_;
        std::int64_t& idle_loop_count_;
        std::int64_t& busy_loop_count_;
        bool& is_active_;
    };
#endif    // HPX_HAVE_BACKGROUND_THREAD_COUNTERS
}    // namespace hpx::threads::detail
