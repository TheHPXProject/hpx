//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2015-2016 Thomas Heller
//  Copyright (c) 2015-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/naming/detail/preprocess_gid_types.hpp>

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    extra_data_id_type extra_data_helper<
        serialization::detail::preprocess_gid_types>::id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }
}    // namespace hpx::util
