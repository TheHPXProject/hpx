//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2016      Thomas Heller
//  Copyright (c) 2011      Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/base_lco_with_value.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    HPX_CXX_EXPORT template <typename Result, typename RemoteResult>
    class promise_lco;
}    // namespace hpx::lcos::detail

///////////////////////////////////////////////////////////////////////////////
template <typename Result, typename RemoteResult>
struct hpx::traits::managed_component_dtor_policy<
    hpx::lcos::detail::promise_lco<Result, RemoteResult>>
{
    using type = managed_object_is_lifetime_controlled;
};    // namespace hpx::traits

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    HPX_CXX_EXPORT template <typename Result, typename RemoteResult>
    class promise_lco_base
      : public lcos::base_lco_with_value<Result, RemoteResult>
    {
    protected:
        using shared_state_type = lcos::detail::future_data<Result>;
        using shared_state_ptr = hpx::intrusive_ptr<shared_state_type>;

        using base_type = lcos::base_lco_with_value<Result, RemoteResult>;

        using result_type = base_type::result_type;

    public:
        explicit promise_lco_base(shared_state_ptr const& shared_state)
          : shared_state_(shared_state)
        {
        }

        void set_value(RemoteResult&& result)
        {
            HPX_ASSERT(shared_state_);
            shared_state_->set_remote_data(HPX_MOVE(result));
        }

        void set_exception(std::exception_ptr const& e)
        {
            HPX_ASSERT(shared_state_);
            shared_state_->set_exception(e);
        }

        // This is the component id. Every component needs to have an
        // embedded enumerator 'value' which is used by the generic action
        // implementation to associate this component with a given action.
        enum
        {
            value = to_int(hpx::components::component_enum_type::promise)
        };

    protected:
        shared_state_ptr shared_state_;

    private:
        // intrusive reference counting, noop since we don't require
        // reference counting here.
        friend void intrusive_ptr_add_ref(promise_lco_base* /*p*/) noexcept {}

        // intrusive reference counting, noop since we don't require
        // reference counting here.
        friend void intrusive_ptr_release(promise_lco_base* /*p*/) noexcept {}
    };

    HPX_CXX_EXPORT template <typename Result, typename RemoteResult>
    class promise_lco : public promise_lco_base<Result, RemoteResult>
    {
    protected:
        using shared_state_type = lcos::detail::future_data<Result>;
        using shared_state_ptr = hpx::intrusive_ptr<shared_state_type>;

        using base_type = promise_lco_base<Result, RemoteResult>;

        using result_type = base_type::result_type;

    public:
        explicit promise_lco(shared_state_ptr const& shared_state)
          : base_type(shared_state)
        {
        }

        result_type get_value()
        {
            result_type* result = this->shared_state_->get_result();
            return HPX_MOVE(*result);
        }
        result_type get_value(error_code& ec)
        {
            result_type* result = this->shared_state_->get_result(ec);
            return HPX_MOVE(*result);
        }

    private:
        template <typename>
        friend struct components::detail_adl_barrier::init;

        void set_back_ptr(components::managed_component<promise_lco>* bp)
        {
            HPX_ASSERT(bp);
            HPX_UNUSED(bp);
        }
    };

    template <>
    class promise_lco<void, hpx::util::unused_type>
      : public promise_lco_base<void, hpx::util::unused_type>
    {
    protected:
        using shared_state_type = lcos::detail::future_data<void>;
        using shared_state_ptr = hpx::intrusive_ptr<shared_state_type>;
        using base_type = promise_lco_base<void, hpx::util::unused_type>;

    public:
        explicit promise_lco(shared_state_ptr const& shared_state)
          : base_type(shared_state)
        {
        }

        hpx::util::unused_type get_value()
        {
            this->shared_state_->get_result();
            return hpx::util::unused;
        }
        hpx::util::unused_type get_value(error_code& ec)
        {
            this->shared_state_->get_result(ec);
            return hpx::util::unused;
        }

    private:
        template <typename>
        friend struct components::detail_adl_barrier::init;

        void set_back_ptr(components::managed_component<promise_lco>* bp)
        {
            HPX_ASSERT(bp);
            HPX_UNUSED(bp);
        }
    };
}    // namespace hpx::lcos::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace traits {

        namespace detail {

            HPX_EXPORT extern util::atomic_count unique_type;
        }

        template <typename Result, typename RemoteResult>
        struct component_type_database<
            lcos::detail::promise_lco<Result, RemoteResult>>
        {
            static components::component_type value;

            static components::component_type get() noexcept
            {
                // Promises are never created remotely, their factories are not
                // registered with AGAS, so we can assign the component types
                // locally.
                if (value ==
                    to_int(hpx::components::component_enum_type::invalid))
                {
                    value = components::derived_component_type(
                        static_cast<components::component_type>(
                            ++detail::unique_type),
                        to_int(components::component_enum_type::
                                base_lco_with_value));
                }
                return value;
            }

            static void set(components::component_type /* t */)
            {
                HPX_ASSERT(false);
            }
        };

        template <typename Result, typename RemoteResult>
        components::component_type component_type_database<
            lcos::detail::promise_lco<Result, RemoteResult>>::value =
            to_int(hpx::components::component_enum_type::invalid);
    }    // namespace traits

    namespace components::detail {

        // Forward declare promise_lco<void> to avoid duplicate instantiations
        template <>
        HPX_ALWAYS_EXPORT hpx::components::managed_component<
            lcos::detail::promise_lco<void, hpx::util::unused_type>>::heap_type&
        component_heap_helper<hpx::components::managed_component<
            lcos::detail::promise_lco<void, hpx::util::unused_type>>>();

        template <typename Result, typename RemoteResult>
        struct HPX_ALWAYS_EXPORT
            component_heap_impl<hpx::components::managed_component<
                lcos::detail::promise_lco<Result, RemoteResult>>>
        {
            using valid = void;

            static hpx::components::managed_component<
                lcos::detail::promise_lco<Result, RemoteResult>>::heap_type&
            call()
            {
                util::reinitializable_static<
                    typename hpx::components::managed_component<lcos::detail::
                            promise_lco<Result, RemoteResult>>::heap_type>
                    heap;
                return heap.get();
            }
        };
    }    // namespace components::detail
}    // namespace hpx
