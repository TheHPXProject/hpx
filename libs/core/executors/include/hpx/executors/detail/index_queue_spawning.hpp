//  Copyright (c) 2019-2020 ETH Zurich
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/detail/bulk_invoke_helper.hpp>
#include <hpx/executors/detail/index_queue_spawning_result.hpp>
#include <hpx/executors/detail/index_queue_spawning_void.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parallel::execution::detail {

    template <typename Launch, typename F, typename S, typename... Ts>
    decltype(auto) index_queue_bulk_sync_execute(
        hpx::threads::thread_description const& desc,
        threads::thread_pool_base* pool, std::size_t first_thread,
        std::size_t num_threads, Launch policy, F&& f, S const& shape,
        hpx::threads::mask_cref_type mask, Ts&&... ts)
    {
        using result_type = detail::bulk_function_result_t<F, S, Ts...>;
        if constexpr (!std::is_void_v<result_type>)
        {
            return index_queue_bulk_sync_execute_result(desc, pool,
                first_thread, num_threads, policy, HPX_FORWARD(F, f), shape,
                mask, HPX_FORWARD(Ts, ts)...);
        }
        else
        {
            return index_queue_bulk_sync_execute_void(desc, pool, first_thread,
                num_threads, policy, HPX_FORWARD(F, f), shape, mask,
                HPX_FORWARD(Ts, ts)...);
        }
    }

    template <typename Launch, typename F, typename S, typename... Ts>
    decltype(auto) index_queue_bulk_sync_execute(
        threads::thread_pool_base* pool, std::size_t first_thread,
        std::size_t num_threads, Launch policy, F&& f, S const& shape,
        hpx::threads::mask_cref_type mask, Ts&&... ts)
    {
        hpx::threads::thread_description const desc(
            f, "index_queue_bulk_sync_execute");

        return index_queue_bulk_sync_execute(desc, pool, first_thread,
            num_threads, policy, HPX_FORWARD(F, f), shape, mask,
            HPX_FORWARD(Ts, ts)...);
    }

    template <typename Launch, typename F, typename S, typename... Ts>
    decltype(auto) index_queue_bulk_async_execute(
        hpx::threads::thread_description const& desc,
        threads::thread_pool_base* pool, std::size_t first_thread,
        std::size_t num_threads, Launch policy, F&& f, S const& shape,
        hpx::threads::mask_cref_type mask, Ts&&... ts)
    {
        using result_type = detail::bulk_function_result_t<F, S, Ts...>;
        if constexpr (!std::is_void_v<result_type>)
        {
            return index_queue_bulk_async_execute_result(desc, pool,
                first_thread, num_threads, policy, HPX_FORWARD(F, f), shape,
                mask, HPX_FORWARD(Ts, ts)...);
        }
        else
        {
            return index_queue_bulk_async_execute_void(desc, pool, first_thread,
                num_threads, policy, HPX_FORWARD(F, f), shape, mask,
                HPX_FORWARD(Ts, ts)...);
        }
    }

    template <typename Launch, typename F, typename S, typename... Ts>
    decltype(auto) index_queue_bulk_async_execute(
        threads::thread_pool_base* pool, std::size_t first_thread,
        std::size_t num_threads, Launch policy, F&& f, S const& shape,
        hpx::threads::mask_cref_type mask, Ts&&... ts)
    {
        hpx::threads::thread_description const desc(
            f, "index_queue_bulk_async_execute");

        return index_queue_bulk_async_execute(desc, pool, first_thread,
            num_threads, policy, HPX_FORWARD(F, f), shape, mask,
            HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::parallel::execution::detail

#include <hpx/config/warnings_suffix.hpp>
