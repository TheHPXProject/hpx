//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2022-2025 Hartmut Kaiser
//  Copyright (c) 2026 Ujjwal Shekhar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file base_object.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/serialization/access.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/serialization/traits/polymorphic_traits.hpp>

#include <type_traits>

#if defined(HPX_HAVE_CXX26_REFLECTION)
#include <bit>
#include <cstddef>
#include <memory>
#include <meta>
#include <optional>
#endif

namespace hpx::serialization {

#if defined(HPX_HAVE_CXX26_REFLECTION)
    namespace detail {

        template <typename MemberType, typename T>
        constexpr decltype(auto) at_offset(T& base, std::size_t offset) noexcept
        {
            using Type = std::conditional_t<std::is_const_v<T>,
                MemberType const, MemberType>;

            auto base_addr = std::bit_cast<std::byte*>(std::addressof(base));
            auto member_ptr = std::bit_cast<Type*>(base_addr + offset);

            return *member_ptr;
        }
    }    // namespace detail
#endif

    HPX_CXX_CORE_EXPORT template <typename Derived, typename Base,
        typename Enable = void>
    struct base_object_type
    {
        explicit constexpr base_object_type(Derived& d) noexcept
          : d_(d)
        {
        }
        Derived& d_;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
#if defined(HPX_HAVE_CXX26_REFLECTION)
            static constexpr std::optional<std::size_t> offset =
                []() consteval -> std::optional<std::size_t> {
                constexpr auto ctx = std::meta::access_context::unchecked();

                for (auto b : std::meta::bases_of(^^Derived, ctx))
                {
                    if (std::meta::type_of(b) == ^^Base)
                    {
                        return static_cast<std::size_t>(
                            std::meta::offset_of(b).bytes);
                    }
                }
                return std::nullopt;
            }();

            static_assert(offset.has_value(),
                "Base class not found in derived class's base list");

            access::serialize(
                ar, detail::at_offset<Base>(d_, offset.value()), 0);
#else
            // legacy path
            access::serialize(ar,
                static_cast<Base&>(const_cast<std::decay_t<Derived>&>(d_)), 0);
#endif
        }
    };

    // we need another specialization to explicitly specify non-virtual calls of
    // virtual functions in intrusively serialized base classes.
    template <typename Derived, typename Base>
    struct base_object_type<Derived, Base,
        std::enable_if_t<hpx::traits::is_intrusive_polymorphic_v<Derived>>>
    {
        explicit constexpr base_object_type(Derived& d) noexcept
          : d_(d)
        {
        }
        Derived& d_;

        template <typename Archive>
        void save(Archive& ar, unsigned) const
        {
            access::save_base_object(ar, static_cast<Base const&>(d_), 0);
        }

        template <typename Archive>
        void load(Archive& ar, unsigned)
        {
            access::load_base_object(ar, static_cast<Base&>(d_), 0);
        }

        HPX_SERIALIZATION_SPLIT_MEMBER();
    };

    HPX_CXX_CORE_EXPORT template <typename Base, typename Derived>
    constexpr base_object_type<Derived, Base> base_object(Derived& d) noexcept
    {
        return base_object_type<Derived, Base>(d);
    }

    // allow our base_object_type to be serialized as prvalue compiler should
    // support good ADL implementation, but it is rather for all hpx
    // serialization library
    HPX_CXX_CORE_EXPORT template <typename D, typename B>
    HPX_FORCEINLINE output_archive& operator<<(
        output_archive& ar, base_object_type<D, B> t)
    {
        ar.save(t);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename D, typename B>
    HPX_FORCEINLINE input_archive& operator>>(
        input_archive& ar, base_object_type<D, B> t)
    {
        ar.load(t);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename D, typename B>
    HPX_FORCEINLINE output_archive& operator&(    //-V524
        output_archive& ar, base_object_type<D, B> t)
    {
        ar.save(t);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename D, typename B>
    HPX_FORCEINLINE input_archive& operator&(    //-V524
        input_archive& ar, base_object_type<D, B> t)
    {
        ar.load(t);
        return ar;
    }
}    // namespace hpx::serialization
