///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/topology.hpp>

#include <hpx/compute/host/distributed_target.hpp>

#include <cstddef>
#include <utility>

namespace hpx::compute::host::distributed {

    void target::serialize(serialization::input_archive& ar, unsigned int)
    {
        ar >> serialization::base_object<hpx::compute::host::target>(*this);
        ar >> locality_;
    }

    void target::serialize(serialization::output_archive& ar, unsigned int)
    {
        ar << serialization::base_object<hpx::compute::host::target>(*this);
        ar << locality_;
    }
}    // namespace hpx::compute::host::distributed

#endif
