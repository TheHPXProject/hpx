//  Copyright (c) 2026 Apoorv Shah
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/modules/timing.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>

namespace hpx::agas {

    static std::atomic<std::uint64_t> agas_rpc_timeout_ms{5000};

    void set_rpc_timeout(hpx::chrono::steady_duration const& timeout) noexcept
    {
        if (timeout.value() >= timeout.value().zero())
        {
            auto const timeout_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeout.value());

            agas_rpc_timeout_ms.store(
                static_cast<std::uint64_t>(timeout_ms.count()),
                std::memory_order_relaxed);
        }
    }

    hpx::chrono::steady_duration get_rpc_timeout() noexcept
    {
        auto const ms = agas_rpc_timeout_ms.load(std::memory_order_relaxed);
        return hpx::chrono::steady_duration(std::chrono::milliseconds(ms));
    }
}    // namespace hpx::agas
