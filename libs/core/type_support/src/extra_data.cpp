//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/type_support/extra_data.hpp>

#include <atomic>

namespace hpx::util::detail {

    namespace {

        std::atomic<extra_data_id_type> id_count(0);
    }    // namespace

    extra_data_id_type next_extra_data_id() noexcept
    {
        return id_count.fetch_add(1, std::memory_order_relaxed);
    }

    extra_data_id_type extra_data_id_count() noexcept
    {
        return id_count.load(std::memory_order_relaxed);
    }
}    // namespace hpx::util::detail
