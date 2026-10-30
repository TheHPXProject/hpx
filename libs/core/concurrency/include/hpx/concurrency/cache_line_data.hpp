//  Copyright (c) 2019-2026 Hartmut Kaiser
//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::util {

    namespace detail {

        // Computes the padding required to fill up a full cache line after
        // data_size bytes.
        HPX_CXX_CORE_EXPORT constexpr std::size_t get_cache_line_padding_size(
            std::size_t const data_size) noexcept
        {
            return (threads::get_cache_line_size() -
                       (data_size % threads::get_cache_line_size())) %
                threads::get_cache_line_size();
        }

        HPX_CXX_CORE_EXPORT template <typename Data>
        struct needs_padding
          : std::integral_constant<bool,
                // NOLINTNEXTLINE(bugprone-sizeof-expression)
                detail::get_cache_line_padding_size(sizeof(Data)) != 0>
        {
        };
    }    // namespace detail

    // Variable 'cacheline_pad' is uninitialized. Always initialize a member
    // variable
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26495)
#endif

    // NOTE: We do not use alignas here because asking for overaligned
    // memory is significantly more expensive than asking for unaligned
    // memory. Padding the struct is cheaper and enough for internal
    // purposes.

    // NOTE: The implementations below are currently identical because of
    // the above issue. Both names are kept for compatibility.

    ///////////////////////////////////////////////////////////////////////////
    // special struct to ensure cache line alignment of a data type
    HPX_CXX_CORE_EXPORT template <typename Data,
        bool NeedsPadding = detail::needs_padding<Data>::value>
    struct cache_aligned_data
    {
        // We have an explicit (default) constructor here to avoid for the
        // entire cache-line to be initialized by the compiler.

        constexpr cache_aligned_data() noexcept(    //-V730
            std::is_nothrow_default_constructible_v<Data>)
          : data_()
        {
        }

        template <typename... Ts,
            typename = std::enable_if_t<std::is_constructible_v<Data, Ts&&...>>>
        cache_aligned_data(Ts&&... ts) noexcept(
            std::is_nothrow_constructible_v<Data, Ts&&...>)
          : data_(HPX_FORWARD(Ts, ts)...)
        {
        }

        // pad to cache line size bytes
        Data data_;

        //  cppcheck-suppress unusedVariable
        char cacheline_pad[detail::get_cache_line_padding_size(
            // NOLINTNEXTLINE(bugprone-sizeof-expression)
            sizeof(Data))];
    };

    template <typename Data>
    struct cache_aligned_data<Data, false>
    {
        constexpr cache_aligned_data() noexcept(
            std::is_nothrow_default_constructible_v<Data>)
          : data_()
        {
        }

        template <typename... Ts,
            typename = std::enable_if_t<std::is_constructible_v<Data, Ts&&...>>>
        cache_aligned_data(Ts&&... ts) noexcept(
            std::is_nothrow_constructible_v<Data, Ts&&...>)
          : data_(HPX_FORWARD(Ts, ts)...)
        {
        }

        // no need to pad to cache line size
        Data data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // special struct to ensure cache line alignment of a data type
    HPX_CXX_CORE_EXPORT template <typename Data,
        bool NeedsPadding = detail::needs_padding<Data>::value>
    struct cache_aligned_data_derived : Data
    {
        // We have an explicit (default) constructor here to avoid for the
        // entire cache-line to be initialized by the compiler.
        constexpr cache_aligned_data_derived() noexcept(    //-V730
            std::is_nothrow_default_constructible_v<Data>)
          : Data()
        {
        }

        template <typename... Ts,
            typename = std::enable_if_t<std::is_constructible_v<Data, Ts&&...>>>
        cache_aligned_data_derived(Ts&&... ts) noexcept(
            std::is_nothrow_constructible_v<Data, Ts&&...>)
          : Data(HPX_FORWARD(Ts, ts)...)
        {
        }

        //  cppcheck-suppress unusedVariable
        char cacheline_pad[detail::get_cache_line_padding_size(
            // NOLINTNEXTLINE(bugprone-sizeof-expression)
            sizeof(Data))];
    };

    template <typename Data>
    struct cache_aligned_data_derived<Data, false> : Data
    {
        constexpr cache_aligned_data_derived() noexcept(
            std::is_nothrow_default_constructible_v<Data>)
          : Data()
        {
        }

        template <typename... Ts,
            typename = std::enable_if_t<std::is_constructible_v<Data, Ts&&...>>>
        cache_aligned_data_derived(Ts&&... ts) noexcept(
            std::is_nothrow_constructible_v<Data, Ts&&...>)
          : Data(HPX_FORWARD(Ts, ts)...)
        {
        }

        // no need to pad to cache line size
    };

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

    ///////////////////////////////////////////////////////////////////////////
    // special struct to data type is cache line aligned and fully occupies a
    // cache line
    HPX_CXX_CORE_EXPORT template <typename Data>
    using cache_line_data = cache_aligned_data<Data>;

    ///////////////////////////////////////////////////////////////////////////
    // Holds data aligned to the destructive interference size
    // (threads::get_destructive_interference_size(), at least the cache line
    // size). Unlike cache_line_data, which only pads, this makes every
    // element of a std::vector start on its own cache line, and keeps it
    // apart from its neighbors as far as needed to avoid false sharing (on
    // x86-64 the 128-byte pairs of lines the spatial prefetcher fetches
    // together). Over-aligned types need the aligned allocation path, so use
    // this only for data that is rarely allocated, e.g. the per-worker data of
    // a scheduler, which is allocated only when the scheduler is created.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct alignas(threads::get_destructive_interference_size())
        aligned_line_data
    {
        // We have an explicit (default) constructor here to avoid for the
        // entire cache-line to be initialized by the compiler.
        constexpr aligned_line_data() noexcept(
            std::is_nothrow_default_constructible_v<T>)
          : data_()
        {
        }

        template <typename... Ts,
            typename = std::enable_if_t<std::is_constructible_v<T, Ts&&...>>>
        aligned_line_data(Ts&&... ts) noexcept(
            std::is_nothrow_constructible_v<T, Ts&&...>)
          : data_(HPX_FORWARD(Ts, ts)...)
        {
        }

        T data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr auto align_up(T value, std::size_t alignment) noexcept
    {
        return T(hpx::bit_cast<std::size_t>(value + (alignment - 1)) &
            ~(alignment - 1));
    }
}    // namespace hpx::util
