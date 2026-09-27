//  Copyright (c) 2014-2015 Hartmut Kaiser
//  Copyright (c)      2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
struct equal_f
{
    equal_f(std::size_t val)
      : val_(val)
    {
    }

    bool operator()(std::size_t lhs) const
    {
        return lhs == val_;
    }

    std::size_t val_;
};

////////////////////////////////////////////////////////////////////////////
void test_replace_if_sent()
{
    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size);
    std::iota(std::begin(c), std::end(c), 1);

    auto pred = [](std::int16_t const& a) -> bool { return a % 42 == 0; };

    auto pre_result = std::count_if(std::begin(c), std::end(c), pred);
    hpx::ranges::replace_if(
        std::begin(c), sentinel<std::int16_t>{50}, pred, std::int16_t(1));
    auto post_result = std::count_if(std::begin(c), std::end(c), pred);

    HPX_TEST(pre_result == 2 && post_result == 1);
}

template <typename ExPolicy>
void test_replace_if_sent(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size);
    std::iota(std::begin(c), std::end(c), 1);

    auto pred = [](std::int16_t const& a) -> bool { return a % 42 == 0; };

    auto pre_result = std::count_if(std::begin(c), std::end(c), pred);
    hpx::ranges::replace_if(policy, std::begin(c),
        test::make_sized_sentinel(std::begin(c), sentinel<std::int16_t>{50}),
        pred, std::int16_t(1));
    auto post_result = std::count_if(std::begin(c), std::end(c), pred);

    HPX_TEST(pre_result == 2 && post_result == 1);
}

template <typename IteratorTag>
void test_replace_if(IteratorTag)
{
    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::begin(d));

    std::size_t idx = std::rand() % c.size();    //-V104

    hpx::ranges::replace_if(c, equal_f(c[idx]), c[idx] + 1);

    std::replace_if(std::begin(d), std::end(d), equal_f(d[idx]), d[idx] + 1);

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_replace_if(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::begin(d));

    std::size_t idx = std::rand() % c.size();    //-V104

    hpx::ranges::replace_if(policy, c, equal_f(c[idx]), c[idx] + 1);

    std::replace_if(std::begin(d), std::end(d), equal_f(d[idx]), d[idx] + 1);

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_replace_if_async(ExPolicy p, IteratorTag)
{
    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::begin(d));

    std::size_t idx = std::rand() % c.size();    //-V104

    auto f = hpx::ranges::replace_if(p, c, equal_f(c[idx]), c[idx] + 1);
    f.wait();

    std::replace_if(std::begin(d), std::end(d), equal_f(d[idx]), d[idx] + 1);

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename IteratorTag>
void test_replace_if()
{
    using namespace hpx::execution;
    test_replace_if(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_replace_if(seq, IteratorTag());
        test_replace_if(par, IteratorTag());
        test_replace_if(par_unseq, IteratorTag());

        test_replace_if_async(seq(task), IteratorTag());
        test_replace_if_async(par(task), IteratorTag());
    }
}

void replace_if_test()
{
    test_replace_if<std::random_access_iterator_tag>();
    test_replace_if<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_replace_if_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try
    {
        hpx::ranges::replace_if(
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            equal_f(42), std::size_t(43));
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
}

template <typename ExPolicy, typename IteratorTag>
void test_replace_if_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try
    {
        hpx::ranges::replace_if(policy,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            equal_f(42), std::size_t(43));
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
void test_replace_if_exception_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::replace_if(p,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            equal_f(42), std::size_t(43));
        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_replace_if_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_replace_if_exception(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_replace_if_exception(seq, IteratorTag());
        test_replace_if_exception(par, IteratorTag());

        test_replace_if_exception_async(seq(task), IteratorTag());
        test_replace_if_exception_async(par(task), IteratorTag());

        test_replace_if_sent();
        test_replace_if_sent(seq);
        test_replace_if_sent(par);
        test_replace_if_sent(par_unseq);
    }
}

void replace_if_exception_test()
{
    test_replace_if_exception<std::random_access_iterator_tag>();
    test_replace_if_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_replace_if_bad_alloc(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::replace_if(
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            equal_f(42), std::size_t(43));
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
}

template <typename ExPolicy, typename IteratorTag>
void test_replace_if_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::replace_if(policy,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            equal_f(42), std::size_t(43));
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
}

template <typename ExPolicy, typename IteratorTag>
void test_replace_if_bad_alloc_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::replace_if(p,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            equal_f(42), std::size_t(43));
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
}

template <typename IteratorTag>
void test_replace_if_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_replace_if_bad_alloc(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_replace_if_bad_alloc(seq, IteratorTag());
        test_replace_if_bad_alloc(par, IteratorTag());

        test_replace_if_bad_alloc_async(seq(task), IteratorTag());
        test_replace_if_bad_alloc_async(par(task), IteratorTag());
    }
}

void replace_if_bad_alloc_test()
{
    test_replace_if_bad_alloc<std::random_access_iterator_tag>();
    test_replace_if_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    replace_if_test();
    replace_if_exception_test();
    replace_if_bad_alloc_test();
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
