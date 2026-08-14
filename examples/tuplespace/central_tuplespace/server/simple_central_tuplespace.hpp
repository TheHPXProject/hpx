//  Copyright (c) 2013 Shuangyang Yang
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/chrono.hpp>
#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos_local.hpp>
#include <hpx/modules/components_base.hpp>

#include <mutex>

#include "tuple.hpp"
#include "tuples_warehouse.hpp"

// #define TS_DEBUG

///////////////////////////////////////////////////////////////////////////////
namespace examples::server {

    ///////////////////////////////////////////////////////////////////////////
    /// This class is a simple central tuplespace (SCTS) as an HPX component.
    /// An HPX component is a class that:
    ///
    ///     * Inherits from a component base class:
    ///       \a hpx::components::component_base
    ///     * Exposes methods that can be called asynchronously and/or remotely.
    ///       These constructs are known as HPX actions.
    ///
    /// By deriving this component from \a locking_hook the runtime system
    /// ensures that all action invocations are serialized. That means that
    /// the system ensures that no two actions are invoked at the same time on
    /// a given component instance. This makes the component thread safe and no
    /// additional locking has to be implemented by the user.
    ///
    /// Components are first-class objects in HPX. This means that they are
    /// globally addressable; all components have a unique GID.
    ///
    /// The SCTS will store all tuples from any objects in a central locality,
    /// to demonstrate the basic function
    ///
    /// (from JavaSpace)
    /// write,
    /// read,
    /// take
    ///
    /// each has the last argument as a timeout value, pre-defined WAIT_FOREVER,
    /// NO_WAIT users can also provide its own timeout values.
    ///
    /// uses mutex, will hurt performance.
    ///
    //[simple_central_tuplespace_server_inherit
    class simple_central_tuplespace
      : public hpx::components::component_base<simple_central_tuplespace>
    //]
    {
    public:
        using tuple_type = hpx::util::storage::tuple;
        using elem_type = hpx::util::storage::tuple::elem_type;
        using mutex_type = hpx::spinlock;

        using tuples_type = examples::server::tuples_warehouse;

        // pre-defined timeout values
        enum
        {
            WAIT_FOREVER = -1,    // <0 means blocking
            NO_WAIT = 0
        };

        //[simple_central_tuplespace_server_ctor
        simple_central_tuplespace() = default;
        //]

        ///////////////////////////////////////////////////////////////////////
        // Exposed functionality of this component.

        //[simple_accumulator_methods

        // put tuple into tuplespace
        // out function
        int write(tuple_type const& tp)
        {
            if (tp.empty())
            {
                return -1;
            }

            {
                std::lock_guard<mutex_type> l(mtx_);

                tuples_.insert(tp);
            }

            return 0;
        }

        // read from tuplespace
        // rd function
        tuple_type read(tuple_type const& tp, double const timeout) const
        {
            tuple_type result;
            hpx::chrono::high_resolution_timer t;

            do
            {
                if (tuples_.empty())
                {
                    continue;
                }

                {
                    std::lock_guard<mutex_type> l(mtx_);

                    result = tuples_.match(tp);
                }

                if (!result.empty())
                {
                    break;    // found
                }
            } while ((timeout < 0) || (timeout > t.elapsed()));

            return result;
        }

        // take from tuplespace
        // in function
        tuple_type take(tuple_type const& tp, double const timeout)
        {
            tuple_type result;
            hpx::chrono::high_resolution_timer t;

            do
            {
                if (tuples_.empty())
                {
                    continue;
                }

                {
                    std::lock_guard<mutex_type> l(mtx_);

                    result = tuples_.match_and_erase(tp);
                }

                if (!result.empty())
                {
                    break;    // found
                }
            } while ((timeout < 0) || (timeout > t.elapsed()));

            return result;
        }

        //]

        ///////////////////////////////////////////////////////////////////////
        // Each of the exposed functions needs to be encapsulated into an
        // action type, generating all required boilerplate code for threads,
        // serialization, etc.

        //[simple_central_tuplespace_action_types
        HPX_DEFINE_COMPONENT_ACTION(simple_central_tuplespace, write)
        HPX_DEFINE_COMPONENT_ACTION(simple_central_tuplespace, read)
        HPX_DEFINE_COMPONENT_ACTION(simple_central_tuplespace, take)
        //]

        //[simple_central_tuplespace_server_data_member
    private:
        tuples_type tuples_;
        mutable mutex_type mtx_;
        //]
    };
}    // namespace examples::server

//[simple_central_tuplespace_registration_declarations
HPX_REGISTER_ACTION_DECLARATION(
    examples::server::simple_central_tuplespace::write_action,
    simple_central_tuplespace_write_action)

HPX_REGISTER_ACTION_DECLARATION(
    examples::server::simple_central_tuplespace::read_action,
    simple_central_tuplespace_read_action)

HPX_REGISTER_ACTION_DECLARATION(
    examples::server::simple_central_tuplespace::take_action,
    simple_central_tuplespace_take_action)
//]

#undef TS_DEBUG

#endif
