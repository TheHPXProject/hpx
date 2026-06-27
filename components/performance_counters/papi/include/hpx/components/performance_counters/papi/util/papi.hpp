//  Copyright (c) 2011-2012 Maciej Brodowicz
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PAPI)

#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/program_options.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <papi.h>

#if PAPI_VERSION_MAJOR(PAPI_VERSION) > 4
#define PAPI_EXTENDED_EVENT_CODES 1
#endif

namespace hpx { namespace performance_counters { namespace papi {
    namespace util {
        using hpx::program_options::options_description;
        using hpx::program_options::variables_map;

        ///////////////////////////////////////////////////////////////////////////
        // PAPI call wrapper
        inline void papi_call(
            int rc, char const* info, char const* fname, int ok = PAPI_OK)
        {
            if (rc != ok)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_success, fname,
                    hpx::util::format("{} ({})", info, PAPI_strerror(rc)));
            }
        }
        inline void papi_call(int rc, std::string const& info,
            char const* fname, int ok = PAPI_OK)
        {
            papi_call(rc, info.c_str(), fname, ok);
        }

        ///////////////////////////////////////////////////////////////////////////
        // generator retrieving information on PAPI events
        class event_info_generator
        {
        protected:
            int const mask_;            // event type mask
            int const cid_;             // component ID
            int event_;                 // next event to get info about
            bool active_;               // false when generation terminated
            PAPI_event_info_t info_;    // event info to be returned

            // set event info
            virtual bool get_info()
            {
                return PAPI_get_event_info(event_, &info_) == PAPI_OK;
            }
            // get the next event in sequence according to enumeration mask
            virtual bool get_next_event()
            {
                return PAPI_enum_event(&event_, mask_) == PAPI_OK;
            }

            event_info_generator()
              : mask_(0)
              , cid_(0)
              , event_(PAPI_NULL)
              , active_(false)
            {
            }
            event_info_generator(int mask, int first, int cid = 0)
              : mask_(mask)
              , cid_(cid)
            {
#if defined(PAPI_EXTENDED_EVENT_CODES)
                reset(first);
#else
                reset(first | PAPI_COMPONENT_MASK(cid));
#endif
            }

        public:
            // required generator interface
            typedef PAPI_event_info_t const* result_type;

            PAPI_event_info_t const* operator()()
            {
                if (!active_)
                    return nullptr;
                while (!get_info())
                    if (!(active_ = get_next_event()))
                        return nullptr;
                active_ = get_next_event();
                return &info_;
            }

            // reset event scan sequence
            void reset(int init, bool force = false)
            {
                if (force)
                {    // init must be a valid event code
                    event_ = init;
                    active_ = true;
                    return;
                }
                // init is the event type flag; obtain the first valid event code
#if defined(PAPI_EXTENDED_EVENT_CODES)
                active_ = (PAPI_enum_cmp_event(&init, PAPI_ENUM_FIRST, cid_) ==
                    PAPI_OK);
#else
                active_ = (PAPI_enum_event(&init, PAPI_ENUM_FIRST) == PAPI_OK);
#endif
                if (active_)
                    event_ = init;
            }
        };

        // PAPI preset-only enumerator
        template <bool all>
        class preset_enumerator : public event_info_generator
        {
        protected:
            bool get_info()
            {    // locally available presets must not have null info_.count
                return event_info_generator::get_info() &&
                    (!all || info_.count > 0);
            }

        public:
            preset_enumerator()
              : event_info_generator(
                    (all) ? PAPI_ENUM_ALL : PAPI_PRESET_ENUM_AVAIL,
                    PAPI_PRESET_MASK)
            {
            }
        };

        // nested enumerator on umasks available for some native events;
        // initiated with a tracepoint event without any unit masks applied
        class native_umask_enumerator : public event_info_generator
        {
        public:
            //native_umask_enumerator():
            //    event_info_generator(PAPI_NTV_ENUM_UMASKS, PAPI_NULL) { }
            explicit native_umask_enumerator(int first = PAPI_NULL)
              : event_info_generator(PAPI_NTV_ENUM_UMASKS, first)
            {
                reset(first);
            }

            void reset(int first)
            {
                event_info_generator::reset(first, true);
                active_ =
                    get_next_event();    // advance to the first umask event
            }
        };

        // PAPI native event enumerator
        template <bool with_umasks>
        class native_enumerator : public event_info_generator
        {
            native_umask_enumerator umask_gen_;
            hpx::util::generator_iterator<native_umask_enumerator> umask_iter_;
            unsigned const component_index_;
            bool umasks_present_, umask_seq_;

            bool get_info()
            {
                if (with_umasks && umasks_present_ && umask_seq_)
                {
                    if (*umask_iter_)
                    {    // nested iteration in progress
                        info_ = **umask_iter_;
                        return true;
                    }
                    return false;
                }
                return event_info_generator::get_info();
            }
            bool get_next_event()
            {
                if (with_umasks && umasks_present_)
                {
                    if (umask_seq_)
                    {
                        if (*++umask_iter_)
                            return true;
                        umask_seq_ = false;
                    }
                    else
                    {
                        umask_gen_.reset(event_);
                        umask_iter_ = hpx::util::generator_iterator<
                            native_umask_enumerator>(&umask_gen_);
                        if (*umask_iter_)
                            return umask_seq_ = true;    // not ==
                    }
                }
                return event_info_generator::get_next_event();
            }

        public:
            explicit native_enumerator(
                unsigned comp, int mask = PAPI_ENUM_EVENTS)
              : event_info_generator(mask, PAPI_NATIVE_MASK, comp)
              , component_index_(comp)
              , umask_seq_(false)
            {
                PAPI_component_info_t const* ci = PAPI_get_component_info(comp);
                if (!ci)
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::performance_counters::papi::util::native_"
                        "enumerator",
                        "invalid PAPI component index");
                umasks_present_ = ci->cntr_umasks;
            }

            void reset(int first, bool force = false)
            {
                umask_seq_ = false;
                event_info_generator::reset(first, force);
            }
        };

        // types of commonly used info generators
        typedef preset_enumerator<true> all_preset_info_gen;
        typedef preset_enumerator<false> avail_preset_info_gen;
        typedef native_enumerator<true> native_info_gen;

        ///////////////////////////////////////////////////////////////////////////
        // map domain description to a number
        int map_domain(std::string const&);

        // command line option description for PAPI counters
        options_description get_options_description();

        // get processed command line options
        variables_map get_options();

        // quick sanity check for command line options
        bool check_options(variables_map const& vm);

        // create list of event strings from command line options that are
        // relevant to this locality
        bool get_local_events(
            std::vector<std::string>& ev, std::vector<std::string> const& opt);

        // list locally available events with detailed information
        void list_events(std::string const& scope);

        // get relevant thread label and index from counter description
        std::uint32_t get_counter_thread(
            counter_path_elements const&, std::string&);

}}}}    // namespace hpx::performance_counters::papi::util

#endif
