//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace sender_test {
    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    template <typename Sender>
    auto wait(Sender&& sender)
    {
        static_assert(ex::is_sender_v<Sender>);
        static_assert(!hpx::traits::is_future_v<std::decay_t<Sender>>);
        auto result = tt::sync_wait(std::forward<Sender>(sender));
        HPX_TEST(result.has_value());
        return result;
    }

    template <typename F>
    void policies(F test)
    {
        using namespace hpx::execution;
        auto sync = ex::explicit_scheduler_executor(
            ex::thread_pool_policy_scheduler(hpx::launch::sync));
        auto async = ex::explicit_scheduler_executor(
            ex::thread_pool_policy_scheduler(hpx::launch::async));
        test(seq(task).on(sync));
        test(unseq(task).on(sync));
        test(par(task).on(async));
        test(par_unseq(task).on(async));
    }

    inline std::vector<int> values(std::size_t size)
    {
        std::vector<int> result(size);
        int i = 0;
        std::generate(
            result.begin(), result.end(), [&i] { return (i++ * 37) % 1009; });
        return result;
    }

    inline int main(int argc, char* argv[], int (*entry)())
    {
        hpx::local::init_params args;
        args.cfg = std::vector<std::string>{"hpx.os_threads=all"};
        HPX_TEST_EQ(hpx::local::init(entry, argc, argv, args), 0);
        return hpx::util::report_errors();
    }
}    // namespace sender_test
