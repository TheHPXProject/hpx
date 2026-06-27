//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file future.hpp
/// \page hpx::future, hpx::shared_future, hpx::make_future, hpx::make_shared_future, hpx::make_ready_future, hpx::make_ready_future_alloc, hpx::make_ready_future_at, hpx::make_ready_future_after, hpx::make_exceptional_future
/// \headerfile hpx/future.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/futures/detail/future_data.hpp>
#include <hpx/futures/future_fwd.hpp>
#include <hpx/futures/traits/acquire_shared_state.hpp>
#include <hpx/futures/traits/detail/future_await_traits.hpp>
#include <hpx/futures/traits/future_access.hpp>
#include <hpx/futures/traits/future_then_result.hpp>
#include <hpx/futures/traits/future_traits.hpp>
#include <hpx/futures/traits/is_future.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::lcos::detail {

    ///////////////////////////////////////////////////////////////////////////
    enum class future_state
    {
        invalid = 0,
        has_value = 1,
        has_exception = 2
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Future, typename Enable = void>
    struct future_unwrap_result;

    template <template <typename> class Future, typename R>
    struct future_unwrap_result<Future<Future<R>>>
    {
        using type = R;
        using wrapped_type = Future<type>;
    };

    template <typename R>
    struct future_unwrap_result<hpx::future<hpx::shared_future<R>>>
    {
        using type = R;
        using wrapped_type = hpx::future<type>;
    };

    HPX_CXX_CORE_EXPORT template <typename Future>
    using future_unwrap_result_t = typename future_unwrap_result<Future>::type;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct future_value : future_data_result<T>
    {
        template <typename U>
        HPX_FORCEINLINE static U get(U&& u)
        {
            return HPX_FORWARD(U, u);
        }

        static T get_default()
        {
            return T();
        }
    };

    template <typename T>
    struct future_value<T&> : future_data_result<T&>
    {
        HPX_FORCEINLINE static T& get(T* u) noexcept
        {
            return *u;
        }

        static T& get_default()
        {
            static T default_;
            return default_;
        }
    };

    template <>
    struct future_value<void> : future_data_result<void>
    {
        HPX_FORCEINLINE static constexpr void get(
            hpx::util::unused_type) noexcept
        {
        }

        static constexpr void get_default() noexcept {}
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename R>
    struct future_get_result
    {
        template <typename SharedState>
        HPX_FORCEINLINE static R* call(
            SharedState const& state, error_code& ec = throws)
        {
            return state->get_result(ec);
        }
    };

    template <>
    struct future_get_result<util::unused_type>
    {
        template <typename SharedState>
        HPX_FORCEINLINE static util::unused_type* call(
            SharedState const& state, error_code& ec = throws)
        {
            return state->get_result_void(ec);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Future, typename F,
        typename ContResult>
    class continuation;

    HPX_CXX_CORE_EXPORT template <typename ContResult>
    struct continuation_result
    {
        using type = ContResult;
    };

    HPX_CXX_CORE_EXPORT template <typename ContResult>
    using continuation_result_t =
        typename continuation_result<ContResult>::type;

    template <typename ContResult>
    struct continuation_result<hpx::future<ContResult>>
    {
        using type = ContResult;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Future,
        typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<continuation_result_t<ContResult>>
    make_continuation(Future&& future, Policy&& policy, F&& f);

    // create non-unwrapping continuations
    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Future,
        typename Executor, typename F>
    inline traits::detail::shared_state_ptr_t<ContResult>
    make_continuation_exec(Future&& future, Executor&& exec, F&& f);

    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Future,
        typename Executor, typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<ContResult>
    make_continuation_exec_policy(
        Future&& future, Executor&& exec, Policy&& policy, F&& f);

    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Allocator,
        typename Future, typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<continuation_result_t<ContResult>>
    make_continuation_alloc(
        Allocator const& a, Future&& future, Policy&& policy, F&& f);

    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Allocator,
        typename Future, typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<ContResult>
    make_continuation_alloc_nounwrap(
        Allocator const& a, Future&& future, Policy&& policy, F&& f);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Future, typename FD,
        typename Enable = void>
    struct future_then_dispatch
    {
        template <typename F>
        HPX_FORCEINLINE static decltype(auto) call(
            Future&& /* fut */, F&& /* f */)
        {
            // dummy impl to fail compilation if this function is called
            static_assert(sizeof(Future) == 0, "Cannot use the \
                    dummy implementation of future_then_dispatch, please use \
                    one of the template specialization.");
        }

        template <typename T0, typename F>
        HPX_FORCEINLINE static decltype(auto) call(
            Future&& /* fut */, T0&& /* t */, F&& /* f */)
        {
            // dummy impl to fail compilation if this function is called
            static_assert(sizeof(Future) == 0, "Cannot use the \
                    dummy implementation of future_then_dispatch, please use \
                    one of the template specialization.");
        }

        template <typename Allocator, typename F>
        HPX_FORCEINLINE static decltype(auto) call_alloc(
            Allocator const& /* alloc */, Future&& /* fut */, F&& /* f */)
        {
            // dummy impl to fail compilation if this function is called
            static_assert(sizeof(Future) == 0, "Cannot use the \
                    dummy implementation of future_then_dispatch::call_alloc, \
                    please use one of the template specialization.");
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Allocator, typename Future>
    traits::detail::shared_state_ptr_t<future_unwrap_result_t<Future>>
    unwrap_alloc(Allocator const& a, Future&& future, error_code& ec = throws);

    template <typename Future>
    hpx::traits::detail::shared_state_ptr_t<future_unwrap_result_t<Future>>
    unwrap(Future&& future, error_code& ec = throws);

    ///////////////////////////////////////////////////////////////////////////
    template <typename Future>
    inline traits::detail::shared_state_ptr_t<void> downcast_to_void(
        Future& future, bool addref) noexcept
    {
        using shared_state_type = traits::detail::shared_state_ptr_t<void>;
        using element_type = shared_state_type::element_type;

        // same as static_pointer_cast, but with addref option
        return shared_state_type(
            static_cast<element_type*>(
                traits::detail::get_shared_state(future).get()),
            addref);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Derived, typename R>
    class future_base
    {
    public:
        using result_type = R;
        using shared_state_type =
            future_data_base<traits::detail::shared_state_ptr_result_t<R>>;

    private:
        template <typename F>
        struct future_then_dispatch
          : lcos::detail::future_then_dispatch<Derived, F>
        {
        };

    public:
        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        future_base() noexcept = default;

        explicit future_base(hpx::intrusive_ptr<shared_state_type> const& p)
          : shared_state_(p)
        {
        }

        explicit future_base(hpx::intrusive_ptr<shared_state_type>&& p)
          : shared_state_(HPX_MOVE(p))
        {
        }

        future_base(future_base const& other) = default;
        future_base(future_base&& other) noexcept = default;
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        ~future_base() = default;

        void swap(future_base& other) noexcept
        {
            shared_state_.swap(other.shared_state_);
        }

        future_base& operator=(future_base const& other) = default;
        future_base& operator=(future_base&& other) noexcept = default;

        // Returns: true only if *this refers to a shared state.
        constexpr bool valid() const noexcept
        {
            return shared_state_ != nullptr;
        }

        // Returns: true if the shared state is ready, false if it isn't.
        bool is_ready() const noexcept
        {
            return shared_state_ != nullptr && shared_state_->is_ready();
        }

        // Returns: true if the shared state is ready and stores a value,
        //          false if it isn't.
        bool has_value() const noexcept
        {
            return shared_state_ != nullptr && shared_state_->has_value();
        }

        // Returns: true if the shared state is ready and stores an exception,
        //          false if it isn't.
        bool has_exception() const noexcept
        {
            return shared_state_ != nullptr && shared_state_->has_exception();
        }

        // Effects:
        //   - Blocks until the future is ready.
        // Returns: The stored exception_ptr if has_exception(), a null
        //          pointer otherwise.
        std::exception_ptr get_exception_ptr() const
        {
            if (!shared_state_)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_state,
                    "future_base<R>::get_exception_ptr",
                    "this future has no valid shared state");
            }

            using result_type = typename shared_state_type::result_type;

            error_code ec(throwmode::lightweight);
            lcos::detail::future_get_result<result_type>::call(
                this->shared_state_, ec);
            if (!ec)
            {
                HPX_ASSERT(!has_exception());
                return std::exception_ptr();
            }
            return hpx::detail::access_exception(ec);
        }

        // Notes: The three functions differ only by input parameters.
        //   - The first only takes a callable object which accepts a future
        //     object as a parameter.
        //   - The second function takes an executor as the first parameter
        //     and a callable object as the second parameter.
        //   - The third function takes a launch policy as the first parameter
        //     and a callable object as the second parameter.
        //   In cases where 'decltype(func(*this))' is future<R>, the
        //   resulting type is future<R> instead of future<future<R>>.
        // Effects:
        //   - The continuation is called when the object's shared state is
        //     ready (has a value or exception stored).
        //   - The continuation launches according to the specified launch
        //     policy or executor.
        //   - When the executor or launch policy is not provided the
        //     continuation inherits the parent's launch policy or executor.
        //   - If the parent was created with std::promise or with a
        //     packaged_task (has no associated launch policy), the
        //     continuation behaves the same as the third overload with a
        //     policy argument of launch::async | launch::deferred and the
        //     same argument for func.
        //   - If the parent has a policy of launch::deferred and the
        //     continuation does not have a specified launch policy or
        //     scheduler, then the parent is filled by immediately calling
        //     .wait(), and the policy of the antecedent is launch::deferred
        // Returns: An object of type future<decltype(func(*this))> that
        //          refers to the shared state created by the continuation.
        // Postcondition:
        //   - The future object is moved to the parameter of the continuation
        //     function.
        //   - valid() == false on original future object immediately after it
        //     returns.
        template <typename F>
        static auto then(Derived&& fut, F&& f, error_code& ec = throws)
            -> decltype(future_then_dispatch<std::decay_t<F>>::call_alloc(
                hpx::util::thread_local_caching_allocator<
                    hpx::lockfree::variable_size_stack,
                    hpx::util::internal_allocator<>>{},
                HPX_MOVE(fut), HPX_FORWARD(F, f)))
        {
            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;

            using result_type =
                decltype(future_then_dispatch<std::decay_t<F>>::call_alloc(
                    allocator_type{}, HPX_MOVE(fut), HPX_FORWARD(F, f)));

            if (!fut.shared_state_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state, "future_base<R>::then",
                    "this future has no valid shared state");
                return result_type();
            }

            return future_then_dispatch<std::decay_t<F>>::call_alloc(
                allocator_type{}, HPX_MOVE(fut), HPX_FORWARD(F, f));
        }

        template <typename F, typename T0>
        static auto then(Derived&& fut, T0&& t0, F&& f, error_code& ec = throws)
            -> decltype(future_then_dispatch<std::decay_t<T0>>::call_alloc(
                hpx::util::thread_local_caching_allocator<
                    hpx::lockfree::variable_size_stack,
                    hpx::util::internal_allocator<>>{},
                HPX_MOVE(fut), HPX_FORWARD(T0, t0), HPX_FORWARD(F, f)))
        {
            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;

            using result_type =
                decltype(future_then_dispatch<std::decay_t<T0>>::call_alloc(
                    allocator_type{}, HPX_MOVE(fut), HPX_FORWARD(T0, t0),
                    HPX_FORWARD(F, f)));

            if (!fut.shared_state_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state, "future_base<R>::then",
                    "this future has no valid shared state");
                return result_type();
            }

            return future_then_dispatch<std::decay_t<T0>>::call_alloc(
                allocator_type{}, HPX_MOVE(fut), HPX_FORWARD(T0, t0),
                HPX_FORWARD(F, f));
        }

        template <typename Allocator, typename F>
        static auto then_alloc(Allocator const& alloc, Derived&& fut, F&& f,
            error_code& ec = throws)
            -> decltype(future_then_dispatch<std::decay_t<F>>::call_alloc(
                alloc, HPX_MOVE(fut), HPX_FORWARD(F, f)))
        {
            using result_type =
                decltype(future_then_dispatch<std::decay_t<F>>::call_alloc(
                    alloc, HPX_MOVE(fut), HPX_FORWARD(F, f)));

            if (!fut.shared_state_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state,
                    "future_base<R>::then_alloc",
                    "this future has no valid shared state");
                return result_type();
            }

            return future_then_dispatch<std::decay_t<F>>::call_alloc(
                alloc, HPX_MOVE(fut), HPX_FORWARD(F, f));
        }

        // Effects: blocks until the shared state is ready.
        void wait(error_code& ec = throws) const
        {
            if (!shared_state_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state, "future_base<R>::wait",
                    "this future has no valid shared state");
                return;
            }
            shared_state_->wait(ec);
        }

        // Effects: none if the shared state contains a deferred function
        //          (30.6.8), otherwise blocks until the shared state is ready
        //          or until the absolute timeout (30.2.4) specified by
        //          abs_time has expired.
        // Returns:
        //   - future_status::deferred if the shared state contains a deferred
        //     function.
        //   - future_status::ready if the shared state is ready.
        //   - future_status::timeout if the function is returning because the
        //     absolute timeout (30.2.4) specified by abs_time has expired.
        // Throws: timeout-related exceptions (30.2.4).
        hpx::future_status wait_until(
            hpx::chrono::steady_time_point const& abs_time,
            error_code& ec = throws) const
        {
            if (!shared_state_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state,
                    "future_base<R>::wait_until",
                    "this future has no valid shared state");
                return hpx::future_status::uninitialized;
            }
            return shared_state_->wait_until(abs_time.value(), ec);
        }

        // Effects: none if the shared state contains a deferred function
        //          (30.6.8), otherwise blocks until the shared state is ready
        //          or until the relative timeout (30.2.4) specified by
        //          rel_time has expired.
        // Returns:
        //   - future_status::deferred if the shared state contains a deferred
        //     function.
        //   - future_status::ready if the shared state is ready.
        //   - future_status::timeout if the function is returning because the
        //     relative timeout (30.2.4) specified by rel_time has expired.
        // Throws: timeout-related exceptions (30.2.4).
        hpx::future_status wait_for(
            hpx::chrono::steady_duration const& rel_time,
            error_code& ec = throws) const
        {
            return wait_until(rel_time.from_now(), ec);
        }

#if defined(HPX_HAVE_CXX20_COROUTINES)
        bool await_ready() const noexcept
        {
            return lcos::detail::await_ready(
                *static_cast<Derived const*>(this));
        }

        template <typename Promise>
        void await_suspend(lcos::detail::coroutine_handle<Promise> rh)
        {
            lcos::detail::await_suspend(*static_cast<Derived*>(this), rh);
        }

        decltype(auto) await_resume()
        {
            return lcos::detail::await_resume(*static_cast<Derived*>(this));
        }
#endif

    protected:
        hpx::intrusive_ptr<shared_state_type> shared_state_;
    };
}    // namespace hpx::lcos::detail

/// Top level HPX namespace
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename R>
    class future : public lcos::detail::future_base<future<R>, R>
    {
    private:
        using base_type = lcos::detail::future_base<future<R>, R>;

    public:
        using result_type = R;
        using shared_state_type = typename base_type::shared_state_type;

    private:
        template <typename Future>
        friend struct hpx::traits::future_access;

        template <typename Future, typename Enable>
        friend struct hpx::traits::detail::future_access_customization_point;

        // Effects: constructs a future object from a shared state
        explicit future(hpx::intrusive_ptr<shared_state_type> const& state)
          : base_type(state)
        {
        }

        explicit future(hpx::intrusive_ptr<shared_state_type>&& state)
          : base_type(HPX_MOVE(state))
        {
        }

        template <typename SharedState>
        explicit future(hpx::intrusive_ptr<SharedState> const& state)
          : base_type(hpx::static_pointer_cast<shared_state_type>(state))
        {
        }

    public:
        // Effects: constructs an empty future object that does not refer to
        //          a shared state.
        // Postcondition: valid() == false.
        constexpr future() noexcept = default;

        // Effects: move constructs a future object that refers to the shared
        //          state that was originally referred to by other (if any).
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     constructor invocation.
        //   - other.valid() == false.
        future(future&& other) noexcept = default;

        future(future const& other) noexcept = delete;

        // Effects: constructs a future object by moving the instance referred
        //          to by rhs and unwrapping the inner future.
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     constructor invocation.
        //   - other.valid() == false.
        future(future<future>&& other) noexcept
          : base_type(
                other.valid() ? lcos::detail::unwrap(HPX_MOVE(other)) : nullptr)
        {
        }

        // Effects: constructs a future object by moving the instance referred
        //          to by rhs and unwrapping the inner future.
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     constructor invocation.
        //   - other.valid() == false.
        future(future<shared_future<R>>&& other) noexcept
          : base_type(
                other.valid() ? lcos::detail::unwrap(HPX_MOVE(other)) : nullptr)
        {
        }

        // Effects: constructs a future<void> object that will be ready when
        //          the given future is ready
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     constructor invocation.
        //   - other.valid() == false.
        template <typename T>
        future(future<T>&& other,
            std::enable_if_t<std::is_void_v<R> && !traits::is_future_v<T>, T>* =
                nullptr) noexcept
          : base_type(other.valid() ?
                    lcos::detail::downcast_to_void(other, false) :
                    nullptr)
        {
            traits::future_access<future<T>>::detach_shared_state(
                HPX_MOVE(other));
        }

        // Effects:
        //   - releases any shared state (30.6.4);
        //   - destroys *this.
        ~future() = default;

        // Effects:
        //   - releases any shared state (30.6.4).
        //   - move assigns the contents of other to *this.
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     assignment.
        //   - other.valid() == false.
        future& operator=(future&& other) noexcept = default;

        future& operator=(future const& other) noexcept = delete;

        // Returns: shared_future<R>(HPX_MOVE(*this)).
        // Postcondition: valid() == false.
        shared_future<R> share() noexcept
        {
            return shared_future<R>(HPX_MOVE(*this));
        }

        // Effects: wait()s until the shared state is ready, then retrieves
        //          the value stored in the shared state.
        // Returns:
        //   - future::get() returns the value v stored in the object's
        //     shared state as HPX_MOVE(v).
        //   - future<R&>::get() returns the reference stored as value in the
        //     object's shared state.
        //   - future<void>::get() returns nothing.
        // Throws: the stored exception, if an exception was stored in the
        //         shared state.
        // Postcondition: valid() == false.
        typename hpx::traits::future_traits<future>::result_type get()
        {
            if (!this->shared_state_)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_state, "future<R>::get",
                    "this future has no valid shared state");
            }

            auto on_exit = hpx::experimental::scope_exit(
                [this] { this->shared_state_.reset(); });

            using result_type = typename shared_state_type::result_type;
            auto* result = lcos::detail::future_get_result<result_type>::call(
                this->shared_state_);

            // no error has been reported, return the result
            return lcos::detail::future_value<R>::get(HPX_MOVE(*result));
        }

        typename hpx::traits::future_traits<future>::result_type get(
            error_code& ec)
        {
            if (!this->shared_state_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state, "future<R>::get",
                    "this future has no valid shared state");
                return lcos::detail::future_value<R>::get_default();
            }

            auto on_exit = hpx::experimental::scope_exit(
                [this] { this->shared_state_.reset(); });

            using result_type = typename shared_state_type::result_type;
            result_type* result =
                lcos::detail::future_get_result<result_type>::call(
                    this->shared_state_, ec);
            if (ec)
            {
                return lcos::detail::future_value<R>::get_default();
            }

            // no error has been reported, return the result
            return lcos::detail::future_value<R>::get(HPX_MOVE(*result));
        }

        using base_type::get_exception_ptr;

        using base_type::has_exception;
        using base_type::has_value;
        using base_type::is_ready;
        using base_type::valid;

        /// \brief Attaches a continuation to \a *this. The behavior is undefined
        ///        if \a *this has no associated shared state (i.e.,
        ///        \a valid()==false ).
        /// \details In cases where \a decltype(func(*this)) is \a future<R>,
        ///          the resulting type is \a future<R> instead of
        ///          \a future<future<R>>.
        ///          Effects:
        ///             - The continuation is called when the object's shared
        ///               state is ready (has a value or exception stored).
        ///             - The continuation launches according to the specified
        ///               launch policy or executor.
        ///             - When the executor or launch policy is not provided the
        ///               continuation inherits the parent's launch policy or
        ///               executor.
        ///             - If the parent was created with \a std::promise or with a
        ///               packaged_task (has no associated launch policy), the
        ///               continuation behaves the same as the third overload
        ///               with a policy argument of \a launch::async |
        ///               \a launch::deferred and the same argument for \a func.
        ///             - If the parent has a policy of \a launch::deferred and the
        ///               continuation does not have a specified launch policy or
        ///               scheduler, then the parent is filled by immediately
        ///               calling \a .wait(), and the policy of the antecedent is
        ///               \a launch::deferred
        ///
        /// \tparam F           The type of the function/function object to use
        ///                     (deduced). F must meet requirements of
        ///                     \a MoveConstructible.
        /// \tparam error_code  The type of error code.
        ///
        /// \param f            A continuation to be attached.
        /// \param ec           Used to hold error code value originated during the
        ///                     operation. Defaults to \a throws -- A special
        ///                     'throw on error' \a error_code.
        /// \returns            An object of type \a future<decltype(func(*this))> that
        ///                     refers to the shared state created by the continuation.
        /// \note               Postcondition:
        ///                        - The future object is moved to the parameter of the
        ///                          continuation function.
        ///                        - valid() == false on original future object immediately
        ///                          after it returns.
        template <typename F>
        decltype(auto) then(F&& f, error_code& ec = throws)
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            // This and the similar ifdefs below for future::then and
            // shared_future::then only work to satisfy nvcc up to at least
            // CUDA 11. Without this nvcc fails to compile some code with
            // "error: cannot use an entity undefined in device code" without
            // specifying what entity it refers to.
            HPX_ASSERT(false);
            // clang-format off
            using future_type = decltype(
                base_type::then(HPX_MOVE(*this), HPX_FORWARD(F, f), ec));
            // clang-format on
            return future_type{};
#else
            auto on_exit = hpx::experimental::scope_exit(
                [this] { this->shared_state_.reset(); });

            return base_type::then(HPX_MOVE(*this), HPX_FORWARD(F, f), ec);
#endif
        }

        /// \copybrief hpx::future::then(F&& f, error_code& ec = throws)
        /// \copydetail hpx::future::then(F&& f, error_code& ec = throws)
        ///
        /// \tparam T0          The type of executor or launch policy.
        /// \tparam F           The type of the function/function object to use
        ///                     (deduced). F must meet requirements of
        ///                     \a MoveConstructible.
        /// \tparam error_code  The type of error code.
        ///
        /// \param t0           The executor or launch policy to be used.
        /// \param f            A continuation to be attached.
        /// \param ec           Used to hold error code value originated during the
        ///                     operation. Defaults to \a throws -- A special
        ///                     'throw on error' \a error_code.
        /// \returns            An object of type \a future<decltype(func(*this))> that
        ///                     refers to the shared state created by the continuation.
        /// \note               Postcondition:
        ///                        - The future object is moved to the parameter of the
        ///                          continuation function.
        ///                        - valid() == false on original future object immediately
        ///                          after it returns.
        template <typename T0, typename F>
        decltype(auto) then(T0&& t0, F&& f, error_code& ec = throws)
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            using future_type = decltype(base_type::then(
                HPX_MOVE(*this), HPX_FORWARD(T0, t0), HPX_FORWARD(F, f), ec));
            return future_type{};
#else
            auto on_exit = hpx::experimental::scope_exit(
                [this] { this->shared_state_.reset(); });

            return base_type::then(
                HPX_MOVE(*this), HPX_FORWARD(T0, t0), HPX_FORWARD(F, f), ec);
#endif
        }

        template <typename Allocator, typename F>
        auto then_alloc(Allocator const& alloc, F&& f, error_code& ec = throws)
            -> decltype(base_type::then_alloc(
#if defined(HPX_CUDA_VERSION) && (HPX_CUDA_VERSION < 1104)
                alloc, std::move(*this), std::forward<F>(f), ec))
#else
                alloc, HPX_MOVE(*this), HPX_FORWARD(F, f), ec))
#endif
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            using future_type = decltype(base_type::then_alloc(
                alloc, std::move(*this), std::forward<F>(f), ec));
            return future_type{};
#else
            auto on_exit = hpx::experimental::scope_exit(
                [this] { this->shared_state_.reset(); });

            return base_type::then_alloc(
                alloc, HPX_MOVE(*this), HPX_FORWARD(F, f), ec);
#endif
        }

        using base_type::wait;
        using base_type::wait_for;
        using base_type::wait_until;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Allow to convert any future<U> into any other future<R> based on an
    // existing conversion path U --> R.
    /// \brief Converts any future of type U to any other future of type R
    ///        based on an existing conversion path from U to R.
    HPX_CXX_CORE_EXPORT template <typename R, typename U>
    hpx::future<R> make_future(hpx::future<U>&& f)
    {
        static_assert(std::is_convertible_v<U, R> || std::is_void_v<R>,
            "the argument type must be implicitly convertible to the requested "
            "result type");

        if constexpr (std::is_convertible_v<hpx::future<U>, hpx::future<R>>)
        {
            return HPX_MOVE(f);
        }
        else
        {
            return f.then(hpx::launch::sync, [](hpx::future<U>&& f) -> R {
                return util::void_guard<R>(), f.get();
            });
        }
    }

    // Allow to convert any future<U> into any other future<R> based on a given
    // conversion function: R conv(U).
    /// \brief Converts any future of type U to any other future of type R
    ///        based on a given conversion function: R conv(U).
    HPX_CXX_CORE_EXPORT template <typename R, typename U, typename Conv>
    hpx::future<R> make_future(hpx::future<U>&& f, Conv&& conv)
    {
        if constexpr (std::is_convertible_v<hpx::future<U>, hpx::future<R>>)
        {
            return HPX_MOVE(f);
        }
        else
        {
            return f.then(hpx::launch::sync,
                [conv = HPX_FORWARD(Conv, conv)](hpx::future<U>&& f) -> R {
                    return HPX_INVOKE(conv, f.get());
                });
        }
    }
}    // namespace hpx

/// Top level HPX namespace
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename R>
    class shared_future : public lcos::detail::future_base<shared_future<R>, R>
    {
        using base_type = lcos::detail::future_base<shared_future<R>, R>;

    public:
        using result_type = R;
        using shared_state_type = typename base_type::shared_state_type;

    private:
        template <typename Future>
        friend struct hpx::traits::future_access;

        template <typename Future, typename Enable>
        friend struct hpx::traits::detail::future_access_customization_point;

        // Effects: constructs a future object from a shared state
        explicit shared_future(
            hpx::intrusive_ptr<shared_state_type> const& state)
          : base_type(state)
        {
        }

        explicit shared_future(hpx::intrusive_ptr<shared_state_type>&& state)
          : base_type(HPX_MOVE(state))
        {
        }

        template <typename SharedState>
        explicit shared_future(hpx::intrusive_ptr<SharedState> const& state)
          : base_type(hpx::static_pointer_cast<shared_state_type>(state))
        {
        }

    public:
        // Effects: constructs an empty future object that does not refer to
        //          a shared state.
        // Postcondition: valid() == false.
        constexpr shared_future() noexcept = default;

        // Effects: constructs a shared_future object that refers to the same
        //          shared state as other (if any).
        // Postcondition: valid() returns the same value as other.valid().
        shared_future(shared_future const& other) = default;

        // Effects: move constructs a future object that refers to the shared
        //          state that was originally referred to by other (if any).
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     constructor invocation.
        //   - other.valid() == false.
        shared_future(shared_future&& other) noexcept = default;

        shared_future(future<R>&& other) noexcept
          : base_type(hpx::traits::detail::get_shared_state(other))
        {
            other = future<R>();
        }

        // Effects: constructs a shared_future object by moving the instance
        //          referred to by rhs and unwrapping the inner future.
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     constructor invocation.
        //   - other.valid() == false.
        shared_future(future<shared_future>&& other) noexcept
          : base_type(
                other.valid() ? lcos::detail::unwrap(other.share()) : nullptr)
        {
        }

        // Effects: constructs a future<void> object that will be ready when
        //          the given future is ready
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     constructor invocation.
        template <typename T>
        shared_future(shared_future<T> const& other,
            std::enable_if_t<std::is_void_v<R> && !traits::is_future_v<T>, T>* =
                nullptr)
          : base_type(other.valid() ?
                    lcos::detail::downcast_to_void(other, true) :
                    nullptr)
        {
        }

        // Effects:
        //   - releases any shared state (30.6.4);
        //   - destroys *this.
        ~shared_future() = default;

        // Effects:
        //   - releases any shared state (30.6.4).
        //   - assigns the contents of other to *this. As a result, *this
        //     refers to the same shared state as other (if any).
        // Postconditions:
        //   - valid() == other.valid().
        shared_future& operator=(shared_future const& other) = default;

        // Effects:
        //   - releases any shared state (30.6.4).
        //   - move assigns the contents of other to *this.
        // Postconditions:
        //   - valid() returns the same value as other.valid() prior to the
        //     assignment.
        //   - other.valid() == false.
        shared_future& operator=(shared_future&& other) noexcept = default;

        // Effects: wait()s until the shared state is ready, then retrieves
        //          the value stored in the shared state.
        // Returns:
        //   - shared_future::get() returns a const reference to the value
        //     stored in the object's shared state.
        //   - shared_future<R&>::get() returns the reference stored as value
        //     in the object's shared state.
        //   - shared_future<void>::get() returns nothing.
        // Throws: the stored exception, if an exception was stored in the
        //         shared state.
        // Postcondition: valid() == false.
        typename hpx::traits::future_traits<shared_future>::result_type get()
            const    //-V659
        {
            if (!this->shared_state_)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_state,
                    "shared_future<R>::get",
                    "this future has no valid shared state");
            }

            using result_type = typename shared_state_type::result_type;
            result_type* result =
                lcos::detail::future_get_result<result_type>::call(
                    this->shared_state_);

            // no error has been reported, return the result
            return lcos::detail::future_value<R>::get(*result);
        }

        typename hpx::traits::future_traits<shared_future>::result_type get(
            error_code& ec) const    //-V659
        {
            using result_type = typename shared_state_type::result_type;
            if (!this->shared_state_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state, "shared_future<R>::get",
                    "this future has no valid shared state");
                static result_type res(
                    lcos::detail::future_value<R>::get_default());
                return res;
            }

            result_type* result =
                lcos::detail::future_get_result<result_type>::call(
                    this->shared_state_, ec);
            if (ec)
            {
                static result_type res(
                    lcos::detail::future_value<R>::get_default());
                return res;
            }

            // no error has been reported, return the result
            return lcos::detail::future_value<R>::get(*result);
        }

        using base_type::get_exception_ptr;

        using base_type::has_exception;
        using base_type::has_value;
        using base_type::is_ready;
        using base_type::valid;

        /// \copydoc hpx::future::then(F&& f, error_code& ec = throws)
        template <typename F>
        decltype(auto) then(F&& f, error_code& ec = throws) const
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            // clang-format off
            using future_type = decltype(
                base_type::then(shared_future(*this), HPX_FORWARD(F, f), ec));
            // clang-format on
            return future_type{};
#else
            return base_type::then(shared_future(*this), HPX_FORWARD(F, f), ec);
#endif
        }

        /// \copydoc hpx::future::then(T0&& t0, F&& f, error_code& ec = throws)
        template <typename T0, typename F>
        decltype(auto) then(T0&& t0, F&& f, error_code& ec = throws) const
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            using future_type = decltype(base_type::then(shared_future(*this),
                HPX_FORWARD(T0, t0), HPX_FORWARD(F, f), ec));
            return future_type{};
#else
            return base_type::then(shared_future(*this), HPX_FORWARD(T0, t0),
                HPX_FORWARD(F, f), ec);
#endif
        }

        template <typename Allocator, typename F>
        auto then_alloc(Allocator const& alloc, F&& f, error_code& ec = throws)
            -> decltype(base_type::then_alloc(
#if defined(HPX_CUDA_VERSION) && (HPX_CUDA_VERSION < 1104)
                alloc, std::move(*this), std::forward<F>(f), ec))
#else
                alloc, HPX_MOVE(*this), HPX_FORWARD(F, f), ec))
#endif
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            using future_type = decltype(base_type::then_alloc(
                alloc, shared_future(*this), HPX_FORWARD(F, f), ec));
            return future_type{};
#else
            return base_type::then_alloc(
                alloc, shared_future(*this), HPX_FORWARD(F, f), ec);
#endif
        }

        using base_type::wait;
        using base_type::wait_for;
        using base_type::wait_until;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Allow to convert any shared_future<U> into any other future<R> based on
    // an existing conversion path U --> R.
    /// \brief Converts any shared_future of type U to any other future of type R
    ///        based on an existing conversion path from U to R.
    HPX_CXX_CORE_EXPORT template <typename R, typename U>
    hpx::future<R> make_future(hpx::shared_future<U> f)
    {
        static_assert(std::is_convertible_v<R, U> || std::is_void_v<R>,
            "the argument type must be implicitly convertible to the requested "
            "result type");

        if constexpr (std::is_convertible_v<hpx::shared_future<U>,
                          hpx::future<R>>)
        {
            return hpx::future<R>(HPX_MOVE(f));
        }
        else
        {
            return f.then(
                hpx::launch::sync, [](hpx::shared_future<U>&& f) -> R {
                    return util::void_guard<R>(), f.get();
                });
        }
    }

    // Allow to convert any future<U> into any other future<R> based on a given
    // conversion function: R conv(U).
    /// \copydoc make_future(hpx::future<U>&& f)
    HPX_CXX_CORE_EXPORT template <typename R, typename U, typename Conv>
    hpx::future<R> make_future(hpx::shared_future<U> f, Conv&& conv)
    {
        static_assert(hpx::is_invocable_r_v<R, Conv, U>,
            "the argument type must be convertible to the requested "
            "result type by using the supplied conversion function");

        if constexpr (std::is_convertible_v<hpx::shared_future<U>,
                          hpx::future<R>>)
        {
            return hpx::future<R>(HPX_MOVE(f));
        }
        else
        {
            return f.then(hpx::launch::sync,
                [conv = HPX_FORWARD(Conv, conv)](hpx::shared_future<U> const& f)
                    -> R { return HPX_INVOKE(conv, f.get()); });
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Convert any type of future<T> or shared_future<T> into a corresponding
    // shared_future<T>.
    /// \brief Converts any future or shared_future of type T to a corresponding
    ///        shared_future of type T
    HPX_CXX_CORE_EXPORT template <typename R>
    hpx::shared_future<R> make_shared_future(hpx::future<R>&& f) noexcept
    {
        return f.share();
    }

    /// \copydoc make_shared_future(hpx::future<R>&& f)
    HPX_CXX_CORE_EXPORT template <typename R>
    hpx::shared_future<R>& make_shared_future(hpx::shared_future<R>& f) noexcept
    {
        return f;
    }

    /// \copydoc make_shared_future(hpx::future<R>&& f)
    HPX_CXX_CORE_EXPORT template <typename R>
    hpx::shared_future<R>&& make_shared_future(
        hpx::shared_future<R>&& f) noexcept
    {
        return HPX_MOVE(f);
    }

    /// \copydoc make_shared_future(hpx::future<R>&& f)
    HPX_CXX_CORE_EXPORT template <typename R>
    hpx::shared_future<R> const& make_shared_future(
        hpx::shared_future<R> const& f) noexcept
    {
        return f;    // NOLINT(bugprone-return-const-ref-from-parameter)
    }
}    // namespace hpx

/// Top level HPX namespace
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // Extension (see wg21.link/P0319), with allocator
    /// \brief Creates a pre-initialized future object with allocator (extension)
    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator,
        typename... Ts>
    std::enable_if_t<std::is_constructible_v<T, Ts&&...> || std::is_void_v<T>,
        future<T>>
    make_ready_future_alloc(Allocator const& a, Ts&&... ts)
    {
        using result_type = T;

        using base_allocator = Allocator;
        using shared_state = traits::shared_state_allocator_t<
            lcos::detail::future_data<result_type>, base_allocator>;

        using other_allocator = typename std::allocator_traits<
            base_allocator>::template rebind_alloc<shared_state>;
        using traits = std::allocator_traits<other_allocator>;

        using init_no_addref = typename shared_state::init_no_addref;

        using unique_ptr = std::unique_ptr<shared_state,
            util::allocator_deleter<other_allocator>>;

        other_allocator alloc(a);
        unique_ptr p(traits::allocate(alloc, 1),
            util::allocator_deleter<other_allocator>{alloc});
        traits::construct(alloc, p.get(), init_no_addref{}, alloc,
            std::in_place, HPX_FORWARD(Ts, ts)...);

        return hpx::traits::future_access<future<result_type>>::create(
            p.release(), false);
    }

    // Extension (see wg21.link/P0319)
    /// \copydoc make_ready_future(T&& init)
    HPX_CXX_CORE_EXPORT template <typename T, typename... Ts>
    HPX_FORCEINLINE std::enable_if_t<
        std::is_constructible_v<T, Ts&&...> || std::is_void_v<T>, future<T>>
    make_ready_future(Ts&&... ts)
    {
        using allocator_type = hpx::util::thread_local_caching_allocator<
            hpx::lockfree::variable_size_stack,
            hpx::util::internal_allocator<>>;
        return make_ready_future_alloc<T>(
            allocator_type{}, HPX_FORWARD(Ts, ts)...);
    }
    ///////////////////////////////////////////////////////////////////////////
    // extension: create a pre-initialized future object, with allocator
    /// \copydoc make_ready_future_alloc(Allocator const& a, Ts&&... ts)
    HPX_CXX_CORE_EXPORT template <int DeductionGuard = 0, typename Allocator,
        typename T>
    future<hpx::util::decay_unwrap_t<T>> make_ready_future_alloc(
        Allocator const& a, T&& init)
    {
        return hpx::make_ready_future_alloc<hpx::util::decay_unwrap_t<T>>(
            a, HPX_FORWARD(T, init));
    }

    // extension: create a pre-initialized future object
    /// \brief The function creates a shared state that is immediately ready
    ///        and returns a future associated with that shared state.
    ///        For the returned future, valid() == true and is_ready() == true
    HPX_CXX_CORE_EXPORT template <int DeductionGuard = 0, typename T>
    HPX_FORCEINLINE future<hpx::util::decay_unwrap_t<T>> make_ready_future(
        T&& init)
    {
        using allocator_type = hpx::util::thread_local_caching_allocator<
            hpx::lockfree::variable_size_stack,
            hpx::util::internal_allocator<>>;
        return hpx::make_ready_future_alloc<hpx::util::decay_unwrap_t<T>>(
            allocator_type{}, HPX_FORWARD(T, init));
    }

    ///////////////////////////////////////////////////////////////////////////
    // extension: create a pre-initialized future object which holds the
    // given error
    /// \brief Creates a pre-initialized future object which holds the
    ///        given error (extension)
    HPX_CXX_CORE_EXPORT template <typename T>
    future<T> make_exceptional_future(std::exception_ptr const& e)
    {
        using shared_state = lcos::detail::future_data<T>;
        using init_no_addref = typename shared_state::init_no_addref;

        hpx::intrusive_ptr<shared_state> p(
            new shared_state(init_no_addref{}, e), false);

        return hpx::traits::future_access<future<T>>::create(HPX_MOVE(p));
    }

    /// \copydoc make_exceptional_future(std::exception_ptr const& e)
    HPX_CXX_CORE_EXPORT template <typename T, typename E>
    future<T> make_exceptional_future(E e)
    {
        try
        {
            throw e;
        }
        catch (...)
        {
            return hpx::make_exceptional_future<T>(std::current_exception());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // extension: create a pre-initialized future object which gets ready at
    // a given point in time
    /// \brief Creates a pre-initialized future object which gets ready at
    ///        a given point in time (extension)
    HPX_CXX_CORE_EXPORT template <int DeductionGuard = 0, typename T>
    future<hpx::util::decay_unwrap_t<T>> make_ready_future_at(
        hpx::chrono::steady_time_point const& abs_time, T&& init)
    {
        using result_type = hpx::util::decay_unwrap_t<T>;
        using shared_state = lcos::detail::timed_future_data<result_type>;
        using init_no_addref = typename shared_state::init_no_addref;

        hpx::intrusive_ptr<shared_state> p(
            new shared_state(
                init_no_addref{}, abs_time.value(), HPX_FORWARD(T, init)),
            false);

        return hpx::traits::future_access<future<result_type>>::create(
            HPX_MOVE(p));
    }

    /// \brief Creates a pre-initialized future object which gets ready after
    ///        a given point in time (extension)
    HPX_CXX_CORE_EXPORT template <int DeductionGuard = 0, typename T>
    future<hpx::util::decay_unwrap_t<T>> make_ready_future_after(
        hpx::chrono::steady_duration const& rel_time, T&& init)
    {
        return hpx::make_ready_future_at(
            rel_time.from_now(), HPX_FORWARD(T, init));
    }

    ///////////////////////////////////////////////////////////////////////////
    // extension: create a pre-initialized future object, with allocator
    /// \copydoc make_ready_future_alloc(Allocator const& a, Ts&&... ts)
    HPX_CXX_CORE_EXPORT template <typename Allocator>
    inline future<void> make_ready_future_alloc(Allocator const& a)
    {
        return hpx::make_ready_future_alloc<void>(a, util::unused);
    }

    // extension: create a pre-initialized future object
    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE future<void> make_ready_future()
    {
        using allocator_type = hpx::util::thread_local_caching_allocator<
            hpx::lockfree::variable_size_stack,
            hpx::util::internal_allocator<>>;
        return make_ready_future_alloc<void>(allocator_type{}, util::unused);
    }

    // Extension (see wg21.link/P0319)
    /// \copydoc make_ready_future(T&& init)
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE std::enable_if_t<std::is_void_v<T>, future<void>>
    make_ready_future()
    {
        return hpx::make_ready_future();
    }

    // extension: create a pre-initialized future object which gets ready at
    // a given point in time
    /// \copydoc make_ready_future_at(hpx::chrono::steady_time_point const& abs_time, T&& init)
    HPX_CXX_CORE_EXPORT inline future<void> make_ready_future_at(
        hpx::chrono::steady_time_point const& abs_time)
    {
        using shared_state = lcos::detail::timed_future_data<void>;
        using init_no_addref = shared_state::init_no_addref;

        hpx::intrusive_ptr<shared_state> p(
            new shared_state(
                init_no_addref{}, abs_time.value(), hpx::util::unused),
            false);

        return hpx::traits::future_access<future<void>>::create(HPX_MOVE(p));
    }

    /// \copydoc make_ready_future_at(hpx::chrono::steady_time_point const& abs_time, T&& init)
    HPX_CXX_CORE_EXPORT template <typename T>
    std::enable_if_t<std::is_void_v<T>, future<void>> make_ready_future_at(
        hpx::chrono::steady_time_point const& abs_time)
    {
        return hpx::make_ready_future_at(abs_time);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \copydoc make_ready_future_after(hpx::chrono::steady_duration const& rel_time, T&& init)
    HPX_CXX_CORE_EXPORT inline future<void> make_ready_future_after(
        hpx::chrono::steady_duration const& rel_time)
    {
        return hpx::make_ready_future_at(rel_time.from_now());
    }

    /// \copydoc make_ready_future_after(hpx::chrono::steady_duration const& rel_time, T&& init)
    HPX_CXX_CORE_EXPORT template <typename T>
    std::enable_if_t<std::is_void_v<T>, future<void>> make_ready_future_after(
        hpx::chrono::steady_duration const& rel_time)
    {
        return hpx::make_ready_future_at(rel_time.from_now());
    }
}    // namespace hpx

namespace hpx::lcos::detail {

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void preprocess_future(
        serialization::output_archive& ar,
        hpx::lcos::detail::future_data_refcnt_base& state);

    HPX_CXX_CORE_EXPORT template <typename Future>
    void serialize_future(
        serialization::output_archive& ar, Future& f, unsigned)
    {
        if (f.valid() && !f.is_ready())
        {
            if (ar.is_preprocessing())
            {
                auto shared_state =
                    hpx::traits::future_access<Future>::get_shared_state(f);

                shared_state->execute_deferred();

                preprocess_future(ar, *shared_state);
            }
            else
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "serialize_future_save",
                    "future must be ready in order for it to be serialized");
            }
            return;
        }

        if (f.has_value())
        {
            constexpr future_state state = future_state::has_value;
            ar << state;
            if constexpr (!std::is_void_v<hpx::traits::future_traits_t<Future>>)
            {
                using value_type =
                    typename hpx::traits::future_traits<Future>::result_type;

                value_type const& value =
                    *(hpx::traits::future_access<Future>::get_shared_state(f)
                            ->get_result());

                if constexpr (!std::is_default_constructible_v<value_type>)
                {
                    using serialization::detail::save_construct_data;
                    save_construct_data(ar, &value, 0);
                }
                ar << value;
            }
        }
        else if (f.has_exception())
        {
            constexpr future_state state = future_state::has_exception;
            std::exception_ptr const exception = f.get_exception_ptr();
            ar << state << exception;
        }
        else
        {
            constexpr future_state state = future_state::invalid;
            ar << state;
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Future>
    void serialize_future(serialization::input_archive& ar, Future& f, unsigned)
    {
        using value_type = hpx::traits::future_traits_t<Future>;

        future_state state = future_state::invalid;
        ar >> state;
        if (state == future_state::has_value)
        {
            if constexpr (std::is_void_v<hpx::traits::future_traits_t<Future>>)
            {
                f = hpx::make_ready_future();
            }
            else
            {
                if constexpr (std::is_default_constructible_v<value_type>)
                {
                    value_type value;
                    ar >> value;

                    f = hpx::make_ready_future(HPX_MOVE(value));
                }
                else
                {
                    value_type&& value =
                        serialization::detail::constructor_selector<
                            value_type>::create(ar);

                    f = hpx::make_ready_future(HPX_MOVE(value));
                }
            }
        }
        else if (state == future_state::has_exception)
        {
            std::exception_ptr exception;
            ar >> exception;

            f = hpx::make_exceptional_future<value_type>(HPX_MOVE(exception));
        }
        else if (state == future_state::invalid)
        {
            f = Future();
        }
        else
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "serialize_future_load",
                "attempting to deserialize a future with an unknown state");
        }
    }
}    // namespace hpx::lcos::detail

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename Archive, typename T>
    HPX_FORCEINLINE void serialize(
        Archive& ar, ::hpx::future<T>& f, unsigned version)
    {
        hpx::lcos::detail::serialize_future(ar, f, version);
    }

    HPX_CXX_CORE_EXPORT template <typename Archive, typename T>
    HPX_FORCEINLINE void serialize(
        Archive& ar, ::hpx::shared_future<T>& f, unsigned version)
    {
        hpx::lcos::detail::serialize_future(ar, f, version);
    }
}    // namespace hpx::serialization

#include <hpx/modules/futures.hpp>
