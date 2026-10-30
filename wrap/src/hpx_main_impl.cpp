//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// HPX::auto_wrap_main on MSVC force-includes hpx/hpx_main.hpp into every
// translation unit, so for static builds the default main() can't come from
// that header. It lives in its own translation unit here instead, so the
// linker picks it only if the executable doesn't define main() itself.
#if defined(HPX_MSVC) && defined(HPX_HAVE_STATIC_LINKING)
#include <hpx/hpx_main_impl.hpp>
#endif
