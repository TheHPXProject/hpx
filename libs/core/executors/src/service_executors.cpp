//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/executors/service_executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/io_service.hpp>

#include <asio/io_context.hpp>
#include <asio/version.hpp>
#if ASIO_VERSION >= 103400
#include <asio/post.hpp>
#endif

namespace hpx::parallel::execution::detail {

    void service_executor::post_to_pool(
        hpx::util::io_service_pool* pool, hpx::function<void()>&& f)
    {
#if ASIO_VERSION >= 103400
        ::asio::post(pool->get_io_service(), HPX_MOVE(f));
#else
        pool->get_io_service().post(HPX_MOVE(f));
#endif
    }
}    // namespace hpx::parallel::execution::detail
