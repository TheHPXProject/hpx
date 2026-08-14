//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>

using namespace hpx::datapar::experimental;

std::atomic<std::size_t> count_single(0);
std::atomic<std::size_t> count_simd(0);

void test_indices(std::size_t)
{
    ++count_single;
}

void test_indices(native_simd<std::size_t> const&)
{
    ++count_simd;
}

int hpx_main()
{
    static_assert(
        std::random_access_iterator<hpx::util::counting_iterator<std::size_t>>);

    constexpr std::size_t size = native_simd<std::size_t>::size();
    constexpr std::size_t N = 133 * size + size - 1;

    hpx::execution::experimental::chunking_parameters param{};
    hpx::execution::experimental::collect_chunking_parameters ccp(param);

    hpx::for_each(hpx::execution::par_simd.with(ccp),
        hpx::util::counting_iterator(std::size_t(0)),
        hpx::util::counting_iterator(N), [&](auto i) { test_indices(i); });

    std::size_t single_count = 0;
    std::size_t simd_count = 0;

    std::size_t remaining = param.num_elements;
    for (std::size_t chunk = 0; chunk != param.num_chunks; ++chunk)
    {
        std::size_t chunk_size = (std::min) (param.chunk_size, remaining);
        single_count += chunk_size % size;
        simd_count += chunk_size / size;
        remaining -= chunk_size;
    }

    HPX_TEST_EQ(count_single.load(), single_count);
    HPX_TEST_EQ(count_simd.load(), simd_count);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
