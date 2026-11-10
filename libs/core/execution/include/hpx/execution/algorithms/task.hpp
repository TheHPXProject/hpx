//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX20_COROUTINES)

#include <hpx/execution_base/stdexec_forward.hpp>
#include <hpx/synchronization/stop_token.hpp>
#include <hpx/type_support/coroutines_support.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace hpx::execution::experimental {

    template <typename T = void>
    class [[nodiscard]] task;

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Memory block for P3552 coroutine frame allocator alignment
        struct alignas(alignof(std::max_align_t)) memblock
        {
            std::byte storage[alignof(std::max_align_t)];
        };

        constexpr std::size_t divmod(
            std::size_t total_size, std::size_t chunk_size) noexcept
        {
            return (total_size / chunk_size) +
                (total_size % chunk_size != 0 ? 1 : 0);
        }

        ///////////////////////////////////////////////////////////////////////
        // Type-erased allocator deallocation interface
        struct any_alloc_base
        {
            virtual ~any_alloc_base() = default;
            virtual void deallocate(void* ptr, std::size_t bytes) noexcept = 0;
        };

        template <typename Alloc>
        struct any_alloc final : any_alloc_base
        {
            using value_type =
                typename std::allocator_traits<Alloc>::value_type;
            static_assert(std::is_same_v<value_type, memblock>);

            explicit any_alloc(Alloc alloc)
              : alloc_(HPX_MOVE(alloc))
            {
            }

            void deallocate(void* ptr, std::size_t bytes) noexcept override
            {
                std::size_t const promise_blocks =
                    divmod(bytes, sizeof(memblock));
                static constexpr std::size_t alloc_blocks =
                    divmod(sizeof(any_alloc<Alloc>), sizeof(memblock));

                auto alloc = HPX_MOVE(alloc_);
                std::destroy_at(this);
                std::allocator_traits<Alloc>::deallocate(alloc,
                    static_cast<memblock*>(ptr), promise_blocks + alloc_blocks);
            }

            Alloc alloc_;
        };

        ///////////////////////////////////////////////////////////////////////
        // Forward stop request callback functor
        struct forward_task_stop_request
        {
            hpx::experimental::in_place_stop_source& stop_source_;

            void operator()() const noexcept
            {
                stop_source_.request_stop();
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // Abstract base for operation state or awaiter driving the task
        struct task_operation_base
        {
            virtual ~task_operation_base() = default;
            virtual hpx::coroutine_handle<> completed() noexcept = 0;
            virtual hpx::coroutine_handle<> canceled() noexcept = 0;
        };

        ///////////////////////////////////////////////////////////////////////
        // promise_base: return value storage specialization
        template <typename T>
        struct task_promise_base
        {
            void return_value(T val)
            {
                data_.template emplace<1>(HPX_MOVE(val));
            }

            std::variant<std::monostate, T, std::exception_ptr> data_{};
        };

        template <>
        struct task_promise_base<void>
        {
            struct void_type
            {
            };

            void return_void() noexcept
            {
                data_.template emplace<1>(void_type{});
            }

            std::variant<std::monostate, void_type, std::exception_ptr> data_{};
        };

        ///////////////////////////////////////////////////////////////////////
        // final_awaiter: symmetric transfer back to continuation or opstate
        struct final_awaiter
        {
            static constexpr bool await_ready() noexcept
            {
                return false;
            }

            template <typename Promise>
            static hpx::coroutine_handle<> await_suspend(
                hpx::coroutine_handle<Promise> h) noexcept
            {
                if (h.promise().state_)
                {
                    return h.promise().state_->completed();
                }
                return h.promise().continuation().handle();
            }

            static void await_resume() noexcept {}
        };

        ///////////////////////////////////////////////////////////////////////
        // task_promise: C++26 P3552 coroutine promise type
        template <typename T>
        struct task_promise
          : task_promise_base<T>
          , hpx::execution::experimental::with_awaitable_senders<
                task_promise<T>>
        {
            task_operation_base* state_{nullptr};
            bool stopped_{false};
            hpx::experimental::in_place_stop_source stop_source_{};
            hpx::experimental::in_place_stop_token stop_token_{
                stop_source_.get_token()};

            task<T> get_return_object() noexcept;

            hpx::suspend_always initial_suspend() noexcept
            {
                return {};
            }

            final_awaiter final_suspend() noexcept
            {
                return {};
            }

            void unhandled_exception() noexcept
            {
                this->data_.template emplace<2>(std::current_exception());
            }

            hpx::coroutine_handle<> unhandled_stopped() noexcept
            {
                stopped_ = true;
                if (state_)
                {
                    return state_->canceled();
                }
                return hpx::noop_coroutine();
            }

            struct env_type
            {
                task_promise const* promise_;

                auto query(hpx::execution::experimental::get_stop_token_t)
                    const noexcept
                {
                    return promise_->stop_token_;
                }
            };

            env_type get_env() const noexcept
            {
                return env_type{this};
            }

            // P3552 Allocator Support
            static void* operator new(std::size_t bytes)
            {
                return operator new(
                    bytes, std::allocator_arg, std::allocator<std::byte>{});
            }

            static void operator delete(void* ptr, std::size_t bytes) noexcept
            {
                std::size_t const promise_blocks =
                    divmod(bytes, sizeof(memblock));
                void* const alloc_loc =
                    static_cast<memblock*>(ptr) + promise_blocks;
                auto* alloc = static_cast<any_alloc_base*>(alloc_loc);
                alloc->deallocate(ptr, bytes);
            }

            template <typename Alloc, typename... Args>
            static void* operator new(std::size_t bytes, std::allocator_arg_t,
                Alloc const& alloc, Args const&...)
            {
                using palloc_t = typename std::allocator_traits<
                    Alloc>::template rebind_alloc<memblock>;
                using pointer_t =
                    typename std::allocator_traits<palloc_t>::pointer;
                static_assert(std::is_pointer_v<pointer_t>,
                    "Allocator pointer type must be a raw pointer");

                static constexpr std::size_t alloc_blocks =
                    divmod(sizeof(any_alloc<palloc_t>), sizeof(memblock));
                std::size_t const promise_blocks =
                    divmod(bytes, sizeof(memblock));

                palloc_t palloc(alloc);
                auto* const ptr = std::allocator_traits<palloc_t>::allocate(
                    palloc, promise_blocks + alloc_blocks);

                void* const alloc_loc = ptr + promise_blocks;
                std::construct_at(static_cast<any_alloc<palloc_t>*>(alloc_loc),
                    HPX_MOVE(palloc));
                return ptr;
            }

            template <typename Alloc, typename... Args>
            static void operator delete(void* ptr, std::size_t bytes,
                std::allocator_arg_t, Alloc const&, Args const&...) noexcept
            {
                operator delete(ptr, bytes);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // task_operation_state: operation state when connecting task to receiver
        template <typename T, typename Receiver>
        struct task_operation_state : task_operation_base
        {
            using promise_type = task_promise<T>;
            using receiver_type = std::decay_t<Receiver>;

            using stop_token_type =
                hpx::execution::experimental::stop_token_of_t<
                    hpx::execution::experimental::env_of_t<receiver_type>>;
            using stop_callback_type =
                typename stop_token_type::template callback_type<
                    forward_task_stop_request>;

            hpx::coroutine_handle<promise_type> coro_;
            receiver_type rcvr_;
            std::optional<stop_callback_type> stop_callback_{};

            task_operation_state(hpx::coroutine_handle<promise_type> coro,
                Receiver&& rcvr) noexcept(std::
                    is_nothrow_move_constructible_v<receiver_type>)
              : coro_(coro)
              , rcvr_(HPX_FORWARD(Receiver, rcvr))
            {
            }

            task_operation_state(task_operation_state&&) = delete;
            task_operation_state(task_operation_state const&) = delete;
            task_operation_state& operator=(task_operation_state&&) = delete;
            task_operation_state& operator=(
                task_operation_state const&) = delete;

            ~task_operation_state() override
            {
                if (coro_)
                {
                    coro_.destroy();
                }
            }

            void start() & noexcept
            {
                if (!coro_)
                {
                    return;
                }

                coro_.promise().state_ = this;

                auto token = hpx::execution::experimental::get_stop_token(
                    hpx::execution::experimental::get_env(rcvr_));

                if (token.stop_requested())
                {
                    auto coro = std::exchange(coro_, {});
                    coro.destroy();
                    hpx::execution::experimental::set_stopped(HPX_MOVE(rcvr_));
                    return;
                }

                if (token.stop_possible())
                {
                    stop_callback_.emplace(token,
                        forward_task_stop_request{
                            coro_.promise().stop_source_});
                }

                coro_.resume();
            }

            hpx::coroutine_handle<> completed() noexcept override
            {
                stop_callback_.reset();
                auto coro = std::exchange(coro_, {});

                if (coro.promise().stopped_)
                {
                    coro.destroy();
                    hpx::execution::experimental::set_stopped(HPX_MOVE(rcvr_));
                }
                else if (coro.promise().data_.index() == 2)
                {
                    auto ep = std::get<2>(HPX_MOVE(coro.promise().data_));
                    coro.destroy();
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(rcvr_), HPX_MOVE(ep));
                }
                else
                {
                    if constexpr (std::is_void_v<T>)
                    {
                        coro.destroy();
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(rcvr_));
                    }
                    else
                    {
                        auto val = std::get<1>(HPX_MOVE(coro.promise().data_));
                        coro.destroy();
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(rcvr_), HPX_MOVE(val));
                    }
                }
                return hpx::noop_coroutine();
            }

            hpx::coroutine_handle<> canceled() noexcept override
            {
                stop_callback_.reset();
                auto coro = std::exchange(coro_, {});
                coro.destroy();
                hpx::execution::experimental::set_stopped(HPX_MOVE(rcvr_));
                return hpx::noop_coroutine();
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // Helper to deduce parent coroutine stop token
        template <typename ParentPromise, typename = void>
        struct parent_stop_token
        {
            using type = hpx::experimental::in_place_stop_token;
        };

        template <typename ParentPromise>
            requires requires(ParentPromise const& p) {
                hpx::execution::experimental::get_stop_token(p.get_env());
            }
        struct parent_stop_token<ParentPromise>
        {
            using type = decltype(hpx::execution::experimental::get_stop_token(
                std::declval<ParentPromise const&>().get_env()));
        };

        template <typename ParentPromise>
        using parent_stop_token_t =
            typename parent_stop_token<ParentPromise>::type;

    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Production C++26 task<T> conforming to P3552
    template <typename T>
    class [[nodiscard]] task
    {
    public:
        using is_sender = void;
        using sender_concept = hpx::execution::experimental::sender_t;
        using promise_type = detail::task_promise<T>;

        task() noexcept = default;

        explicit task(hpx::coroutine_handle<promise_type> coro) noexcept
          : coro_(coro)
        {
        }

        task(task&& other) noexcept
          : coro_(std::exchange(other.coro_, {}))
        {
        }

        task& operator=(task&&) = delete;
        task(task const&) = delete;
        task& operator=(task const&) = delete;

        ~task()
        {
            if (coro_)
            {
                coro_.destroy();
            }
        }

        template <typename Self, typename... Env>
        static consteval auto get_completion_signatures(
            Self&&, Env&&...) noexcept
        {
            if constexpr (std::is_void_v<T>)
            {
                return hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr),
                    hpx::execution::experimental::set_stopped_t()>{};
            }
            else
            {
                return hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(T),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr),
                    hpx::execution::experimental::set_stopped_t()>{};
            }
        }

        template <typename Receiver>
        detail::task_operation_state<T, Receiver> connect(Receiver&& rcvr) &&
        {
            return detail::task_operation_state<T, Receiver>{
                std::exchange(coro_, {}), HPX_FORWARD(Receiver, rcvr)};
        }

        template <typename ParentPromise = void>
        struct awaiter : detail::task_operation_base
        {
            using stop_token_type =
                std::conditional_t<std::is_void_v<ParentPromise>,
                    hpx::experimental::in_place_stop_token,
                    detail::parent_stop_token_t<ParentPromise>>;

            using stop_callback_type =
                typename stop_token_type::template callback_type<
                    detail::forward_task_stop_request>;

            hpx::coroutine_handle<promise_type> coro_;
            hpx::coroutine_handle<> continuation_{};
            hpx::coroutine_handle<> stopped_continuation_{};
            std::optional<stop_callback_type> stop_callback_{};
            void* parent_promise_{nullptr};
            hpx::coroutine_handle<> (*unhandled_stopped_fn_)(void*){nullptr};

            explicit awaiter(hpx::coroutine_handle<promise_type> coro) noexcept
              : coro_(coro)
            {
            }

            awaiter(awaiter&& other) noexcept
              : coro_(std::exchange(other.coro_, {}))
              , continuation_(std::exchange(other.continuation_, {}))
              , stopped_continuation_(
                    std::exchange(other.stopped_continuation_, {}))
              , parent_promise_(std::exchange(other.parent_promise_, nullptr))
              , unhandled_stopped_fn_(
                    std::exchange(other.unhandled_stopped_fn_, nullptr))
            {
            }

            awaiter(awaiter const&) = delete;
            awaiter& operator=(awaiter const&) = delete;
            awaiter& operator=(awaiter&&) = delete;

            ~awaiter() override
            {
                if (coro_)
                {
                    coro_.destroy();
                }
            }

            static constexpr bool await_ready() noexcept
            {
                return false;
            }

            template <typename ParentPromise2>
            hpx::coroutine_handle<> await_suspend(
                hpx::coroutine_handle<ParentPromise2> parent) noexcept
            {
                coro_.promise().set_continuation(parent);
                coro_.promise().state_ = this;
                continuation_ = parent;

                // 1. Extract unhandled_stopped continuation for symmetric
                // transfer
                parent_promise_ = &parent.promise();
                unhandled_stopped_fn_ =
                    [](void* p) noexcept -> hpx::coroutine_handle<> {
                    return static_cast<ParentPromise2*>(p)->unhandled_stopped();
                };

                // 2. Safely bridge the parent stop token to nested child task
                if constexpr (requires {
                                  hpx::execution::experimental::get_stop_token(
                                      parent.promise().get_env());
                              })
                {
                    auto token = hpx::execution::experimental::get_stop_token(
                        parent.promise().get_env());

                    if constexpr (std::is_constructible_v<stop_callback_type,
                                      decltype(token),
                                      detail::forward_task_stop_request>)
                    {
                        if (token.stop_possible())
                        {
                            stop_callback_.emplace(token,
                                detail::forward_task_stop_request{
                                    coro_.promise().stop_source_});
                        }
                    }
                }

                return coro_;
            }

            hpx::coroutine_handle<> completed() noexcept override
            {
                stop_callback_.reset();
                return continuation_;
            }

            hpx::coroutine_handle<> canceled() noexcept override
            {
                stop_callback_.reset();
                // Transfer control to parent's stop path (bypassing
                // await_resume)
                if (unhandled_stopped_fn_)
                {
                    stopped_continuation_ =
                        unhandled_stopped_fn_(parent_promise_);
                }
                return stopped_continuation_ ? stopped_continuation_ :
                                               continuation_;
            }

            T await_resume()
            {
                auto coro = std::exchange(coro_, {});
                struct scoped_destroy
                {
                    hpx::coroutine_handle<promise_type> c;
                    ~scoped_destroy()
                    {
                        if (c)
                        {
                            c.destroy();
                        }
                    }
                } cleanup{coro};

                if (coro.promise().data_.index() == 2)
                {
                    std::rethrow_exception(
                        std::get<2>(HPX_MOVE(coro.promise().data_)));
                }

                if constexpr (!std::is_void_v<T>)
                {
                    return std::get<1>(HPX_MOVE(coro.promise().data_));
                }
            }
        };

        // General operator co_await
        friend awaiter<> operator co_await(task&& self) noexcept
        {
            return awaiter<>{std::exchange(self.coro_, {})};
        }

        // Context-aware as_awaitable
        template <typename ParentPromise>
        awaiter<ParentPromise> as_awaitable(ParentPromise&) && noexcept
        {
            return awaiter<ParentPromise>{std::exchange(coro_, {})};
        }

    private:
        hpx::coroutine_handle<promise_type> coro_{};
    };

    namespace detail {
        template <typename T>
        task<T> task_promise<T>::get_return_object() noexcept
        {
            return task<T>(
                hpx::coroutine_handle<task_promise>::from_promise(*this));
        }
    }    // namespace detail

}    // namespace hpx::execution::experimental

namespace hpx::experimental {
    using hpx::execution::experimental::task;
}

#endif    // HPX_HAVE_CXX20_COROUTINES
