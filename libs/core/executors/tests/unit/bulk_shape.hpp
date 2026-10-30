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

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <numeric>
#include <ranges>
#include <type_traits>
#include <vector>

namespace executor_test {
    class move_only_shape
    {
    public:
        explicit move_only_shape(std::vector<int> values)
          : values_(HPX_MOVE(values))
        {
        }

        move_only_shape(move_only_shape&&) = default;
        move_only_shape& operator=(move_only_shape&&) = default;
        move_only_shape(move_only_shape const&) = delete;
        move_only_shape& operator=(move_only_shape const&) = delete;

        auto begin() const
        {
            return values_.begin();
        }

        auto end() const
        {
            return values_.end();
        }

        std::size_t size() const noexcept
        {
            return values_.size();
        }

    private:
        std::vector<int> values_;
    };

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

        std::vector<int> filter_input{1, 2, 3};
        auto filter_shape = filter_input |
            std::views::filter([](int value) { return value % 2 != 0; });
        using filter_shape_type = decltype(filter_shape);
        static_assert(!std::ranges::forward_range<filter_shape_type const>);
        static_assert(!requires(filter_shape_type shape) {
            exec.bulk_async_execute(void_work{}, shape);
        });
        static_assert(!requires(filter_shape_type shape) {
            exec.bulk_sync_execute(void_work{}, shape);
        });
        static_assert(!requires(
            filter_shape_type shape, hpx::shared_future<void> predecessor) {
            exec.bulk_then_execute(void_work{}, shape, predecessor);
        });

        using move_only_element_shape = std::vector<std::unique_ptr<int>>;
        static_assert(!requires(move_only_element_shape shape) {
            exec.bulk_async_execute(void_work{}, shape);
        });
        static_assert(!requires(move_only_element_shape shape) {
            exec.bulk_sync_execute(void_work{}, shape);
        });
        static_assert(!requires(move_only_element_shape shape,
            hpx::shared_future<void> predecessor) {
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

            std::vector<int> reordered_shape{7, 3, 9};
            result = values(exec.bulk_then_execute(
                [](int value, hpx::shared_future<void> ready) {
                    ready.get();
                    return 2 * value;
                },
                reordered_shape, predecessor));
            HPX_TEST(result == std::vector<int>({14, 6, 18}));
        }

        // The shape may own its elements and die before a sender starts.
        auto work = exec.bulk_async_execute(
            [](int value) { return value; }, std::forward_list<int>{7, 3, 9});
        result = values(HPX_MOVE(work));
        HPX_TEST(result == std::vector<int>({7, 3, 9}));

        // Random-access shapes are also copied into asynchronous operations.
        auto random_access_work = exec.bulk_async_execute(
            [](int value) { return value; }, std::vector<int>{9, 5, 1});
        result = values(HPX_MOVE(random_access_work));
        HPX_TEST(result == std::vector<int>({9, 5, 1}));

        // A forward transform view may produce prvalues when dereferenced.
        std::forward_list<int> forward_values{1, 2, 3};
        auto prvalue_shape = forward_values |
            std::views::transform([](int value) { return 2 * value; });
        static_assert(std::ranges::forward_range<decltype(prvalue_shape)>);
        static_assert(!std::is_reference_v<
            std::ranges::range_reference_t<decltype(prvalue_shape)>>);
        result = values(exec.bulk_async_execute(
            [](int value) { return value; }, prvalue_shape));
        HPX_TEST(result == std::vector<int>({2, 4, 6}));

        std::list<int> sized_shape{4, 8, 15, 16, 23, 42};
        static_assert(std::ranges::sized_range<decltype(sized_shape)>);
        static_assert(!std::ranges::random_access_range<decltype(sized_shape)>);
        result = values(exec.bulk_async_execute(
            [](int value) { return value; }, sized_shape));
        HPX_TEST_EQ(result.size(), std::ranges::size(sized_shape));
        HPX_TEST(std::ranges::equal(result, sized_shape));

        move_only_shape move_only(std::vector<int>{2, 7, 1, 8});
        static_assert(!std::is_copy_constructible_v<move_only_shape>);
        static_assert(std::ranges::random_access_range<move_only_shape const>);
        result = values(exec.bulk_async_execute(
            [](int value) { return value; }, move_only));
        HPX_TEST(result == std::vector<int>({2, 7, 1, 8}));

        auto empty = hpx::util::iterator_range(shape.begin(), shape.begin());
        wait(exec.bulk_async_execute([](int) { HPX_TEST(false); }, empty));
        result = values(exec.bulk_async_execute(
            [](int) {
                HPX_TEST(false);
                return 0;
            },
            empty));
        HPX_TEST(result.empty());

        exec.bulk_sync_execute([](int) { HPX_TEST(false); }, empty);
        wait(exec.bulk_then_execute(
            [](int, hpx::shared_future<void>) { HPX_TEST(false); }, empty,
            predecessor));

        if constexpr (ResultContinuation)
        {
            result = values(exec.bulk_then_execute(
                [](int, hpx::shared_future<void>) {
                    HPX_TEST(false);
                    return 0;
                },
                empty, predecessor));
            HPX_TEST(result.empty());
        }
    }
}    // namespace executor_test
