//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/serialization/detail/device_chunk_transport.hpp>

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    extra_data_id_type
    extra_data_helper<serialization::detail::device_transport>::id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }
}    // namespace hpx::util
