//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX20_COROUTINES)

#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

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

    HPX_CXX_CORE_EXPORT struct task_scheduler : stdexec::task_scheduler
    {
        using stdexec::task_scheduler::task_scheduler;
        task_scheduler()
          : stdexec::task_scheduler(stdexec::inline_scheduler{})
        {
        }
    };

    HPX_CXX_CORE_EXPORT struct default_task_environment
    {
        using allocator_type = std::allocator<std::byte>;
        using scheduler_type = hpx::execution::experimental::task_scheduler;
        using stop_source_type = hpx::experimental::in_place_stop_source;
        using error_types =
            hpx::execution::experimental::completion_signatures<
                hpx::execution::experimental::set_error_t(std::exception_ptr)>;

        default_task_environment() = default;

        template <typename Env>
        constexpr explicit default_task_environment(Env const&) noexcept
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // with_error: P3552 error yield value wrapper
    template <typename Error>
    struct with_error
    {
        using type = std::decay_t<Error>;
        type error;
    };

    template <typename Error>
    with_error(Error) -> with_error<Error>;

    HPX_CXX_CORE_EXPORT template <typename T = void,
        typename TaskEnv = default_task_environment>
    class [[nodiscard]] task;

    namespace detail {

        template <typename Env, typename Default, typename = void>
        struct env_allocator
        {
            using type = Default;
        };

        template <typename Env, typename Default>
        struct env_allocator<Env, Default,
            std::void_t<typename Env::allocator_type>>
        {
            using type = typename Env::allocator_type;
        };

        template <typename Env, typename Default, typename = void>
        struct env_scheduler
        {
            using type = Default;
        };

        template <typename Env, typename Default>
        struct env_scheduler<Env, Default,
            std::void_t<typename Env::scheduler_type>>
        {
            using type = typename Env::scheduler_type;
        };

        template <typename Env, typename Default, typename = void>
        struct env_stop_source
        {
            using type = Default;
        };

        template <typename Env, typename Default>
        struct env_stop_source<Env, Default,
            std::void_t<typename Env::stop_source_type>>
        {
            using type = typename Env::stop_source_type;
        };

        template <typename Env, typename Default, typename = void>
        struct env_error_types
        {
            using type = Default;
        };

        template <typename Env, typename Default>
        struct env_error_types<Env, Default,
            std::void_t<typename Env::error_types>>
        {
            using type = typename Env::error_types;
        };

        template <typename TaskEnv>
        struct task_env_traits
        {
            using allocator_type =
                typename env_allocator<TaskEnv, std::allocator<std::byte>>::type;
            using scheduler_type = typename env_scheduler<TaskEnv,
                hpx::execution::experimental::task_scheduler>::type;
            using stop_source_type = typename env_stop_source<TaskEnv,
                hpx::experimental::in_place_stop_source>::type;
            using error_types = typename env_error_types<TaskEnv,
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>>::type;
        };

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
        // Helper to locate std::allocator_arg in any parameter position
        template <typename... Args>
        struct find_allocator
        {
            static auto get(Args const&...)
            {
                return std::allocator<std::byte>{};
            }
        };

        template <typename Alloc, typename... Rest>
        struct find_allocator<std::allocator_arg_t, Alloc, Rest...>
        {
            static auto get(
                std::allocator_arg_t, Alloc const& alloc, Rest const&...)
            {
                return alloc;
            }
        };

        template <typename First, typename... Rest>
        struct find_allocator<First, Rest...>
        {
            static auto get(First const&, Rest const&... rest)
            {
                return find_allocator<Rest...>::get(rest...);
            }
        };

        template <typename... Args>
        struct has_allocator_arg : std::false_type
        {
        };

        template <typename Alloc, typename... Rest>
        struct has_allocator_arg<std::allocator_arg_t, Alloc, Rest...>
          : std::true_type
        {
        };

        template <typename First, typename... Rest>
        struct has_allocator_arg<First, Rest...> : has_allocator_arg<Rest...>
        {
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
            using storage_type = std::conditional_t<std::is_reference_v<T>,
                std::add_pointer_t<T>, T>;

            void return_value(T val)
            {
                if constexpr (std::is_reference_v<T>)
                {
                    data_.template emplace<1>(std::addressof(val));
                }
                else
                {
                    data_.template emplace<1>(HPX_MOVE(val));
                }
            }

            std::variant<std::monostate, storage_type, std::exception_ptr>
                data_{};
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
        HPX_CXX_CORE_EXPORT template <typename T,
            typename TaskEnv = default_task_environment>
        struct task_promise
          : task_promise_base<T>
          , hpx::execution::experimental::with_awaitable_senders<
                task_promise<T, TaskEnv>>
        {
            using traits_type = task_env_traits<TaskEnv>;
            using allocator_type = typename traits_type::allocator_type;
            using scheduler_type = typename traits_type::scheduler_type;
            using stop_source_type = typename traits_type::stop_source_type;
            using error_types = typename traits_type::error_types;

            task_operation_base* state_{nullptr};
            bool stopped_{false};
            stop_source_type stop_source_{};
            decltype(stop_source_.get_token()) stop_token_{
                stop_source_.get_token()};
            [[no_unique_address]] TaskEnv env_{};
            scheduler_type scheduler_{};
            std::optional<allocator_type> alloc_{};
            bool has_custom_alloc_{false};

            task_promise() = default;

            template <typename... Args>
                requires (sizeof...(Args) > 0)
            explicit task_promise(Args const&... args)
            {
                if constexpr (has_allocator_arg<Args...>::value)
                {
                    has_custom_alloc_ = true;
                    if constexpr (std::is_constructible_v<allocator_type,
                                      decltype(find_allocator<
                                          Args...>::get(args...))>)
                    {
                        alloc_.emplace(
                            find_allocator<Args...>::get(args...));
                    }
                }
            }

            task<T, TaskEnv> get_return_object() noexcept;

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

            template <typename Error>
            auto yield_value(with_error<Error> e) noexcept
            {
                if constexpr (std::is_same_v<std::decay_t<Error>,
                                  std::exception_ptr>)
                {
                    this->data_.template emplace<2>(HPX_MOVE(e.error));
                }
                else
                {
                    this->data_.template emplace<2>(
                        std::make_exception_ptr(HPX_MOVE(e.error)));
                }
                return final_awaiter{};
            }

            struct env_type
            {
                task_promise const* promise_;

                auto query(hpx::execution::experimental::get_stop_token_t)
                    const noexcept
                {
                    return promise_->stop_token_;
                }

                auto query(hpx::execution::experimental::get_allocator_t)
                    const noexcept
                {
                    if (promise_->alloc_.has_value())
                    {
                        return *promise_->alloc_;
                    }
                    if constexpr (requires {
                                      hpx::execution::experimental::
                                          get_allocator(promise_->env_);
                                  })
                    {
                        return typename task_promise::allocator_type(
                            hpx::execution::experimental::get_allocator(
                                promise_->env_));
                    }
                    else if constexpr (std::is_default_constructible_v<
                                           allocator_type>)
                    {
                        return typename task_promise::allocator_type{};
                    }
                    else
                    {
                        std::terminate();
                    }
                }

                auto query(hpx::execution::experimental::get_scheduler_t)
                    const noexcept
                {
                    return promise_->scheduler_;
                }

                auto query(hpx::execution::experimental::get_start_scheduler_t)
                    const noexcept
                {
                    return promise_->scheduler_;
                }

                template <typename Tag>
                    requires(bool(hpx::execution::experimental::
                                 forwarding_query(Tag{})) &&
                        requires(Tag tag, TaskEnv const& env) { tag(env); })
                decltype(auto) query(Tag tag) const
                    noexcept(noexcept(tag(promise_->env_)))
                {
                    return tag(promise_->env_);
                }
            };

            env_type get_env() const noexcept
            {
                return env_type{this};
            }

            // P3552 Allocator Support
            template <typename Alloc>
            static void* allocate_with(std::size_t bytes, Alloc const& alloc)
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

            static void* operator new(std::size_t bytes)
            {
                return allocate_with(bytes, std::allocator<std::byte>{});
            }

            template <typename... Args>
            static void* operator new(std::size_t bytes, Args const&... args)
            {
                return allocate_with(
                    bytes, find_allocator<Args...>::get(args...));
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

            template <typename... Args>
            static void operator delete(
                void* ptr, std::size_t bytes, Args const&...) noexcept
            {
                operator delete(ptr, bytes);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // task_operation_state: operation state when connecting task to receiver
        template <typename T, typename TaskEnv, typename Receiver>
        struct task_operation_state : task_operation_base
        {
            using promise_type = task_promise<T, TaskEnv>;
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
                coro_.promise().state_ = this;

                if constexpr (std::is_constructible_v<TaskEnv,
                                  decltype(hpx::execution::experimental::get_env(
                                      rcvr_))>)
                {
                    coro_.promise().env_ =
                        TaskEnv(hpx::execution::experimental::get_env(rcvr_));
                }
                else
                {
                    coro_.promise().env_ = TaskEnv{};
                }

                if constexpr (requires {
                                  hpx::execution::experimental::get_scheduler(
                                      hpx::execution::experimental::get_env(
                                          rcvr_));
                              })
                {
                    if constexpr (std::is_constructible_v<
                                      typename promise_type::scheduler_type,
                                      decltype(hpx::execution::experimental::
                                              get_scheduler(
                                                  hpx::execution::
                                                      experimental::get_env(
                                                          rcvr_)))>)
                    {
                        coro_.promise().scheduler_ =
                            typename promise_type::scheduler_type(
                                hpx::execution::experimental::get_scheduler(
                                    hpx::execution::experimental::get_env(
                                        rcvr_)));
                    }
                }

                if (!coro_.promise().has_custom_alloc_)
                {
                    if constexpr (requires {
                                      hpx::execution::experimental::
                                          get_allocator(
                                              hpx::execution::experimental::
                                                  get_env(rcvr_));
                                  })
                    {
                        if constexpr (std::is_constructible_v<
                                          typename promise_type::allocator_type,
                                          decltype(hpx::execution::
                                                  experimental::get_allocator(
                                                      hpx::execution::
                                                          experimental::get_env(
                                                              rcvr_)))>)
                        {
                            coro_.promise().alloc_.emplace(
                                hpx::execution::experimental::get_allocator(
                                    hpx::execution::experimental::get_env(
                                        rcvr_)));
                        }
                    }
                }
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
                    else if constexpr (std::is_reference_v<T>)
                    {
                        auto* ptr = std::get<1>(HPX_MOVE(coro.promise().data_));
                        coro.destroy();
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(rcvr_), static_cast<T>(*ptr));
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
    // Production C++26 task<T, TaskEnv> conforming to P3552
    HPX_CXX_CORE_EXPORT template <typename T, typename TaskEnv>
    class [[nodiscard]] task
    {
    public:
        using is_sender = void;
        using sender_concept = hpx::execution::experimental::sender_t;
        using promise_type = detail::task_promise<T, TaskEnv>;

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
        detail::task_operation_state<T, TaskEnv, Receiver> connect(
            Receiver&& rcvr) &&
        {
            return detail::task_operation_state<T, TaskEnv, Receiver>{
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

                // 3. Forward parent promise's environment and scheduler
                if constexpr (requires { parent.promise().get_env(); })
                {
                    if constexpr (std::is_constructible_v<TaskEnv,
                                      decltype(parent.promise().get_env())>)
                    {
                        coro_.promise().env_ =
                            TaskEnv(parent.promise().get_env());
                    }

                    if constexpr (requires {
                                      hpx::execution::experimental::
                                          get_scheduler(
                                              parent.promise().get_env());
                                  })
                    {
                        if constexpr (std::is_constructible_v<
                                          typename promise_type::
                                              scheduler_type,
                                          decltype(hpx::execution::
                                                  experimental::get_scheduler(
                                                      parent.promise()
                                                          .get_env()))>)
                        {
                            coro_.promise().scheduler_ =
                                typename promise_type::scheduler_type(
                                    hpx::execution::experimental::
                                        get_scheduler(
                                            parent.promise().get_env()));
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
                    return unhandled_stopped_fn_(parent_promise_);
                }
                return continuation_;
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
                    if constexpr (std::is_reference_v<T>)
                    {
                        return static_cast<T>(
                            *std::get<1>(coro.promise().data_));
                    }
                    else
                    {
                        return std::get<1>(HPX_MOVE(coro.promise().data_));
                    }
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
        template <typename T, typename TaskEnv>
        task<T, TaskEnv> task_promise<T, TaskEnv>::get_return_object() noexcept
        {
            return task<T, TaskEnv>(
                hpx::coroutine_handle<task_promise>::from_promise(*this));
        }
    }    // namespace detail

}    // namespace hpx::execution::experimental

#endif    // HPX_HAVE_CXX20_COROUTINES
