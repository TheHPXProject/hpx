//  Copyright (c) 2024-2026 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

void test_is_sorted_projection()
{
    using namespace hpx::execution;
    using element = std::pair<int, int>;

    std::vector<element> sorted_c = {{10, 1}, {20, 3}, {30, 5}, {40, 8}};
    std::vector<element> unsorted_c = {{10, 1}, {20, 5}, {30, 3}, {40, 8}};

    auto proj = [](element const& p) { return p.second; };

    HPX_TEST(hpx::is_sorted(
        sorted_c.begin(), sorted_c.end(), std::less<int>{}, proj));
    HPX_TEST(!hpx::is_sorted(
        unsorted_c.begin(), unsorted_c.end(), std::less<int>{}, proj));

    HPX_TEST(hpx::is_sorted(
        seq, sorted_c.begin(), sorted_c.end(), std::less<int>{}, proj));
    HPX_TEST(!hpx::is_sorted(
        seq, unsorted_c.begin(), unsorted_c.end(), std::less<int>{}, proj));

    HPX_TEST(hpx::is_sorted(
        par, sorted_c.begin(), sorted_c.end(), std::less<int>{}, proj));
    HPX_TEST(!hpx::is_sorted(
        par, unsorted_c.begin(), unsorted_c.end(), std::less<int>{}, proj));

    HPX_TEST(hpx::is_sorted(
        par(task), sorted_c.begin(), sorted_c.end(), std::less<int>{}, proj)
            .get());
    HPX_TEST(!hpx::is_sorted(
        par(task), unsorted_c.begin(), unsorted_c.end(), std::less<int>{}, proj)
            .get());

    HPX_TEST(hpx::is_sorted(
        par, sorted_c.begin(), sorted_c.begin(), std::less<int>{}, proj));
    HPX_TEST(hpx::is_sorted(
        par, sorted_c.begin(), sorted_c.begin() + 1, std::less<int>{}, proj));
}

void test_is_sorted_until_projection()
{
    using namespace hpx::execution;
    using element = std::pair<int, int>;

    std::vector<element> c = {{10, 1}, {20, 3}, {30, 8}, {40, 5}, {50, 9}};

    auto proj = [](element const& p) { return p.second; };

    auto it_seq =
        hpx::is_sorted_until(c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(it_seq != c.end());
    HPX_TEST_EQ(it_seq->second, 5);

    auto it_seq2 =
        hpx::is_sorted_until(seq, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(it_seq2 != c.end());
    HPX_TEST_EQ(it_seq2->second, 5);

    auto it_par = hpx::is_sorted_until(
        hpx::execution::par, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(it_par != c.end() && it_par->second == 5);

    auto it_task =
        hpx::is_sorted_until(hpx::execution::par(hpx::execution::task),
            c.begin(), c.end(), std::less<int>{}, proj)
            .get();
    HPX_TEST(it_task != c.end() && it_task->second == 5);

    std::vector<element> fully_sorted = {{10, 1}, {20, 3}, {30, 5}};
    auto it_end = hpx::is_sorted_until(
        par, fully_sorted.begin(), fully_sorted.end(), std::less<int>{}, proj);
    HPX_TEST(it_end == fully_sorted.end());

    auto it_empty = hpx::is_sorted_until(par, fully_sorted.begin(),
        fully_sorted.begin(), std::less<int>{}, proj);
    HPX_TEST(it_empty == fully_sorted.begin());
}

void test_is_partitioned_projection()
{
    using namespace hpx::execution;
    using element = std::pair<int, int>;

    std::vector<element> partitioned_c = {{10, 2}, {20, 4}, {30, 1}, {40, 3}};
    std::vector<element> not_partitioned_c = {
        {10, 2}, {20, 1}, {30, 4}, {40, 3}};

    auto proj = [](element const& p) { return p.second; };
    auto pred = [](int v) { return v % 2 == 0; };

    HPX_TEST(hpx::is_partitioned(
        partitioned_c.begin(), partitioned_c.end(), pred, proj));
    HPX_TEST(!hpx::is_partitioned(
        not_partitioned_c.begin(), not_partitioned_c.end(), pred, proj));

    HPX_TEST(hpx::is_partitioned(
        seq, partitioned_c.begin(), partitioned_c.end(), pred, proj));
    HPX_TEST(!hpx::is_partitioned(
        seq, not_partitioned_c.begin(), not_partitioned_c.end(), pred, proj));

    HPX_TEST(hpx::is_partitioned(
        par, partitioned_c.begin(), partitioned_c.end(), pred, proj));
    HPX_TEST(!hpx::is_partitioned(
        par, not_partitioned_c.begin(), not_partitioned_c.end(), pred, proj));

    HPX_TEST(hpx::is_partitioned(
        par(task), partitioned_c.begin(), partitioned_c.end(), pred, proj)
            .get());
    HPX_TEST(!hpx::is_partitioned(par(task), not_partitioned_c.begin(),
        not_partitioned_c.end(), pred, proj)
            .get());

    HPX_TEST(hpx::is_partitioned(
        par, partitioned_c.begin(), partitioned_c.begin(), pred, proj));
    HPX_TEST(hpx::is_partitioned(
        par, partitioned_c.begin(), partitioned_c.begin() + 1, pred, proj));
}

int hpx_main(hpx::program_options::variables_map&)
{
    test_is_sorted_projection();
    test_is_sorted_until_projection();
    test_is_partitioned_projection();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
