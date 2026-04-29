//  Copyright (c) 2022 Shreyas Atre
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <any>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

template <typename C = void>
struct is_valid_helper : std::integral_constant<bool, !std::is_same_v<C, void>>
{
};

template <template <typename...> typename C, typename... T>
struct is_valid : is_valid_helper<C<T...>>
{
};

template <template <typename...> typename T, typename... As>
inline constexpr bool well_formed = is_valid<T, As...>::value;

template <typename T>
inline constexpr bool stop_token_provider = std::integral_constant<bool,
    !std::is_same_v<hpx::execution::experimental::stop_token_of_t<T>,
        void>>::value;

template <typename T>
inline constexpr bool indirect_stop_token_provider = stop_token_provider<
    hpx::util::invoke_result_t<hpx::execution::experimental::get_env_t, T>>;

template <>
inline constexpr bool indirect_stop_token_provider<void> =
    stop_token_provider<hpx::execution::experimental::empty_env>;

template <typename Fn, typename = std::enable_if_t<std::is_invocable_v<Fn>>,
    typename = std::enable_if_t<std::is_nothrow_move_constructible_v<Fn> &&
        std::is_nothrow_invocable_v<Fn>>>

// clang-format off
struct scope_guard
{
    Fn fn_;
    scope_guard(Fn fn) noexcept
      : fn_((Fn&&) fn)
    {
    }
    ~scope_guard()
    {
        ((Fn&&) fn_)();
    }
};
// clang-format on

struct forward_stop_request
{
    hpx::experimental::in_place_stop_source& stop_source_;
    void operator()() noexcept
    {
        stop_source_.request_stop();
    }
};
template <typename ParentPromise, typename = void>
struct default_awaiter_context
{
    explicit default_awaiter_context(
        default_awaiter_context&, ParentPromise&) noexcept
    {
    }
};

////////////////////////////////////////////////////////////////////////////////
// This is the context that is associated with basic_task's promise type
// by default. It handles forwarding of stop requests from parent to child.
struct default_task_context_impl
  : hpx::functional::tag<default_task_context_impl>
{
    hpx::experimental::in_place_stop_token stop_token_;

    // This is the context associated with basic_task's awaiter. By default
    // it does nothing.
    template <typename ParentPromise, typename T>
    friend struct default_awaiter_context;

    friend auto tag_invoke(hpx::execution::experimental::get_stop_token_t,
        default_task_context_impl const& self) noexcept
        -> hpx::experimental::in_place_stop_token
    {
        return self.stop_token_;
    }

public:
    default_task_context_impl() = default;

    bool stop_requested() const noexcept
    {
        return stop_token_.stop_requested();
    }

    template <typename ThisPromise>
    using promise_context_t = default_task_context_impl;

    template <typename ThisPromise, typename ParentPromise = void>
    using awaiter_context_t = default_awaiter_context<ParentPromise>;
};

////////////////////////////////////////////////////////////////////////////////
// This is the context to be associated with basic_task's awaiter when
// the parent coroutine's promise type is known, is a stop_token_provider,
// and its stop token type is neither in_place_stop_token nor unstoppable.
template <typename ParentPromise>
struct default_awaiter_context<ParentPromise,
    std::enable_if_t<indirect_stop_token_provider<ParentPromise>>>
{
    using stop_token_t = hpx::execution::experimental::stop_token_of_t<
        hpx::execution::experimental::env_of_t<ParentPromise>>;
    using stop_callback_t =
        typename stop_token_t::template callback_type<forward_stop_request>;

    explicit default_awaiter_context(
        default_task_context_impl& self, ParentPromise& parent) noexcept
      // Register a callback that will request stop on this basic_task's
      // stop_source when stop is requested on the parent coroutine's stop
      // token.
      : stop_callback_{hpx::execution::experimental::get_stop_token(
                           hpx::execution::experimental::get_env(parent)),
            forward_stop_request{stop_source_}}
    {
        static_assert(std::is_nothrow_constructible_v<stop_callback_t,
            stop_token_t, forward_stop_request>);
        self.stop_token_ = stop_source_.get_token();
    }

    hpx::experimental::in_place_stop_source stop_source_{};
    stop_callback_t stop_callback_;
};

// If the parent coroutine's type has a stop token of type in_place_stop_token,
// we don't need to register a stop callback.
template <typename ParentPromise>
struct default_awaiter_context<ParentPromise,
    std::enable_if_t<
        std::is_same_v<hpx::experimental::in_place_stop_source,
            hpx::execution::experimental::stop_token_of_t<
                hpx::execution::experimental::env_of_t<ParentPromise>>> &&
        indirect_stop_token_provider<ParentPromise>>>
{
    explicit default_awaiter_context(
        default_task_context_impl& self, ParentPromise& parent) noexcept
    {
        self.stop_token_ = hpx::execution::experimental::get_stop_token(
            hpx::execution::experimental::get_env(parent));
    }
};

template <typename Token>
inline bool unstoppable_token = !Token::stop_possible();

// If the parent coroutine's stop token is unstoppable, there's no point
// forwarding stop tokens or stop requests at all.
template <typename ParentPromise>
struct default_awaiter_context<ParentPromise,
    std::enable_if_t<
        unstoppable_token<hpx::execution::experimental::stop_token_of_t<
            hpx::execution::experimental::env_of_t<ParentPromise>>> &&
        indirect_stop_token_provider<ParentPromise>>>
{
    explicit default_awaiter_context(
        default_task_context_impl&, ParentPromise&) noexcept
    {
    }
};

// Finally, if we don't know the parent coroutine's promise type, assume the
// worst and save a type-erased stop callback.
template <>
struct default_awaiter_context<void>
{
    explicit default_awaiter_context(default_task_context_impl&, auto&) noexcept
    {
    }

    template <typename ParentPromise,
        typename =
            std::enable_if_t<indirect_stop_token_provider<ParentPromise>>>
    explicit default_awaiter_context(
        default_task_context_impl& self, ParentPromise& parent)
    {
        // Register a callback that will request stop on this basic_task's
        // stop_source when stop is requested on the parent coroutine's stop
        // token.
        using stop_token_t = hpx::execution::experimental::stop_token_of_t<
            hpx::execution::experimental::env_of_t<ParentPromise>>;
        using stop_callback_t =
            typename stop_token_t::template callback_type<forward_stop_request>;

        if constexpr (std::is_same_v<stop_token_t,
                          hpx::experimental::in_place_stop_token>)
        {
            self.stop_token_ = hpx::execution::experimental::get_stop_token(
                hpx::execution::experimental::get_env(parent));
        }
        else if (auto token = hpx::execution::experimental::get_stop_token(
                     hpx::execution::experimental::get_env(parent));
            token.stop_possible())
        {
            stop_callback_.emplace<stop_callback_t>(
                std::move(token), forward_stop_request{stop_source_});
            self.stop_token_ = stop_source_.get_token();
        }
    }

    hpx::experimental::in_place_stop_source stop_source_{};
    std::any stop_callback_{};
};

template <typename ValueType>
using default_task_context = default_task_context_impl;

template <typename Promise, class ParentPromise = void>
using awaiter_context_t = typename hpx::execution::experimental::env_of_t<
    Promise>::template awaiter_context_t<Promise, ParentPromise>;

////////////////////////////////////////////////////////////////////////////////
// In a base class so it can be specialized when T is void:
template <typename T>
struct promise_base
{
    void return_value(T value) noexcept
    {
        data_.template emplace<1>(std::move(value));
    }
    std::variant<std::monostate, T, std::exception_ptr> data_{};
};

template <>
struct promise_base<void>
{
    struct _void
    {
    };
    void return_void() noexcept
    {
        data_.template emplace<1>(_void{});
    }
    std::variant<std::monostate, _void, std::exception_ptr> data_{};
};

HPX_HAS_MEMBER_XXX_TRAIT_DEF(stop_requested)

template <typename T>
struct stop_requested_ret_bool
  : std::integral_constant<bool,
        std::is_same_v<decltype(std::declval<T>().stop_requested()), bool>>
{
};

template <typename T, typename = void>
inline constexpr bool stop_requested_ret_bool_v = false;

template <typename T>
inline constexpr bool
    stop_requested_ret_bool_v<T, std::enable_if_t<has_stop_requested_v<T>>> =
        stop_requested_ret_bool<T>::value;

////////////////////////////////////////////////////////////////////////////////
// basic_task
template <typename T, typename Context = default_task_context<T>>
class basic_task
{
    struct _promise;

public:
    using promise_type = _promise;
    using type = basic_task;
    using id = basic_task;

    basic_task(basic_task&& that) noexcept
      : coro_(std::exchange(that.coro_, {}))
    {
    }

    ~basic_task()
    {
        if (coro_)
            coro_.destroy();
    }

private:
    struct final_awaitable
    {
        static std::false_type await_ready() noexcept
        {
            return {};
        }
        static hpx::coroutine_handle<> await_suspend(
            hpx::coroutine_handle<_promise> h) noexcept
        {
            return h.promise().continuation().handle();
        }
        static void await_resume() noexcept {}
    };

    struct _promise
      : promise_base<T>
      , hpx::execution::experimental::with_awaitable_senders<_promise>
    {
        basic_task get_return_object() noexcept
        {
            return basic_task(
                hpx::coroutine_handle<_promise>::from_promise(*this));
        }
        hpx::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        final_awaitable final_suspend() noexcept
        {
            return {};
        }
        void unhandled_exception() noexcept
        {
            this->data_.template emplace<2>(std::current_exception());
        }
        using context_t =
            typename Context::template promise_context_t<_promise>;
        friend context_t tag_invoke(hpx::execution::experimental::get_env_t,
            _promise const& self) noexcept
        {
            return self.context_;
        }
        context_t context_;
    };

    template <typename ParentPromise = void>
    struct task_awaitable
    {
        hpx::coroutine_handle<_promise> coro_;
        std::optional<awaiter_context_t<_promise, ParentPromise>> context_{};

        ~task_awaitable()
        {
            if (coro_)
            {
                coro_.destroy();
            }
        }

        static std::false_type await_ready() noexcept
        {
            return {};
        }

        template <typename ParentPromise2>
        hpx::coroutine_handle<> await_suspend(
            hpx::coroutine_handle<ParentPromise2> parent) noexcept
        {
            static_assert(
                hpx::meta::one_of<ParentPromise, ParentPromise2, void>::value);
            coro_.promise().set_continuation(parent);
            context_.emplace(coro_.promise().context_, parent.promise());
            if constexpr (stop_requested_ret_bool_v<decltype(coro_.promise())>)
            {
                if (coro_.promise().stop_requested())
                    return parent.promise().unhandled_stopped();
            }
            return coro_;
        }
        T await_resume()
        {
            context_.reset();
            scope_guard on_exit{
                [this]() noexcept { std::exchange(coro_, {}).destroy(); }};
            if (coro_.promise().data_.index() == 2)
                std::rethrow_exception(
                    std::get<2>(std::move(coro_.promise().data_)));
            if constexpr (!std::is_void_v<T>)
                return std::get<1>(std::move(coro_.promise().data_));
        }
    };

    // Make this task awaitable within a particular context:
    template <typename ParentPromise>
    friend task_awaitable<ParentPromise> tag_invoke(
        hpx::execution::experimental::as_awaitable_t, basic_task&& self,
        ParentPromise&) noexcept
    {
        return task_awaitable<ParentPromise>{std::exchange(self.coro_, {})};
    }

    // Make this task generally awaitable:
    friend task_awaitable<> operator co_await(basic_task&& self) noexcept
    {
        static_assert(well_formed<awaiter_context_t, _promise>);
        return task_awaitable<>{std::exchange(self.coro_, {})};
    }

    // Specify basic_task's completion signatures
    //   This is only necessary when basic_task is not generally awaitable
    //   owing to constraints imposed by its Context parameter.
    template <typename... Ts>
    using task_traits_t = hpx::execution::experimental::completion_signatures<
        hpx::execution::experimental::set_value_t(Ts...),
        hpx::execution::experimental::set_error_t(std::exception_ptr),
        hpx::execution::experimental::set_stopped_t()>;

    // clang-format off
    friend auto tag_invoke(
        hpx::execution::experimental::get_completion_signatures_t,
        const basic_task&, auto) -> std::conditional_t<std::is_void_v<T>,
                                     task_traits_t<>, task_traits_t<T>>;
    // clang-format on

    explicit basic_task(hpx::coroutine_handle<promise_type> hcoro) noexcept
      : coro_(hcoro)
    {
    }

    hpx::coroutine_handle<promise_type> coro_;
};

template <typename T, typename Context = default_task_context<T>>
using task = basic_task<T, Context>;
