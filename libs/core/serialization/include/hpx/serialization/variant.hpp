//  copyright (c) 2005
//  troy d. straszheim <troy@resophonic.com>
//  http://www.resophonic.com
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2020-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/config/defines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/serialization/macros.hpp>
#include <hpx/serialization/serialization_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>

namespace hpx::serialization {

    namespace detail {

        ////////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct std_variant_save_visitor
        {
            explicit constexpr std_variant_save_visitor(
                output_archive& ar) noexcept
              : ar_(ar)
            {
            }

            template <typename T>
            void operator()(T const& value) const
            {
                ar_ << value;
            }

        private:
            output_archive& ar_;
        };

        ////////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename... Ts>
        struct std_variant_impl;

        template <typename T, typename... Ts>
        struct std_variant_impl<T, Ts...>
        {
            template <typename V>
            static void load(input_archive& ar, std::size_t which, V& v)
            {
                if (which == 0)
                {
                    T value;
                    ar >> value;
                    v.template emplace<T>(HPX_MOVE(value));
                    return;
                }
                std_variant_impl<Ts...>::load(ar, which - 1, v);
            }
        };

        template <>
        struct std_variant_impl<>
        {
            template <typename V>
            static constexpr void load(input_archive&, std::size_t, V&) noexcept
            {
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    void save(output_archive& ar, std::variant<Ts...> const& v, unsigned)
    {
        auto const which = static_cast<std::uint64_t>(v.index());
        ar << which;
        detail::std_variant_save_visitor visitor(ar);
        std::visit(visitor, v);
    }

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    void load(input_archive& ar, std::variant<Ts...>& v, unsigned)
    {
        std::uint64_t which;
        ar >> which;
        if (static_cast<std::size_t>(which) >= sizeof...(Ts))
        {
            // this might happen if a type was removed from the list of variant
            // types
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "load<Archive, Variant, version>",
                "type was removed from the list of variant types");
        }
        detail::std_variant_impl<Ts...>::load(
            ar, static_cast<std::size_t>(which), v);
    }

    HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE(HPX_CXX_CORE_EXPORT,
        (template <typename... Ts>), (std::variant<Ts...>) )
}    // namespace hpx::serialization
