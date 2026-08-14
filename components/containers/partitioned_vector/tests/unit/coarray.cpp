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

void bulk_test(hpx::lcos::spmd_block block, std::size_t height,
    std::size_t width, std::size_t elt_size, std::string name)
{
    using hpx::container::placeholders::_;
    using const_iterator = typename std::vector<double>::const_iterator;

    std::size_t numlocs = block.get_num_images();

    hpx::coarray<double, 3> a(block, name, {height, width, _}, elt_size);

    std::size_t idx = block.this_image() * height * width;

    for (std::size_t j = 0; j < width; j++)
        for (std::size_t i = 0; i < height; i++)
        {
            // It's a local write operation
            a(i, j, _) =
                std::vector<double>(elt_size, static_cast<double>(idx));
            idx++;
        }

    block.sync_all();

    if (block.this_image() == 0)
    {
        std::size_t idx = 0;

        for (std::size_t k = 0; k < numlocs; k++)
            for (std::size_t j = 0; j < width; j++)
                for (std::size_t i = 0; i < height; i++)
                {
                    std::vector<double> result(
                        elt_size, static_cast<double>(idx));

                    // It's a Get operation
                    std::vector<double> value =
                        (std::vector<double>) a(i, j, k);

                    const_iterator it1 = result.begin(), it2 = value.begin();
                    const_iterator end1 = result.end();

                    for (; it1 != end1; ++it1, ++it2)
                    {
                        HPX_TEST_EQ(*it1, *it2);
                    }

                    idx++;
                }
    }
}
HPX_PLAIN_ACTION(bulk_test, bulk_test_action)

int main()
{
    std::size_t const height = 32;
    std::size_t const width = 4;
    std::size_t const elt_size = 4;

    std::string coarray_name("my_coarray");

    hpx::future<void> join = hpx::lcos::define_spmd_block(
        "block", 2, bulk_test_action(), height, width, elt_size, coarray_name);

    hpx::wait_all(join);

    return 0;
}
#endif
