//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/thread_support.hpp>

#include <concepts>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

namespace hpx::traits {

    namespace detail {

        // An iterator type that gives the size of its dereference itself.
        HPX_CXX_CORE_EXPORT template <typename Iter>
        concept declares_dereference_task_size = requires {
            {
                Iter::dereference_task_size
            } -> std::convertible_to<threads::task_size>;
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        concept tuple_like = requires { std::tuple_size<T>::value; };

        // Whether a dereference that returns a T reads elements that exist
        // already: T is a reference, or a tuple of them (hpx::tuple,
        // std::tuple, std::pair), as returned by zip iterators.
        HPX_CXX_CORE_EXPORT template <typename T>
        consteval bool refers_to_existing_elements()
        {
            if constexpr (std::is_reference_v<T>)
            {
                return true;
            }
            else if constexpr (tuple_like<T>)
            {
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return (refers_to_existing_elements<
                                std::tuple_element_t<Is, T>>() &&
                        ...);
                }(std::make_index_sequence<std::tuple_size_v<T>>());
            }
            else
            {
                return false;
            }
        }

        // An iterator adaptor (see hpx::util::iterator_adaptor) that returns
        // what the iterator it wraps returns, which is what its dereference
        // does unless the adaptor replaces it.
        HPX_CXX_CORE_EXPORT template <typename Iter>
        concept forwarding_iterator_adaptor =
            requires(Iter const& it) {
                typename Iter::base_type;
                typename std::iter_reference_t<typename Iter::base_type>;
                { it.base() } -> std::same_as<typename Iter::base_type const&>;
            } &&
            std::is_same_v<std::iter_reference_t<Iter>,
                std::iter_reference_t<typename Iter::base_type>>;
    }    // namespace detail

    /// The size of dereferencing an iterator of the type \a Iter, which an
    /// algorithm combines with the size of the function it calls with the
    /// element (see hpx::threads::combined_task_size).
    ///
    /// An iterator that returns a reference (T&, T const&, T&&, ...), or a
    /// tuple of references (as a zip iterator does), reads elements that exist
    /// already, its dereference is a nano task. An iterator that returns a
    /// value computes it or returns a proxy, its size is unknown. An iterator
    /// adaptor that returns what the iterator it wraps returns has the size
    /// of that iterator. An iterator type can give the size itself by
    /// declaring
    ///
    /// \code
    ///     static constexpr hpx::threads::task_size dereference_task_size =
    ///         hpx::threads::task_size::nano;
    /// \endcode
    ///
    /// for instance in a class derived from hpx::util::iterator_facade or
    /// hpx::util::iterator_adaptor, or this trait can be specialized.
    ///
    /// value is the size known from the type of Iter. call(it) returns the
    /// size for the iterator it, which can be known only at run time, for
    /// instance for an iterator that calls a function stored in an
    /// hpx::function.
    HPX_CXX_CORE_EXPORT template <typename Iter>
    struct iterator_dereference_task_size
    {
        static constexpr threads::task_size value =
            detail::refers_to_existing_elements<std::iter_reference_t<Iter>>() ?
            threads::task_size::nano :
            threads::task_size::unknown;

        [[nodiscard]] static constexpr threads::task_size call(
            Iter const& /*it*/) noexcept
        {
            return value;
        }
    };

    // The size the iterator type gives itself.
    template <typename Iter>
        requires(std::is_same_v<Iter, std::remove_cvref_t<Iter>> &&
            detail::declares_dereference_task_size<Iter>)
    struct iterator_dereference_task_size<Iter>
    {
        static constexpr threads::task_size value = Iter::dereference_task_size;

        [[nodiscard]] static constexpr threads::task_size call(
            Iter const& /*it*/) noexcept
        {
            return value;
        }
    };

    // An adaptor that returns what the iterator it wraps returns.
    template <typename Iter>
        requires(std::is_same_v<Iter, std::remove_cvref_t<Iter>> &&
            !detail::declares_dereference_task_size<Iter> &&
            detail::forwarding_iterator_adaptor<Iter>)
    struct iterator_dereference_task_size<Iter>
    {
        static constexpr threads::task_size value =
            iterator_dereference_task_size<typename Iter::base_type>::value;

        [[nodiscard]] static constexpr threads::task_size call(
            Iter const& it) noexcept
        {
            return iterator_dereference_task_size<
                typename Iter::base_type>::call(it.base());
        }
    };

    // A qualified iterator has the size of the unqualified one.
    template <typename Iter>
        requires(!std::is_same_v<Iter, std::remove_cvref_t<Iter>>)
    struct iterator_dereference_task_size<Iter>
      : iterator_dereference_task_size<std::remove_cvref_t<Iter>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Iter>
    inline constexpr threads::task_size iterator_dereference_task_size_v =
        iterator_dereference_task_size<Iter>::value;
}    // namespace hpx::traits
