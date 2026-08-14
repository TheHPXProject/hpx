//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/components_base.hpp>
#include <hpx/modules/thread_support.hpp>

#include <hpx/async_distributed/detail/promise_lco.hpp>

namespace hpx::traits::detail {

    util::atomic_count unique_type(
        to_int(components::component_enum_type::last));
}    // namespace hpx::traits::detail

using promise_lco_void = hpx::components::managed_component<
    hpx::lcos::detail::promise_lco<void, hpx::util::unused_type>>;

HPX_REGISTER_COMPONENT_HEAP(promise_lco_void)
