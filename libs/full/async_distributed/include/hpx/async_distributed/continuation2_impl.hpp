//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/async_distributed/detail/post_continue_fwd.hpp>
#include <hpx/async_distributed/detail/post_implementations_fwd.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx::actions {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Cont, typename F>
    struct continuation2_impl
    {
    private:
        using cont_type = std::decay_t<Cont>;
        using function_type = std::decay_t<F>;

    public:
        continuation2_impl() = default;

        template <typename Cont_, typename F_>
        continuation2_impl(Cont_&& cont, hpx::id_type const& target, F_&& f)
          : cont_(HPX_FORWARD(Cont_, cont))
          , target_(target)
          , f_(HPX_FORWARD(F_, f))
        {
        }

        virtual ~continuation2_impl() = default;

        template <typename T>
        util::invoke_result_t<function_type, hpx::id_type,
            util::invoke_result_t<cont_type, hpx::id_type, T>>
        operator()(hpx::id_type const& lco, T&& t) const
        {
            using hpx::placeholders::_2;
            hpx::post_continue(
                cont_, hpx::bind(f_, lco, _2), target_, HPX_FORWARD(T, t));

            // Unfortunately we need to default construct the return value,
            // this possibly imposes an additional restriction of return types.
            using result_type =
                util::invoke_result_t<function_type, hpx::id_type,
                    util::invoke_result_t<cont_type, hpx::id_type, T>>;
            return result_type();
        }

    private:
        // serialization support
        friend class hpx::serialization::access;

        template <typename Archive>
        HPX_FORCEINLINE void serialize(Archive& ar, unsigned int const)
        {
            // clang-format off
            ar & cont_ & target_ & f_;
            // clang-format on
        }

        cont_type cont_;    // continuation type
        hpx::id_type target_;
        function_type f_;
        // set_value action  (default: set_lco_value_continuation)
    };
}    // namespace hpx::actions
