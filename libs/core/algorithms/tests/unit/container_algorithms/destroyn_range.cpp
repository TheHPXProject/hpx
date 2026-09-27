//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/memory.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_utils.hpp"

std::atomic<std::size_t> destruct_count(0);
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

struct destructable
{
    destructable()
      : value_(0)
    {
    }

    ~destructable()
    {
        ++destruct_count;
    }

    std::uint32_t value_;
};

std::size_t const data_size = 10007;

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_destroy_n(IteratorTag)
{
    typedef destructable* base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    destructable* p =
        (destructable*) std::malloc(data_size * sizeof(destructable));

    // value-initialize data in array
    std::for_each(p, p + data_size, [](destructable& d) {
        ::new (static_cast<void*>(std::addressof(d))) destructable;
    });

    destruct_count.store(0);

    hpx::ranges::destroy_n(iterator(p), data_size);

    HPX_TEST_EQ(destruct_count.load(), data_size);

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_destroy_n(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef destructable* base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    destructable* p =
        (destructable*) std::malloc(data_size * sizeof(destructable));

    // value-initialize data in array
    std::for_each(p, p + data_size, [](destructable& d) {
        ::new (static_cast<void*>(std::addressof(d))) destructable;
    });

    destruct_count.store(0);

    hpx::ranges::destroy_n(
        std::forward<ExPolicy>(policy), iterator(p), data_size);

    HPX_TEST_EQ(destruct_count.load(), data_size);

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_destroy_n_async(ExPolicy&& policy, IteratorTag)
{
    typedef destructable* base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    destructable* p =
        (destructable*) std::malloc(data_size * sizeof(destructable));

    // value-initialize data in array
    std::for_each(p, p + data_size, [](destructable& d) {
        ::new (static_cast<void*>(std::addressof(d))) destructable;
    });

    destruct_count.store(0);

    auto f = hpx::ranges::destroy_n(
        std::forward<ExPolicy>(policy), iterator(p), data_size);
    f.wait();

    HPX_TEST_EQ(destruct_count.load(), data_size);

    std::free(p);
}

template <typename IteratorTag>
void test_destroy_n()
{
    using namespace hpx::execution;

    test_destroy_n(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_destroy_n(seq, IteratorTag());
        test_destroy_n(par, IteratorTag());
        test_destroy_n(par_unseq, IteratorTag());

        test_destroy_n_async(seq(task), IteratorTag());
        test_destroy_n_async(par(task), IteratorTag());
    }
}

void destroy_n_test()
{
    test_destroy_n<std::random_access_iterator_tag>();
    test_destroy_n<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_destroy_n_exception(IteratorTag)
{
    typedef test::count_instances_v<destructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    // value-initialize data in array
    std::for_each(p, p + data_size, [](data_type& d) {
        ::new (static_cast<void*>(std::addressof(d))) data_type;
    });

    HPX_TEST_EQ(data_type::instance_count.load(), data_size);

    std::uniform_int_distribution<> dis(0, static_cast<int>(data_size - 1));
    std::atomic<std::size_t> throw_after(dis(gen));    //-V104
    std::size_t throw_after_ = throw_after.load();

    bool caught_exception = false;
    try
    {
        hpx::ranges::destroy_n(decorated_iterator(p,
                                   [&throw_after]() {
                                       if (throw_after-- == 0)
                                           throw std::runtime_error("test");
                                   }),
            data_size);
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<hpx::execution::sequenced_policy,
            IteratorTag>::call(hpx::execution::seq, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST_LTE(data_type::instance_count.load(),
        std::size_t(data_size - throw_after_));

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_destroy_n_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::count_instances_v<destructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    // value-initialize data in array
    std::for_each(p, p + data_size, [](data_type& d) {
        ::new (static_cast<void*>(std::addressof(d))) data_type;
    });

    HPX_TEST_EQ(data_type::instance_count.load(), data_size);

    std::uniform_int_distribution<> dis(0, static_cast<int>(data_size - 1));
    std::atomic<std::size_t> throw_after(dis(gen));    //-V104
    std::size_t throw_after_ = throw_after.load();

    bool caught_exception = false;
    try
    {
        hpx::ranges::destroy_n(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::runtime_error("test");
                }),
            data_size);
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
    HPX_TEST_LTE(data_type::instance_count.load(),
        std::size_t(data_size - throw_after_));

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_destroy_n_exception_async(ExPolicy&& policy, IteratorTag)
{
    typedef test::count_instances_v<destructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    // value-initialize data in array
    std::for_each(p, p + data_size, [](data_type& d) {
        ::new (static_cast<void*>(std::addressof(d))) data_type;
    });

    HPX_TEST_EQ(data_type::instance_count.load(), data_size);

    std::uniform_int_distribution<> dis(0, static_cast<int>(data_size - 1));
    std::atomic<std::size_t> throw_after(dis(gen));    //-V104
    std::size_t throw_after_ = throw_after.load();

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::destroy_n(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::runtime_error("test");
                }),
            data_size);

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
    HPX_TEST_LTE(data_type::instance_count.load(),
        std::size_t(data_size - throw_after_));

    std::free(p);
}

template <typename IteratorTag>
void test_destroy_n_exception()
{
    using namespace hpx::execution;

    test_destroy_n_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_destroy_n_exception(seq, IteratorTag());
        test_destroy_n_exception(par, IteratorTag());

        test_destroy_n_exception_async(seq(task), IteratorTag());
        test_destroy_n_exception_async(par(task), IteratorTag());
    }
}

void destroy_n_exception_test()
{
    test_destroy_n_exception<std::random_access_iterator_tag>();
    test_destroy_n_exception<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_destroy_n_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::count_instances_v<destructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    // value-initialize data in array
    std::for_each(p, p + data_size, [](data_type& d) {
        ::new (static_cast<void*>(std::addressof(d))) data_type;
    });

    HPX_TEST_EQ(data_type::instance_count.load(), data_size);

    std::uniform_int_distribution<> dis(0, static_cast<int>(data_size - 1));
    std::atomic<std::size_t> throw_after(dis(gen));    //-V104
    std::size_t throw_after_ = throw_after.load();

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::destroy_n(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::bad_alloc();
                }),
            data_size);

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
    HPX_TEST_LTE(data_type::instance_count.load(),
        std::size_t(data_size - throw_after_));

    std::free(p);
}

template <typename ExPolicy, typename IteratorTag>
void test_destroy_n_bad_alloc_async(ExPolicy&& policy, IteratorTag)
{
    typedef test::count_instances_v<destructable> data_type;
    typedef data_type* base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    // value-initialize data in array
    std::for_each(p, p + data_size, [](data_type& d) {
        ::new (static_cast<void*>(std::addressof(d))) data_type;
    });

    HPX_TEST_EQ(data_type::instance_count.load(), data_size);

    std::uniform_int_distribution<> dis(0, static_cast<int>(data_size - 1));
    std::atomic<std::size_t> throw_after(dis(gen));    //-V104
    std::size_t throw_after_ = throw_after.load();

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::destroy_n(policy,
            decorated_iterator(p,
                [&throw_after]() {
                    if (throw_after-- == 0)
                        throw std::bad_alloc();
                }),
            data_size);

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
    HPX_TEST_LTE(data_type::instance_count.load(),
        std::size_t(data_size - throw_after_));

    std::free(p);
}

template <typename IteratorTag>
void test_destroy_n_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_destroy_n_bad_alloc(seq, IteratorTag());
        test_destroy_n_bad_alloc(par, IteratorTag());

        test_destroy_n_bad_alloc_async(seq(task), IteratorTag());
        test_destroy_n_bad_alloc_async(par(task), IteratorTag());
    }
}

void destroy_n_bad_alloc_test()
{
    test_destroy_n_bad_alloc<std::random_access_iterator_tag>();
    test_destroy_n_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    destroy_n_test();
    destroy_n_exception_test();
    destroy_n_bad_alloc_test();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
