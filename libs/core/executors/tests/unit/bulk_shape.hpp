//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <type_traits>
#include <vector>

namespace executor_test {
    struct forward_iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        using pointer = int*;
        using reference = int&;

        std::vector<int>::iterator current;
        std::shared_ptr<std::atomic<std::size_t>> increments;

        int& operator*() const
        {
            return *current;
        }

        forward_iterator& operator++()
        {
            ++*increments;
            ++current;
            return *this;
        }

        forward_iterator operator++(int)
        {
            auto old = *this;
            ++*this;
            return old;
        }

        friend bool operator==(
            forward_iterator const&, forward_iterator const&) = default;
    };

    struct void_work
    {
        void operator()(int) const {}
    };

    template <typename T>
    void wait(T&& work)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            work.get();
        else if constexpr (hpx::execution::experimental::is_sender_v<
                               std::decay_t<T>>)
            hpx::this_thread::experimental::sync_wait(HPX_FORWARD(T, work));
        else
            hpx::wait_all(work);
    }

    template <typename T>
    std::vector<int> values(T&& work)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return work.get();
        else if constexpr (hpx::execution::experimental::is_sender_v<
                               std::decay_t<T>>)
        {
            auto result =
                hpx::this_thread::experimental::sync_wait(HPX_FORWARD(T, work));
            HPX_TEST(result.has_value());
            return hpx::get<0>(HPX_MOVE(result.value()));
        }
        else
        {
            std::vector<int> result;
            for (auto& future : work)
                result.push_back(future.get());
            return result;
        }
    }

    template <bool ResultContinuation, typename Executor>
    void test_bulk_shape(Executor& exec)
    {
        using input_shape = std::ranges::subrange<std::istream_iterator<int>>;
        static_assert(!std::ranges::forward_range<input_shape>);
        static_assert(!requires(input_shape shape) {
            exec.bulk_async_execute(void_work{}, shape);
        });
        static_assert(!requires(
            input_shape shape) { exec.bulk_sync_execute(void_work{}, shape); });
        static_assert(
            !requires(input_shape shape, hpx::shared_future<void> predecessor) {
                exec.bulk_then_execute(void_work{}, shape, predecessor);
            });

        std::vector<int> input(64);
        std::iota(input.begin(), input.end(), 100);
        auto increments = std::make_shared<std::atomic<std::size_t>>(0);
        hpx::util::iterator_range shape(
            forward_iterator{input.begin(), increments},
            forward_iterator{input.end(), increments});
        static_assert(std::ranges::forward_range<decltype(shape)>);
        static_assert(!std::ranges::random_access_range<decltype(shape)>);

        wait(exec.bulk_async_execute([](auto&& value) { ++value; }, shape));
        HPX_TEST_LTE(increments->load(), 2 * input.size());
        HPX_TEST_EQ(input.front(), 101);
        HPX_TEST_EQ(input.back(), 164);

        *increments = 0;
        auto result = values(exec.bulk_async_execute(
            [](int value) { return 2 * value; }, shape));
        HPX_TEST_LTE(increments->load(), 2 * input.size());
        HPX_TEST_EQ(result.size(), input.size());
        for (std::size_t i = 0; i < input.size(); ++i)
            HPX_TEST_EQ(result[i], 2 * input[i]);

        *increments = 0;
        exec.bulk_sync_execute([](auto&& value) { ++value; }, shape);
        HPX_TEST_LTE(increments->load(), 2 * input.size());
        HPX_TEST_EQ(input.front(), 102);
        HPX_TEST_EQ(input.back(), 165);

        auto predecessor = hpx::make_ready_future().share();
        *increments = 0;
        wait(exec.bulk_then_execute(
            [](auto&& value, hpx::shared_future<void> ready) {
                ready.get();
                ++value;
            },
            shape, predecessor));
        HPX_TEST_LTE(increments->load(), 2 * input.size());
        HPX_TEST_EQ(input.front(), 103);
        HPX_TEST_EQ(input.back(), 166);

        if constexpr (ResultContinuation)
        {
            *increments = 0;
            result = values(exec.bulk_then_execute(
                [](int value, hpx::shared_future<void> ready) {
                    ready.get();
                    return 3 * value;
                },
                shape, predecessor));
            HPX_TEST_LTE(increments->load(), 2 * input.size());
            for (std::size_t i = 0; i < input.size(); ++i)
                HPX_TEST_EQ(result[i], 3 * input[i]);
        }

        // The shape may own its elements and die before a sender starts.
        auto work = exec.bulk_async_execute(
            [](int value) { return value; }, std::forward_list<int>{7, 3, 9});
        result = values(HPX_MOVE(work));
        HPX_TEST(result == std::vector<int>({7, 3, 9}));

        auto empty = hpx::util::iterator_range(shape.begin(), shape.begin());
        wait(exec.bulk_async_execute([](int) { HPX_TEST(false); }, empty));
    }
}    // namespace executor_test
