//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2011-2012 Maciej Brodowicz
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PAPI)

#include <hpx/modules/components_base.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <cstdint>
#include <map>
#include <vector>

#include <papi.h>

namespace hpx { namespace performance_counters { namespace papi {
    namespace server {
        ///////////////////////////////////////////////////////////////////////////
        // Remarks:
        // As of papi-4.2.0 using bundled pfmlib many PAPI functions are
        // subtly broken when accessing performance counter interface in Linux
        // kernel.
        // To maintain the HPX notion of single monitored quantity per counter
        // instance, the events need to be added (and removed if counter "stop"
        // function is to be reliably implemented) individually.

        // What has been tried so far:
        // 1) Multiple PAPI event sets with one event per set are not properly
        // coordinated by the library. Probably the only way this can work is when
        // at most one event set per thread is active at a time, but that doesn't
        // provide the necessary functionality.
        // 2) Using a shared event set with PAPI_add_event works, but the removal
        // via PAPI_remove_event causes the remaining counter(s) to stop counting,
        // even though the event set status is indicated as PAPI_RUNNING.
        // 3) Shared event set that is initialized from scratch by explicitly
        // adding all remaining events when one of the existing events needs to be
        // removed seems to work. However, the collected counts will be lost
        // during event set cleanup. Maintaning the correct total counts requires
        // careful tracking of values at both PAPI library and HPX levels.
        // 4) To simplify (3), in theory one could use PAPI_write to reinstate
        // the counter values before resuming counting. PAPI_write doesn't seem
        // to be working on all platforms and in addition some kernels may
        // explicitly disable it for accesses from user space.
        // 5) The alternative to using PAPI_read is to invoke PAPI_accum to
        // accumulate the momentary counts in user space counter array, with the
        // side effect of resetting the values of PAPI counters. This simplifies
        // the handling of counter values when adjustments to event set need to be
        // performed. Unfortunately, just like for (2) the simple removal of events
        // does not seem to work correctly, so sticking with (3) for event set
        // shrinking is the preferred approach.

        ///////////////////////////////////////////////////////////////////////////
        enum counter_status
        {
            // counter explicitly stopped or never activated
            PAPI_COUNTER_STOPPED = 0,
            // counter is currently counting events
            PAPI_COUNTER_ACTIVE,
            // counter activation requested, but was rejected by lower layer
            PAPI_COUNTER_SUSPENDED,
            // counter permanently deactivated because the underlying thread terminated
            PAPI_COUNTER_TERMINATED
        };

        class papi_counter;

        ///////////////////////////////////////////////////////////////////////////
        struct thread_counters
        {
        public:
            typedef hpx::spinlock mutex_type;

        private:
            typedef std::vector<papi_counter*> ctable_type;

            // lock for all thread counters
            mutable mutex_type mtx_;
            // monitored thread index (as assigned by papi_thread_mapper)
            std::uint32_t const thread_index_;
            // related PAPI event set
            int evset_;
            // time stamp of last update
            std::int64_t timestamp_;
            // accumulated counter values
            std::vector<long long> counts_;
            // array of pointers to corresponding HPX counter objects
            ctable_type cnttab_;

        public:
            thread_counters()
              : thread_index_(-1)
              , evset_(PAPI_NULL)
              , timestamp_(-1)
            {
            }
            thread_counters(std::uint32_t);
            ~thread_counters();

            // add new PAPI event to the event set
            bool add_event(papi_counter* cnt);
            // remove an event from the event set
            bool remove_event(papi_counter* cnt);
            // obtain current value for active counter i
            bool read_value(papi_counter*, bool reset);

            // terminate counting due to thread going out of scope;
            // this is the only explicitly locked function
            bool terminate(std::uint32_t tix);

            std::uint32_t get_thread_index() const
            {
                return thread_index_;
            }
            mutex_type& get_lock() const
            {
                return mtx_;
            }

        protected:
            // start all active counters
            bool start_all();
            // safely stop all counters in event set and accumulate values in counts_
            bool stop_all();
            // cleanup internal state due to termination
            bool finalize();
        };

        ///////////////////////////////////////////////////////////////////////////
        class papi_counter_base
        {
        public:
            HPX_NON_COPYABLE(papi_counter_base);

        public:
            typedef hpx::spinlock mutex_type;

        private:
            typedef std::map<std::uint32_t, thread_counters*> ttable_type;

            //// shared state
            // mutex to protect shared state
            static mutex_type base_mtx_;
            // array of counter sets for each thread
            static ttable_type thread_state_;

        public:
            papi_counter_base() {};

            // lookup or create thread_counters instance for thread tix
            thread_counters* get_thread_counters(std::uint32_t tix);

            mutex_type& get_global_mtx()
            {
                return base_mtx_;
            }
        };

        ///////////////////////////////////////////////////////////////////////////
        class HPX_COMPONENT_EXPORT papi_counter
          : public hpx::performance_counters::server::base_performance_counter
          , public hpx::components::component_base<papi_counter>
          , protected papi_counter_base
        {
            friend struct thread_counters;

            // PAPI event associated with the counter and index into counter array
            int const event_;
            // index associated with active PAPI counter
            std::uint32_t index_;
            // most recently retrieved counter value
            long long value_;
            // timestamp corresponding to cached counter value
            std::int64_t timestamp_;
            // counting status
            counter_status status_;
            // pointer to shared thread_counters struct
            thread_counters* counters_;

        public:
            typedef hpx::components::component_base<papi_counter> base_type;
            typedef hpx::components::component<papi_counter> wrapping_type;
            typedef papi_counter type_holder;
            typedef hpx::performance_counters::server::base_performance_counter
                base_type_holder;

            papi_counter()
              : event_(PAPI_NULL)
              , index_(-1)
              , value_(0)
              , timestamp_(-1)
              , status_(PAPI_COUNTER_STOPPED)
              , counters_(nullptr)
            {
            }
            papi_counter(hpx::performance_counters::counter_info const& info);

            // start the counter
            virtual bool start();
            // stop the counter
            virtual bool stop();
            // various reset flavors
            virtual void reset();
            virtual void reset_counter_value()
            {
                reset();
            }

            // get the current value of this performance counter
            hpx::performance_counters::counter_value get_counter_value(
                bool reset = false);

            // called just before the instance gets destructed
            void finalize();

            // PAPI event associated with this counter instance
            int get_event() const
            {
                return event_;
            }
            // offset into counter array
            int get_counter_index() const
            {
                return index_;
            }
            // counter status
            counter_status get_status() const
            {
                return status_;
            }
            // currently cached value
            long long get_value() const
            {
                return value_;
            }

            naming::address get_current_address() const;

        private:
            //// methods below operate on shared state and require locking in the caller

            // initialize counter metadata
            void update_index(std::uint32_t i)
            {
                index_ = i;
            }

            // update counter value and timestamp
            void update_state(std::int64_t tstamp, long long cnt,
                counter_status st = PAPI_COUNTER_ACTIVE)
            {
                timestamp_ = tstamp;
                value_ = cnt;
                if (st != PAPI_COUNTER_ACTIVE)
                    index_ = -1;
                status_ = st;
            }

            // update counter status
            void update_status(counter_status st)
            {
                status_ = st;
            }

            // copy state to counter_value object
            void copy_value(hpx::performance_counters::counter_value& val)
            {
                val.time_ = timestamp_;
                val.value_ = value_;
                val.status_ =
                    hpx::performance_counters::counter_status::new_data;
                val.scaling_ = 1;
                val.scale_inverse_ = false;
            }

            // stop low level counter
            bool stop_counter()
            {
                if (status_ == PAPI_COUNTER_ACTIVE)
                {
                    if (!counters_->remove_event(this))
                        return false;
                }
                return true;
            }
            // reset counter
            void reset_counter()
            {
                // if active, clear the previous contents of low level counter
                if (status_ == PAPI_COUNTER_ACTIVE)
                    counters_->read_value(this, true);

                value_ = 0;
            }
        };
}}}}    // namespace hpx::performance_counters::papi::server

#endif
