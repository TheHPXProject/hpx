//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/async_colocated/functional/colocated_helpers.hpp>
#include <hpx/async_colocated/macros.hpp>

namespace hpx::detail {

    template <typename Action, typename Ts = typename Action::arguments_type>
    struct post_colocated_bound_action;

    template <typename Action, typename... Ts>
    struct post_colocated_bound_action<Action, hpx::tuple<Ts...>>
    {
        using type = hpx::detail::bound_action<Action,
            hpx::util::make_index_pack<1 + sizeof...(Ts)>,
            hpx::detail::bound<hpx::util::functional::extract_locality,
                hpx::util::index_pack<0, 1>, hpx::detail::placeholder<2ul>,
                hpx::id_type>,
            Ts...>;
    };
}    // namespace hpx::detail
