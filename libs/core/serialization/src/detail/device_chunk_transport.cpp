//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/serialization/detail/device_chunk_transport.hpp>

#include <atomic>
#include <cstddef>
#include <limits>

namespace hpx::serialization::detail {

    namespace {

        std::atomic<std::size_t>& host_threshold() noexcept
        {
            static std::atomic<std::size_t> threshold{
                (std::numeric_limits<std::size_t>::max)()};
            return threshold;
        }
    }    // namespace

    std::size_t host_transport_threshold() noexcept
    {
        return host_threshold().load(std::memory_order_relaxed);
    }

    void set_host_transport_threshold(std::size_t const threshold) noexcept
    {
        host_threshold().store(threshold, std::memory_order_relaxed);
    }
}    // namespace hpx::serialization::detail

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    extra_data_id_type
    extra_data_helper<serialization::detail::device_transport>::id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }
}    // namespace hpx::util
