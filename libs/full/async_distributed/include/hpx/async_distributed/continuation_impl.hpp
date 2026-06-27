//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/async_distributed/detail/post_implementations_fwd.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx::actions {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Cont>
    struct continuation_impl
    {
    private:
        using cont_type = std::decay_t<Cont>;

    public:
        continuation_impl() = default;

        template <typename Cont_>
        continuation_impl(Cont_&& cont, hpx::id_type const& target)
          : cont_(HPX_FORWARD(Cont_, cont))
          , target_(target)
        {
        }

        virtual ~continuation_impl() = default;

        template <typename T>
        util::invoke_result_t<cont_type, hpx::id_type, T> operator()(
            hpx::id_type const& lco, T&& t) const
        {
            hpx::post_c(cont_, lco, target_, HPX_FORWARD(T, t));

            // Unfortunately we need to default construct the return value,
            // this possibly imposes an additional restriction of return types.
            using result_type =
                util::invoke_result_t<cont_type, hpx::id_type, T>;
            return result_type();
        }

    private:
        // serialization support
        friend class hpx::serialization::access;

        template <typename Archive>
        HPX_FORCEINLINE void serialize(Archive& ar, unsigned int const)
        {
            // clang-format off
            ar & cont_ & target_;
            // clang-format on
        }

        cont_type cont_;
        hpx::id_type target_;
    };
}    // namespace hpx::actions
