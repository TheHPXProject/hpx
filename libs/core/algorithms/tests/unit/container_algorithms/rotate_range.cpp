//  Copyright (c) 2007-2014 Hartmut Kaiser
//  Copyright (c) 2021 Chuanqiu He
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
// test case for iterator - sentinel_value
template <typename IteratorTag>
void test_rotate_sent(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d1;

    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::back_inserter(d1));

    std::size_t mid_pos = std::rand() % c.size();    //-V104
    auto mid = std::begin(c);
    std::advance(mid, mid_pos);

    hpx::ranges::rotate(
        std::begin(c), mid, sentinel<std::size_t>{*(std::end(c) - 1)});

    auto mid1 = std::begin(d1);
    std::advance(mid1, mid_pos);
    std::rotate(std::begin(d1), mid1, std::end(d1) - 1);

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d1),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size() - 1);
}

template <typename ExPolicy, typename IteratorTag>
void test_rotate_sent(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d1;

    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::back_inserter(d1));

    std::size_t mid_pos = std::rand() % c.size();    //-V104
    auto mid = std::begin(c);
    std::advance(mid, mid_pos);

    hpx::ranges::rotate(
        policy, std::begin(c), mid, sentinel<std::size_t>{*(std::end(c) - 1)});

    auto mid1 = std::begin(d1);
    std::advance(mid1, mid_pos);
    std::rotate(std::begin(d1), mid1, std::end(d1) - 1);

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d1),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size() - 1);
}

//add ranges test w/o ExPolicy
template <typename IteratorTag>
void test_rotate(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using test_vector =
        test::test_container<std::vector<std::size_t>, IteratorTag>;

    test_vector c(10007);
    std::vector<std::size_t> d1;

    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::back_inserter(d1));

    std::size_t mid_pos = std::rand() % c.size();    //-V104
    auto mid = std::begin(c);
    std::advance(mid, mid_pos);

    hpx::ranges::rotate(c, iterator(mid));

    base_iterator mid1 = std::begin(d1);
    std::advance(mid1, mid_pos);
    std::rotate(std::begin(d1), mid1, std::end(d1));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()),
        std::begin(d1), [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_rotate(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d1;

    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::back_inserter(d1));

    std::size_t mid_pos = std::rand() % c.size();    //-V104
    auto mid = std::begin(c);
    std::advance(mid, mid_pos);

    hpx::ranges::rotate(policy, c, mid);

    auto mid1 = std::begin(d1);
    std::advance(mid1, mid_pos);
    std::rotate(std::begin(d1), mid1, std::end(d1));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d1),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_rotate_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d1;

    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::back_inserter(d1));

    std::size_t mid_pos = std::rand() % c.size();    //-V104
    auto mid = std::begin(c);
    std::advance(mid, mid_pos);

    auto f = hpx::ranges::rotate(p, c, mid);
    f.wait();

    auto mid1 = std::begin(d1);
    std::advance(mid1, mid_pos);
    std::rotate(std::begin(d1), mid1, std::end(d1));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d1),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size());
}

template <typename IteratorTag>
void test_rotate_sentinel(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using test_vector =
        test::test_sentinel_container<std::vector<std::size_t>, IteratorTag>;

    test_vector c(10007);
    std::vector<std::size_t> d1;

    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::back_inserter(d1));

    std::size_t mid_pos = std::rand() % c.size();    //-V104
    auto mid = std::begin(c);
    std::advance(mid, mid_pos);

    hpx::ranges::rotate(c, iterator(mid));

    base_iterator mid1 = std::begin(d1);
    std::advance(mid1, mid_pos);
    std::rotate(std::begin(d1), mid1, std::end(d1));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()),
        std::begin(d1), [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_rotate_sentinel(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using test_vector =
        test::test_sentinel_container<std::vector<std::size_t>, IteratorTag>;

    test_vector c(10007);
    std::vector<std::size_t> d1;

    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::back_inserter(d1));

    std::size_t mid_pos = std::rand() % c.size();    //-V104
    auto mid = std::begin(c);
    std::advance(mid, mid_pos);

    hpx::ranges::rotate(policy, c, iterator(mid));

    base_iterator mid1 = std::begin(d1);
    std::advance(mid1, mid_pos);
    std::rotate(std::begin(d1), mid1, std::end(d1));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()),
        std::begin(d1), [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d1.size());
}

template <typename IteratorTag>
void test_rotate()
{
    using namespace hpx::execution;

    test_rotate_sent(IteratorTag());
    test_rotate_sent(seq, IteratorTag());
    test_rotate_sent(par, IteratorTag());
    test_rotate_sent(par_unseq, IteratorTag());

    test_rotate(IteratorTag());
    test_rotate(seq, IteratorTag());
    test_rotate(par, IteratorTag());
    test_rotate(par_unseq, IteratorTag());

    test_rotate_sentinel(IteratorTag());
    test_rotate_sentinel(seq, IteratorTag());
    test_rotate_sentinel(par, IteratorTag());
    test_rotate_sentinel(par_unseq, IteratorTag());

    test_rotate_async(seq(task), IteratorTag());
    test_rotate_async(par(task), IteratorTag());
}

void rotate_test()
{
    test_rotate<std::random_access_iterator_tag>();
    test_rotate<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_rotate_exception(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    base_iterator mid = std::begin(c);

    // move at least one element to guarantee an exception to be thrown
    std::size_t delta =
        (std::max) (std::rand() % c.size(), std::size_t(2));    //-V104
    std::advance(mid, delta);

    bool caught_exception = false;
    try
    {
        hpx::ranges::rotate(hpx::util::iterator_range(
                                decorated_iterator(std::begin(c),
                                    []() { throw std::runtime_error("test"); }),
                                decorated_iterator(std::end(c))),
            decorated_iterator(mid));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
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
void test_rotate_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    base_iterator mid = std::begin(c);

    // move at least one element to guarantee an exception to be thrown
    std::size_t delta =
        (std::max) (std::rand() % c.size(), std::size_t(2));    //-V104
    std::advance(mid, delta);

    bool caught_exception = false;
    try
    {
        hpx::ranges::rotate(policy,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            decorated_iterator(mid));
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
void test_rotate_exception_async(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    base_iterator mid = std::begin(c);

    // move at least one element to guarantee an exception to be thrown
    std::size_t delta =
        (std::max) (std::rand() % c.size(), std::size_t(2));    //-V104
    std::advance(mid, delta);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::rotate(p,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            decorated_iterator(mid));
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
void test_rotate_exception()
{
    using namespace hpx::execution;

    test_rotate_exception(IteratorTag());
    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_rotate_exception(seq, IteratorTag());
    test_rotate_exception(par, IteratorTag());

    test_rotate_exception_async(seq(task), IteratorTag());
    test_rotate_exception_async(par(task), IteratorTag());
}

void rotate_exception_test()
{
    test_rotate_exception<std::random_access_iterator_tag>();
    test_rotate_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_rotate_bad_alloc(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    base_iterator mid = std::begin(c);

    // move at least one element to guarantee an exception to be thrown
    std::size_t delta =
        (std::max) (std::rand() % c.size(), std::size_t(2));    //-V104
    std::advance(mid, delta);

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::rotate(
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            decorated_iterator(mid));
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
void test_rotate_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    base_iterator mid = std::begin(c);

    // move at least one element to guarantee an exception to be thrown
    std::size_t delta =
        (std::max) (std::rand() % c.size(), std::size_t(2));    //-V104
    std::advance(mid, delta);

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::rotate(policy,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            decorated_iterator(mid));
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
void test_rotate_bad_alloc_async(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    base_iterator mid = std::begin(c);

    // move at least one element to guarantee an exception to be thrown
    std::size_t delta =
        (std::max) (std::rand() % c.size(), std::size_t(2));    //-V104
    std::advance(mid, delta);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::rotate(p,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            decorated_iterator(mid));
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
void test_rotate_bad_alloc()
{
    using namespace hpx::execution;
    test_rotate_bad_alloc(IteratorTag());
    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_rotate_bad_alloc(seq, IteratorTag());
    test_rotate_bad_alloc(par, IteratorTag());

    test_rotate_bad_alloc_async(seq(task), IteratorTag());
    test_rotate_bad_alloc_async(par(task), IteratorTag());
}

void rotate_bad_alloc_test()
{
    test_rotate_bad_alloc<std::random_access_iterator_tag>();
    test_rotate_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    rotate_test();
    rotate_exception_test();
    rotate_bad_alloc_test();
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
