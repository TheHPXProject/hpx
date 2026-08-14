//  Copyright (c) 2007-2008 Anshul Tandon
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/runtime_distributed.hpp>
#include <hpx/runtime_distributed/applier.hpp>
#include <hpx/runtime_distributed/runtime_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace hpx::applier {

    applier::applier()
#if defined(HPX_HAVE_NETWORKING)
      : parcel_handler_(nullptr)
      , thread_manager_(nullptr)
#else
      : thread_manager_(nullptr)
#endif
    {
    }

#if defined(HPX_HAVE_NETWORKING)
    void applier::init(parcelset::parcelhandler& ph, threads::threadmanager& tm)
    {
        parcel_handler_ = &ph;
        thread_manager_ = &tm;
    }
#else
    void applier::init(threads::threadmanager& tm)
    {
        thread_manager_ = &tm;
    }
#endif

    void applier::initialize(std::uint64_t rts)
    {
        agas::addressing_service& agas_client = naming::get_agas_client();
        runtime_support_id_ =
            hpx::id_type(agas_client.get_local_locality().get_msb(), rts,
                hpx::id_type::management_type::unmanaged);
    }

#if defined(HPX_HAVE_NETWORKING)
    parcelset::parcelhandler& applier::get_parcel_handler()
    {
        return *parcel_handler_;
    }
#endif

    threads::threadmanager& applier::get_thread_manager()
    {
        return *thread_manager_;
    }

    naming::gid_type const& applier::get_raw_locality(error_code& ec) const
    {
        return hpx::naming::get_agas_client().get_local_locality(ec);
    }

    std::uint32_t applier::get_locality_id(error_code& ec) const
    {
        return naming::get_locality_id_from_gid(get_raw_locality(ec));
    }

    bool applier::get_raw_remote_localities(
        std::vector<naming::gid_type>& prefixes,
        components::component_type type, error_code& ec) const
    {
#if defined(HPX_HAVE_NETWORKING)
        return parcel_handler_->get_raw_remote_localities(prefixes, type, ec);
#else
        HPX_UNUSED(prefixes);
        HPX_UNUSED(type);
        HPX_UNUSED(ec);
        return true;
#endif
    }

    bool applier::get_remote_localities(std::vector<hpx::id_type>& prefixes,
        components::component_type type, error_code& ec) const
    {
#if defined(HPX_HAVE_NETWORKING)
        std::vector<naming::gid_type> raw_prefixes;
        if (!parcel_handler_->get_raw_remote_localities(raw_prefixes, type, ec))
            return false;

        for (naming::gid_type& gid : raw_prefixes)
            prefixes.emplace_back(
                gid, hpx::id_type::management_type::unmanaged);
#endif
        HPX_UNUSED(prefixes);
        HPX_UNUSED(type);
        HPX_UNUSED(ec);
        return true;
    }

    bool applier::get_raw_localities(std::vector<naming::gid_type>& prefixes,
        components::component_type type) const
    {
#if defined(HPX_HAVE_NETWORKING)
        return parcel_handler_->get_raw_localities(prefixes, type);
#else
        naming::gid_type id;
        naming::get_agas_client().get_console_locality(id);
        prefixes.emplace_back(id);
        HPX_UNUSED(prefixes);
        HPX_UNUSED(type);
        return true;
#endif
    }

    bool applier::get_localities(
        std::vector<hpx::id_type>& prefixes, error_code& ec) const
    {
        std::vector<naming::gid_type> raw_prefixes;
#if defined(HPX_HAVE_NETWORKING)
        if (!parcel_handler_->get_raw_localities(raw_prefixes,
                to_int(hpx::components::component_enum_type::invalid), ec))
            return false;

        for (naming::gid_type& gid : raw_prefixes)
            prefixes.emplace_back(
                gid, hpx::id_type::management_type::unmanaged);
#else
        prefixes.emplace_back(agas::get_console_locality());
#endif
        HPX_UNUSED(prefixes);
        HPX_UNUSED(ec);
        return true;
    }

    bool applier::get_localities(std::vector<hpx::id_type>& prefixes,
        components::component_type type, error_code& ec) const
    {
#if defined(HPX_HAVE_NETWORKING)
        std::vector<naming::gid_type> raw_prefixes;
        if (!parcel_handler_->get_raw_localities(raw_prefixes, type, ec))
            return false;

        for (naming::gid_type& gid : raw_prefixes)
            prefixes.emplace_back(
                gid, hpx::id_type::management_type::unmanaged);
#else
        prefixes.emplace_back(agas::get_console_locality());
#endif
        HPX_UNUSED(prefixes);
        HPX_UNUSED(type);
        HPX_UNUSED(ec);
        return true;
    }

    applier& get_applier()
    {
        return hpx::get_runtime_distributed().get_applier();
    }

    applier* get_applier_ptr()
    {
        return &hpx::get_runtime_distributed().get_applier();
    }
}    // namespace hpx::applier
