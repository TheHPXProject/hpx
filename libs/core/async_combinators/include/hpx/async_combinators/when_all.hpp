//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file when_all.hpp
/// \page hpx::when_all
/// \headerfile hpx/future.hpp

#pragma once

#if defined(DOXYGEN)
/// Top level HPX namespace
namespace hpx {
    /// \brief function \a when_all creates a future object that becomes ready
    ///        when all elements in a set of \a future and \a shared_future
    ///        objects become ready. It is an operator allowing to join on the
    ///        result of all given futures. It AND-composes all given future
    ///        objects and returns a new future object representing the same
    ///        list of futures after they finished executing.
    ///
    /// \param first    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a when_all should wait.
    /// \param last     [in] The iterator pointing to the last element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a when_all should wait.
    ///
    /// \return   Returns a future holding the same list of futures as has
    ///           been passed to \a when_all.
    ///           - future<Container<future<R>>>: If the input cardinality is
    ///             unknown at compile time and the futures are all have the
    ///             same type. The order of the futures in the output container
    ///             will be the same as given by the input iterator.
    ///
    /// \note Calling this version of \a when_all where first == last, returns
    ///       a future with an empty container that is immediately ready.
    ///       Each future and shared_future is waited upon and then copied into
    ///       the collection of the output (returned) future, maintaining the
    ///       order of the futures in the input collection.
    ///       The future returned by \a when_all will not throw an exception,
    ///       but the futures held in the output collection may.
    template <typename InputIter,
        typename Container = vector<
            future<typename std::iterator_traits<InputIter>::value_type>>>
    hpx::future<Container> when_all(InputIter first, InputIter last);

    /// \copybrief when_all(InputIter first, InputIter last)
    ///
    /// \param values   [in] A range holding an arbitrary amount of \a future
    ///                 or \a shared_future objects for which \a when_all
    ///                 should wait.
    ///
    /// \return   Returns a future holding the same list of futures as has
    ///           been passed to when_all.
    ///           - future<Container<future<R>>>: If the input cardinality is
    ///             unknown at compile time and the futures are all have the
    ///             same type.
    ///
    /// \note Calling this version of \a when_all where the input container is
    ///       empty, returns a future with an empty container that is immediately
    ///       ready.
    ///       Each future and shared_future is waited upon and then copied into
    ///       the collection of the output (returned) future, maintaining the
    ///       order of the futures in the input collection.
    ///       The future returned by \a when_all will not throw an exception,
    ///       but the futures held in the output collection may.
    template <typename Range>
    hpx::future<Range> when_all(Range&& values);

    /// \copybrief when_all(InputIter first, InputIter last)
    ///
    /// \param futures  [in] An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a when_all should wait.
    ///
    /// \return   Returns a future holding the same list of futures as has
    ///           been passed to \a when_all.
    ///           - future<tuple<future<T0>, future<T1>, future<T2>...>>: If
    ///             inputs are fixed in number and are of heterogeneous types.
    ///             The inputs can be any arbitrary number of future objects.
    ///           - future<tuple<>> if \a when_all is called with zero arguments.
    ///             The returned future will be initially ready.
    ///
    /// \note Each future and shared_future is waited upon and then copied into
    ///       the collection of the output (returned) future, maintaining the
    ///       order of the futures in the input collection.
    ///       The future returned by \a when_all will not throw an exception,
    ///       but the futures held in the output collection may.
    template <typename... T>
    hpx::future<hpx::tuple<hpx::future<T>...>> when_all(T&&... futures);

    /// \copybrief when_all(InputIter first, InputIter last)
    ///
    /// \param begin    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_n should wait.
    /// \param count    [in] The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \return   Returns a future holding the same list of futures as has
    ///           been passed to \a when_all_n.
    ///           - future<Container<future<R>>>: If the input cardinality is
    ///             unknown at compile time and the futures are all have the
    ///             same type. The order of the futures in the output vector
    ///             will be the same as given by the input iterator.
    ///
    /// \throws This function will throw errors which are encountered while
    ///         setting up the requested operation only. Errors encountered
    ///         while executing the operations delivering the results to be
    ///         stored in the futures are reported through the futures
    ///         themselves.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise, it throws an instance of
    ///           hpx::exception.
    ///
    /// \note     None of the futures in the input sequence are invalidated.
    template <typename InputIter,
        typename Container = vector<
            future<typename std::iterator_traits<InputIter>::value_type>>>
    hpx::future<Container> when_all_n(InputIter begin, std::size_t count);
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct when_all_result
    {
        using type = T;

        static type call(T&& t) noexcept
        {
            return HPX_MOVE(t);
        }
    };

    template <typename T>
    struct when_all_result<hpx::tuple<T>,
        std::enable_if_t<hpx::traits::is_future_range_v<T>>>
    {
        using type = T;

        static type call(hpx::tuple<T>&& t) noexcept
        {
            return HPX_MOVE(hpx::get<0>(t));
        }
    };

    template <typename T>
    using when_all_result_t = typename when_all_result<T>::type;

    template <typename Tuple>
    class async_when_all_frame : public future_data<when_all_result_t<Tuple>>
    {
    public:
        using result_type = when_all_result_t<Tuple>;
        using type = hpx::future<result_type>;
        using base_type = hpx::lcos::detail::future_data<result_type>;

        explicit async_when_all_frame(
            typename base_type::init_no_addref no_addref) noexcept
          : base_type(no_addref)
        {
        }

        template <typename T>
        auto operator()(hpx::util::async_traverse_visit_tag, T&& current)
            -> decltype(async_visit_future(HPX_FORWARD(T, current)))
        {
            return async_visit_future(HPX_FORWARD(T, current));
        }

        // clang-format off
        template <typename T, typename N>
        auto operator()(hpx::util::async_traverse_detach_tag, T&& current,
            N&& next) -> decltype(async_detach_future(HPX_FORWARD(T, current),
                          HPX_FORWARD(N, next)))
        {
            return async_detach_future(
                HPX_FORWARD(T, current), HPX_FORWARD(N, next));
        }
        // clang-format on

        template <typename T>
        void operator()(hpx::util::async_traverse_complete_tag, T&& pack)
        {
            this->set_data(when_all_result<Tuple>::call(HPX_FORWARD(T, pack)));
        }
    };

    template <typename... T>
    typename async_when_all_frame<
        hpx::tuple<hpx::traits::acquire_future_t<T>...>>::type
    when_all_impl(T&&... args)
    {
        using result_type = hpx::tuple<hpx::traits::acquire_future_t<T>...>;
        using frame_type = async_when_all_frame<result_type>;
        using no_addref = typename frame_type::base_type::init_no_addref;

        using allocator_type = hpx::util::thread_local_caching_allocator<
            hpx::lockfree::variable_size_stack,
            hpx::util::internal_allocator<>>;
        auto frame = hpx::util::traverse_pack_async_allocator(allocator_type{},
            hpx::util::async_traverse_in_place_tag<frame_type>{}, no_addref{},
            hpx::traits::acquire_future_disp()(HPX_FORWARD(T, args))...);

        return hpx::traits::future_access<typename frame_type::type>::create(
            HPX_MOVE(frame));
    }
}    // namespace hpx::lcos::detail

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct when_all_t final
      : hpx::functional::tag<when_all_t>
    {
    private:
        // different versions of clang-format disagree
        // clang-format off
        template <typename... Args>
        friend auto tag_invoke(when_all_t, Args&&... args) -> decltype(
            hpx::lcos::detail::when_all_impl(HPX_FORWARD(Args, args)...))
        // clang-format on
        {
            return hpx::lcos::detail::when_all_impl(HPX_FORWARD(Args, args)...);
        }

        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend decltype(auto) tag_invoke(
            when_all_t, Iterator begin, Iterator end)
        {
            using container_type = std::vector<
                hpx::lcos::detail::future_iterator_traits_t<Iterator>>;
            return hpx::lcos::detail::when_all_impl(
                hpx::lcos::detail::acquire_future_iterators<Iterator,
                    container_type>(begin, end));
        }

        friend auto tag_invoke(when_all_t)
        {
            return hpx::make_ready_future(hpx::tuple<>());
        }
    } when_all{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct when_all_n_t final
      : hpx::functional::tag<when_all_n_t>
    {
    private:
        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend decltype(auto) tag_invoke(
            when_all_n_t, Iterator begin, std::size_t count)
        {
            using container_type = std::vector<
                hpx::lcos::detail::future_iterator_traits_t<Iterator>>;
            return hpx::lcos::detail::when_all_impl(
                hpx::lcos::detail::acquire_future_n<Iterator, container_type>(
                    begin, count));
        }
    } when_all_n{};
}    // namespace hpx

#endif    // DOXYGEN
