//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/continuation2_impl.hpp>
#include <hpx/async_distributed/continuation_impl.hpp>
#include <hpx/async_distributed/set_lco_value_continuation.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx {

    HPX_CXX_EXPORT inline hpx::actions::set_lco_value_continuation
    make_continuation()
    {
        return {};
    }

    HPX_CXX_EXPORT template <typename Cont>
    hpx::actions::continuation_impl<std::decay_t<Cont>> make_continuation(
        Cont&& cont)
    {
        using cont_type = std::decay_t<Cont>;
        return hpx::actions::continuation_impl<cont_type>(
            HPX_FORWARD(Cont, cont),
            naming::get_id_from_locality_id(agas::get_locality_id()));
    }

    HPX_CXX_EXPORT template <typename Cont>
    inline hpx::actions::continuation_impl<std::decay_t<Cont>>
    make_continuation(Cont&& f, hpx::id_type const& target)
    {
        using cont_type = std::decay_t<Cont>;
        return hpx::actions::continuation_impl<cont_type>(
            HPX_FORWARD(Cont, f), target);
    }

    HPX_CXX_EXPORT template <typename Cont, typename F>
        requires(!std::is_same_v<std::decay_t<F>, hpx::id_type>)
    hpx::actions::continuation2_impl<std::decay_t<Cont>, std::decay_t<F>>
    make_continuation(Cont&& cont, F&& f)
    {
        using cont_type = std::decay_t<Cont>;
        using function_type = std::decay_t<F>;

        return hpx::actions::continuation2_impl<cont_type, function_type>(
            HPX_FORWARD(Cont, cont),
            naming::get_id_from_locality_id(agas::get_locality_id()),
            HPX_FORWARD(F, f));
    }

    HPX_CXX_EXPORT template <typename Cont, typename F>
    hpx::actions::continuation2_impl<std::decay_t<Cont>, std::decay_t<F>>
    make_continuation(Cont&& cont, hpx::id_type const& target, F&& f)
    {
        using cont_type = std::decay_t<Cont>;
        using function_type = std::decay_t<F>;

        return hpx::actions::continuation2_impl<cont_type, function_type>(
            HPX_FORWARD(Cont, cont), target, HPX_FORWARD(F, f));
    }
}    // namespace hpx
