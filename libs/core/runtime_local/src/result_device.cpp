//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/runtime_local/result_device.hpp>

#include <atomic>

namespace hpx::detail {

    namespace {

        // constant-initialized (constexpr constructor): registrations from
        // static initializers of other translation units are safe
        std::atomic<result_device_function> result_device_f{nullptr};
    }    // namespace

    void set_result_device_function(result_device_function f) noexcept
    {
        result_device_f.store(f, std::memory_order_release);
    }

    int result_device() noexcept
    {
        result_device_function const f =
            result_device_f.load(std::memory_order_acquire);
        return f != nullptr ? f() : -1;
    }
}    // namespace hpx::detail
