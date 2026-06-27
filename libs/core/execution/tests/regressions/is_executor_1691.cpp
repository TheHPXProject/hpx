//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2024-20 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>

#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
struct my_executor : hpx::execution::parallel_executor
{
};

namespace hpx::execution::experimental {

    template <>
    struct is_one_way_executor<my_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<my_executor> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<my_executor> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    my_executor exec;

    std::vector<int> v(100);

    hpx::ranges::for_each(hpx::execution::par.on(exec), v, [](int) {});

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
