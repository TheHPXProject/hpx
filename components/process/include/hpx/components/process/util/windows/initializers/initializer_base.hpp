// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)

namespace hpx::components::process::windows::initializers {

    struct initializer_base
    {
        template <typename WindowsExecutor>
        constexpr void on_CreateProcess_setup(WindowsExecutor&) const noexcept
        {
        }

        template <typename WindowsExecutor>
        constexpr void on_CreateProcess_error(WindowsExecutor&) const noexcept
        {
        }

        template <typename WindowsExecutor>
        constexpr void on_CreateProcess_success(WindowsExecutor&) const noexcept
        {
        }
    };
}    // namespace hpx::components::process::windows::initializers

#endif
