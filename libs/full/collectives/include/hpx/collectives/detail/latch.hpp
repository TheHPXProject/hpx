//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <exception>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace lcos { namespace server {

    /// A latch can be used to synchronize a specific number of threads,
    /// blocking all of the entering threads until all of the threads have
    /// entered the latch.
    class latch
      : public lcos::base_lco_with_value<bool, std::ptrdiff_t>
      , public components::managed_component_base<latch>
    {
    private:
        typedef components::managed_component_base<latch> base_type;

    public:
        typedef lcos::base_lco_with_value<bool, std::ptrdiff_t>
            base_type_holder;

        // disambiguate base classes
        using base_type::finalize;

        typedef base_type::wrapping_type wrapping_type;

        static components::component_type get_component_type()
        {
            return to_int(hpx::components::component_enum_type::latch);
        }
        static void set_component_type(components::component_type) {}

        naming::address get_current_address() const
        {
            return naming::address(
                naming::get_gid_from_locality_id(agas::get_locality_id()),
                components::get_component_type<latch>(),
                const_cast<latch*>(this));
        }

    public:
        // This is the component type id. Every component type needs to have an
        // embedded enumerator 'value' which is used by the generic action
        // implementation to associate this component with a given action.
        enum
        {
            value = to_int(hpx::components::component_enum_type::latch)
        };

        latch()
          : latch_(0)
        {
        }

        explicit latch(std::ptrdiff_t number_of_threads)
          : latch_(number_of_threads)
        {
        }

        // standard LCO action implementations

        /// The function \a set_event will block the number of entering
        /// \a threads (as given by the constructor parameter \a number_of_threads),
        /// releasing all waiting threads as soon as the last \a thread
        /// entered this function.
        ///
        /// This is invoked whenever the arrive_and_wait() function is called
        void set_event()
        {
            latch_.arrive_and_wait();
        }

        /// This is invoked whenever the count_down() function is called
        void set_value(std::ptrdiff_t&& n)    //-V669
        {
            latch_.count_down(n);
        }

        /// This is invoked whenever the is_ready() function is called
        bool get_value()
        {
            return latch_.try_wait();
        }
        bool get_value(hpx::error_code&)
        {
            return latch_.try_wait();
        }

        /// The \a function set_exception is called whenever a
        /// \a set_exception_action is applied on an instance of a LCO. This
        /// function just forwards to the virtual function \a set_exception,
        /// which is overloaded by the derived concrete LCO.
        ///
        /// \param e      [in] The exception encapsulating the error to report
        ///               to this LCO instance.
        void set_exception(std::exception_ptr const& e)
        {
            try
            {
                latch_.abort_all();
                std::rethrow_exception(e);
            }
            catch (std::exception const& e)
            {
                // rethrow again, but this time using the native hpx mechanics
                HPX_THROW_EXCEPTION(hpx::error::no_success,
                    "latch::set_exception", hpx::diagnostic_information(e));
            }
        }

        typedef hpx::components::server::create_component_action<latch,
            std::ptrdiff_t>
            create_component_action;

        // additional functionality
        void wait() const
        {
            latch_.wait();
        }
        HPX_DEFINE_COMPONENT_ACTION(latch, wait, wait_action)

    private:
        hpx::lcos::local::latch latch_;
    };
}}}    // namespace hpx::lcos::server

HPX_REGISTER_ACTION_DECLARATION(
    hpx::lcos::server::latch::create_component_action,
    hpx_lcos_server_latch_create_component_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::lcos::server::latch::wait_action, hpx_lcos_server_latch_wait_action)

HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    bool, std::ptrdiff_t, bool_std_ptrdiff)
