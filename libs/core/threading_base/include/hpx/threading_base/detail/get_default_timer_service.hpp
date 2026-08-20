//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c)      2018 Thomas Heller
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c) 2008-2009 Chirag Dekate, Anshul Tandon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/function.hpp>

namespace hpx::threads::detail {

    using get_default_timer_service_type = hpx::function<::asio::io_context&()>;
    HPX_CORE_EXPORT void set_get_default_timer_service(
        get_default_timer_service_type f);
    HPX_CORE_EXPORT ::asio::io_context& get_default_timer_service();
}    // namespace hpx::threads::detail
