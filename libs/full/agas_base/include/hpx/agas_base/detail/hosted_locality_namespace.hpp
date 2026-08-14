//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/agas_base/locality_namespace.hpp>
#include <hpx/agas_base/server/locality_namespace.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hpx::agas::detail {

    struct hosted_locality_namespace : locality_namespace
    {
        explicit hosted_locality_namespace(naming::address addr);

        naming::address::address_type ptr() const override
        {
            return addr_.address_;
        }
        naming::address addr() const override
        {
            return addr_;
        }
        hpx::id_type gid() const override
        {
            return gid_;
        }

        std::uint32_t allocate(parcelset::endpoints_type const& endpoints,
            std::uint64_t count, std::uint32_t num_threads,
            naming::gid_type const& suggested_prefix) override;

        void free(naming::gid_type const& locality) override;

        std::vector<std::uint32_t> localities() override;

        parcelset::endpoints_type resolve_locality(
            naming::gid_type const& locality) override;

        std::uint32_t get_num_localities() override;
        hpx::future<std::uint32_t> get_num_localities_async() override;

        std::vector<std::uint32_t> get_num_threads() override;
        hpx::future<std::vector<std::uint32_t>> get_num_threads_async()
            override;

        std::uint32_t get_num_overall_threads() override;
        hpx::future<std::uint32_t> get_num_overall_threads_async() override;

    private:
        hpx::id_type gid_;
        naming::address addr_;
    };
}    // namespace hpx::agas::detail

HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    hpx::parcelset::endpoints_type, parcelset_endpoints_type)

#endif
