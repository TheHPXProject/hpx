//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file wait_any.hpp
/// \page hpx::wait_any, hpx::wait_any_nothrow, hpx::wait_any_n, hpx::wait_any_n_nothrow
/// \headerfile hpx/future.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param first    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_any should wait.
    /// \param last     [in] The iterator pointing to the last element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_any should wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_any(InputIter first, InputIter last);

    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param futures  [in] A vector holding an arbitrary amount of \a future or
    ///                 \a shared_future objects for which \a wait_any should
    ///                 wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename R>
    void wait_any(std::vector<future<R>>& futures);

    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param futures  [in] Amn array holding an arbitrary amount of \a future or
    ///                 \a shared_future objects for which \a wait_any should
    ///                 wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename R, std::size_t N>
    void wait_any(std::array<future<R>, N>& futures);

    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param futures  [in] An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_any should wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename... T>
    void wait_any(T&&... futures);

    /// The function \a wait_any_n is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param first    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_any_n should wait.
    /// \param count    [in] The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \note The function \a wait_any_n returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any_n
    ///       returns.
    ///
    /// \note           The function wait_any_n will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_n_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_any_n(InputIter first, std::size_t count);
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/async_combinators/wait_some.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_any_nothrow_t final
    {
        template <typename Future>
        HPX_FORCEINLINE bool operator()(
            std::vector<Future> const& futures) const
        {
            return hpx::wait_some_nothrow(1, futures);
        }

        template <typename Future>
        HPX_FORCEINLINE bool operator()(std::vector<Future>& lazy_values) const
        {
            return (*this)(const_cast<std::vector<Future> const&>(lazy_values));
        }

        template <typename Future>
        HPX_FORCEINLINE bool operator()(std::vector<Future>&& lazy_values) const
        {
            return (*this)(const_cast<std::vector<Future> const&>(lazy_values));
        }

        template <typename Future, std::size_t N>
        HPX_FORCEINLINE bool operator()(
            std::array<Future, N> const& futures) const
        {
            return hpx::wait_some_nothrow(1, futures);
        }

        template <typename Future, std::size_t N>
        HPX_FORCEINLINE bool operator()(
            std::array<Future, N>& lazy_values) const
        {
            return (*this)(
                const_cast<std::array<Future, N> const&>(lazy_values));
        }

        template <typename Future, std::size_t N>
        HPX_FORCEINLINE bool operator()(
            std::array<Future, N>&& lazy_values) const
        {
            return (*this)(
                const_cast<std::array<Future, N> const&>(lazy_values));
        }

        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        HPX_FORCEINLINE bool operator()(Iterator begin, Iterator end) const
        {
            return hpx::wait_some_nothrow(1, begin, end);
        }

        HPX_FORCEINLINE bool operator()() const
        {
            return hpx::wait_some_nothrow(0);
        }

        template <typename... Ts>
        HPX_FORCEINLINE bool operator()(Ts&&... ts) const
        {
            return hpx::wait_some_nothrow(1, HPX_FORWARD(Ts, ts)...);
        }
    } wait_any_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_any_t final
    {
    private:
        template <typename Future>
        static HPX_FORCEINLINE void wait_any_impl(
            std::vector<Future> const& futures)
        {
            hpx::wait_some(1, futures);
        }

        template <typename Future, std::size_t N>
        static HPX_FORCEINLINE void wait_any_impl(
            std::array<Future, N> const& futures)
        {
            hpx::wait_some(1, futures);
        }

    public:
        template <typename Future>
        HPX_FORCEINLINE void operator()(
            std::vector<Future> const& futures) const
        {
            wait_any_t::wait_any_impl(futures);
        }

        template <typename Future>
        HPX_FORCEINLINE void operator()(std::vector<Future>& lazy_values) const
        {
            wait_any_t::wait_any_impl(
                const_cast<std::vector<Future> const&>(lazy_values));
        }

        template <typename Future>
        HPX_FORCEINLINE void operator()(std::vector<Future>&& lazy_values) const
        {
            wait_any_t::wait_any_impl(
                const_cast<std::vector<Future> const&>(lazy_values));
        }

        template <typename Future, std::size_t N>
        HPX_FORCEINLINE void operator()(
            std::array<Future, N> const& futures) const
        {
            wait_any_t::wait_any_impl(futures);
        }

        template <typename Future, std::size_t N>
        HPX_FORCEINLINE void operator()(
            std::array<Future, N>& lazy_values) const
        {
            wait_any_t::wait_any_impl(
                const_cast<std::array<Future, N> const&>(lazy_values));
        }

        template <typename Future, std::size_t N>
        HPX_FORCEINLINE void operator()(
            std::array<Future, N>&& lazy_values) const
        {
            wait_any_t::wait_any_impl(
                const_cast<std::array<Future, N> const&>(lazy_values));
        }

        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        HPX_FORCEINLINE void operator()(Iterator begin, Iterator end) const
        {
            hpx::wait_some(1, begin, end);
        }

        HPX_FORCEINLINE void operator()() const
        {
            hpx::wait_some(0);
        }

        template <typename... Ts>
        HPX_FORCEINLINE void operator()(Ts&&... ts) const
        {
            hpx::wait_some(1, HPX_FORWARD(Ts, ts)...);
        }
    } wait_any{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_any_n_nothrow_t final
    {
        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        HPX_FORCEINLINE bool operator()(Iterator begin, std::size_t count) const
        {
            return hpx::wait_some_n_nothrow(1, begin, count);
        }
    } wait_any_n_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_any_n_t final
    {
        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        HPX_FORCEINLINE void operator()(Iterator begin, std::size_t count) const
        {
            hpx::wait_some_n(1, begin, count);
        }
    } wait_any_n{};
}    // namespace hpx

#endif    // DOXYGEN
