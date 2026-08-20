//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/hpx_main_winsocket.hpp>

#if defined(HPX_WINDOWS)

#if !defined(WIN32)
#define WIN32
#endif
// clang-format off
#include <winsock2.h>
#include <windows.h>
#include <asio/detail/winsock_init.hpp>
// clang-format on

namespace hpx { namespace detail {
    // Make sure the Winsocket library is explicitly initialized before main
    // is executed.
    struct winsocket_init_helper
    {
        static ::asio::detail::winsock_init<> const& init()
        {
            static const ::asio::detail::winsock_init<> init_winsocket =
                ::asio::detail::winsock_init<>(false);
            return init_winsocket;
        }
    };

    // This function makes sure this file is actually linked to the executable.
    void init_winsocket()
    {
        winsocket_init_helper::init();
    }
}}    // namespace hpx::detail

#endif
