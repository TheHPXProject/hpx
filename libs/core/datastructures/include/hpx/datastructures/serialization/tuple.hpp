//  Copyright (c) 2011-2013 Thomas Heller
//  Copyright (c) 2011-2025 Hartmut Kaiser
//  Copyright (c) 2013-2015 Agustin Berge
//  Copyright (c)      2019 Mikael Simberg
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/datastructures/tuple.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::traits {

    template <typename... Ts>
    struct is_bitwise_serializable<::hpx::tuple<Ts...>>
      : ::hpx::util::all_of<
            hpx::traits::is_bitwise_serializable<std::remove_const_t<Ts>>...>
    {
    };

    template <typename... Ts>
    struct is_not_bitwise_serializable<::hpx::tuple<Ts...>>
      : std::integral_constant<bool,
            !is_bitwise_serializable_v<::hpx::tuple<Ts...>>>
    {
    };
}    // namespace hpx::traits

namespace hpx::util::detail {

    HPX_CXX_CORE_EXPORT template <typename Archive, typename Is, typename... Ts>
    struct serialize_with_index_pack;

    HPX_CXX_CORE_EXPORT template <typename Archive, typename Is, typename... Ts>
    struct load_construct_data_with_index_pack;

    HPX_CXX_CORE_EXPORT template <typename Archive, typename Is, typename... Ts>
    struct save_construct_data_with_index_pack;

    template <typename Archive, std::size_t... Is, typename... Ts>
    struct serialize_with_index_pack<Archive, hpx::util::index_pack<Is...>,
        Ts...>
    {
        template <typename T>
        static void call(Archive& ar, T& t, unsigned int)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALLOW_CONST_TUPLE_MEMBERS)
            (hpx::serialization::detail::serialize_one(ar, hpx::get<Is>(t)),
                ...);
#else
            (hpx::serialization::detail::serialize_one(
                 ar, const_cast<std::remove_const_t<Ts>&>(hpx::get<Is>(t))),
                ...);
#endif
        }
    };

    template <typename Archive, std::size_t... Is, typename... Ts>
    struct load_construct_data_with_index_pack<Archive,
        hpx::util::index_pack<Is...>, Ts...>
    {
        template <typename T>
        static void load_element(Archive& ar, T& t)
        {
            constexpr bool is_polymorphic =
                hpx::traits::is_intrusive_polymorphic_v<T> ||
                hpx::traits::is_nonintrusive_polymorphic_v<T>;

            if constexpr (is_polymorphic)
            {
                std::unique_ptr<T> data(
                    serialization::detail::constructor_selector_ptr<T>::create(
                        ar));
                t = HPX_MOVE(*data);
            }
            else
            {
                if constexpr (!std::is_default_constructible_v<T>)
                {
                    // non-default constructible types are handled by the user
                    using serialization::detail::load_construct_data;
                    load_construct_data(ar, &t, 0);
                }
                else
                {
                    // default constructible types still need to be constructed
                    ::new (&t) T;
                }
                ar >> t;
            }
        }

        static void call(Archive& ar, hpx::tuple<Ts...>& t, unsigned int)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALLOW_CONST_TUPLE_MEMBERS)
            (load_element(ar, hpx::get<Is>(t)), ...);
#else
            (load_element(
                 ar, const_cast<std::remove_const_t<Ts>&>(hpx::get<Is>(t))),
                ...);
#endif
        }
    };

    template <typename Archive, std::size_t... Is, typename... Ts>
    struct save_construct_data_with_index_pack<Archive,
        hpx::util::index_pack<Is...>, Ts...>
    {
        template <typename T>
        static void save_element(Archive& ar, T& t)
        {
            if constexpr (!std::is_default_constructible_v<T>)
            {
                using serialization::detail::save_construct_data;
                save_construct_data(ar, &t, 0);
            }
            ar << t;
        }

        static void call(Archive& ar, hpx::tuple<Ts...> const& t, unsigned int)
        {
            (save_element(ar, hpx::get<Is>(t)), ...);
        }
    };
}    // namespace hpx::util::detail

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename Archive, typename... Ts>
    void serialize(Archive& ar, hpx::tuple<Ts...>& t, unsigned int version)
    {
        using Is = hpx::util::make_index_pack_t<sizeof...(Ts)>;
        hpx::util::detail::serialize_with_index_pack<Archive, Is, Ts...>::call(
            ar, t, version);
    }

    HPX_CXX_CORE_EXPORT template <typename Archive>
    void serialize(Archive&, hpx::tuple<>&, unsigned)
    {
    }

    HPX_CXX_CORE_EXPORT template <typename Archive, typename... Ts>
    void load_construct_data(
        Archive& ar, hpx::tuple<Ts...>* t, unsigned int version)
    {
        using Is = hpx::util::make_index_pack_t<sizeof...(Ts)>;
        hpx::util::detail::load_construct_data_with_index_pack<Archive, Is,
            Ts...>::call(ar, *t, version);
    }

    HPX_CXX_CORE_EXPORT template <typename Archive, typename... Ts>
    void save_construct_data(
        Archive& ar, hpx::tuple<Ts...> const* t, unsigned int version)
    {
        using Is = hpx::util::make_index_pack_t<sizeof...(Ts)>;
        hpx::util::detail::save_construct_data_with_index_pack<Archive, Is,
            Ts...>::call(ar, *t, version);
    }
}    // namespace hpx::serialization
