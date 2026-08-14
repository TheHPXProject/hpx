///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/compute/host/get_targets.hpp>
#include <hpx/modules/compute_local.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::compute::host::distributed {

    HPX_CXX_EXPORT struct HPX_EXPORT target : hpx::compute::host::target
    {
    public:
        // Constructs default target
        target()
          : hpx::compute::host::target()
          , locality_(hpx::find_here())
        {
        }

        // Constructs target from a given mask of processing units
        explicit target(hpx::threads::mask_type mask)
          : hpx::compute::host::target(mask)
          , locality_(hpx::find_here())
        {
        }

        explicit target(hpx::id_type const& locality)
          : hpx::compute::host::target()
          , locality_(locality)
        {
        }

        target(hpx::id_type const& locality, hpx::threads::mask_type mask)
          : hpx::compute::host::target(mask)
          , locality_(locality)
        {
        }

        explicit target(hpx::compute::host::target const& target)
          : hpx::compute::host::target(target)
          , locality_(hpx::find_here())
        {
        }

        hpx::id_type const& get_locality() const noexcept
        {
            return locality_;
        }

        static hpx::future<std::vector<target>> get_targets(
            hpx::id_type const& locality)
        {
            return host::distributed::get_targets(locality);
        }

        friend bool operator==(target const& lhs, target const& rhs)
        {
            return static_cast<hpx::compute::host::target>(lhs) ==
                static_cast<hpx::compute::host::target>(rhs) &&
                lhs.locality_ == rhs.locality_;
        }

    private:
        friend class hpx::serialization::access;

        void serialize(serialization::input_archive& ar, unsigned int);
        void serialize(serialization::output_archive& ar, unsigned int);

        hpx::id_type locality_;
    };
}    // namespace hpx::compute::host::distributed

#include <hpx/config/warnings_suffix.hpp>

#endif
