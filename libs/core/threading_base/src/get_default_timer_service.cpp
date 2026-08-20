//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2020 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/errors/error.hpp>
#include <hpx/errors/exception.hpp>
#include <hpx/threading_base/detail/get_default_timer_service.hpp>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#endif
#include <asio/io_context.hpp>

namespace hpx::threads::detail {

    static get_default_timer_service_type get_default_timer_service_f;

    void set_get_default_timer_service(get_default_timer_service_type f)
    {
        get_default_timer_service_f = HPX_MOVE(f);
    }

    ::asio::io_context& get_default_timer_service()
    {
        if (detail::get_default_timer_service_f)
        {
            return detail::get_default_timer_service_f();
        }
        else
        {
#if defined(HPX_HAVE_TIMER_POOL)
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::threads::detail::get_default_timer_service",
                "No timer service installed. When running timed "
                "threads without a runtime a timer service has to be "
                "installed manually using "
                "hpx::threads::detail::set_get_default_timer_service.");
#else
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::threads::detail::get_default_timer_service",
                "No timer service installed. Rebuild HPX with "
                "HPX_WITH_TIMER_POOL=ON or provide a timer service "
                "manually using "
                "hpx::threads::detail::set_get_default_timer_service.");
#endif
        }
    }
}    // namespace hpx::threads::detail
