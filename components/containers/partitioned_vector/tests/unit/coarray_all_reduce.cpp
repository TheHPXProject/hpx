//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/components/containers/coarray/coarray.hpp>

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// coarray<double> is predefined in the partitioned_vector module
#if defined(HPX_HAVE_STATIC_LINKING)
HPX_REGISTER_COARRAY(double)
#endif

void bulk_test(hpx::lcos::spmd_block block, std::string values)
{
    using hpx::container::placeholders::_;

    hpx::coarray<double, 1> a(block, values, {_}, 1);

    double num_images = static_cast<double>(block.get_num_images());
    double reduced_value = 0.;
    double guessed_value = (0. + num_images - 1.) * num_images / 2.;

    // Each image writes its local value
    a(_)[0] = static_cast<double>(block.this_image());

    block.sync_all();

    for (std::size_t i = 0; i < static_cast<std::size_t>(num_images); i++)
    {
        // Each image performs the reduction operation
        reduced_value += a(i)[0];
    }

    block.sync_all();

    HPX_TEST_EQ(reduced_value, guessed_value);
}
HPX_PLAIN_ACTION(bulk_test, bulk_test_action)

int main()
{
    std::string values("values");

    hpx::future<void> join =
        hpx::lcos::define_spmd_block("block", 2, bulk_test_action(), values);

    hpx::wait_all(join);

    return 0;
}
#endif
