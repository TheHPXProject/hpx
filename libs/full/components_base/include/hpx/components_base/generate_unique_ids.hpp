//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/naming_base.hpp>

#include <cstddef>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    // The unique_id_ranges class is a type responsible for generating unique
    // ids for components, parcels, threads etc.
    HPX_CXX_EXPORT class HPX_EXPORT unique_id_ranges
    {
        // size of the id range returned by get_id
        static constexpr std::size_t range_delta = 0x100000;

    public:
        // Generate next unique component id
        naming::gid_type get_id(std::size_t count = 1);

        void set_range(
            naming::gid_type const& lower, naming::gid_type const& upper);

    private:
        hpx::util::spinlock mtx_;

        // The range of available ids for components
        naming::gid_type lower_;
        naming::gid_type upper_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
