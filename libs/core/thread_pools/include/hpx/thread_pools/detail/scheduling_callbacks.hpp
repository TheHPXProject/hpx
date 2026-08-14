//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace hpx::threads::detail {

    HPX_CXX_CORE_EXPORT struct scheduling_callbacks
    {
        using outer_callback_type = hpx::move_only_function<bool()>;
        using callback_type = hpx::move_only_function<void()>;
        using background_callback_type = hpx::move_only_function<bool()>;

        explicit scheduling_callbacks(outer_callback_type&& outer,
            callback_type&& inner = callback_type(),
            background_callback_type&& background = background_callback_type(),
            std::size_t max_background_threads =
                (std::numeric_limits<std::size_t>::max)(),
            std::size_t max_idle_loop_count = HPX_IDLE_LOOP_COUNT_MAX,
            std::size_t max_busy_loop_count = HPX_BUSY_LOOP_COUNT_MAX)
          : outer_(HPX_MOVE(outer))
          , inner_(HPX_MOVE(inner))
          , background_(HPX_MOVE(background))
          , max_background_threads_(max_background_threads)
          , max_idle_loop_count_(static_cast<std::int64_t>(max_idle_loop_count))
          , max_busy_loop_count_(static_cast<std::int64_t>(max_busy_loop_count))
        {
        }

        outer_callback_type outer_;
        callback_type inner_;
        background_callback_type background_;
        std::size_t const max_background_threads_;
        std::int64_t const max_idle_loop_count_;
        std::int64_t const max_busy_loop_count_;
    };
}    // namespace hpx::threads::detail
