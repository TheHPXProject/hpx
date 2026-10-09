//  Copyright (c) 2005-2023 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__FreeBSD__) || defined(__APPLE__)

#include <hpx/coroutines/detail/posix_utility.hpp>

namespace hpx::threads::coroutines::detail::posix {

    ///////////////////////////////////////////////////////////////////////////
    // this global variable is used to control whether guard pages will be used
    // or not
    bool use_guard_pages = true;

    // Default to MADV_FREE (or keep-resident) when recycling mmap'd stacks.
    // See hpx.stacks.unbind_on_reset and #6793.
    int unbind_on_reset = 1;
}    // namespace hpx::threads::coroutines::detail::posix

#endif
