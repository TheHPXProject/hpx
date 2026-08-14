//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>

#include <hpx/futures/detail/future_data.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/futures/traits/future_access.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::lcos::local {

    ///////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename Result, typename F, typename Executor,
            typename Base = lcos::detail::task_base<Result>>
        struct task_object;

        template <typename Result, typename F, typename Base>
        struct task_object<Result, F, void, Base> : Base
        {
            using base_type = Base;
            using result_type = typename Base::result_type;
            using init_no_addref = typename Base::init_no_addref;

            std::decay_t<F> f_;

            explicit task_object(F const& f)
              : f_(f)
            {
            }

            explicit task_object(F&& f) noexcept
              : f_(HPX_MOVE(f))
            {
            }

            task_object(init_no_addref no_addref, F const& f)
              : base_type(no_addref)
              , f_(f)
            {
            }

            task_object(init_no_addref no_addref, F&& f) noexcept
              : base_type(no_addref)
              , f_(HPX_MOVE(f))
            {
            }

            task_object(task_object const&) = delete;
            task_object(task_object&&) = delete;
            task_object& operator=(task_object const&) = delete;
            task_object& operator=(task_object&&) = delete;

            ~task_object() override = default;

            void do_run() noexcept override
            {
                hpx::intrusive_ptr<base_type> this_(this);
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if constexpr (std::is_void_v<Result>)
                        {
                            f_();
                            this->set_value(result_type());
                        }
                        else
                        {
                            this->set_value(f_());
                        }
                    },
                    [&](std::exception_ptr ep) {
                        this->set_exception(HPX_MOVE(ep));
                    });
            }

        protected:
            // run in a separate thread
            threads::thread_id_ref_type post(threads::thread_pool_base* pool,
                char const* annotation, launch policy, error_code& ec) override
            {
                if (this->started_test_and_set())
                {
                    return threads::invalid_thread_id;
                }

                auto hint = policy.hint();
                if (hint.runs_as_child_mode() ==
                    hpx::threads::thread_execution_hint::run_as_child)
                {
                    if (!pool->get_scheduler()->supports_direct_execution())
                    {
                        hint.runs_as_child_mode(
                            hpx::threads::thread_execution_hint::none);
                        policy.set_hint(hint);
                    }
                }

                hpx::intrusive_ptr<base_type> this_(this);
                if (policy == launch::fork)
                {
                    threads::thread_init_data data(
                        threads::make_thread_function_nullary(
                            &base_type::run_impl, HPX_MOVE(this_)),
                        threads::thread_description(f_, annotation),
                        policy.priority(),
                        threads::thread_schedule_hint(
                            hpx::threads::thread_schedule_hint_mode::thread,
                            static_cast<std::int16_t>(get_worker_thread_num()),
                            hint.placement_mode(), hint.runs_as_child_mode()),
                        policy.stacksize(),
                        threads::thread_schedule_state::pending_do_not_schedule,
                        true);

                    if (hint.runs_as_child_mode() ==
                        hpx::threads::thread_execution_hint::run_as_child)
                    {
                        HPX_ASSERT(
                            this->runs_child_ == threads::invalid_thread_id);
                        threads::register_thread(
                            data, pool, this->runs_child_, ec);
                        return this->runs_child_;
                    }

                    return threads::register_thread(data, pool, ec);
                }

                if (hint.runs_as_child_mode() ==
                    hpx::threads::thread_execution_hint::run_as_child)
                {
                    // create the thread without running it
                    threads::thread_init_data data(
                        threads::make_thread_function_nullary(
                            &base_type::run_impl, std::move(this_)),
                        threads::thread_description(f_, annotation),
                        policy.priority(), policy.hint(), policy.stacksize(),
                        threads::thread_schedule_state::suspended, true);

                    HPX_ASSERT(this->runs_child_ == threads::invalid_thread_id);
                    threads::register_thread(data, pool, this->runs_child_, ec);

                    // now run the thread
                    threads::set_thread_state(this->runs_child_.noref(),
                        threads::thread_schedule_state::pending,
                        threads::thread_restart_state::signaled,
                        policy.priority(), true, ec);

                    return this->runs_child_;
                }

                threads::thread_init_data data(
                    threads::make_thread_function_nullary(
                        &base_type::run_impl, HPX_MOVE(this_)),
                    threads::thread_description(f_, annotation),
                    policy.priority(), policy.hint(), policy.stacksize(),
                    threads::thread_schedule_state::pending);

                return threads::register_work(data, pool, ec);
            }
        };

        template <typename Allocator, typename Result, typename F,
            typename Base>
        struct task_object_allocator : task_object<Result, F, void, Base>
        {
            using base_type = task_object<Result, F, void, Base>;
            using result_type = typename base_type::result_type;
            using init_no_addref = typename base_type::init_no_addref;

            using other_allocator = typename std::allocator_traits<
                Allocator>::template rebind_alloc<task_object_allocator>;

            task_object_allocator(other_allocator const& alloc, F const& f)
              : base_type(f)
              , alloc_(alloc)
            {
            }

            task_object_allocator(other_allocator const& alloc, F&& f) noexcept
              : base_type(HPX_MOVE(f))
              , alloc_(alloc)
            {
            }

            task_object_allocator(init_no_addref no_addref,
                other_allocator const& alloc, F const& f)
              : base_type(no_addref, f)
              , alloc_(alloc)
            {
            }

            task_object_allocator(init_no_addref no_addref,
                other_allocator const& alloc, F&& f) noexcept
              : base_type(no_addref, HPX_MOVE(f))
              , alloc_(alloc)
            {
            }

            task_object_allocator(task_object_allocator const&) = delete;
            task_object_allocator(task_object_allocator&&) = delete;
            task_object_allocator& operator=(
                task_object_allocator const&) = delete;
            task_object_allocator& operator=(task_object_allocator&&) = delete;

            ~task_object_allocator() override = default;

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

        template <typename Result, typename F, typename Executor, typename Base>
        struct task_object : task_object<Result, F, void, Base>
        {
            using base_type = task_object<Result, F, void, Base>;
            using result_type = typename base_type::result_type;
            using init_no_addref = typename base_type::init_no_addref;

            Executor* exec_ = nullptr;

            explicit task_object(F const& f)
              : base_type(f)
            {
            }

            explicit task_object(F&& f) noexcept
              : base_type(HPX_MOVE(f))
            {
            }

            task_object(Executor& exec, F const& f)
              : base_type(f)
              , exec_(&exec)
            {
            }

            task_object(Executor& exec, F&& f) noexcept
              : base_type(HPX_MOVE(f))
              , exec_(&exec)
            {
            }

            task_object(init_no_addref no_addref, F const& f)
              : base_type(no_addref, f)
            {
            }

            task_object(init_no_addref no_addref, F&& f) noexcept
              : base_type(no_addref, HPX_MOVE(f))
            {
            }

            task_object(Executor& exec, init_no_addref no_addref, F const& f)
              : base_type(no_addref, f)
              , exec_(&exec)
            {
            }

            task_object(
                Executor& exec, init_no_addref no_addref, F&& f) noexcept
              : base_type(no_addref, HPX_MOVE(f))
              , exec_(&exec)
            {
            }

            task_object(task_object const&) = delete;
            task_object(task_object&&) = delete;
            task_object& operator=(task_object const&) = delete;
            task_object& operator=(task_object&&) = delete;

            ~task_object() = default;

        protected:
            // run in a separate thread
            threads::thread_id_ref_type post(threads::thread_pool_base* pool,
                char const* annotation, launch policy, error_code& ec) override
            {
                if (exec_)
                {
                    if (!this->started_test_and_set())
                    {
                        hpx::intrusive_ptr<base_type> this_(this);
                        parallel::execution::post(*exec_,
                            util::deferred_call(
                                &base_type::run_impl, HPX_MOVE(this_)),
                            exec_->get_schedulehint(), annotation);
                    }
                    return threads::invalid_thread_id;
                }

                return this->base_type::post(pool, annotation, policy, ec);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Result, typename F, typename Executor>
        struct cancelable_task_object;

        template <typename Result, typename F>
        struct cancelable_task_object<Result, F, void>
          : task_object<Result, F, void,
                lcos::detail::cancelable_task_base<Result>>
        {
            using base_type = task_object<Result, F, void,
                lcos::detail::cancelable_task_base<Result>>;
            using result_type = typename base_type::result_type;
            using init_no_addref = typename base_type::init_no_addref;

            explicit cancelable_task_object(F const& f)
              : base_type(f)
            {
            }

            explicit cancelable_task_object(F&& f) noexcept
              : base_type(HPX_MOVE(f))
            {
            }

            cancelable_task_object(init_no_addref no_addref, F const& f)
              : base_type(no_addref, f)
            {
            }

            cancelable_task_object(init_no_addref no_addref, F&& f) noexcept
              : base_type(no_addref, HPX_MOVE(f))
            {
            }

            cancelable_task_object(cancelable_task_object const&) = delete;
            cancelable_task_object(cancelable_task_object&&) = delete;
            cancelable_task_object& operator=(
                cancelable_task_object const&) = delete;
            cancelable_task_object& operator=(
                cancelable_task_object&&) = delete;

            ~cancelable_task_object() = default;
        };

        template <typename Allocator, typename Result, typename F>
        struct cancelable_task_object_allocator
          : cancelable_task_object<Result, F, void>
        {
            using base_type = cancelable_task_object<Result, F, void>;
            using result_type = typename base_type::result_type;
            using init_no_addref = typename base_type::init_no_addref;

            using other_allocator = typename std::allocator_traits<Allocator>::
                template rebind_alloc<cancelable_task_object_allocator>;

            cancelable_task_object_allocator(
                other_allocator const& alloc, F const& f)
              : base_type(f)
              , alloc_(alloc)
            {
            }

            cancelable_task_object_allocator(
                other_allocator const& alloc, F&& f) noexcept
              : base_type(HPX_MOVE(f))
              , alloc_(alloc)
            {
            }

            cancelable_task_object_allocator(init_no_addref no_addref,
                other_allocator const& alloc, F const& f)
              : base_type(no_addref, f)
              , alloc_(alloc)
            {
            }

            cancelable_task_object_allocator(init_no_addref no_addref,
                other_allocator const& alloc, F&& f) noexcept
              : base_type(no_addref, HPX_MOVE(f))
              , alloc_(alloc)
            {
            }

            cancelable_task_object_allocator(
                cancelable_task_object_allocator const&) = delete;
            cancelable_task_object_allocator(
                cancelable_task_object_allocator&&) = delete;
            cancelable_task_object_allocator& operator=(
                cancelable_task_object_allocator const&) = delete;
            cancelable_task_object_allocator& operator=(
                cancelable_task_object_allocator&&) = delete;

            ~cancelable_task_object_allocator() = default;

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

        template <typename Result, typename F, typename Executor>
        struct cancelable_task_object
          : task_object<Result, F, Executor,
                lcos::detail::cancelable_task_base<Result>>
        {
            using base_type = task_object<Result, F, Executor,
                lcos::detail::cancelable_task_base<Result>>;
            using result_type = typename base_type::result_type;
            using init_no_addref = typename base_type::init_no_addref;

            explicit cancelable_task_object(F const& f)
              : base_type(f)
            {
            }

            explicit cancelable_task_object(F&& f) noexcept
              : base_type(HPX_MOVE(f))
            {
            }

            cancelable_task_object(Executor& exec, F const& f)
              : base_type(exec, f)
            {
            }

            cancelable_task_object(Executor& exec, F&& f) noexcept
              : base_type(exec, HPX_MOVE(f))
            {
            }

            cancelable_task_object(init_no_addref no_addref, F const& f)
              : base_type(no_addref, f)
            {
            }

            cancelable_task_object(init_no_addref no_addref, F&& f) noexcept
              : base_type(no_addref, HPX_MOVE(f))
            {
            }

            cancelable_task_object(
                Executor& exec, init_no_addref no_addref, F const& f)
              : base_type(exec, no_addref, f)
            {
            }

            cancelable_task_object(
                Executor& exec, init_no_addref no_addref, F&& f) noexcept
              : base_type(exec, no_addref, HPX_MOVE(f))
            {
            }

            cancelable_task_object(cancelable_task_object const&) = delete;
            cancelable_task_object(cancelable_task_object&&) = delete;
            cancelable_task_object& operator=(
                cancelable_task_object const&) = delete;
            cancelable_task_object& operator=(
                cancelable_task_object&&) = delete;

            ~cancelable_task_object() = default;
        };
    }    // namespace detail
}    // namespace hpx::lcos::local

namespace hpx::traits::detail {

    template <typename Result, typename F, typename Base, typename Allocator>
    struct shared_state_allocator<
        lcos::local::detail::task_object<Result, F, void, Base>, Allocator>
    {
        using type = lcos::local::detail::task_object_allocator<Allocator,
            Result, F, Base>;
    };

    template <typename Result, typename F, typename Allocator>
    struct shared_state_allocator<
        lcos::local::detail::cancelable_task_object<Result, F, void>, Allocator>
    {
        using type =
            lcos::local::detail::cancelable_task_object_allocator<Allocator,
                Result, F>;
    };
}    // namespace hpx::traits::detail

namespace hpx::lcos::local {

    ///////////////////////////////////////////////////////////////////////////
    // The futures_factory is very similar to a packaged_task except that it
    // allows for the owner to go out of scope before the future becomes ready.
    // We provide this class to avoid semantic differences to the C++11
    // std::packaged_task, while otoh it is a very convenient way for us to
    // implement hpx::async.
    HPX_CXX_CORE_EXPORT template <typename Func, bool Cancelable = false>
    class futures_factory;

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Result, bool Cancelable, typename Executor = void>
        struct create_task_object;

        template <typename Result>
        struct create_task_object<Result, false, void>
        {
            using return_type =
                hpx::intrusive_ptr<lcos::detail::task_base<Result>>;
            using init_no_addref =
                typename lcos::detail::future_data_refcnt_base::init_no_addref;

            template <typename F>
            static return_type call(F&& f)
            {
                return return_type(new task_object<Result, F, void>(
                                       init_no_addref{}, HPX_FORWARD(F, f)),
                    false);
            }

            template <typename R>
            static return_type call(R (*f)())
            {
                return return_type(new task_object<Result, Result (*)(), void>(
                                       init_no_addref{}, f),
                    false);
            }

            template <typename Allocator, typename F>
            static return_type call(Allocator const& a, F&& f)
            {
                using base_allocator = Allocator;
                using shared_state = traits::shared_state_allocator_t<
                    task_object<Result, F, void>, base_allocator>;

                using other_allocator = typename std::allocator_traits<
                    base_allocator>::template rebind_alloc<shared_state>;
                using traits = std::allocator_traits<other_allocator>;

                using unique_ptr = std::unique_ptr<shared_state,
                    util::allocator_deleter<other_allocator>>;

                other_allocator alloc(a);
                unique_ptr p(traits::allocate(alloc, 1),
                    util::allocator_deleter<other_allocator>{alloc});
                traits::construct(
                    alloc, p.get(), init_no_addref{}, alloc, HPX_FORWARD(F, f));

                return return_type(p.release(), false);
            }

            template <typename Allocator, typename R>
            static return_type call(Allocator const& a, R (*f)())
            {
                using base_allocator = Allocator;
                using shared_state = traits::shared_state_allocator_t<
                    task_object<Result, Result (*)(), void>, base_allocator>;

                using other_allocator = typename std::allocator_traits<
                    base_allocator>::template rebind_alloc<shared_state>;
                using traits = std::allocator_traits<other_allocator>;

                using init_no_addref = typename shared_state::init_no_addref;

                using unique_ptr = std::unique_ptr<shared_state,
                    util::allocator_deleter<other_allocator>>;

                other_allocator alloc(a);
                unique_ptr p(traits::allocate(alloc, 1),
                    util::allocator_deleter<other_allocator>{alloc});
                traits::construct(alloc, p.get(), init_no_addref{}, alloc, f);

                return return_type(p.release(), false);
            }
        };

        template <typename Result, typename Executor>
        struct create_task_object<Result, false, Executor>
          : create_task_object<Result, false, void>
        {
            using return_type =
                hpx::intrusive_ptr<lcos::detail::task_base<Result>>;
            using init_no_addref =
                typename lcos::detail::future_data_refcnt_base::init_no_addref;

            template <typename F>
            static return_type call(Executor& exec, F&& f)
            {
                return return_type(new task_object<Result, F, Executor>(exec,
                                       init_no_addref{}, HPX_FORWARD(F, f)),
                    false);
            }

            template <typename R>
            static return_type call(Executor& exec, R (*f)())
            {
                return return_type(
                    new task_object<Result, Result (*)(), Executor>(
                        exec, init_no_addref{}, f),
                    false);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Result>
        struct create_task_object<Result, true, void>
        {
            using return_type =
                hpx::intrusive_ptr<lcos::detail::task_base<Result>>;
            using init_no_addref =
                typename lcos::detail::future_data_refcnt_base::init_no_addref;

            template <typename F>
            static return_type call(F&& f)
            {
                return return_type(new cancelable_task_object<Result, F, void>(
                                       init_no_addref{}, HPX_FORWARD(F, f)),
                    false);
            }

            template <typename R>
            static return_type call(R (*f)())
            {
                return return_type(
                    new cancelable_task_object<Result, Result (*)(), void>(
                        init_no_addref{}, f),
                    false);
            }

            template <typename Allocator, typename F>
            static return_type call(Allocator const& a, F&& f)
            {
                using base_allocator = Allocator;
                using shared_state = traits::shared_state_allocator_t<
                    cancelable_task_object<Result, F, void>, base_allocator>;

                using other_allocator = typename std::allocator_traits<
                    base_allocator>::template rebind_alloc<shared_state>;
                using traits = std::allocator_traits<other_allocator>;

                using unique_ptr = std::unique_ptr<shared_state,
                    util::allocator_deleter<other_allocator>>;

                other_allocator alloc(a);
                unique_ptr p(traits::allocate(alloc, 1),
                    util::allocator_deleter<other_allocator>{alloc});
                traits::construct(
                    alloc, p.get(), init_no_addref{}, alloc, HPX_FORWARD(F, f));

                return return_type(p.release(), false);
            }

            template <typename Allocator, typename R>
            static return_type call(Allocator const& a, R (*f)())
            {
                using base_allocator = Allocator;
                using shared_state = traits::shared_state_allocator_t<
                    cancelable_task_object<Result, Result (*)(), void>,
                    base_allocator>;

                using other_allocator = typename std::allocator_traits<
                    base_allocator>::template rebind_alloc<shared_state>;
                using traits = std::allocator_traits<other_allocator>;

                using init_no_addref = typename shared_state::init_no_addref;

                using unique_ptr = std::unique_ptr<shared_state,
                    util::allocator_deleter<other_allocator>>;

                other_allocator alloc(a);
                unique_ptr p(traits::allocate(alloc, 1),
                    util::allocator_deleter<other_allocator>{alloc});
                traits::construct(alloc, p.get(), init_no_addref{}, alloc, f);

                return return_type(p.release(), false);
            }
        };

        template <typename Result, typename Executor>
        struct create_task_object<Result, true, Executor>
          : create_task_object<Result, true, void>
        {
            using return_type =
                hpx::intrusive_ptr<lcos::detail::task_base<Result>>;
            using init_no_addref =
                typename lcos::detail::future_data_refcnt_base::init_no_addref;

            template <typename F>
            static return_type call(Executor& exec, F&& f)
            {
                return return_type(
                    new cancelable_task_object<Result, F, Executor>(
                        exec, init_no_addref{}, HPX_FORWARD(F, f)),
                    false);
            }

            template <typename R>
            static return_type call(Executor& exec, R (*f)())
            {
                return return_type(
                    new cancelable_task_object<Result, Result (*)(), Executor>(
                        exec, init_no_addref{}, f),
                    false);
            }
        };
    }    // namespace detail

    template <typename Result, bool Cancelable>
    class futures_factory<Result(), Cancelable>
    {
    protected:
        using task_impl_type = lcos::detail::task_base<Result>;

    public:
        // construction and destruction
        futures_factory() = default;

        template <typename Executor, typename F>
        explicit futures_factory(Executor& exec, F&& f)
          : task_(
                detail::create_task_object<Result, Cancelable, Executor>::call(
                    exec, HPX_FORWARD(F, f)))
        {
        }

        template <typename Executor>
        explicit futures_factory(Executor& exec, Result (*f)())
          : task_(
                detail::create_task_object<Result, Cancelable, Executor>::call(
                    exec, f))
        {
        }

        template <typename F,
            typename Enable = std::enable_if_t<
                !std::is_same_v<std::decay_t<F>, futures_factory>>>
        explicit futures_factory(F&& f)
          : task_(detail::create_task_object<Result, Cancelable>::call(
                hpx::util::thread_local_caching_allocator<
                    hpx::lockfree::variable_size_stack,
                    hpx::util::internal_allocator<>>{},
                HPX_FORWARD(F, f)))
        {
        }

        explicit futures_factory(Result (*f)())
          : task_(detail::create_task_object<Result, Cancelable>::call(
                hpx::util::thread_local_caching_allocator<
                    hpx::lockfree::variable_size_stack,
                    hpx::util::internal_allocator<>>{},
                f))
        {
        }

        ~futures_factory() = default;

        futures_factory(futures_factory const& rhs) = delete;
        futures_factory& operator=(futures_factory const& rhs) = delete;

        futures_factory(futures_factory&& rhs) noexcept
          : task_(HPX_MOVE(rhs.task_))
          , future_obtained_(rhs.future_obtained_)
        {
            rhs.task_.reset();
            rhs.future_obtained_ = false;
        }

        futures_factory& operator=(futures_factory&& rhs) noexcept
        {
            if (this != &rhs)
            {
                task_ = HPX_MOVE(rhs.task_);
                future_obtained_ = rhs.future_obtained_;

                rhs.task_.reset();
                rhs.future_obtained_ = false;
            }
            return *this;
        }

        // synchronous execution
        void operator()() const
        {
            if (!task_)
            {
                HPX_THROW_EXCEPTION(hpx::error::task_moved,
                    "futures_factory<Result()>::operator()",
                    "futures_factory invalid (has it been moved?)");
            }
            task_->run();
        }

        // asynchronous execution
        threads::thread_id_ref_type post(
            char const* annotation = "futures_factory::post",
            launch policy = launch::async, error_code& ec = throws) const
        {
            return post(threads::detail::get_self_or_default_pool(), annotation,
                HPX_MOVE(policy), ec);
        }

        threads::thread_id_ref_type post(threads::thread_pool_base* pool,
            char const* annotation = "futures_factory::post",
            launch policy = launch::async, error_code& ec = throws) const
        {
            if (!task_)
            {
                HPX_THROW_EXCEPTION(hpx::error::task_moved,
                    "futures_factory<Result()>::post()",
                    "futures_factory invalid (has it been moved?)");
            }
            return task_->post(pool, annotation, HPX_MOVE(policy), ec);
        }

        // This is the same as get_future, except that it moves the
        // shared state into the returned future.
        hpx::future<Result> get_future(error_code& ec = throws)
        {
            if (!task_)
            {
                HPX_THROWS_IF(ec, hpx::error::task_moved,
                    "futures_factory<Result()>::get_future",
                    "futures_factory invalid (has it been moved?)");
                return hpx::future<Result>();
            }
            if (future_obtained_)
            {
                HPX_THROWS_IF(ec, hpx::error::future_already_retrieved,
                    "futures_factory<Result()>::get_future",
                    "future already has been retrieved from this factory");
                return hpx::future<Result>();
            }

            future_obtained_ = true;

            using traits::future_access;
            return future_access<hpx::future<Result>>::create(HPX_MOVE(task_));
        }

        constexpr bool valid() const noexcept
        {
            return !!task_;
        }

        void set_exception(std::exception_ptr const& e)
        {
            if (!task_)
            {
                HPX_THROW_EXCEPTION(hpx::error::task_moved,
                    "futures_factory<Result()>::set_exception",
                    "futures_factory invalid (has it been moved?)");
            }
            task_->set_exception(e);
        }

    protected:
        hpx::intrusive_ptr<task_impl_type> task_;
        bool future_obtained_ = false;
    };
}    // namespace hpx::lcos::local
