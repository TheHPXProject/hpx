//  Copyright (c) 2007-2011 Hartmut Kaiser
//  Copyright (c) 2011-2012 Maciej Brodowicz
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PAPI)

#include <hpx/components/performance_counters/papi/server/papi.hpp>
#include <hpx/components/performance_counters/papi/util/papi.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#if defined(__linux__) && !defined(__ANDROID) && !defined(ANDROID)
#include <sys/syscall.h>
#else
#error "The PAPI performance counter component can only be used on Linux"
#endif

///////////////////////////////////////////////////////////////////////////////
namespace papi_ns = hpx::performance_counters::papi;

typedef hpx::components::component<papi_ns::server::papi_counter>
    papi_counter_type;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC(
    papi_counter_type, papi_counter, "base_performance_counter")

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace performance_counters { namespace papi {
    namespace server {
#define NS_STR "hpx::performance_counters::papi::server::"

        ///////////////////////////////////////////////////////////////////////////
        // static members
        papi_counter_base::mutex_type papi_counter_base::base_mtx_;
        papi_counter_base::ttable_type papi_counter_base::thread_state_;

        using hpx::performance_counters::papi::util::papi_call;

        ///////////////////////////////////////////////////////////////////////////
        // methods
        thread_counters::thread_counters(std::uint32_t tix)
          : thread_index_(tix)
          , evset_(PAPI_NULL)
        {
            char const* locstr =
                "hpx::performance_counters::papi::server::thread_counters()";
            hpx::util::thread_mapper& tm =
                hpx::get_runtime().get_thread_mapper();
            papi_call(PAPI_create_eventset(&evset_),
                "could not create PAPI event set", locstr);
            papi_call(PAPI_assign_eventset_component(evset_, 0),
                "cannot assign component index to event set", locstr);
            pid_t tid = tm.get_linux_thread_id(tix);
            if (tid == -1)
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    NS_STR "thread_counters::thread_counters()",
                    "unable to retrieve correct OS thread ID for profiling "
                    "(perhaps thread was not registered)");
            papi_call(PAPI_attach(evset_, tid),
                "failed to attach thread to PAPI event set", locstr);
            tm.register_callback(
                tix, [&](std::uint32_t i) { return this->terminate(i); });
        }

        thread_counters::~thread_counters()
        {
            {
                std::lock_guard<mutex_type> m(mtx_);
                finalize();
            }
            // callback cancellation moved outside the mutex to avoid potential deadlock
            hpx::get_runtime().get_thread_mapper().revoke_callback(
                thread_index_);
        }

        bool thread_counters::add_event(papi_counter* cnt)
        {
            stop_all();
            if (PAPI_add_event(evset_, cnt->get_event()) != PAPI_OK)
                return false;
            counts_.push_back(cnt->get_value());
            cnttab_.push_back(cnt);
            cnt->update_index(counts_.size() - 1);
            return start_all();
        }

        bool thread_counters::remove_event(papi_counter* cnt)
        {
            if (!stop_all() || PAPI_cleanup_eventset(evset_) != PAPI_OK)
                return false;

            // store the most recent value
            int rmix = cnt->get_counter_index();
            cnt->update_state(timestamp_, counts_[rmix], PAPI_COUNTER_STOPPED);
            // erase entries corresponding to removed event
            counts_.erase(counts_.begin() + rmix);
            cnttab_.erase(cnttab_.begin() + rmix);
            // For the lack of better strategy the remaining events are added in the
            // same order as before. This avoids reordering of remaining counter
            // values and at least some surprises on architectures with asymmetric
            // functionality of counting registers.
            unsigned i = 0;
            while (i < counts_.size())
            {
                papi_counter* c = cnttab_[i];
                if (PAPI_add_event(evset_, c->get_event()) != PAPI_OK)
                {    // failed to add event
                    c->update_state(timestamp_, counts_[c->get_counter_index()],
                        PAPI_COUNTER_SUSPENDED);
                    counts_.erase(counts_.begin() + i);
                    cnttab_.erase(cnttab_.begin() + i);
                    continue;
                }
                // update indices of remaining counters
                c->update_index(i++);
            }
            return start_all();
        }

        bool thread_counters::read_value(papi_counter* cnt, bool reset)
        {
            {
                std::lock_guard<papi_counter_base::mutex_type> lk(
                    cnt->get_global_mtx());

                if (PAPI_accum(evset_, &counts_[0]) != PAPI_OK)
                    return false;
            }
            timestamp_ = static_cast<std::int64_t>(hpx::get_system_uptime());
            cnt->update_state(timestamp_, counts_[cnt->get_counter_index()]);
            if (reset)
                counts_[cnt->get_counter_index()] = 0;
            return true;
        }

        bool thread_counters::terminate(std::uint32_t)
        {
            std::lock_guard<mutex_type> m(mtx_);
            return finalize();
        }

        bool thread_counters::start_all()
        {
            if (counts_.empty())
                return true;    // nothing to count
            return PAPI_start(evset_) == PAPI_OK;
        }

        bool thread_counters::stop_all()
        {
            int stat;
            if (PAPI_state(evset_, &stat) != PAPI_OK)
                return false;

            if ((stat & PAPI_RUNNING) != 0)
            {
                std::vector<long long> tmp(counts_.size());
                if (PAPI_stop(evset_, &tmp[0]) != PAPI_OK)
                    return false;
                // accumulate existing counts before modifying event set
                if (PAPI_accum(evset_, &counts_[0]) != PAPI_OK)
                    return false;
                timestamp_ =
                    static_cast<std::int64_t>(hpx::get_system_uptime());
            }
            return true;
        }

        bool thread_counters::finalize()
        {
            if (evset_ != PAPI_NULL)
            {
                if (stop_all())
                {
                    for (std::uint32_t i = 0; i < cnttab_.size(); ++i)
                    {    // retrieve the most recent values for the still active counters
                        papi_counter* c = cnttab_[i];
                        if (c->get_status() == PAPI_COUNTER_ACTIVE)
                            c->update_state(timestamp_, counts_[i],
                                PAPI_COUNTER_TERMINATED);
                        else
                            c->update_status(PAPI_COUNTER_TERMINATED);
                    }
                }
                // release event set resources
                return PAPI_destroy_eventset(&evset_) == PAPI_OK;
            }
            return true;
        }

        ///////////////////////////////////////////////////////////////////////////
        thread_counters* papi_counter_base::get_thread_counters(
            std::uint32_t tix)
        {
            std::lock_guard<mutex_type> m(base_mtx_);

            // create entry for the thread associated with the counter if it doesn't exist
            ttable_type::iterator it = thread_state_.find(tix);
            if (it == thread_state_.end())
            {
                hpx::util::ignore_all_while_checking i;
                HPX_UNUSED(i);

                return thread_state_[tix] = new thread_counters(tix);
            }
            else
                return it->second;
        }

        ///////////////////////////////////////////////////////////////////////////
        papi_counter::papi_counter(
            hpx::performance_counters::counter_info const& info)
          : base_type_holder(info)
          , event_(PAPI_NULL)
          , index_(-1)
          , value_(0)
          , timestamp_(-1)
          , status_(PAPI_COUNTER_STOPPED)
        {
            char const* locstr = NS_STR "papi_counter()";

            // extract path elements from counter name
            counter_path_elements cpe;
            get_counter_path_elements(info.fullname_, cpe);
            // convert event name to code and check availability
            {
                std::lock_guard<papi_counter_base::mutex_type> lk(
                    this->get_global_mtx());
                // papi_call might throw, leading to potential "lock held while
                // suspending thread" errors. At this point, it is safe to ignore
                // the lock.
                hpx::util::ignore_all_while_checking il;
                HPX_UNUSED(il);

                papi_call(PAPI_event_name_to_code(
                              const_cast<char*>(cpe.countername_.c_str()),
                              const_cast<int*>(&event_)),
                    cpe.countername_ +
                        " does not seem to be a valid event name",
                    locstr);
                papi_call(PAPI_query_event(event_),
                    "event " + cpe.countername_ +
                        " is not available on this platform",
                    locstr);
            }
            // find OS thread associated with the counter
            std::string label;
            std::uint32_t tix = util::get_counter_thread(cpe, label);
            if (tix == hpx::util::thread_mapper::invalid_index)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_success, locstr,
                    "could not find thread " + label);
            }
            // associate low level counters object
            counters_ = get_thread_counters(tix);
            if (!counters_)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_success, locstr,
                    "failed to find low level counters for thread " + label);
            }
            // counting is not enabled here; it has to be started explicitly
        }

        hpx::performance_counters::counter_value
        papi_counter::get_counter_value(bool reset)
        {
            std::lock_guard<thread_counters::mutex_type> m(
                counters_->get_lock());

            if (status_ == PAPI_COUNTER_ACTIVE)
                counters_->read_value(this, reset);

            hpx::performance_counters::counter_value value;

            if (timestamp_ != -1)
                copy_value(value);
            else
                value.status_ =
                    hpx::performance_counters::counter_status::invalid_data;

            // clear local copy
            if (reset)
                value_ = 0;

            value.count_ = ++invocation_count_;

            return value;
        }

        bool papi_counter::start()
        {
            std::lock_guard<papi_counter_base::mutex_type> lk(get_global_mtx());

            if (status_ == PAPI_COUNTER_ACTIVE)
                return true;
            if (counters_->add_event(this))
            {
                status_ = PAPI_COUNTER_ACTIVE;
                return true;
            }
            status_ = PAPI_COUNTER_SUSPENDED;
            return false;
        }

        bool papi_counter::stop()
        {
            std::lock_guard<thread_counters::mutex_type> m(
                counters_->get_lock());

            return stop_counter();
        }

        void papi_counter::reset()
        {
            std::lock_guard<thread_counters::mutex_type> m(
                counters_->get_lock());

            reset_counter();
        }

        void papi_counter::finalize()
        {
            std::lock_guard<thread_counters::mutex_type> m(
                counters_->get_lock());

            stop_counter();
            base_type_holder::finalize();
            base_type::finalize();
        }

        naming::address papi_counter::get_current_address() const
        {
            return naming::address(
                naming::get_gid_from_locality_id(hpx::get_locality_id()),
                components::get_component_type<papi_counter>(),
                const_cast<papi_counter*>(this));
        }

}}}}    // namespace hpx::performance_counters::papi::server

#endif
