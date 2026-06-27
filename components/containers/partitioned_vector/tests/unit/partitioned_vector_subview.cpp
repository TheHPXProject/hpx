//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/partitioned_vector_view.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_local_view.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_view.hpp>

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// The vector types to be used are defined in partitioned_vector module.
#if defined(HPX_HAVE_STATIC_LINKING)
HPX_REGISTER_PARTITIONED_VECTOR(double)
#endif

void bulk_test(hpx::lcos::spmd_block block, std::size_t N, std::size_t tile,
    std::size_t elt_size, std::string vec_name)
{
    using const_iterator = typename std::vector<double>::const_iterator;
    using vector_type = hpx::partitioned_vector<double>;
    using view_type = hpx::partitioned_vector_view<double, 2>;

    vector_type my_vector;
    my_vector.connect_to(hpx::launch::sync, vec_name);

    view_type my_view(block, my_vector.begin(), my_vector.end(), {N, N});

    std::size_t idx = 0;

    for (std::size_t j = 0; j < N; j += tile)
        for (std::size_t i = 0; i < N; i += tile)
        {
            view_type my_subview(block, &my_view(i, j),
                &my_view(i + tile - 1, j + tile - 1), {tile, tile}, {N, N});

            auto local = hpx::local_view(my_subview);

            for (auto it = local.begin(); it != local.end(); it++)
            {
                // It's a local write operation
                *it = std::vector<double>(elt_size, static_cast<double>(idx));
            }

            auto left_it = local.begin();
            auto right_it = local.cbegin();

            // Note: Useless computation, since we assign segments to themselves
            for (; left_it != local.end(); left_it++, right_it++)
            {
                // Check that dereferencing iterator and const_iterator does not
                // retrieve the same type
                HPX_TEST((!std::is_same<decltype(*left_it),
                    decltype(*right_it)>::value));

                // It's a local write operation
                *left_it = *right_it;
            }

            idx++;
        }

    block.sync_all();

    if (block.this_image() == 0)
    {
        int idx = 0;

        for (std::size_t j = 0; j < N; j += tile)
            for (std::size_t i = 0; i < N; i += tile)
            {
                std::vector<double> result(elt_size, double(idx));

                for (std::size_t jj = j, jj_end = j + tile; jj < jj_end; jj++)
                    for (std::size_t ii = i, ii_end = i + tile; ii < ii_end;
                        ii++)
                    {
                        // It's a Get operation
                        std::vector<double> value =
                            (std::vector<double>) my_view(ii, jj);

                        const_iterator it1 = result.begin(),
                                       it2 = value.begin();
                        const_iterator end1 = result.end();

                        for (; it1 != end1; ++it1, ++it2)
                        {
                            HPX_TEST_EQ(*it1, *it2);
                        }
                    }
                idx++;
            }
    }
}
HPX_PLAIN_ACTION(bulk_test, bulk_test_action)

int main()
{
    using vector_type = hpx::partitioned_vector<double>;

    std::size_t N = 20;
    std::size_t tile = 5;
    std::size_t elt_size = 8;

    // (N+1) replaces N for padding purpose
    std::size_t raw_size = N * (N + 1) * elt_size;

    vector_type my_vector(raw_size,
        hpx::container_layout(N * (N + 1), hpx::find_all_localities()));

    std::string vec_name("my_vector");
    my_vector.register_as(hpx::launch::sync, vec_name);

    hpx::future<void> join = hpx::lcos::define_spmd_block(
        "block", 2, bulk_test_action(), N, tile, elt_size, vec_name);

    join.get();

    return 0;
}
#endif
