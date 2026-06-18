//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>
#include <utility>

namespace hpx::parallel::execution::detail {

    // Detection concepts for executor member functions.
    // These enable the CPO bridges in execution.hpp to forward
    // to member functions instead of requiring friend tag_invoke.

    template <typename Executor, typename F, typename... Ts>
    concept has_sync_execute_member =
        requires(Executor&& exec, F&& f, Ts&&... ts) {
            HPX_FORWARD(Executor, exec)
                .sync_execute(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        };

    template <typename Executor, typename F, typename... Ts>
    concept has_async_execute_member =
        requires(Executor&& exec, F&& f, Ts&&... ts) {
            HPX_FORWARD(Executor, exec)
                .async_execute(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        };

    template <typename Executor, typename F, typename Future, typename... Ts>
    concept has_then_execute_member =
        requires(Executor&& exec, F&& f, Future&& pred, Ts&&... ts) {
            HPX_FORWARD(Executor, exec)
                .then_execute(HPX_FORWARD(F, f), HPX_FORWARD(Future, pred),
                    HPX_FORWARD(Ts, ts)...);
        };

    template <typename Executor, typename F, typename... Ts>
    concept has_post_member = requires(Executor&& exec, F&& f, Ts&&... ts) {
        HPX_FORWARD(Executor, exec)
            .post(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    };

    template <typename Executor, typename F, typename Shape, typename... Ts>
    concept has_bulk_sync_execute_member =
        requires(Executor&& exec, F&& f, Shape const& shape, Ts&&... ts) {
            HPX_FORWARD(Executor, exec)
                .bulk_sync_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        };

    template <typename Executor, typename F, typename Shape, typename... Ts>
    concept has_bulk_async_execute_member =
        requires(Executor&& exec, F&& f, Shape const& shape, Ts&&... ts) {
            HPX_FORWARD(Executor, exec)
                .bulk_async_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        };

    template <typename Executor, typename F, typename Shape, typename Future,
        typename... Ts>
    concept has_bulk_then_execute_member = requires(
        Executor&& exec, F&& f, Shape const& shape, Future&& pred, Ts&&... ts) {
        HPX_FORWARD(Executor, exec)
            .bulk_then_execute(HPX_FORWARD(F, f), shape,
                HPX_FORWARD(Future, pred), HPX_FORWARD(Ts, ts)...);
    };

    template <typename Executor>
    concept has_to_non_par_member =
        requires(Executor const& exec) { exec.to_non_par(); };

    template <typename Executor, typename F, typename... Fs>
    concept has_async_invoke_member =
        requires(Executor&& exec, F&& f, Fs&&... fs) {
            HPX_FORWARD(Executor, exec)
                .async_invoke(HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
        };

    template <typename Executor, typename F, typename... Fs>
    concept has_sync_invoke_member =
        requires(Executor&& exec, F&& f, Fs&&... fs) {
            HPX_FORWARD(Executor, exec)
                .sync_invoke(HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
        };

}    // namespace hpx::parallel::execution::detail
