//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file wait_some.hpp
/// \page hpx::wait_some
/// \headerfile hpx/future.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// The function \a wait_some is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns a new future object representing the same list of futures
    /// after n of them finished executing.
    ///
    /// \param n        [in] The number of futures out of the arguments which
    ///                 have to become ready in order for the function to return.
    /// \param first    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a when_all should wait.
    /// \param last     [in] The iterator pointing to the last element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a when_all should wait.
    ///
    /// \note The function \a wait_some returns after \a n futures have become
    ///       ready. All input futures are still valid after \a wait_some
    ///       returns.
    ///
    /// \note           The function wait_some will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_some_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_some(std::size_t n, InputIter first, InputIter last);

    /// The function \a wait_some is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns a new future object representing the same list of futures
    /// after n of them finished executing.
    ///
    /// \param n        [in] The number of futures out of the arguments which
    ///                 have to become ready in order for the returned future
    ///                 to get ready.
    /// \param futures  [in] A vector holding an arbitrary amount of \a future
    ///                 or \a shared_future objects for which \a wait_some
    ///                 should wait.
    ///
    /// \note The function \a wait_some returns after \a n futures have become
    ///       ready. All input futures are still valid after \a wait_some
    ///       returns.
    ///
    /// \note           The function wait_some will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_some_nothrow
    ///                 instead.
    ///
    template <typename R>
    void wait_some(std::size_t n, std::vector<future<R>>&& futures);

    /// The function \a wait_some is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns a new future object representing the same list of futures
    /// after n of them finished executing.
    ///
    /// \param n        [in] The number of futures out of the arguments which
    ///                 have to become ready in order for the returned future
    ///                 to get ready.
    /// \param futures  [in] An array holding an arbitrary amount of \a future
    ///                 or \a shared_future objects for which \a wait_some
    ///                 should wait.
    ///
    /// \note The function \a wait_some returns after \a n futures have become
    ///       ready. All input futures are still valid after \a wait_some
    ///       returns.
    ///
    /// \note           The function wait_some will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_some_nothrow
    ///                 instead.
    ///
    template <typename R, std::size_t N>
    void wait_some(std::size_t n, std::array<future<R>, N>&& futures);

    /// The function \a wait_some is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns a new future object representing the same list of futures
    /// after n of them finished executing.
    ///
    /// \param n        [in] The number of futures out of the arguments which
    ///                 have to become ready in order for the returned future
    ///                 to get ready.
    /// \param futures  [in] An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_some should wait.
    ///
    /// \note The function \a wait_all returns after \a n futures have become
    ///       ready. All input futures are still valid after \a wait_some
    ///       returns.
    ///
    /// \note           The function wait_some will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_some_nothrow
    ///                 instead.
    ///
    template <typename... T>
    void wait_some(std::size_t n, T&&... futures);

    /// The function \a wait_some_n is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns a new future object representing the same list of futures
    /// after n of them finished executing.
    ///
    /// \param n        [in] The number of futures out of the arguments which
    ///                 have to become ready in order for the returned future
    ///                 to get ready.
    /// \param first    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a when_all should wait.
    /// \param count    [in] The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \note The function \a wait_some_n returns after \a n futures have become
    ///       ready. All input futures are still valid after \a wait_some_n
    ///       returns.
    ///
    /// \note           The function wait_some_n will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_some_n_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_some_n(std::size_t n, InputIter first, std::size_t count);
}    // namespace hpx
#else

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_combinators/detail/throw_if_exceptional.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Sequence>
        struct wait_some;

        template <typename Sequence>
        struct set_wait_some_callback_impl
        {
            explicit set_wait_some_callback_impl(
                wait_some<Sequence>& wait) noexcept
              : wait_(wait)
            {
            }

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 140000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
            template <typename SharedState>
            void operator()(SharedState const& shared_state) const
            {
                hpx::intrusive_ptr<wait_some<Sequence>> this_ = &wait_;
                if constexpr (!traits::is_shared_state_v<SharedState>)
                {
                    apply(shared_state);
                }
                else
                {
                    std::size_t counter =
                        wait_.count_.load(std::memory_order_relaxed);

                    if (counter < wait_.needed_count_ && shared_state)
                    {
                        if (!shared_state->is_ready(std::memory_order_relaxed))
                        {
                            // handle future only if not enough futures are
                            // ready yet also, do not touch any futures that are
                            // already ready
                            shared_state->execute_deferred();

                            // execute_deferred might have made the future ready
                            if (!shared_state->is_ready(
                                    std::memory_order_relaxed))
                            {
                                auto state = shared_state;
                                shared_state->set_on_completed(
                                    [this_ = HPX_MOVE(this_),
                                        state = HPX_MOVE(state)] {
                                        this_->on_future_ready(
                                            state->has_exception());
                                    });
                                return;
                            }
                        }
                    }

                    wait_.on_future_ready(shared_state->has_exception());
                }
            }
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 140000
#pragma GCC diagnostic pop
#endif

            template <typename Tuple, std::size_t... Is>
            HPX_FORCEINLINE void apply(
                Tuple const& tuple, hpx::util::index_pack<Is...>) const
            {
                ((*this)(hpx::get<Is>(tuple)), ...);
            }

            template <typename... Ts>
            HPX_FORCEINLINE void apply(hpx::tuple<Ts...> const& sequence) const
            {
                apply(sequence, hpx::util::make_index_pack_t<sizeof...(Ts)>());
            }

            template <typename Sequence_>
            HPX_FORCEINLINE void apply(Sequence_ const& sequence) const
            {
                std::for_each(sequence.begin(), sequence.end(), *this);
            }

            wait_some<Sequence>& wait_;
        };

        template <typename Sequence>
        void set_on_completed_callback(wait_some<Sequence>& wait)
        {
            set_wait_some_callback_impl<Sequence> callback(wait);
            callback.apply(wait.values_);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Sequence>
        struct wait_some
        {
            void on_future_ready(bool has_exception)
            {
                std::unique_lock l(mtx_.data_);

                // check whether the current future is exceptional
                if (has_exception)
                {
                    has_exceptional_results_ = true;
                }

                if (count_.fetch_add(1) + 1 == needed_count_)
                {
                    HPX_ASSERT_LOCKED(l, !notified_);
                    notified_ = true;
                    cond_.data_.notify_all(HPX_MOVE(l));
                }
            }

        private:
            wait_some(wait_some const&) = delete;
            wait_some(wait_some&&) = delete;

            wait_some& operator=(wait_some const&) = delete;
            wait_some& operator=(wait_some&&) = delete;

        public:
            template <typename S>
            wait_some(S&& values, std::size_t n) noexcept
              : values_(HPX_FORWARD(S, values))
              , count_(0)
              , needed_count_(n)
              , refcount_(1)
            {
                HPX_ASSERT_MSG(n > 0,
                    "wait_some should have to wait for at least one future to "
                    "become ready");
            }

            ~wait_some() = default;

            bool operator()()
            {
                // set callback functions to executed wait future is ready
                set_on_completed_callback(*this);

                // if all the requested futures are already set, our callback
                // above has already been called often enough, otherwise we
                // suspend ourselves
                {
                    std::unique_lock l(mtx_.data_);
                    if (count_.load(std::memory_order_acquire) < needed_count_)
                    {
                        HPX_ASSERT_LOCKED(l, !notified_);
                        cond_.data_.wait(l, "hpx::wait_some::operator()()");
                    }
                }

                // at least N futures should be ready
                HPX_ASSERT(
                    count_.load(std::memory_order_acquire) >= needed_count_);

                return has_exceptional_results_;
            }

            using argument_type =
                std::conditional_t<std::is_reference_v<Sequence>, Sequence,
                    std::decay_t<Sequence>>;

            argument_type values_;
            std::atomic<std::size_t> count_;
            std::size_t const needed_count_;
            bool has_exceptional_results_ = false;
            bool notified_ = false;

            mutable util::cache_line_data<hpx::spinlock> mtx_ = {"wait_some"};
            mutable util::cache_line_data<
                hpx::lcos::local::detail::condition_variable>
                cond_;

        private:
            friend void intrusive_ptr_add_ref(wait_some* p) noexcept
            {
                p->refcount_.increment();
            }

            friend void intrusive_ptr_release(wait_some* p) noexcept
            {
                if (0 == p->refcount_.decrement())
                {
                    // The thread that decrements the reference count to zero must
                    // perform an acquire to ensure that it doesn't start destructing
                    // the object until all previous writes have drained.
                    std::atomic_thread_fence(std::memory_order_acquire);
                    delete p;
                }
            }

            hpx::util::atomic_count refcount_;
        };

        template <typename Sequence>
        auto get_wait_some_frame(Sequence&& values, std::size_t n)
        {
            return hpx::intrusive_ptr<wait_some<Sequence>>(
                new wait_some<Sequence>(HPX_FORWARD(Sequence, values), n),
                false);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_some_nothrow_t final
      : hpx::functional::tag<wait_some_nothrow_t>
    {
    private:
        template <typename Future>
        static bool wait_some_nothrow_impl(
            std::size_t n, std::vector<Future> const& values)
        {
            static_assert(hpx::traits::is_future_v<Future>,
                "invalid use of hpx::wait_some");

            if (n == 0)
            {
                return false;
            }

            if (n > values.size())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "hpx::wait_some",
                    "number of results to wait for is out of bounds");
            }

            auto lazy_values = traits::acquire_shared_state_disp()(values);
            auto f = detail::get_wait_some_frame(HPX_MOVE(lazy_values), n);

            return (*f)();
        }

        template <typename Future>
        friend bool tag_invoke(wait_some_nothrow_t, std::size_t n,
            std::vector<Future> const& values)
        {
            return wait_some_nothrow_t::wait_some_nothrow_impl(n, values);
        }

        template <typename Future>
        friend HPX_FORCEINLINE bool tag_invoke(
            wait_some_nothrow_t, std::size_t n, std::vector<Future>& values)
        {
            return wait_some_nothrow_t::wait_some_nothrow_impl(
                n, const_cast<std::vector<Future> const&>(values));
        }

        template <typename Future>
        friend HPX_FORCEINLINE bool tag_invoke(
            wait_some_nothrow_t, std::size_t n, std::vector<Future>&& values)
        {
            return wait_some_nothrow_t::wait_some_nothrow_impl(
                n, const_cast<std::vector<Future> const&>(values));
        }

        template <typename Future, std::size_t N>
        static bool wait_some_nothrow_impl(
            std::size_t n, std::array<Future, N> const& values)
        {
            static_assert(
                hpx::traits::is_future_v<Future>, "invalid use of wait_some");

            if (n == 0)
            {
                return false;
            }

            if (n > values.size())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "hpx::wait_some",
                    "number of results to wait for is out of bounds");
            }

            auto lazy_values = traits::acquire_shared_state_disp()(values);
            auto f = detail::get_wait_some_frame(HPX_MOVE(lazy_values), n);

            return (*f)();
        }

        template <typename Future, std::size_t N>
        friend bool tag_invoke(wait_some_nothrow_t, std::size_t n,
            std::array<Future, N> const& values)
        {
            return wait_some_nothrow_t::wait_some_nothrow_impl(n, values);
        }

        template <typename Future, std::size_t N>
        friend HPX_FORCEINLINE bool tag_invoke(wait_some_nothrow_t,
            std::size_t n, std::array<Future, N>& lazy_values)
        {
            return wait_some_nothrow_t::wait_some_nothrow_impl(
                n, const_cast<std::array<Future, N> const&>(lazy_values));
        }

        template <typename Future, std::size_t N>
        friend HPX_FORCEINLINE bool tag_invoke(wait_some_nothrow_t,
            std::size_t n, std::array<Future, N>&& lazy_values)
        {
            return wait_some_nothrow_t::wait_some_nothrow_impl(
                n, const_cast<std::array<Future, N> const&>(lazy_values));
        }

        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend bool tag_invoke(
            wait_some_nothrow_t, std::size_t n, Iterator begin, Iterator end)
        {
            auto values = traits::acquire_shared_state<Iterator>()(begin, end);
            auto f = detail::get_wait_some_frame(HPX_MOVE(values), n);

            return (*f)();
        }

        friend bool tag_invoke(wait_some_nothrow_t, std::size_t n)
        {
            if (n != 0)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::wait_some_nothrow",
                    "number of results to wait for is out of bounds");
            }
            return false;
        }

        template <typename T>
        friend bool tag_invoke(
            wait_some_nothrow_t, std::size_t n, hpx::future<T>&& f)
        {
            if (n != 1)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "hpx::wait_some",
                    "number of results to wait for is out of bounds");
            }

            f.wait();
            return f.has_exception();
        }

        template <typename T>
        friend bool tag_invoke(
            wait_some_nothrow_t, std::size_t n, hpx::shared_future<T>&& f)
        {
            if (n != 1)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "hpx::wait_some",
                    "number of results to wait for is out of bounds");
            }

            f.wait();
            return f.has_exception();
        }

        template <typename... Ts>
        friend bool tag_invoke(wait_some_nothrow_t, std::size_t n, Ts&&... ts)
        {
            if (n == 0)
            {
                return false;
            }

            if (n > sizeof...(Ts))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::lcos::wait_some",
                    "number of results to wait for is out of bounds");
            }

            using result_type =
                hpx::tuple<traits::detail::shared_state_ptr_for_t<Ts>...>;

            result_type values(traits::detail::get_shared_state(ts)...);
            auto f = detail::get_wait_some_frame(HPX_MOVE(values), n);

            return (*f)();
        }
    } wait_some_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_some_t final
      : hpx::functional::tag<wait_some_t>
    {
    private:
        template <typename Future>
        friend void tag_invoke(
            wait_some_t, std::size_t n, std::vector<Future> const& values)
        {
            if (hpx::wait_some_nothrow(n, values))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future>
        friend void tag_invoke(
            wait_some_t, std::size_t n, std::vector<Future>& values)
        {
            if (hpx::wait_some_nothrow(
                    n, const_cast<std::vector<Future> const&>(values)))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future>
        friend void tag_invoke(
            wait_some_t, std::size_t n, std::vector<Future>&& values)
        {
            if (hpx::wait_some_nothrow(
                    n, const_cast<std::vector<Future> const&>(values)))
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        template <typename Future, std::size_t N>
        friend void tag_invoke(wait_some_t, std::size_t n,
            std::array<Future, N> const& lazy_values)
        {
            if (hpx::wait_some_nothrow(n, lazy_values))
            {
                hpx::detail::throw_if_exceptional(lazy_values);
            }
        }

        template <typename Future, std::size_t N>
        friend void tag_invoke(
            wait_some_t, std::size_t n, std::array<Future, N>& lazy_values)
        {
            if (hpx::wait_some_nothrow(
                    n, const_cast<std::array<Future, N> const&>(lazy_values)))
            {
                hpx::detail::throw_if_exceptional(lazy_values);
            }
        }

        template <typename Future, std::size_t N>
        friend void tag_invoke(
            wait_some_t, std::size_t n, std::array<Future, N>&& lazy_values)
        {
            if (hpx::wait_some_nothrow(
                    n, const_cast<std::array<Future, N> const&>(lazy_values)))
            {
                hpx::detail::throw_if_exceptional(lazy_values);
            }
        }

        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend void tag_invoke(
            wait_some_t, std::size_t n, Iterator begin, Iterator end)
        {
            auto values = traits::acquire_shared_state<Iterator>()(begin, end);
            auto f = detail::get_wait_some_frame(values, n);

            if ((*f)())
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }

        friend void tag_invoke(wait_some_t, std::size_t n)
        {
            if (n != 0)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "hpx::wait_some",
                    "number of results to wait for is out of bounds");
            }
        }

        template <typename T>
        friend void tag_invoke(wait_some_t, std::size_t n, hpx::future<T>&& f)
        {
            if (hpx::wait_some_nothrow(n, HPX_MOVE(f)))
            {
                hpx::detail::throw_if_exceptional(f);
            }
        }

        template <typename T>
        friend void tag_invoke(
            wait_some_t, std::size_t n, hpx::shared_future<T>&& f)
        {
            if (hpx::wait_some_nothrow(n, HPX_MOVE(f)))
            {
                hpx::detail::throw_if_exceptional(f);
            }
        }

        template <typename... Ts>
        friend void tag_invoke(wait_some_t, std::size_t n, Ts&&... ts)
        {
            if (hpx::wait_some_nothrow(n, ts...))
            {
                hpx::detail::throw_if_exceptional(HPX_FORWARD(Ts, ts)...);
            }
        }
    } wait_some{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_some_n_nothrow_t final
      : hpx::functional::tag<wait_some_n_nothrow_t>
    {
    private:
        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend bool tag_invoke(wait_some_n_nothrow_t, std::size_t n,
            Iterator begin, std::size_t count)
        {
            auto values =
                traits::acquire_shared_state<Iterator>()(begin, count);
            auto f = detail::get_wait_some_frame(HPX_MOVE(values), n);

            return (*f)();
        }
    } wait_some_n_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_some_n_t final
      : hpx::functional::tag<wait_some_n_t>
    {
    private:
        template <typename Iterator,
            typename Enable =
                std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
        friend void tag_invoke(
            wait_some_n_t, std::size_t n, Iterator begin, std::size_t count)
        {
            auto values =
                traits::acquire_shared_state<Iterator>()(begin, count);
            auto f = detail::get_wait_some_frame(values, n);

            if ((*f)())
            {
                hpx::detail::throw_if_exceptional(values);
            }
        }
    } wait_some_n{};
}    // namespace hpx

#endif    // DOXYGEN
