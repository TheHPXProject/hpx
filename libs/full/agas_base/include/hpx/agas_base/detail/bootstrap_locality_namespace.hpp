//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/agas_base/locality_namespace.hpp>
#include <hpx/agas_base/server/locality_namespace.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace hpx::agas::detail {

    struct bootstrap_locality_namespace : locality_namespace
    {
        explicit bootstrap_locality_namespace(
            server::primary_namespace* primary);

        naming::address::address_type ptr() const override
        {
            return const_cast<server::locality_namespace*>(&server_);
        }
        naming::address addr() const override;
        hpx::id_type gid() const override;

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

        void register_server_instance(std::uint32_t locality_id) override;

        void unregister_server_instance(error_code& ec) override;

        virtual server::locality_namespace* get_service() override
        {
            return &server_;
        }

    private:
        server::locality_namespace server_;
    };
}    // namespace hpx::agas::detail
