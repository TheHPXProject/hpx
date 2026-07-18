//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//  Copyright (c) 2016 Lukas Troska
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file wait_each.hpp
/// \page hpx::wait_each
/// \headerfile hpx/future.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// The function \a wait_each is an operator allowing to join on the results
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready. \a wait_each returns
    /// after all futures have been become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param futures  A vector holding an arbitrary amount of \a future or
    ///                 \a shared_future objects for which \a wait_each should
    ///                 wait.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename Future>
    void wait_each(F&& f, std::vector<Future>&& futures);

    /// The function \a wait_each is an operator allowing to join on the results
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready. \a wait_each returns
    /// after all futures have been become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_each should wait.
    /// \param end      The iterator pointing to the last element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_each should wait.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename Iterator>
    void wait_each(F&& f, Iterator begin, Iterator end);

    /// The function \a wait_each is an operator allowing to join on the results
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready. \a wait_each returns
    /// after all futures have been become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param futures  An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_each should wait.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename... T>
    void wait_each(F&& f, T&&... futures);

    /// The function \a wait_each is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_each_n should wait.
    /// \param count    The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename Iterator>
    void wait_each_n(F&& f, Iterator begin, std::size_t count);
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/async_combinators/detail/throw_if_exceptional.hpp>
#include <hpx/async_combinators/when_each.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_each_nothrow_t final
      : hpx::functional::tag<wait_each_nothrow_t>
    {
    private:
        template <typename F, typename Future>
        friend void tag_invoke(
            wait_each_nothrow_t, F&& f, std::vector<Future>& values)
        {
            hpx::when_each(HPX_FORWARD(F, f), values).wait();
        }

        template <typename F, typename Future>
        friend void tag_invoke(
            wait_each_nothrow_t, F&& f, std::vector<Future>&& values)
        {
            hpx::when_each(HPX_FORWARD(F, f), HPX_MOVE(values)).wait();
        }

        template <typename F, typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend void tag_invoke(
            wait_each_nothrow_t, F&& f, Iterator begin, Iterator end)
        {
            hpx::when_each(HPX_FORWARD(F, f), begin, end).wait();
        }

        template <typename F>
        friend void tag_invoke(wait_each_nothrow_t, F&& f)
        {
            hpx::when_each(HPX_FORWARD(F, f)).wait();
        }

        template <typename F, typename... Ts,
            typename Enable =
                std::enable_if_t<!traits::is_future_v<std::decay_t<F>> &&
                    util::all_of_v<traits::is_future<Ts>...>>>
        friend void tag_invoke(wait_each_nothrow_t, F&& f, Ts&&... ts)
        {
            hpx::when_each(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...).wait();
        }
    } wait_each_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_each_t final
      : hpx::functional::tag<wait_each_t>
    {
    private:
        template <typename F, typename Future>
        friend void tag_invoke(wait_each_t, F&& f, std::vector<Future>& values)
        {
            auto result = hpx::when_each(HPX_FORWARD(F, f), values);
            result.wait();
            hpx::detail::throw_if_exceptional(HPX_MOVE(result));
        }

        template <typename F, typename Future>
        friend void tag_invoke(wait_each_t, F&& f, std::vector<Future>&& values)
        {
            auto result = hpx::when_each(HPX_FORWARD(F, f), HPX_MOVE(values));
            result.wait();
            hpx::detail::throw_if_exceptional(HPX_MOVE(result));
        }

        template <typename F, typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend void tag_invoke(wait_each_t, F&& f, Iterator begin, Iterator end)
        {
            auto result = hpx::when_each(HPX_FORWARD(F, f), begin, end);
            result.wait();
            hpx::detail::throw_if_exceptional(HPX_MOVE(result));
        }

        template <typename F>
        friend void tag_invoke(wait_each_t, F&& f)
        {
            auto result = hpx::when_each(HPX_FORWARD(F, f));
            result.wait();
            hpx::detail::throw_if_exceptional(HPX_MOVE(result));
        }

        template <typename F, typename... Ts,
            typename Enable =
                std::enable_if_t<!traits::is_future_v<std::decay_t<F>> &&
                    util::all_of_v<traits::is_future<Ts>...>>>
        friend void tag_invoke(wait_each_t, F&& f, Ts&&... ts)
        {
            auto result =
                hpx::when_each(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            result.wait();
            hpx::detail::throw_if_exceptional(HPX_MOVE(result));
        }
    } wait_each{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_each_n_nothrow_t final
      : hpx::functional::tag<wait_each_n_nothrow_t>
    {
    private:
        template <typename F, typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend void tag_invoke(
            wait_each_n_nothrow_t, F&& f, Iterator begin, std::size_t count)
        {
            hpx::when_each_n(HPX_FORWARD(F, f), begin, count).wait();
        }
    } wait_each_n_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_each_n_t final
      : hpx::functional::tag<wait_each_n_t>
    {
    private:
        template <typename F, typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend void tag_invoke(
            wait_each_n_t, F&& f, Iterator begin, std::size_t count)
        {
            auto result = hpx::when_each_n(HPX_FORWARD(F, f), begin, count);
            result.wait();
            hpx::detail::throw_if_exceptional(HPX_MOVE(result));
        }
    } wait_each_n{};    // namespace hpx
}    // namespace hpx

#endif    // DOXYGEN
