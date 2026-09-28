//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <random>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
    template <typename T>
    auto value(T&& result)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return result.get();
        else
            return HPX_FORWARD(T, result);
    }

    struct record
    {
        int key;
        int origin;
        friend bool operator==(record const&, record const&) = default;
    };

    void test_merge_partitions()
    {
        std::mt19937 random(6602);
        auto policy = hpx::execution::par.with(
            hpx::execution::experimental::static_chunk_size(1));
        for (int size = 0; size != 35; ++size)
        {
            std::vector<record> a, b;
            for (int i = 0; i != size; ++i)
            {
                a.push_back({int(random() % 8), 0});
                b.push_back({int(random() % 8), 1});
            }
            std::ranges::sort(a, {}, &record::key);
            std::ranges::sort(b, {}, &record::key);
            std::vector<record> expected(a.size() + b.size());
            std::ranges::merge(
                a, b, expected.begin(), {}, &record::key, &record::key);
            for (int capacity = 0; capacity <= 2 * size; ++capacity)
            {
                std::vector<record> output(capacity);
                auto result = hpx::ranges::merge(policy, a, b, output,
                    std::ranges::less{}, &record::key, &record::key);
                HPX_TEST(
                    std::equal(output.begin(), output.end(), expected.begin()));
                auto first_count = std::count_if(output.begin(), output.end(),
                    [](record r) { return r.origin == 0; });
                HPX_TEST(result.in1 == a.begin() + first_count);
                HPX_TEST(result.in2 == b.begin() + capacity - first_count);
                HPX_TEST(result.out == output.end());
            }
        }
    }

    struct converted_record
    {
        int rank = 0;
        converted_record& operator=(record const& source)
        {
            rank = source.key;
            return *this;
        }
    };

    template <typename Policy>
    void test_partial_sort_conversion(Policy policy)
    {
        std::vector<record> input{{3, 0}, {1, 0}, {4, 0}, {2, 0}};
        std::vector<converted_record> output(2);
        auto result =
            value(hpx::ranges::partial_sort_copy(policy, input, output,
                std::ranges::less{}, &record::key, &converted_record::rank));
        HPX_TEST(result.in == input.end());
        HPX_TEST(result.out == output.end());
        HPX_TEST_EQ(output[0].rank, 1);
        HPX_TEST_EQ(output[1].rank, 2);
        output.clear();
        result = value(hpx::ranges::partial_sort_copy(policy, input, output,
            std::ranges::less{}, &record::key, &converted_record::rank));
        HPX_TEST(result.in == input.end());
        HPX_TEST(result.out == output.end());
    }

    template <typename Policy>
    void test_exceptions(Policy policy)
    {
        std::vector<int> input{1, 2, 3}, output(2);
        auto comparison = [](int, int) -> bool {
            throw std::runtime_error("comparison");
        };
        bool caught = false;
        try
        {
            value(hpx::ranges::merge(policy, input, input, output, comparison));
        }
        catch (hpx::exception_list const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
        caught = false;
        try
        {
            value(hpx::ranges::set_union(
                policy, input, input, output, comparison));
        }
        catch (hpx::exception_list const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
    }

    void test_sender_results()
    {
        namespace ex = hpx::execution::experimental;
        namespace tt = hpx::this_thread::experimental;
        auto exec =
            ex::explicit_scheduler_executor(ex::thread_pool_scheduler{});
        auto policy = hpx::execution::par(hpx::execution::task).on(exec);
        std::vector<int> a{1, 2, 4}, b{2, 3}, output(3);
        auto merged = tt::sync_wait(hpx::ranges::merge(policy, a, b, output));
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 2}));
        HPX_TEST(hpx::get<0>(*merged).out == output.end());
        auto combined =
            tt::sync_wait(hpx::ranges::set_union(policy, a, b, output));
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 3}));
        HPX_TEST(hpx::get<0>(*combined).in1 == a.begin() + 2);
        HPX_TEST(hpx::get<0>(*combined).in2 == b.end());
    }

    template <typename Policy>
    void test(Policy policy)
    {
        test_partial_sort_conversion(policy);
        using namespace hpx::ranges;
        std::vector<int> a{1, 2, 2, 4, 7}, b{2, 3, 4, 4, 8};
        std::vector<int> output(4);
        auto result = value(merge(policy, a, b, output));
        HPX_TEST(result.in1 == a.begin() + 3);
        HPX_TEST(result.in2 == b.begin() + 1);
        HPX_TEST(result.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 2, 2}));
        output.resize(10);
        result = value(merge(policy, a, b, output));
        HPX_TEST(result.in1 == a.end());
        HPX_TEST(result.in2 == b.end());
        HPX_TEST(std::ranges::equal(
            output, std::array{1, 2, 2, 2, 3, 4, 4, 4, 7, 8}));
        output.resize(4);
        result = value(set_union(policy, a, b, output));
        HPX_TEST(result.in1 == a.begin() + 3);
        HPX_TEST(result.in2 == b.begin() + 2);
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 2, 3}));
        output.resize(1);
        result = value(set_intersection(policy, a, b, output));
        HPX_TEST(result.in1 == a.begin() + 3);
        HPX_TEST(result.in2 == b.begin() + 2);
        HPX_TEST_EQ(output[0], 2);
        output.resize(2);
        auto difference = value(set_difference(policy, a, b, output));
        HPX_TEST(difference.in == a.begin() + 4);
        HPX_TEST(difference.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2}));
        output.resize(3);
        result = value(set_symmetric_difference(policy, a, b, output));
        HPX_TEST(result.in1 == a.begin() + 4);
        HPX_TEST(result.in2 == b.begin() + 3);
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 3}));
        output.resize(8);
        result = value(set_union(policy, a, b, output));
        HPX_TEST(result.in1 == a.end());
        HPX_TEST(result.in2 == b.end());
        HPX_TEST(
            std::ranges::equal(output, std::array{1, 2, 2, 3, 4, 4, 7, 8}));
        output.resize(2);
        result = value(set_intersection(policy, a, b, output));
        HPX_TEST(result.in1 == a.end());
        HPX_TEST(result.in2 == b.end());
        HPX_TEST(std::ranges::equal(output, std::array{2, 4}));
        output.clear();
        result = value(set_intersection(policy, a, b, output));
        HPX_TEST(result.in1 == a.begin() + 1);
        HPX_TEST(result.in2 == b.begin());
        result = value(merge(policy, a, b, output));
        HPX_TEST(result.in1 == a.begin());
        HPX_TEST(result.in2 == b.begin());
        a.clear();
        result = value(set_intersection(policy, a, b, output));
        HPX_TEST(result.in1 == a.end());
        HPX_TEST(result.in2 == b.end());

        a = {3, 1, 2, 5, 4};
        output.resize(3);
        auto sorted = value(partial_sort_copy(policy, a, output));
        HPX_TEST(sorted.in == a.end());
        HPX_TEST(sorted.out == output.end());
        HPX_TEST(std::ranges::equal(output, std::array{1, 2, 3}));
        a = {1, 2, 2, 4, 7};
        HPX_TEST(value(includes(policy, a, std::array{2, 2, 7})));
        HPX_TEST(!value(includes(policy, a, std::array{2, 2, 2})));
        auto swapped = value(swap_ranges(policy, a, output));
        HPX_TEST(swapped.in1 == a.begin() + 3);
        HPX_TEST(swapped.in2 == output.end());
        a = {1, 3, 5, 2, 4};
        HPX_TEST(value(inplace_merge(policy, a, a.begin() + 3)) == a.end());
        HPX_TEST(std::ranges::is_sorted(a));
    }
}    // namespace

int hpx_main()
{
    using namespace hpx::execution;
    test(seq);
    test(par);
    test(par_unseq);
    test(seq(task));
    test(par(task));
    test_merge_partitions();
    test_sender_results();
    test_exceptions(seq);
    test_exceptions(par);
    test_exceptions(seq(task));
    test_exceptions(par(task));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
