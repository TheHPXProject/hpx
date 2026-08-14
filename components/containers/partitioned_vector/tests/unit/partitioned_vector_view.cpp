//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/partitioned_vector_view.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_local_view.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_view.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// The vector types to be used are defined in partitioned_vector module.
#if defined(HPX_HAVE_STATIC_LINKING)
HPX_REGISTER_PARTITIONED_VECTOR(double)
#endif

void bulk_test(hpx::lcos::spmd_block block, std::size_t height,
    std::size_t width, std::size_t local_height, std::size_t local_width,
    std::size_t local_leading_dimension, std::string in_name,
    std::string out_name)
{
    using const_iterator = typename std::vector<double>::const_iterator;

    using vector_type = hpx::partitioned_vector<double>;

    using view_type = hpx::partitioned_vector_view<double, 2>;

    vector_type vector_in;
    vector_type vector_out;

    vector_in.connect_to(hpx::launch::sync, in_name);
    vector_out.connect_to(hpx::launch::sync, out_name);

    view_type in(block, vector_in.begin(), vector_in.end(), {height, width});
    view_type out(block, vector_out.begin(), vector_out.end(), {height, width});

    // Ensure that only one image is doing put operations
    if (block.this_image() == 0)
    {
        std::size_t idx = 0;

        // traverse all the indexed elements
        for (auto&& v : in)
        {
            std::vector<double> data(local_height * local_width);
            std::size_t local_idx = 0;

            for (double& d : data)
            {
                d = static_cast<double>(idx + local_idx++);
            }

            // Put operation
            v = std::move(data);
            idx++;
        }
    }

    block.sync_all();

    // Outer Transpose operation
    for (std::size_t j = 0; j < width; j++)
        for (std::size_t i = 0; i < height; i++)
        {
            // Put operation
            out(j, i) = in(i, j);
        }

    block.sync_all();

    // Inner Transpose operation
    for (auto& v : hpx::local_view(out))
    {
        for (std::size_t jj = 0; jj < local_width - 1; jj++)
            for (std::size_t ii = jj + 1; ii < local_height; ii++)
            {
                std::swap(v[jj + ii * local_leading_dimension],
                    v[ii + jj * local_leading_dimension]);
            }
    }

    block.sync_all();

    // Test the result of the computation
    if (block.this_image() == 0)
    {
        int idx = 0;
        std::vector<double> result(local_height * local_width);

        for (std::size_t j = 0; j < width; j++)
            for (std::size_t i = 0; i < height; i++)
            {
                std::size_t local_idx = 0;

                for (double& r : result)
                {
                    r = static_cast<double>(idx + local_idx++);
                }

                // transpose the guess result
                for (std::size_t jj = 0; jj < local_width - 1; jj++)
                    for (std::size_t ii = jj + 1; ii < local_height; ii++)
                    {
                        std::swap(result[jj + ii * local_leading_dimension],
                            result[ii + jj * local_leading_dimension]);
                    }

                // It's a Get operation
                std::vector<double> value = (std::vector<double>) out(j, i);

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

void async_bulk_test(hpx::lcos::spmd_block block, std::size_t height,
    std::size_t width, std::size_t local_height, std::size_t local_width,
    std::size_t local_leading_dimension, std::string in_name,
    std::string out_name)
{
    using const_iterator = typename std::vector<double>::const_iterator;

    using vector_type = hpx::partitioned_vector<double>;

    using view_type =
        hpx::partitioned_vector_view<double, 2, std::vector<double>>;

    vector_type vector_in;
    vector_type vector_out;

    vector_in.connect_to(hpx::launch::sync, in_name);
    vector_out.connect_to(hpx::launch::sync, out_name);

    view_type in(block, vector_in.begin(), vector_in.end(), {height, width});
    view_type out(block, vector_out.begin(), vector_out.end(), {height, width});

    // Ensure that only one image is doing put operations
    if (block.this_image() == 0)
    {
        std::size_t idx = 0;

        // traverse all the indexed elements
        for (auto&& v : in)
        {
            std::vector<double> data(local_height * local_width);
            std::size_t local_idx = 0;

            for (double& d : data)
            {
                d = static_cast<double>(idx + local_idx++);
            }

            // Put operation
            v = std::move(data);
            idx++;
        }
    }

    block.sync_all(hpx::launch::async)
        .then([&block, &in, &out, width, height](hpx::future<void> event) {
            event.get();

            // Outer Transpose operation
            for (std::size_t j = 0; j < width; j++)
                for (std::size_t i = 0; i < height; i++)
                {
                    // It's a Put operation
                    out(j, i) = in(i, j);
                }

            return block.sync_all(hpx::launch::async);
        })
        .then([&block, &out, local_width, local_height,
                  local_leading_dimension](hpx::future<void> event) {
            event.get();

            // Inner Transpose operation
            for (auto& v : hpx::local_view(out))
            {
                for (std::size_t jj = 0; jj < local_width - 1; jj++)
                    for (std::size_t ii = jj + 1; ii < local_height; ii++)
                    {
                        std::swap(v[jj + ii * local_leading_dimension],
                            v[ii + jj * local_leading_dimension]);
                    }
            }

            return block.sync_all(hpx::launch::async);
        })
        .then([&block, &out, width, height, local_width, local_height,
                  local_leading_dimension](hpx::future<void> event) {
            event.get();

            // Test the result of the computation
            if (block.this_image() == 0)
            {
                int idx = 0;
                std::vector<double> result(local_height * local_width);

                for (std::size_t j = 0; j < width; j++)
                    for (std::size_t i = 0; i < height; i++)
                    {
                        std::size_t local_idx = 0;

                        for (double& r : result)
                        {
                            r = static_cast<double>(idx + local_idx++);
                        }

                        // transpose the guess result
                        for (std::size_t jj = 0; jj < local_width - 1; jj++)
                            for (std::size_t ii = jj + 1; ii < local_height;
                                ii++)
                            {
                                std::swap(
                                    result[jj + ii * local_leading_dimension],
                                    result[ii + jj * local_leading_dimension]);
                            }

                        // It's a Get operation
                        std::vector<double> value =
                            (std::vector<double>) out(j, i);

                        const_iterator it1 = result.begin(),
                                       it2 = value.begin();
                        const_iterator end1 = result.end();

                        for (; it1 != end1; ++it1, ++it2)
                        {
                            HPX_TEST_EQ(*it1, *it2);
                        }

                        idx++;
                    }
            }
        })
        .get();
}
HPX_PLAIN_ACTION(async_bulk_test, async_bulk_test_action)

int main()
{
    using vector_type = hpx::partitioned_vector<double>;

    std::size_t const height = 8;
    std::size_t const width = 8;

    std::size_t local_height = 8;
    std::size_t local_width = 8;
    std::size_t local_leading_dimension = local_height;

    std::size_t raw_size = (height * width) * (local_height * local_width);

    auto layout =
        hpx::container_layout(height * width, hpx::find_all_localities());

    // Vector instantiations for test 1
    vector_type in1(raw_size, layout);
    vector_type out1(raw_size, layout);

    std::string in1_name("in1");
    std::string out1_name("out1");

    in1.register_as(hpx::launch::sync, in1_name);
    out1.register_as(hpx::launch::sync, out1_name);

    // Vector instantiations for test 2
    vector_type in2(raw_size, layout);
    vector_type out2(raw_size, layout);

    std::string in2_name("in2");
    std::string out2_name("out2");

    in2.register_as(hpx::launch::sync, in2_name);
    out2.register_as(hpx::launch::sync, out2_name);

    // Launch tests
    hpx::future<void> join1 = hpx::lcos::define_spmd_block("block1", 2,
        bulk_test_action(), height, width, local_height, local_width,
        local_leading_dimension, in1_name, out1_name);

    hpx::future<void> join2 = hpx::lcos::define_spmd_block("block2", 2,
        async_bulk_test_action(), height, width, local_height, local_width,
        local_leading_dimension, in2_name, out2_name);

    hpx::wait_all(join1, join2);

    return 0;
}
#endif
