//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The partitioners collect the results of all chunks in a vector whose
// elements are written concurrently. Chunk results of type bool must not end
// up in a std::vector<bool>, as its elements share memory locations (this
// caused hpx::ranges::contains to sporadically return false).

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

template <typename T>
struct chunk_result
{
    using type = T;
};

template <typename T>
struct chunk_result<hpx::future<T>>
{
    using type = T;
};

template <typename T>
struct chunk_result<hpx::shared_future<T>>
{
    using type = T;
};

template <typename ExPolicy>
void test_partitioner_bool_result(ExPolicy&& policy)
{
    using iterator = std::vector<int>::iterator;
    using policy_type = std::decay_t<ExPolicy>;

    std::vector<int> c(10007, 0);
    c[c.size() / 2] = 1;

    auto f1 = [](iterator first, std::size_t count) -> bool {
        auto const last = std::next(first, static_cast<std::ptrdiff_t>(count));
        return std::find(first, last, 1) != last;
    };

    auto f2 = [](auto&& results) {
        using element_type = std::decay_t<decltype(*hpx::util::begin(results))>;
        static_assert(std::is_same_v<typename chunk_result<element_type>::type,
                          std::uint8_t>,
            "bool chunk results must be collected as std::uint8_t");

        return std::any_of(hpx::util::begin(results), hpx::util::end(results),
            hpx::functional::unwrap{});
    };

    auto result = hpx::parallel::util::partitioner<policy_type, bool>::call(
        HPX_FORWARD(ExPolicy, policy), std::begin(c), c.size(), f1, f2);

    if constexpr (hpx::is_async_execution_policy_v<policy_type>)
    {
        HPX_TEST(result.get());
    }
    else
    {
        HPX_TEST(result);
    }
}

int hpx_main()
{
    using namespace hpx::execution;

    // the algorithms use the partitioners for parallel policies only
    test_partitioner_bool_result(par);
    test_partitioner_bool_result(par_unseq);

    test_partitioner_bool_result(par(task));
    test_partitioner_bool_result(par_unseq(task));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
