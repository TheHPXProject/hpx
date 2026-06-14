//  Copyright (c) 2019 Jeff Trull
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <cstddef>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct test_async_executor : hpx::execution::parallel_executor
{
    template <typename F, typename T, typename... Ts>
    decltype(auto) async_execute(
        F&& f, T&& t, std::size_t chunk_size, Ts&&... ts) const
    {
        // make sure the chunk_size is equal to what was specified below
        HPX_TEST_EQ(chunk_size, std::size_t(50000));

        using base_type = hpx::execution::parallel_executor;
        return hpx::parallel::execution::async_execute(
            static_cast<base_type const&>(*this), std::forward<F>(f),
            std::forward<T>(t), chunk_size, std::forward<Ts>(ts)...);
    }
};

namespace hpx::execution::experimental {

    template <>
    struct is_two_way_executor<test_async_executor> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

int hpx_main()
{
    using namespace hpx::util;

    // create a fixed chunk size to be used in the algorithm
    hpx::execution::experimental::static_chunk_size fixed(50000);

    // helper-executor to verify the used chunk-size
    test_async_executor exec;

    // this does not seem to be obeyed!
    auto ex = hpx::execution::par.on(exec).with(fixed);

    // create and fill random vector of desired size
    std::random_device rnd_device;
    std::mt19937 mersenne_engine{rnd_device()};
    std::uniform_int_distribution<int> dist{1, 20};

    std::size_t sz = 16750000;

    std::vector<int> data(sz);
    std::generate(
        data.begin(), data.end(), [&]() { return dist(mersenne_engine); });

    std::vector<int> result(sz + 1);

    hpx::exclusive_scan(ex, data.begin(), data.end(), result.begin(), 0);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // By default this should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=2"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);

    return hpx::util::report_errors();
}
