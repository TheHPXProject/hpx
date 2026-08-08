//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file wait_all.hpp
/// \page hpx::wait_all, hpx::wait_all_nothrow, hpx::wait_all_n, hpx::wait_all_n_nothrow
/// \headerfile hpx/future.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param first    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all should wait.
    /// \param last     The iterator pointing to the element after the last one
    ///                 of a sequence of \a future or \a shared_future objects
    ///                 for which \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         alive, unmoved, and unmodified until wait_all returns and all
    ///         inspection of the futures is complete.
    template <typename InputIter>
    void wait_all(InputIter first, InputIter last);

    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         alive, unmoved, and unmodified until wait_all returns and all
    ///         inspection of the futures is complete.
    template <typename R>
    void wait_all(std::vector<future<R>>&& futures);

    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         alive, unmoved, and unmodified until wait_all returns and all
    ///         inspection of the futures is complete.
    template <typename R, std::size_t N>
    void wait_all(std::array<future<R>, N>&& futures);

    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param f        A \a future or \a shared_future for which
    ///                 \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after the future has become
    ///       ready. The input future is still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the future while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    template <typename T>
    void wait_all(hpx::future<T> const& f);

    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param futures  An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    template <typename... T>
    void wait_all(T&&... futures);

    /// The function \a wait_all_n is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_n should wait.
    /// \param count    The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \return         The function \a wait_all_n will return an iterator
    ///                 referring to the first element in the input sequence
    ///                 after the last processed element.
    ///
    /// \note The function \a wait_all_n returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all_n
    ///       returns.
    ///
    /// \note           The function wait_all_n will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_n_nothrow
    ///                 instead.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         alive, unmoved, and unmodified until wait_all returns and all
    ///         inspection of the futures is complete.
    template <typename InputIter>
    void wait_all_n(InputIter begin, std::size_t count);

    /// The function \a wait_all_nothrow is an operator allowing to join on the
    /// result of all given futures. It AND-composes all future objects given
    /// and returns after they finished executing, similar to \a wait_all. It
    /// differs from \a wait_all in that it will not rethrow any exceptions
    /// captured by the given futures while they became ready.
    ///
    /// \param first    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_nothrow should wait.
    /// \param last     The iterator pointing to the element after the last one
    ///                 of a sequence of \a future or \a shared_future objects
    ///                 for which \a wait_all_nothrow should wait.
    ///
    /// \return         Returns \a true if any of the given futures held an
    ///                 exception once it became ready, and \a false otherwise.
    ///
    /// \note The function \a wait_all_nothrow returns after all futures have
    ///       become ready. All input futures are still valid after
    ///       \a wait_all_nothrow returns.
    ///
    /// \note           Unlike \a wait_all, this function will not rethrow any
    ///                 exceptions captured by the futures while becoming ready.
    ///                 Any such exceptions are not rethrown by this call; the
    ///                 caller can use the returned \a bool to detect their
    ///                 presence.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         alive, unmoved, and unmodified until wait_all returns and all
    ///         inspection of the futures is complete.
    template <typename InputIter>
    bool wait_all_nothrow(InputIter first, InputIter last);

    /// The function \a wait_all_nothrow is an operator allowing to join on the
    /// result of all given futures. It AND-composes all future objects given
    /// and returns after they finished executing, similar to \a wait_all. It
    /// differs from \a wait_all in that it will not rethrow any exceptions
    /// captured by the given futures while they became ready.
    ///
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all_nothrow should wait.
    ///
    /// \return         Returns \a true if any of the given futures held an
    ///                 exception once it became ready, and \a false otherwise.
    ///
    /// \note The function \a wait_all_nothrow returns after all futures have
    ///       become ready. All input futures are still valid after
    ///       \a wait_all_nothrow returns.
    ///
    /// \note           Unlike \a wait_all, this function will not rethrow any
    ///                 exceptions captured by the futures while becoming ready.
    ///                 Any such exceptions are not rethrown by this call; the
    ///                 caller can use the returned \a bool to detect their
    ///                 presence.
    ///
    template <typename R>
    bool wait_all_nothrow(std::vector<future<R>>&& futures);

    /// The function \a wait_all_nothrow is an operator allowing to join on the
    /// result of all given futures. It AND-composes all future objects given
    /// and returns after they finished executing, similar to \a wait_all. It
    /// differs from \a wait_all in that it will not rethrow any exceptions
    /// captured by the given futures while they became ready.
    ///
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all_nothrow should wait.
    ///
    /// \return         Returns \a true if any of the given futures held an
    ///                 exception once it became ready, and \a false otherwise.
    ///
    /// \note The function \a wait_all_nothrow returns after all futures have
    ///       become ready. All input futures are still valid after
    ///       \a wait_all_nothrow returns.
    ///
    /// \note           Unlike \a wait_all, this function will not rethrow any
    ///                 exceptions captured by the futures while becoming ready.
    ///                 Any such exceptions are not rethrown by this call; the
    ///                 caller can use the returned \a bool to detect their
    ///                 presence.
    ///
    template <typename R, std::size_t N>
    bool wait_all_nothrow(std::array<future<R>, N>&& futures);

    /// The function \a wait_all_nothrow is an operator allowing to join on the
    /// result of all given futures. It AND-composes all future objects given
    /// and returns after they finished executing, similar to \a wait_all. It
    /// differs from \a wait_all in that it will not rethrow any exceptions
    /// captured by the given future while it became ready.
    ///
    /// \param f        A \a future or \a shared_future for which
    ///                 \a wait_all_nothrow should wait.
    ///
    /// \return         Returns \a true if the given future held an exception
    ///                 once it became ready, and \a false otherwise.
    ///
    /// \note The function \a wait_all_nothrow returns after the future has
    ///       become ready. The input future is still valid after
    ///       \a wait_all_nothrow returns.
    ///
    /// \note           Unlike \a wait_all, this function will not rethrow any
    ///                 exception captured by the future while becoming ready.
    ///                 Any such exception is silently discarded; the caller can
    ///                 use the returned \a bool to detect its presence.
    ///
    template <typename T>
    bool wait_all_nothrow(hpx::future<T> const& f);

    /// The function \a wait_all_nothrow is an operator allowing to join on the
    /// result of all given futures. It AND-composes all future objects given
    /// and returns after they finished executing, similar to \a wait_all. It
    /// differs from \a wait_all in that it will not rethrow any exceptions
    /// captured by the given futures while they became ready.
    ///
    /// \param futures  An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_all_nothrow should wait.
    ///
    /// \return         Returns \a true if any of the given futures held an
    ///                 exception once it became ready, and \a false otherwise.
    ///
    /// \note The function \a wait_all_nothrow returns after all futures have
    ///       become ready. All input futures are still valid after
    ///       \a wait_all_nothrow returns.
    ///
    /// \note           Unlike \a wait_all, this function will not rethrow any
    ///                 exceptions captured by the futures while becoming ready.
    ///                 Any such exceptions are not rethrown by this call; the
    ///                 caller can use the returned \a bool to detect their
    ///                 presence.
    ///
    template <typename... T>
    bool wait_all_nothrow(T&&... futures);

    /// The function \a wait_all_n_nothrow is an operator allowing to join on
    /// the result of all given futures. It AND-composes all future objects
    /// given and returns after they finished executing, similar to
    /// \a wait_all_n. It differs from \a wait_all_n in that it will not
    /// rethrow any exceptions captured by the given futures while they became
    /// ready.
    ///
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_n_nothrow should wait.
    /// \param count    The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \return         The function \a wait_all_n_nothrow returns a pair
    ///                 consisting of an iterator referring to the first element
    ///                 in the input sequence after the last processed element,
    ///                 and a \a bool that is \a true if any of the given
    ///                 futures held an exception once it became ready, and \a
    ///                 false otherwise.
    ///
    /// \note The function \a wait_all_n_nothrow returns after all futures have
    ///       become ready. All input futures are still valid after
    ///       \a wait_all_n_nothrow returns.
    ///
    /// \note           Unlike \a wait_all_n, this function will not rethrow
    ///                 any exceptions captured by the futures while becoming
    ///                 ready. Any such exceptions are not rethrown by this
    ///                 call; the caller can use the returned \a bool to detect
    ///                 their presence.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         alive, unmoved, and unmodified until wait_all returns and all
    ///         inspection of the futures is complete.
    template <typename InputIter>
    std::pair<InputIter, bool> wait_all_n_nothrow(
        InputIter begin, std::size_t count);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/async_combinators/detail/throw_if_exceptional.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#if !defined(HPX_INTEL_VERSION)
#define HPX_WAIT_ALL_FORCEINLINE HPX_FORCEINLINE
#else
#define HPX_WAIT_ALL_FORCEINLINE
#endif

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////
    template <typename Future, typename Enable = void>
    struct is_future_or_shared_state : traits::is_future<Future>
    {
    };

    template <typename R>
    struct is_future_or_shared_state<
        hpx::intrusive_ptr<hpx::lcos::detail::future_data_base<R>>>
      : std::true_type
    {
    };

    template <typename R>
    struct is_future_or_shared_state<std::reference_wrapper<R>>
      : is_future_or_shared_state<R>
    {
    };

    template <typename R>
    inline constexpr bool is_future_or_shared_state_v =
        is_future_or_shared_state<R>::value;

    ///////////////////////////////////////////////////////////////////////
    template <typename Range, typename Enable = void>
    struct is_future_or_shared_state_range : std::false_type
    {
    };

    template <typename T>
    struct is_future_or_shared_state_range<std::vector<T>>
      : is_future_or_shared_state<T>
    {
    };

    template <typename T, std::size_t N>
    struct is_future_or_shared_state_range<std::array<T, N>>
      : is_future_or_shared_state<T>
    {
    };

    template <typename R>
    inline constexpr bool is_future_or_shared_state_range_v =
        is_future_or_shared_state_range<R>::value;

    ///////////////////////////////////////////////////////////////////////
    template <typename Future, typename Enable = void>
    struct future_or_shared_state_result;

    template <typename Future>
    struct future_or_shared_state_result<Future,
        std::enable_if_t<hpx::traits::is_future_v<Future>>>
      : hpx::traits::future_traits<Future>
    {
    };

    template <typename R>
    struct future_or_shared_state_result<
        hpx::intrusive_ptr<hpx::lcos::detail::future_data_base<R>>>
    {
        using type = R;
    };

    template <typename R>
    using future_or_shared_state_result_t =
        future_or_shared_state_result<R>::type;

    ///////////////////////////////////////////////////////////////////////
    template <typename Tuple, typename Derived = void>
    struct wait_all_frame    //-V690
      : hpx::lcos::detail::future_data<void>
    {
    private:
        using base_type = hpx::lcos::detail::future_data<void>;
        using init_no_addref = base_type::init_no_addref;

        using derived_type = std::conditional_t<std::is_void_v<Derived>,
            wait_all_frame<Tuple>, Derived>;

        derived_type& derived()
        {
            return static_cast<derived_type&>(*this);
        }
        derived_type const& derived() const
        {
            return static_cast<derived_type const&>(*this);
        }

        wait_all_frame(wait_all_frame const&) = delete;
        wait_all_frame(wait_all_frame&&) = delete;

        wait_all_frame& operator=(wait_all_frame const&) = delete;
        wait_all_frame& operator=(wait_all_frame&&) = delete;

        template <std::size_t I>
        struct is_end
          : std::integral_constant<bool, hpx::tuple_size_v<Tuple> == I>
        {
        };

        template <std::size_t I>
        static constexpr bool is_end_v = is_end<I>::value;

    public:
        explicit wait_all_frame(Tuple const& t)
          : base_type(init_no_addref{})
          , t_(t)
        {
        }

        explicit wait_all_frame(Tuple&& t) noexcept
          : base_type(init_no_addref{})
          , t_(HPX_MOVE(t))
        {
        }

    protected:
        // Current element is a range (vector or array) of futures
        template <std::size_t I, typename Iter>
        void await_range(Iter next, Iter end)
        {
            hpx::intrusive_ptr<wait_all_frame> this_(this);
            for (/**/; next != end && derived().continue_waiting(); ++next)
            {
                auto next_future_data =
                    hpx::traits::detail::get_shared_state(*next);

                if (next_future_data)
                {
                    if (!next_future_data->is_ready(std::memory_order_relaxed))
                    {
                        next_future_data->execute_deferred();

                        // execute_deferred might have made the future ready
                        if (!next_future_data->is_ready(
                                std::memory_order_relaxed))
                        {
                            // Attach a continuation to this future which will
                            // re-evaluate it and continue to the next element
                            // in the sequence (if any).
                            next_future_data->set_on_completed(
                                [this_ = HPX_MOVE(this_), next, end]() mutable {
                                    this_->template await_range<I>(
                                        HPX_MOVE(next), HPX_MOVE(end));
                                });

                            // explicitly destruct iterators as those might
                            // become dangling after we make ourselves ready
                            next = std::decay_t<Iter>{};
                            end = std::decay_t<Iter>{};

                            return;
                        }
                    }

                    // check whether the current future is exceptional
                    if (next_future_data->has_exception())
                    {
                        has_exceptional_results_.store(
                            true, std::memory_order_release);
                    }
                }
            }

            // explicitly destruct iterators as those might become dangling
            // after we make ourselves ready
            next = std::decay_t<Iter>{};
            end = std::decay_t<Iter>{};

            // All elements of the sequence are ready now, proceed to the next
            // argument.
            do_await<I + 1>();
        }

        template <std::size_t I>
        HPX_FORCEINLINE void await_range()
        {
            await_range<I>(
                hpx::util::begin(hpx::util::unwrap_ref(hpx::get<I>(t_))),
                hpx::util::end(hpx::util::unwrap_ref(hpx::get<I>(t_))));
        }

        // Current element is a simple future
        template <std::size_t I>
        HPX_FORCEINLINE void await_future()
        {
            if (!derived().continue_waiting())
            {
                return;
            }

            hpx::intrusive_ptr<wait_all_frame> this_(this);
            auto next_future_data =
                hpx::traits::detail::get_shared_state(hpx::get<I>(t_));

            if (next_future_data)
            {
                if (!next_future_data->is_ready(std::memory_order_relaxed))
                {
                    next_future_data->execute_deferred();

                    // execute_deferred might have made the future ready
                    if (!next_future_data->is_ready(std::memory_order_relaxed))
                    {
                        // Attach a continuation to this future which will
                        // re-evaluate it and continue to the next argument (if
                        // any).
                        next_future_data->set_on_completed(
                            [this_ = HPX_MOVE(this_)]() -> void {
                                this_->template await_future<I>();
                            });

                        return;
                    }
                }

                // check whether the current future is exceptional
                if (next_future_data->has_exception())
                {
                    has_exceptional_results_.store(
                        true, std::memory_order_release);
                }
            }

            do_await<I + 1>();
        }

        template <std::size_t I>
        HPX_FORCEINLINE void do_await()
        {
            if (!derived().continue_waiting())
            {
                return;
            }

            // Check if end of the tuple is reached
            if constexpr (is_end_v<I>)
            {
                // simply make ourselves ready
                this->set_data(util::unused);
            }
            else
            {
                using future_type =
                    hpx::util::decay_unwrap_t<hpx::tuple_element_t<I, Tuple>>;

                if constexpr (is_future_or_shared_state_v<future_type>)
                {
                    await_future<I>();
                }
                else
                {
                    static_assert(
                        is_future_or_shared_state_range_v<future_type>,
                        "element must be future or range of futures");
                    await_range<I>();
                }
            }
        }

    public:
        bool wait_all()
        {
            do_await<0>();

            // If there are still futures which are not ready, suspend
            // and wait.
            if (!this->is_ready(std::memory_order_relaxed))
            {
                this->wait();
            }

            // return whether at least one of the futures has become
            // exceptional
            return has_exceptional_results_.load(std::memory_order_acquire);
        }

        /// Returns whether at least one of the awaited futures completed with
        /// an exception.
        bool has_exceptional_results() const noexcept
        {
            return has_exceptional_results_.load(std::memory_order_acquire);
        }

        /// Extension point allowing derived frames to abort waiting early
        /// (e.g. on timeout). Returning false from this function stops
        /// further waiting/traversal of the remaining futures.
        constexpr static bool continue_waiting() noexcept
        {
            return true;
        }

    private:
        Tuple t_;
        std::atomic<bool> has_exceptional_results_ = false;
    };
}    // namespace hpx::detail

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_nothrow_t final
    {
    private:
        template <typename Future>
        static bool wait_all_nothrow_impl(std::vector<Future> const& values)
        {
            if (!values.empty())
            {
                using result_type = hpx::tuple<std::vector<Future> const&>;
                using frame_type = hpx::detail::wait_all_frame<result_type>;

                result_type data(values);

                // frame is initialized with initial reference count
                hpx::intrusive_ptr<frame_type> frame(
                    new frame_type(data), false);
                return frame->wait_all();
            }
            return false;
        }

        template <typename Future, std::size_t N>
        static bool wait_all_nothrow_impl(std::array<Future, N> const& values)
        {
            using result_type = hpx::tuple<std::array<Future, N> const&>;
            using frame_type = hpx::detail::wait_all_frame<result_type>;

            result_type data(values);

            // frame is initialized with initial reference count
            hpx::intrusive_ptr<frame_type> frame(new frame_type(data), false);
            return frame->wait_all();
        }

    public:
        template <typename Future>
        bool operator()(std::vector<Future> const& values) const
        {
            return wait_all_nothrow_t::wait_all_nothrow_impl(values);
        }

        template <typename Future>
        HPX_WAIT_ALL_FORCEINLINE bool operator()(
            std::vector<Future>& values) const
        {
            return wait_all_nothrow_t::wait_all_nothrow_impl(
                const_cast<std::vector<Future> const&>(values));
        }

        template <typename Future>
        HPX_WAIT_ALL_FORCEINLINE bool operator()(
            std::vector<Future>&& values) const
        {
            return wait_all_nothrow_t::wait_all_nothrow_impl(
                const_cast<std::vector<Future> const&>(values));
        }

        template <typename Future, std::size_t N>
        bool operator()(std::array<Future, N> const& values) const
        {
            return wait_all_nothrow_t::wait_all_nothrow_impl(values);
        }

        template <typename Future, std::size_t N>
        HPX_WAIT_ALL_FORCEINLINE bool operator()(
            std::array<Future, N>& values) const
        {
            return wait_all_nothrow_t::wait_all_nothrow_impl(
                const_cast<std::array<Future, N> const&>(values));
        }

        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        bool operator()(Iterator begin, Iterator end) const
        {
            if (begin == end)
            {
                return false;
            }

            auto values = traits::acquire_shared_state<Iterator>()(begin, end);
            return wait_all_nothrow_t::wait_all_nothrow_impl(values);
        }

        HPX_WAIT_ALL_FORCEINLINE constexpr bool operator()() const noexcept
        {
            return false;
        }

        template <typename... Ts>
        bool operator()(Ts&&... ts) const
        {
            if constexpr (sizeof...(Ts) != 0)
            {
                using result_type =
                    hpx::tuple<traits::detail::shared_state_ptr_for_t<Ts>...>;
                using frame_type = detail::wait_all_frame<result_type>;

                result_type values =
                    result_type(hpx::traits::detail::get_shared_state(ts)...);

                // frame is initialized with initial reference count
                hpx::intrusive_ptr<frame_type> frame(
                    new frame_type(values), false);
                return frame->wait_all();
            }
            else
            {
                return false;
            }
        }

        template <typename T>
        HPX_WAIT_ALL_FORCEINLINE bool operator()(hpx::future<T> const& f) const
        {
            f.wait();
            return f.has_exception();
        }

        template <typename T>
        HPX_WAIT_ALL_FORCEINLINE bool operator()(
            hpx::shared_future<T> const& f) const
        {
            f.wait();
            return f.has_exception();
        }
    } wait_all_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_t final
    {
        template <typename Future>
        HPX_WAIT_ALL_FORCEINLINE void operator()(
            std::vector<Future> const& values) const
        {
            if (hpx::wait_all_nothrow(values))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future>
        HPX_WAIT_ALL_FORCEINLINE void operator()(
            std::vector<Future>& values) const
        {
            if (hpx::wait_all_nothrow(
                    const_cast<std::vector<Future> const&>(values)))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future>
        HPX_WAIT_ALL_FORCEINLINE void operator()(
            std::vector<Future>&& values) const
        {
            if (hpx::wait_all_nothrow(
                    const_cast<std::vector<Future> const&>(values)))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future, std::size_t N>
        HPX_WAIT_ALL_FORCEINLINE void operator()(
            std::array<Future, N> const& values) const
        {
            if (hpx::wait_all_nothrow(values))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future, std::size_t N>
        HPX_WAIT_ALL_FORCEINLINE void operator()(
            std::array<Future, N>& values) const
        {
            if (hpx::wait_all_nothrow(
                    const_cast<std::array<Future, N> const&>(values)))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future, std::size_t N>
        HPX_WAIT_ALL_FORCEINLINE void operator()(
            std::array<Future, N>&& values) const
        {
            if (hpx::wait_all_nothrow(
                    const_cast<std::array<Future, N> const&>(values)))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        void operator()(Iterator begin, Iterator end) const
        {
            if (begin != end)
            {
                auto values =
                    traits::acquire_shared_state<Iterator>()(begin, end);
                if (hpx::wait_all_nothrow(values))
                {
                    hpx::detail::throw_if_exceptional(values);
                }
            }
        }

        HPX_WAIT_ALL_FORCEINLINE void operator()() const noexcept {}

        template <typename... Ts>
        HPX_WAIT_ALL_FORCEINLINE void operator()(Ts&&... ts) const
        {
            if (hpx::wait_all_nothrow(ts...))
            {
                hpx::detail::throw_if_exceptional(HPX_FORWARD(Ts, ts)...);
            }
        }

        template <typename T>
        HPX_WAIT_ALL_FORCEINLINE void operator()(hpx::future<T> const& f) const
        {
            if (hpx::wait_all_nothrow(f))
            {
                hpx::detail::throw_if_exceptional(f);
            }
        }

        template <typename T>
        HPX_WAIT_ALL_FORCEINLINE void operator()(
            hpx::shared_future<T> const& f) const
        {
            if (hpx::wait_all_nothrow(f))
            {
                hpx::detail::throw_if_exceptional(f);
            }
        }
    } wait_all{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_n_nothrow_t final
    {
        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        bool operator()(Iterator begin, std::size_t count) const
        {
            if (count == 0)
            {
                return false;
            }

            auto values =
                traits::acquire_shared_state<Iterator>()(begin, count);
            return hpx::wait_all_nothrow(values);
        }
    } wait_all_n_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_n_t final
    {
        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        void operator()(Iterator begin, std::size_t count) const
        {
            if (count != 0)
            {
                auto values =
                    traits::acquire_shared_state<Iterator>()(begin, count);
                if (hpx::wait_all_nothrow(values))
                {
                    hpx::detail::throw_if_exceptional(values);
                }
            }
        }
    } wait_all_n{};
}    // namespace hpx

#undef HPX_WAIT_ALL_FORCEINLINE

#endif    // DOXYGEN
