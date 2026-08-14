//  Copyright (c) 2016 Bibek Wagle
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/timing.hpp>

#include <memory>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    namespace detail {

        class pool_timer;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT pool_timer
    {
    public:
        pool_timer();

        pool_timer(pool_timer const&) = delete;
        pool_timer(pool_timer&&) = delete;
        pool_timer& operator=(pool_timer const&) = delete;
        pool_timer& operator=(pool_timer&&) = delete;

        pool_timer(hpx::function<bool()> const& f,
            hpx::function<void()> const& on_term,
            std::string const& description = "", bool pre_shutdown = true);

        ~pool_timer();

        // late initialization (after default constructor use)
        bool init(hpx::function<bool()> const& f,
            hpx::function<void()> const& on_term,
            std::string const& description = "", bool pre_shutdown = true);

        bool start(hpx::chrono::steady_duration const& time_duration,
            bool evaluate = false) const;
        bool stop() const;

        [[nodiscard]] bool is_started() const;
        [[nodiscard]] bool is_terminated() const;

        [[nodiscard]] bool is_valid() const;

    private:
        std::shared_ptr<detail::pool_timer> timer_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
