//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2014-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/futures/detail/future_data.hpp>
#include <hpx/futures/traits/acquire_shared_state.hpp>
#include <hpx/futures/traits/future_access.hpp>
#include <hpx/futures/traits/future_traits.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/threading_base.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    template <typename Future, typename SourceState, typename DestinationState>
    HPX_FORCEINLINE void transfer_result(
        SourceState&& src, DestinationState& dest)
    {
        hpx::detail::try_catch_exception_ptr(
            [&]() {
                traits::future_access<Future>::transfer_result(
                    traits::future_access<Future>::create(
                        HPX_FORWARD(SourceState, src)),
                    *dest);
            },
            [&](std::exception_ptr ep) {
                (*dest).set_exception(HPX_MOVE(ep));
            });
    }

    template <typename Func, typename Future, typename Continuation>
    void invoke_continuation_nounwrap(
        Func& func, Future&& future, Continuation& cont)
    {
        hpx::intrusive_ptr<Continuation> keep_alive(&cont);
        hpx::detail::try_catch_exception_ptr(
            [&]() {
                constexpr bool is_void =
                    std::is_void_v<util::invoke_result_t<Func, Future&&>>;

                if constexpr (is_void)
                {
                    func(HPX_FORWARD(Future, future));
                    cont.set_value(util::unused);
                }
                else
                {
                    cont.set_value(func(HPX_FORWARD(Future, future)));
                }
            },
            [&](std::exception_ptr ep) { cont.set_exception(HPX_MOVE(ep)); });
    }

    template <typename Func, typename Future, typename Continuation>
    void invoke_continuation(Func& func, Future&& future, Continuation& cont)
    {
        using inner_future = util::invoke_result_t<Func, Future>;
        constexpr bool is_unique_future =
            traits::detail::is_unique_future_v<inner_future>;

        if constexpr (!is_unique_future)
        {
            hpx::scoped_annotation annotate(func);
            invoke_continuation_nounwrap(
                func, HPX_FORWARD(Future, future), cont);
        }
        else
        {
            hpx::intrusive_ptr<Continuation> cont_(&cont);

            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    using inner_shared_state_ptr =
                        traits::detail::shared_state_ptr_for_t<inner_future>;

                    // take by value, as the future may go away immediately
                    inner_shared_state_ptr inner_state =
                        traits::detail::get_shared_state(
                            func(HPX_FORWARD(Future, future)));
                    typename inner_shared_state_ptr::element_type* ptr =
                        inner_state.get();

                    if (ptr == nullptr)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::no_state,
                            "invoke_continuation",
                            "the inner future has no valid shared state");
                    }

                    // Bind an on_completed handler to this future that will
                    // transfer its result to the new future.
                    ptr->execute_deferred();
                    ptr->set_on_completed(
                        [inner_state = HPX_MOVE(inner_state),
                            cont_ = HPX_MOVE(cont_)]() mutable -> void {
                            return transfer_result<inner_future>(
                                HPX_MOVE(inner_state), cont_);
                        });
                },
                [&](std::exception_ptr ep) {
                    cont.set_exception(HPX_MOVE(ep));
                });
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Future, typename F, typename ContResult>
    class continuation : public detail::future_data<ContResult>
    {
    private:
        using base_type = future_data<ContResult>;

        using mutex_type = typename base_type::mutex_type;
        using result_type = typename base_type::result_type;

    protected:
        using base_type::mtx_;

        threads::thread_id_type get_id() const
        {
            std::lock_guard<mutex_type> l(mtx_);
            return id_;
        }
        void set_id(threads::thread_id_type id)
        {
            std::lock_guard<mutex_type> l(mtx_);
            id_ = HPX_MOVE(id);
        }

        struct reset_id
        {
            explicit reset_id(continuation& target)
              : target_(target)
            {
                if (threads::get_self_ptr() != nullptr)
                    target.set_id(threads::get_self_id());
            }

            reset_id(reset_id const&) = delete;
            reset_id(reset_id&&) = delete;
            reset_id& operator=(reset_id const&) = delete;
            reset_id& operator=(reset_id&&) = delete;

            ~reset_id()
            {
                target_.set_id(threads::invalid_thread_id);
            }

            continuation& target_;
        };

    public:
        using init_no_addref = typename base_type::init_no_addref;

        template <typename Func,
            typename Enable = std::enable_if_t<
                !std::is_same_v<std::decay_t<Func>, continuation>>>
        // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
        explicit continuation(Func&& f)
          : started_(false)
          , id_(threads::invalid_thread_id)
          , f_(HPX_FORWARD(Func, f))
        {
        }

        template <typename Func>
        continuation(init_no_addref no_addref, Func&& f)
          : base_type(no_addref)
          , started_(false)
          , id_(threads::invalid_thread_id)
          , f_(HPX_FORWARD(Func, f))
        {
        }

    protected:
        void ensure_started()
        {
            std::lock_guard<mutex_type> l(mtx_);
            if (started_)
            {
                HPX_THROW_EXCEPTION(hpx::error::task_already_started,
                    "continuation::ensure_started",
                    "this task has already been started");
            }
            started_ = true;
        }

        template <bool Unwrap>
        void run_impl(traits::detail::shared_state_ptr_for_t<Future>&& f)
        {
            auto future = traits::future_access<std::decay_t<Future>>::create(
                HPX_MOVE(f));
            if constexpr (Unwrap)
            {
                invoke_continuation(f_, HPX_MOVE(future), *this);
            }
            else
            {
                invoke_continuation_nounwrap(f_, HPX_MOVE(future), *this);
            }
        }

        template <bool Unwrap>
        void run(traits::detail::shared_state_ptr_for_t<Future>&& f)
        {
            ensure_started();
            run_impl<Unwrap>(HPX_MOVE(f));
        }

        template <bool Unwrap, typename Spawner>
        void async(traits::detail::shared_state_ptr_for_t<Future>&& f,
            Spawner&& spawner)
        {
            ensure_started();

            HPX_ASSERT(!this->runs_child_);

            hpx::intrusive_ptr<continuation> this_(this);
            hpx::threads::thread_description desc(f_, "async");
            spawner(
                [this_ = HPX_MOVE(this_), f = HPX_MOVE(f)]() mutable -> void {
                    this_->template run_impl<Unwrap>(HPX_MOVE(f));
                },
                desc, this->runs_child_);
        }

    public:
        // cancellation support
        bool cancelable() const noexcept override
        {
            return true;
        }

        void cancel() override
        {
            std::unique_lock<mutex_type> l(this->mtx_);
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    if (!this->started_)
                        HPX_THROW_THREAD_INTERRUPTED_EXCEPTION();

                    if (this->is_ready())
                        return;    // nothing we can do

#if defined(HPX_MSVC)
// 26110: Caller failing to hold lock 'l'
#pragma warning(push)
#pragma warning(disable : 26110)
#endif
                    if (id_ != threads::invalid_thread_id)
                    {
                        // interrupt the executing thread
                        threads::interrupt_thread(id_);

                        this->started_ = true;

                        l.unlock();
                        this->set_error(hpx::error::future_cancelled,
                            "continuation<Future, ContResult>::cancel",
                            "future has been canceled");
                    }
                    else
                    {
                        l.unlock();
                        HPX_THROW_EXCEPTION(
                            hpx::error::future_can_not_be_cancelled,
                            "continuation<Future, ContResult>::cancel",
                            "future can't be canceled at this time");
                    }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
                },
                [&](std::exception_ptr ep) {
                    this->started_ = true;
                    this->set_exception(ep);
                    std::rethrow_exception(HPX_MOVE(ep));
                });
        }

    public:
        ///////////////////////////////////////////////////////////////////////
        template <bool Unwrap, typename Spawner, typename Future_,
            typename Policy>
        void attach(Future_&& future, Spawner&& spawner, Policy&& policy,
            error_code& /*ec*/ = throws)
        {
            using shared_state_ptr =
                traits::detail::shared_state_ptr_for_t<Future_>;

            // bind an on_completed handler to this future that will invoke the
            // continuation
            hpx::intrusive_ptr<continuation> this_(this);
            shared_state_ptr state =
                traits::detail::get_shared_state(HPX_FORWARD(Future_, future));
            typename shared_state_ptr::element_type* ptr = state.get();

            if (ptr == nullptr)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_state,
                    "continuation::attach",
                    "the future to attach has no valid shared state");
            }

            ptr->execute_deferred();
            ptr->set_on_completed(
                [this_ = HPX_MOVE(this_), state = HPX_MOVE(state),
                    policy = HPX_FORWARD(Policy, policy),
                    spawner = HPX_FORWARD(Spawner, spawner)]() mutable -> void {
                    if (hpx::has_async_policy(policy))
                    {
                        this_->template async<Unwrap>(
                            HPX_MOVE(state), HPX_FORWARD(Spawner, spawner));
                    }
                    else
                    {
                        this_->template run<Unwrap>(HPX_MOVE(state));
                    }
                });
        }

    protected:
        bool started_;
        threads::thread_id_type id_;
        std::decay_t<F> f_;
    };

    template <typename Allocator, typename Future, typename F,
        typename ContResult>
    class continuation_allocator : public continuation<Future, F, ContResult>
    {
        using base_type = continuation<Future, F, ContResult>;

        using other_allocator = typename std::allocator_traits<
            Allocator>::template rebind_alloc<continuation_allocator>;

    public:
        using init_no_addref = typename base_type::init_no_addref;

        template <typename Func>
        continuation_allocator(other_allocator const& alloc, Func&& f)
          : base_type(HPX_FORWARD(Func, f))
          , alloc_(alloc)
        {
        }

        template <typename Func>
        continuation_allocator(
            init_no_addref no_addref, other_allocator const& alloc, Func&& f)
          : base_type(no_addref, HPX_FORWARD(Func, f))
          , alloc_(alloc)
        {
        }

    private:
        void destroy() noexcept override
        {
            using traits = std::allocator_traits<other_allocator>;

            other_allocator alloc(alloc_);
            traits::destroy(alloc, this);
            traits::deallocate(alloc, this, 1);
        }

        other_allocator alloc_;
    };
}    // namespace hpx::lcos::detail

template <typename Future, typename F, typename ContResult, typename Allocator>
struct hpx::traits::detail::shared_state_allocator<
    hpx::lcos::detail::continuation<Future, F, ContResult>, Allocator>
{
    using type =
        lcos::detail::continuation_allocator<Allocator, Future, F, ContResult>;
};

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename ContResult>
    class unwrap_continuation : public future_data<ContResult>
    {
    private:
        template <typename Outer>
        void on_outer_ready(
            traits::detail::shared_state_ptr_for_t<Outer>&& outer_state)
        {
            using inner_future = traits::future_traits_t<Outer>;
            using inner_shared_state_ptr =
                traits::detail::shared_state_ptr_for_t<inner_future>;

            // Bind an on_completed handler to this future which will transfer
            // its result to the new future.
            hpx::intrusive_ptr<unwrap_continuation> this_(this);

            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    // if we get here, this future is ready
                    Outer outer = traits::future_access<Outer>::create(
                        HPX_MOVE(outer_state));

                    // take by value, as the future will go away immediately
                    inner_shared_state_ptr inner_state =
                        traits::detail::get_shared_state(outer.get());
                    typename inner_shared_state_ptr::element_type* ptr =
                        inner_state.get();

                    if (ptr == nullptr)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::no_state,
                            "unwrap_continuation<ContResult>::on_outer_ready",
                            "the inner future has no valid shared state");
                    }

                    ptr->execute_deferred();

                    // attach continuation on inner ready
                    ptr->set_on_completed(
                        [this_ = HPX_MOVE(this_),
                            inner = HPX_MOVE(inner_state)]() mutable -> void {
                            transfer_result<inner_future>(
                                HPX_MOVE(inner), this_);
                        });
                },
                [&](std::exception_ptr ep) {
                    this->set_exception(HPX_MOVE(ep));
                });
        }

    public:
        using init_no_addref = typename future_data<ContResult>::init_no_addref;

        unwrap_continuation() = default;

        explicit unwrap_continuation(init_no_addref no_addref) noexcept
          : future_data<ContResult>(no_addref)
        {
        }

        template <typename Future>
        void attach(Future&& future)
        {
            using outer_shared_state_ptr =
                traits::detail::shared_state_ptr_for_t<Future>;

            // Bind an on_completed handler to this future which will wait for
            // the inner future and will transfer its result to the new future.
            hpx::intrusive_ptr<unwrap_continuation> this_(this);

            outer_shared_state_ptr outer_state =
                traits::detail::get_shared_state(future);
            typename outer_shared_state_ptr::element_type* ptr =
                outer_state.get();

            if (ptr == nullptr)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_state,
                    "unwrap_continuation<ContResult>::attach",
                    "the future has no valid shared state");
            }

            ptr->execute_deferred();
            ptr->set_on_completed(
                [this_ = HPX_MOVE(this_),
                    outer = HPX_MOVE(outer_state)]() mutable -> void {
                    this_->template on_outer_ready<Future>(HPX_MOVE(outer));
                });
        }
    };

    template <typename Allocator, typename ContResult>
    class unwrap_continuation_allocator : public unwrap_continuation<ContResult>
    {
        using base_type = unwrap_continuation<ContResult>;

        using other_allocator = typename std::allocator_traits<
            Allocator>::template rebind_alloc<unwrap_continuation_allocator>;

    public:
        using init_no_addref = typename base_type::init_no_addref;

        explicit unwrap_continuation_allocator(other_allocator const& alloc)
          : alloc_(alloc)
        {
        }

        unwrap_continuation_allocator(
            init_no_addref no_addref, other_allocator const& alloc)
          : base_type(no_addref)
          , alloc_(alloc)
        {
        }

    private:
        void destroy() noexcept override
        {
            using traits = std::allocator_traits<other_allocator>;

            other_allocator alloc(alloc_);
            traits::destroy(alloc, this);
            traits::deallocate(alloc, this, 1);
        }

        other_allocator alloc_;
    };
}    // namespace hpx::lcos::detail

template <typename ContResult, typename Allocator>
struct hpx::traits::detail::shared_state_allocator<
    hpx::lcos::detail::unwrap_continuation<ContResult>, Allocator>
{
    using type =
        lcos::detail::unwrap_continuation_allocator<Allocator, ContResult>;
};

namespace hpx::lcos::detail {

    template <typename Allocator, typename Future>
    traits::detail::shared_state_ptr_t<future_unwrap_result_t<Future>>
    unwrap_impl_alloc(Allocator const& a, Future&& future, error_code& /*ec*/)
    {
        using base_allocator = Allocator;
        using result_type = future_unwrap_result_t<Future>;

        using shared_state = traits::shared_state_allocator_t<
            detail::unwrap_continuation<result_type>, base_allocator>;

        using other_allocator = typename std::allocator_traits<
            base_allocator>::template rebind_alloc<shared_state>;
        using traits = std::allocator_traits<other_allocator>;

        using init_no_addref = typename shared_state::init_no_addref;

        using unique_ptr = std::unique_ptr<shared_state,
            util::allocator_deleter<other_allocator>>;

        other_allocator alloc(a);
        unique_ptr p(traits::allocate(alloc, 1),
            util::allocator_deleter<other_allocator>{alloc});

        traits::construct(alloc, p.get(), init_no_addref{}, alloc);

        // create a continuation
        hpx::traits::detail::shared_state_ptr_t<result_type> result(
            p.release(), false);
        static_cast<shared_state*>(result.get())
            ->attach(HPX_FORWARD(Future, future));
        return result;
    }

    HPX_CXX_CORE_EXPORT template <typename Allocator, typename Future>
    traits::detail::shared_state_ptr_t<future_unwrap_result_t<Future>>
    unwrap_alloc(Allocator const& a, Future&& future, error_code& ec)
    {
        return unwrap_impl_alloc(a, HPX_FORWARD(Future, future), ec);
    }

    HPX_CXX_CORE_EXPORT template <typename Future>
    traits::detail::shared_state_ptr_t<future_unwrap_result_t<Future>> unwrap(
        Future&& future, error_code& ec)
    {
        using allocator_type = hpx::util::thread_local_caching_allocator<
            hpx::lockfree::variable_size_stack,
            hpx::util::internal_allocator<>>;
        return unwrap_impl_alloc(
            allocator_type{}, HPX_FORWARD(Future, future), ec);
    }
}    // namespace hpx::lcos::detail
