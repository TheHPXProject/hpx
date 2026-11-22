//  Copyright (c) 2019-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/lcos_local/detail/preprocess_future.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    extra_data_id_type
    extra_data_helper<serialization::detail::preprocess_futures>::id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }
}    // namespace hpx::util

namespace hpx::lcos::detail {

    void preprocess_future(serialization::output_archive& ar,
        hpx::lcos::detail::future_data_refcnt_base& state)
    {
        auto& handle_futures =
            ar.get_extra_data<serialization::detail::preprocess_futures>();

        handle_futures.await_future(state);
    }
}    // namespace hpx::lcos::detail
