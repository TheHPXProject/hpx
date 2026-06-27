//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2020 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/threading_base/detail/get_default_pool.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/thread_description.hpp>
#include <hpx/threading_base/thread_pool_base.hpp>

// The following implementation has been divided for Linux and Mac OSX
#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                                      \
    (defined(__linux) || defined(__linux__) || defined(linux) ||               \
        defined(__APPLE__))

namespace hpx_start {

    // Redefining weak variables defined in hpx_main.hpp to facilitate error
    // checking and make sure correct errors are thrown. It is added again to
    // make sure that these variables are defined correctly in cases where
    // hpx_main functionality is not used.
    HPX_SYMBOL_EXPORT bool is_linked __attribute__((weak)) = false;
    HPX_SYMBOL_EXPORT bool include_libhpx_wrap __attribute__((weak)) = false;
}    // namespace hpx_start

#endif

namespace hpx::threads::detail {

    static get_default_pool_type get_default_pool;

    void set_get_default_pool(get_default_pool_type f)
    {
        get_default_pool = HPX_MOVE(f);
    }

    thread_pool_base* get_self_or_default_pool()
    {
        thread_pool_base* pool = nullptr;
        if (auto const* thrd_data = get_self_id_data())
        {
            pool = thrd_data->get_scheduler_base()->get_parent_pool();
        }
        else if (detail::get_default_pool)
        {
            pool = detail::get_default_pool();
            HPX_ASSERT(pool);
        }
        else
        {
// The following implementation has been divided for Linux and Mac OSX
#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                                      \
    (defined(__linux) || defined(__linux__) || defined(linux) ||               \
        defined(__APPLE__))

            // hpx_main.hpp is included but not linked to libhpx_wrap
            if (!hpx_start::is_linked && hpx_start::include_libhpx_wrap)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "hpx::threads::detail::get_self_or_default_pool",
                    "Attempting to use hpx_main.hpp functionality "
                    "without linking to libhpx_wrap. If you're using "
                    "CMake, make sure to add HPX::wrap_main to "
                    "target_link_libraries. If you're linking "
                    "manually (e.g., via Makefiles or Godbolt), "
                    "ensure you pass '-Wl,-wrap=main' to the linker "
                    "on Linux or '-Wl,-e,_initialize_main' on macOS. "
                    "Consult the HPX documentation for the full "
                    "library link order and other subtle nuances.");
            }
#endif
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::threads::detail::get_self_or_default_pool",
                "Attempting to register a thread outside the HPX "
                "runtime and no default pool handler is installed. "
                "Did you mean to run this on an HPX thread?");
        }
        return pool;
    }
}    // namespace hpx::threads::detail
