//  Copyright (c) 2026 Ujjwal Shekhar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/serialization/config/defines.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/traits/brace_initializable_traits.hpp>
#include <hpx/serialization/traits/is_bitwise_serializable.hpp>
#include <hpx/serialization/traits/is_not_bitwise_serializable.hpp>
#include <hpx/serialization/traits/polymorphic_traits.hpp>

#include <type_traits>
#include <utility>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    class is_serialization_supported
    {
    public:
        static constexpr bool has_serialize =
            hpx::traits::is_intrusive_polymorphic_v<T> || std::is_empty_v<T> ||
            hpx::traits::has_serialize_adl_v<T>;

        static constexpr bool has_optimized =
            hpx::traits::is_bitwise_serializable_v<T> ||
            !hpx::traits::is_not_bitwise_serializable_v<T>;

        static constexpr bool has_refl_serialize =
#if defined(HPX_HAVE_CXX26_REFLECTION)
            true;
#else
            false;
#endif

        static constexpr bool value =
            hpx::traits::is_nonintrusive_polymorphic_v<T> ||
            hpx::traits::has_struct_serialization_v<T> || has_serialize ||
            has_optimized || has_refl_serialize;

        using type = std::bool_constant<value>;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_serialization_supported_v =
        is_serialization_supported<T>::value;

}    // namespace hpx::traits
