//  Copyright (c) 2014-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////////
struct counter
{
    std::size_t count = 0;
    void operator()(std::size_t& v)
    {
        ++count;
        v = 42;
    }
};

template <typename IteratorTag>
void test_for_each_seq(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    counter f;
    auto res =
        hpx::ranges::for_each(hpx::util::iterator_range(iterator(std::begin(c)),
                                  iterator(std::end(c))),
            std::ref(f));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
    HPX_TEST(res.in == iterator(std::end(c)));
    HPX_TEST_EQ(static_cast<counter>(res.fun).count, c.size());
    HPX_TEST_EQ(f.count, c.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_for_each(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value);

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    iterator result = hpx::ranges::for_each(std::forward<ExPolicy>(policy),
        hpx::util::iterator_range(
            iterator(std::begin(c)), iterator(std::end(c))),
        [](std::size_t& v) { v = 42; });
    HPX_TEST(result == iterator(std::end(c)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_for_each_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    hpx::future<iterator> f = hpx::ranges::for_each(std::forward<ExPolicy>(p),
        hpx::util::iterator_range(
            iterator(std::begin(c)), iterator(std::end(c))),
        [](std::size_t& v) { v = 42; });
    HPX_TEST(f.get() == iterator(std::end(c)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

////////////////////////////////////////////////////////////////////////////////
struct counter_exception
{
    std::size_t count = 0;
    [[noreturn]] void operator()(std::size_t&)
    {
        ++count;
        throw std::runtime_error("test");
    }
};

template <typename IteratorTag>
void test_for_each_exception_seq(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    counter_exception f;
    try
    {
        hpx::ranges::for_each(hpx::util::iterator_range(iterator(std::begin(c)),
                                  iterator(std::end(c))),
            std::ref(f));

        HPX_TEST(false);
    }
    catch (hpx::exception_list const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST_EQ(f.count, static_cast<std::size_t>(1));
}

template <typename ExPolicy, typename IteratorTag>
void test_for_each_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value);

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try
    {
        hpx::ranges::for_each(policy,
            hpx::util::iterator_range(
                iterator(std::begin(c)), iterator(std::end(c))),
            [](std::size_t&) { throw std::runtime_error("test"); });

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_for_each_exception_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<iterator> f = hpx::ranges::for_each(p,
            hpx::util::iterator_range(
                iterator(std::begin(c)), iterator(std::end(c))),
            [](std::size_t&) { throw std::runtime_error("test"); });
        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
    }
    catch (std::runtime_error const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        caught_exception = true;
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

////////////////////////////////////////////////////////////////////////////////
struct counter_bad_alloc
{
    std::size_t count = 0;
    [[noreturn]] void operator()(std::size_t&)
    {
        ++count;
        throw std::bad_alloc();
    }
};

template <typename IteratorTag>
void test_for_each_bad_alloc_seq(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    counter_bad_alloc f;
    try
    {
        hpx::ranges::for_each(hpx::util::iterator_range(iterator(std::begin(c)),
                                  iterator(std::end(c))),
            std::ref(f));

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST_EQ(f.count, static_cast<std::size_t>(1));
}

template <typename ExPolicy, typename IteratorTag>
void test_for_each_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value);

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try
    {
        hpx::ranges::for_each(policy,
            hpx::util::iterator_range(
                iterator(std::begin(c)), iterator(std::end(c))),
            [](std::size_t&) { throw std::bad_alloc(); });

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_for_each_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<iterator> f = hpx::ranges::for_each(p,
            hpx::util::iterator_range(
                iterator(std::begin(c)), iterator(std::end(c))),
            [](std::size_t&) { throw std::bad_alloc(); });
        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_for_each_sender(
    [[maybe_unused]] Policy l, [[maybe_unused]] ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    auto rng = hpx::util::iterator_range(
        iterator(std::begin(c)), iterator(std::end(c)));
    auto f = [](std::size_t& v) { v = 42; };

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    auto result = hpx::get<0>(
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        *tt::sync_wait(ex::just(rng, f) | hpx::ranges::for_each(p.on(exec))));
    HPX_TEST(result == iterator(std::end(c)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_for_each_sender_bulk(
    [[maybe_unused]] Policy l, [[maybe_unused]] ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    auto rng = hpx::util::iterator_range(
        iterator(std::begin(c)), iterator(std::end(c)));
    auto f = [](std::size_t& v) { v = 42; };

    // Test stdexec bulk sender directly (not using HPX for_each algorithm)
    auto result = hpx::get<0>(
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        *tt::sync_wait(
            ex::just(rng, f) | ex::let_value([](auto&& rng, auto&& f) {
                auto begin_it = rng.begin();
                return ex::bulk(ex::just(),
                           std::distance(rng.begin(), rng.end()),
                           [begin_it, f = HPX_FORWARD(decltype(f), f)](
                               std::size_t i) mutable {
                               auto it = begin_it;
                               std::advance(it, i);
                               f(*it);
                           }) |
                    ex::then([rng]() { return rng.end(); });
            })));
    HPX_TEST(result == iterator(std::end(c)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_for_each_exception_sender(
    [[maybe_unused]] Policy l, ExPolicy&& p, IteratorTag)
{
    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    auto rng = hpx::util::iterator_range(
        iterator(std::begin(c)), iterator(std::end(c)));
    auto f = [](std::size_t&) { throw std::runtime_error("test"); };

    bool caught_exception = false;
    try
    {
        auto result = tt::sync_wait(
            ex::just(rng, f) | ex::let_value([](auto&& rng, auto&& f) {
                auto begin_it = rng.begin();
                return ex::bulk(ex::just(),
                    std::distance(rng.begin(), rng.end()),
                    [begin_it, f = HPX_FORWARD(decltype(f), f)](
                        std::size_t i) mutable {
                        auto it = begin_it;
                        std::advance(it, i);
                        f(*it);
                    });
            }));

        // If sync_wait returns without exception, check if result indicates error
        if (!result.has_value())
        {
            caught_exception = true;
        }
        else
        {
            HPX_TEST(false);
        }
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
    }
    catch (std::runtime_error const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        caught_exception = true;
    }

    HPX_TEST(caught_exception);
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_for_each_bad_alloc_sender(
    [[maybe_unused]] Policy l, [[maybe_unused]] ExPolicy&& p, IteratorTag)
{
    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    auto rng = hpx::util::iterator_range(
        iterator(std::begin(c)), iterator(std::end(c)));
    auto f = [](std::size_t&) { throw std::bad_alloc(); };

    bool caught_exception = false;
    try
    {
        tt::sync_wait(
            ex::just(rng, f) | ex::let_value([](auto&& rng, auto&& f) {
                auto begin_it = rng.begin();
                return ex::bulk(ex::just(),
                    std::distance(rng.begin(), rng.end()),
                    [begin_it, f = HPX_FORWARD(decltype(f), f)](
                        std::size_t i) mutable {
                        auto it = begin_it;
                        std::advance(it, i);
                        f(*it);
                    });
            }));

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}
