//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

template <typename Policy>
void test(Policy policy)
{
    auto run = [](auto operation) {
        if constexpr (hpx::execution_policy_has_scheduler_executor_v<Policy>)
            sender_test::wait(operation());
        else if constexpr (hpx::is_async_execution_policy_v<Policy>)
            operation().get();
        else
            operation();
    };
    for (std::size_t size : {0, 1, 17, 10007})
    {
        for (int stride : {1, 3, 5004, 20000})
        {
            std::vector<int> values(size, -1);
            std::size_t index = 0;
            auto first = values.begin();
            auto body = [first, stride](auto it, std::size_t i) {
                HPX_TEST_EQ(static_cast<std::size_t>((it - first) / stride), i);
                *it = 42;
            };
            std::size_t const iterations = size / stride + (size % stride != 0);
            run([&] {
                return hpx::experimental::for_loop_n_strided(policy, first,
                    size, stride, hpx::experimental::induction(index), body);
            });
            HPX_TEST_EQ(index, iterations);
            for (std::size_t i = 0; i != size; ++i)
                HPX_TEST_EQ(values[i], i % stride == 0 ? 42 : -1);

            std::fill(values.begin(), values.end(), -1);
            index = 0;
            run([&] {
                return hpx::experimental::for_loop_strided(policy, first,
                    values.end(), stride, hpx::experimental::induction(index),
                    body);
            });
            HPX_TEST_EQ(index, iterations);
            for (std::size_t i = 0; i != size; ++i)
                HPX_TEST_EQ(values[i], i % stride == 0 ? 42 : -1);
        }
    }
}

int hpx_main()
{
    sender_test::policies([](auto policy) { test(policy); });
    using namespace hpx::execution;
    test(seq);
    test(par);
    test(seq(task));
    test(par(task));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
