//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/async_distributed/packaged_action.hpp>

#include <asio/error.hpp>

#include <system_error>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    bool is_asio_error(std::error_code const& ec)
    {
        return ec ==
            ::asio::error::make_error_code(::asio::error::connection_reset);
    }
}    // namespace hpx::lcos::detail

#endif
