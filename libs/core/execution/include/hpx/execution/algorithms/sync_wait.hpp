//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//  Copyright (c) 2022 Chuanqiu He
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/execution/algorithms/detail/sync_wait_domain.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>

#include <utility>

namespace hpx::this_thread::experimental {

    // HPX-aware sync_wait CPO.
    //
    // When called from an HPX worker thread, dispatch through
    // `hpx::synchronization::detail::sync_wait_domain`, which uses cooperative
    // HPX waiting (hpx::spinlock + hpx::condition_variable_any) instead of
    // stdexec's default run_loop. The default run_loop OS-blocks the calling
    // worker thread (futex_wait), which can deadlock at low worker-thread
    // counts when the sender chain depends on other HPX work (e.g. an
    // `hpx::async` task that has not yet been picked up, or a user-managed
    // `stdexec::run_loop` driven by another HPX thread).
    //
    // When called from a non-HPX (OS) thread, fall back to stdexec's default
    // sync_wait, which is correct in that context.
    HPX_CXX_CORE_EXPORT inline constexpr struct sync_wait_t
    {
        template <hpx::execution::experimental::sender Sender>
        constexpr auto operator()(Sender&& sndr) const
        {
            if (hpx::threads::get_self_ptr() != nullptr)
            {
                return hpx::execution::experimental::detail::sync_wait_domain{}
                    .apply_sender(hpx::execution::experimental::sync_wait_t{},
                        HPX_FORWARD(Sender, sndr));
            }
            return hpx::execution::experimental::sync_wait(
                HPX_FORWARD(Sender, sndr));
        }
    } sync_wait{};

    // HPX-aware sync_wait_with_variant.
    //
    // Prevents OS-level blocking when waiting on a sender from an HPX worker
    // thread. Uses HPX-friendly waiting instead and also works for senders
    HPX_CXX_CORE_EXPORT inline constexpr struct sync_wait_with_variant_t
    {
        template <hpx::execution::experimental::sender Sender>
        constexpr auto operator()(Sender&& sndr) const
        {
            if (hpx::threads::get_self_ptr() != nullptr)
            {
                return hpx::execution::experimental::detail::sync_wait_domain{}
                    .apply_sender(hpx::execution::experimental::
                                      sync_wait_with_variant_t{},
                        HPX_FORWARD(Sender, sndr));
            }
            return hpx::execution::experimental::sync_wait_with_variant(
                HPX_FORWARD(Sender, sndr));
        }
    } sync_wait_with_variant{};
}    // namespace hpx::this_thread::experimental
