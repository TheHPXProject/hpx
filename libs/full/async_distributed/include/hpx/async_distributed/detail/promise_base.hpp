//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2016-2017 Thomas Heller
//  Copyright (c) 2011      Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/detail/promise_lco.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <memory>
#include <utility>

namespace hpx::lcos::detail {

    HPX_CXX_EXPORT template <typename Result>
    struct promise_data : task_base<Result>
    {
        using init_no_addref = typename task_base<Result>::init_no_addref;

        promise_data() = default;

        explicit promise_data(init_no_addref no_addref)
          : task_base<Result>(no_addref)
        {
        }

        void set_task(hpx::move_only_function<void()>&& f)
        {
            f_ = HPX_MOVE(f);
        }

        void mark_as_started()
        {
            this->task_base<Result>::started_test_and_set();
        }

    private:
        void do_run()
        {
            if (!f_)
                return;    // do nothing if no deferred task is given

            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    f_();            // trigger action
                    this->wait();    // wait for value to come back
                    return;
                },
                [&](std::exception_ptr ep) {
                    this->set_exception(HPX_MOVE(ep));
                });
        }

        hpx::move_only_function<void()> f_;
    };

    template <typename Result, typename Allocator>
    struct promise_data_allocator : promise_data<Result>
    {
        using init_no_addref = typename promise_data<Result>::init_no_addref;
        using other_allocator = typename std::allocator_traits<
            Allocator>::template rebind_alloc<promise_data_allocator>;

        explicit promise_data_allocator(other_allocator const& alloc)
          : alloc_(alloc)
        {
        }

        promise_data_allocator(
            init_no_addref no_addref, other_allocator const& alloc)
          : promise_data<Result>(no_addref)
          , alloc_(alloc)
        {
        }

    private:
        void destroy() noexcept
        {
            using traits = std::allocator_traits<other_allocator>;

            other_allocator alloc(alloc_);
            traits::destroy(alloc, this);
            traits::deallocate(alloc, this, 1);
        }

    private:
        other_allocator alloc_;
    };

    ///////////////////////////////////////////////////////////////////////
    struct set_id_helper
    {
        template <typename SharedState>
        HPX_FORCEINLINE static void call(hpx::traits::detail::wrap_int,
            SharedState const& /*shared_state*/, id_type const& /*id*/,
            bool& /*id_retrieved*/)
        {
            // by default, do nothing
        }

        template <typename SharedState>
        HPX_FORCEINLINE static auto call(int, SharedState const& shared_state,
            id_type const& id, bool& id_retrieved)
            -> decltype(shared_state->set_id(id))
        {
            shared_state->set_id(id);
            id_retrieved = true;
        }
    };

    template <typename SharedState>
    HPX_FORCEINLINE void call_set_id(
        SharedState const& shared_state, id_type const& id, bool& id_retrieved)
    {
        set_id_helper::call(0, shared_state, id, id_retrieved);
    }
}    // namespace hpx::lcos::detail

namespace hpx::traits::detail {

    // specialize for promise_data to extract corresponding, allocator-
    // based shared state type
    template <typename R, typename Allocator>
    struct shared_state_allocator<lcos::detail::promise_data<R>, Allocator>
    {
        using type = lcos::detail::promise_data_allocator<R, Allocator>;
    };
}    // namespace hpx::traits::detail

namespace hpx::lcos::detail {

    // Promise base contains the actual implementation for the remotely
    // settable promise. It consists of two parts:
    //  1) The shared state, which is used to signal the future
    //  2) The LCO, which is used to set the promise remotely
    //
    // The LCO is a component, which lifetime is completely controlled by
    // the shared state. That is, in the GID that we send along as the
    // continuation can be unmanaged, AGAS doesn't participate in the
    // promise lifetime management. The LCO is being reset once the shared
    // state either contains a value or the data, which is set through the
    // LCO interface, which can go out of scope safely when the shared
    // state is set.
    template <typename Result, typename RemoteResult, typename SharedState>
    class promise_base : public hpx::detail::promise_base<Result, SharedState>
    {
        using base_type = hpx::detail::promise_base<Result, SharedState>;

    protected:
        using result_type = Result;
        using shared_state_type = SharedState;
        using shared_state_ptr = hpx::intrusive_ptr<shared_state_type>;

    public:
        promise_base()
          : base_type()
          , id_retrieved_(false)
        {
            init_shared_state();
        }

        template <typename Allocator>
        promise_base(std::allocator_arg_t, Allocator const& a)
          : base_type(std::allocator_arg, a)
          , id_retrieved_(false)
        {
            init_shared_state();
        }

        promise_base(promise_base&& other) noexcept
          : base_type(HPX_MOVE(static_cast<base_type&&>(other)))
          , id_retrieved_(other.id_retrieved_)
          , id_(HPX_MOVE(other.id_))
          , addr_(HPX_MOVE(other.addr_))
        {
            other.id_retrieved_ = false;
            other.id_ = hpx::invalid_id;
            other.addr_ = naming::address();
        }

        ~promise_base()
        {
            check_abandon_shared_state(
                "hpx::detail::promise_base<R>::~promise_base()");
            this->shared_state_.reset();
        }

        promise_base& operator=(promise_base&& other) noexcept
        {
            base_type::operator=(HPX_MOVE(static_cast<base_type&&>(other)));
            id_retrieved_ = other.id_retrieved_;
            id_ = HPX_MOVE(other.id_);
            addr_ = HPX_MOVE(other.addr_);

            other.id_retrieved_ = false;
            other.id_ = hpx::invalid_id;
            other.addr_ = naming::address();
            return *this;
        }

        hpx::id_type get_id(
            bool mark_as_started = true, error_code& ec = throws) const
        {
            if (this->shared_state_ == nullptr)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state,
                    "detail::promise_base<Result, RemoteResult>::get_id",
                    "this promise has no valid shared state");
                return hpx::invalid_id;
            }
            if (!addr_ || !id_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state,
                    "detail::promise_base<Result, RemoteResult>::get_id",
                    "this promise has no valid LCO");
                return hpx::invalid_id;
            }
            if (!this->future_retrieved_)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "promise<Result>::get_id",
                    "future has not been retrieved from this promise yet");
                return hpx::invalid_id;
            }

            if (mark_as_started)
            {
                this->shared_state_->mark_as_started();
            }

            id_retrieved_ = true;
            return id_;
        }

        hpx::id_type get_id(error_code& ec) const
        {
            return get_id(true, ec);
        }

        hpx::id_type get_unmanaged_gid(error_code& ec = throws) const
        {
            return get_id(false, ec);
        }

        naming::address resolve(error_code& ec = throws) const
        {
            if (!addr_ || !id_)
            {
                HPX_THROWS_IF(ec, hpx::error::no_state,
                    "detail::promise_base<Result, RemoteResult>::get_id",
                    "this promise has no valid LCO");
                return naming::address();
            }
            return addr_;
        }

    private:
        // This helper is used to keep the component alive until the
        // completion handler has been called. We need to manually free
        // the component here, since we don't rely on reference counting
        // anymore
        struct keep_alive    //-V690
        {
            using wrapped_type = promise_lco<Result, RemoteResult>;
            using wrapping_type = components::managed_component<wrapped_type>;

            using wrapping_ptr =
                std::unique_ptr<wrapping_type, void (*)(wrapping_type*)>;

            static void wrapping_deleter(wrapping_type* ptr)
            {
                std::destroy_at(ptr);
                hpx::components::component_heap<wrapping_type>().free(ptr);
            }

            wrapping_ptr ptr_;

            explicit keep_alive(wrapping_ptr& ptr)
              : ptr_(ptr.release(), &wrapping_deleter)
            {
            }

            keep_alive(keep_alive&& o) noexcept
              : ptr_(o.ptr_.release(), &wrapping_deleter)
            {
            }

            keep_alive& operator=(keep_alive&& o) = default;

            void operator()()
            {
                delete ptr_->get();    // delete wrapped_type
            }
        };

    protected:
        void init_shared_state()
        {
            using wrapped_type = typename keep_alive::wrapped_type;
            using wrapping_type = typename keep_alive::wrapping_type;

            // The lifetime of the LCO (component) part is completely
            // handled by the shared state, we create the object to get our
            // gid and then attach it to the completion handler of the
            // shared state.
            using wrapping_ptr = typename keep_alive::wrapping_ptr;

            auto ptr = hpx::components::component_heap<wrapping_type>().alloc();
            wrapping_ptr lco_ptr(
                new (ptr) wrapping_type(new wrapped_type(this->shared_state_)),
                &keep_alive::wrapping_deleter);

            id_ = lco_ptr->get_unmanaged_id();
            addr_ = naming::address(agas::get_locality(),
                components::get_component_type<wrapped_type>(), lco_ptr.get());

            // Pass id to shared state if it exposes the set_id() function
            detail::call_set_id(this->shared_state_, id_, id_retrieved_);

            this->shared_state_->set_on_completed(keep_alive(lco_ptr));
        }

        void check_abandon_shared_state(char const* fun)
        {
            if (this->shared_state_ != nullptr && this->future_retrieved_ &&
                !(this->shared_state_->is_ready() || id_retrieved_))
            {
                this->shared_state_->set_error(hpx::error::broken_promise, fun,
                    "abandoning not ready shared state");
                return;
            }
        }
        mutable bool id_retrieved_;

        hpx::id_type id_;
        naming::address addr_;
    };
}    // namespace hpx::lcos::detail
