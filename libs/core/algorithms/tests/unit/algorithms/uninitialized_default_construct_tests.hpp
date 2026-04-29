//  Copyright (c) 2014-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/memory.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>

#include "test_utils.hpp"

struct default_constructable
{
    default_constructable()
      : value_(42)
    {
    }
    std::int32_t value_;
};

struct value_constructable
{
    std::int32_t value_;
};

std::size_t const data_size = 10007;

////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef default_constructable* base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    default_constructable* p = (default_constructable*) std::malloc(
        data_size * sizeof(default_constructable));
    std::memset(
        static_cast<void*>(p), 0xcd, data_size * sizeof(default_constructable));

    hpx::uninitialized_default_construct(
        std::forward<ExPolicy>(policy), iterator(p), iterator(p + data_size));

    std::size_t count = 0;
    std::for_each(p, p + data_size, [&count](default_constructable v1) {
        HPX_TEST_EQ(v1.value_, 42);
        ++count;
    });
    HPX_TEST_EQ(count, data_size);

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_async(ExPolicy&& policy, IteratorTag)
{
    typedef default_constructable* base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    default_constructable* p = (default_constructable*) std::malloc(
        data_size * sizeof(default_constructable));
    std::memset(
        static_cast<void*>(p), 0xcd, data_size * sizeof(default_constructable));

    auto f = hpx::uninitialized_default_construct(
        std::forward<ExPolicy>(policy), iterator(p), iterator(p + data_size));
    f.wait();

    std::size_t count = 0;
    std::for_each(p, p + data_size, [&count](default_constructable v1) {
        HPX_TEST_EQ(v1.value_, 42);
        ++count;
    });
    HPX_TEST_EQ(count, data_size);

    std::free(p);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    using base_iterator = default_constructable*;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    default_constructable* p = (default_constructable*) std::malloc(
        data_size * sizeof(default_constructable));
    std::memset(
        static_cast<void*>(p), 0xcd, data_size * sizeof(default_constructable));

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    tt::sync_wait(ex::just(iterator(p), iterator(p + data_size)) |
        hpx::uninitialized_default_construct(ex_policy.on(exec)));

    std::size_t count = 0;
    std::for_each(p, p + data_size, [&count](default_constructable v1) {
        HPX_TEST_EQ(v1.value_, 42);
        ++count;
    });
    HPX_TEST_EQ(count, data_size);

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef value_constructable* base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    value_constructable* p = (value_constructable*) std::malloc(
        data_size * sizeof(value_constructable));
    std::memset(
        static_cast<void*>(p), 0xcd, data_size * sizeof(value_constructable));

    hpx::uninitialized_default_construct(
        std::forward<ExPolicy>(policy), iterator(p), iterator(p + data_size));

    std::size_t count = 0;
    std::for_each(p, p + data_size, [&count](value_constructable v1) {
        HPX_TEST_EQ(v1.value_, (std::int32_t) 0xcdcdcdcd);
        ++count;
    });
    HPX_TEST_EQ(count, data_size);

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_async2(ExPolicy&& policy, IteratorTag)
{
    typedef value_constructable* base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    value_constructable* p = (value_constructable*) std::malloc(
        data_size * sizeof(value_constructable));
    std::memset(
        static_cast<void*>(p), 0xcd, data_size * sizeof(value_constructable));

    auto f = hpx::uninitialized_default_construct(
        std::forward<ExPolicy>(policy), iterator(p), iterator(p + data_size));
    f.wait();

    std::size_t count = 0;
    std::for_each(p, p + data_size, [&count](value_constructable v1) {
        HPX_TEST_EQ(v1.value_, (std::int32_t) 0xcdcdcdcd);
        ++count;
    });
    HPX_TEST_EQ(count, data_size);

    std::free(p);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_sender2(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    using base_iterator = value_constructable*;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    value_constructable* p = (value_constructable*) std::malloc(
        data_size * sizeof(value_constructable));
    std::memset(
        static_cast<void*>(p), 0xcd, data_size * sizeof(value_constructable));

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    tt::sync_wait(ex::just(iterator(p), iterator(p + data_size)) |
        hpx::uninitialized_default_construct(ex_policy.on(exec)));

    std::size_t count = 0;
    std::for_each(p, p + data_size, [&count](value_constructable v1) {
        HPX_TEST_EQ(v1.value_, (std::int32_t) 0xcdcdcdcd);
        ++count;
    });
    HPX_TEST_EQ(count, data_size);

    std::free(p);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_exception(
    ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::count_instances_v<default_constructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_exception = false;
    try
    {
        hpx::uninitialized_default_construct(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::runtime_error("test");
                }),
            decorated_iterator(p + data_size));
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
    HPX_TEST_EQ(data_type::instance_count.load(), std::size_t(0));
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_exception_async(
    ExPolicy policy, IteratorTag)
{
    typedef test::count_instances_v<default_constructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::uninitialized_default_construct(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::runtime_error("test");
                }),
            decorated_iterator(p + data_size));

        returned_from_algorithm = true;
        f.get();

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
    HPX_TEST(returned_from_algorithm);
    HPX_TEST_EQ(data_type::instance_count.load(), std::size_t(0));
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_exception_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    using data_type = test::count_instances_v<default_constructable>;
    using base_iterator = data_type*;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

        auto f = hpx::uninitialized_default_construct(ex_policy.on(exec),
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::runtime_error("test");
                }),
            decorated_iterator(p + data_size));

        returned_from_algorithm = true;
        tt::sync_wait(HPX_MOVE(f));

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(ex_policy, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
    HPX_TEST_EQ(data_type::instance_count.load(), std::size_t(0));
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_bad_alloc(
    ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::count_instances_v<default_constructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_bad_alloc = false;
    try
    {
        hpx::uninitialized_default_construct(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::bad_alloc();
                }),
            decorated_iterator(p + data_size));

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST_EQ(data_type::instance_count.load(), std::size_t(0));
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_bad_alloc_async(
    ExPolicy policy, IteratorTag)
{
    typedef test::count_instances_v<default_constructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::uninitialized_default_construct(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::bad_alloc();
                }),
            decorated_iterator(p + data_size));

        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST(returned_from_algorithm);
    HPX_TEST_EQ(data_type::instance_count.load(), std::size_t(0));
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_bad_alloc_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    typedef test::count_instances_v<default_constructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

        auto f = hpx::uninitialized_default_construct(ex_policy.on(exec),
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::bad_alloc();
                }),
            decorated_iterator(p + data_size));

        returned_from_algorithm = true;
        tt::sync_wait(HPX_MOVE(f));

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST(returned_from_algorithm);
    HPX_TEST_EQ(data_type::instance_count.load(), std::size_t(0));
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}
