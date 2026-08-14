////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    HPX_CXX_EXPORT struct notification_header;

    HPX_CXX_EXPORT struct HPX_EXPORT big_boot_barrier
    {
    public:
        big_boot_barrier(big_boot_barrier const&) = delete;
        big_boot_barrier(big_boot_barrier&&) = delete;
        big_boot_barrier& operator=(big_boot_barrier const&) = delete;
        big_boot_barrier& operator=(big_boot_barrier&&) = delete;

    private:
        parcelset::parcelport* pp;
        parcelset::endpoints_type const& endpoints;

        service_mode const service_type;
        parcelset::locality const bootstrap_agas;

        std::condition_variable cond;
        std::mutex mtx;
        std::size_t connected;

        using thunk_type = hpx::move_only_function<void()>*;
        hpx::lockfree::queue<thunk_type,
            hpx::util::aligned_allocator<thunk_type>>
            thunks;

        std::vector<parcelset::endpoints_type> localities;

        void spin();

        void notify();

    public:
        struct scoped_lock
        {
        private:
            big_boot_barrier& bbb;

        public:
            explicit scoped_lock(big_boot_barrier& bbb_)
              : bbb(bbb_)
            {
                bbb.mtx.lock();
            }

            ~scoped_lock()
            {
                bbb.notify();
            }
        };

        big_boot_barrier(parcelset::parcelport* pp_,
            parcelset::endpoints_type const& endpoints_,
            util::runtime_configuration const& ini_);

        ~big_boot_barrier()
        {
            hpx::move_only_function<void()>* f;
            while (thunks.pop(f))
                delete f;
        }

        parcelset::locality here()
        {
            return bootstrap_agas;
        }
        parcelset::endpoints_type const& get_endpoints()
        {
            return endpoints;
        }

        template <typename Action, typename... Args>
        void apply(std::uint32_t source_locality_id,
            std::uint32_t target_locality_id, parcelset::locality dest,
            Action act, Args&&... args);

        template <typename Action, typename... Args>
        void apply_late(std::uint32_t source_locality_id,
            std::uint32_t target_locality_id, parcelset::locality const& dest,
            Action act, Args&&... args);

        void apply_notification(std::uint32_t source_locality_id,
            std::uint32_t target_locality_id, parcelset::locality const& dest,
            notification_header&& hdr);

        void wait_bootstrap();
        void wait_hosted(std::string const& locality_name,
            naming::address::address_type primary_ns_ptr,
            naming::address::address_type symbol_ns_ptr);

        // no-op on non-bootstrap localities
        void trigger();

        void add_thunk(hpx::move_only_function<void()>* f);

        void add_locality_endpoints(std::uint32_t locality_id,
            parcelset::endpoints_type const& endpoints);
    };

    HPX_CXX_EXPORT HPX_EXPORT void create_big_boot_barrier(
        parcelset::parcelport* pp_, parcelset::endpoints_type const& endpoints_,
        util::runtime_configuration const& ini_);

    HPX_CXX_EXPORT HPX_EXPORT void destroy_big_boot_barrier();

    HPX_CXX_EXPORT HPX_EXPORT big_boot_barrier& get_big_boot_barrier();
}    // namespace hpx::agas

#include <hpx/config/warnings_suffix.hpp>

#endif
