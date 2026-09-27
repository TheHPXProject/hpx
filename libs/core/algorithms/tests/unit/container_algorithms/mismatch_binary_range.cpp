//  Copyright (c) 2014-2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, 10006);

template <typename IteratorTag>
void test_mismatch_binary1(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result =
            hpx::ranges::mismatch(range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)));

        // verify values
        HPX_TEST_EQ(std::size_t(std::distance(begin1, result.in1)), c1.size());
        HPX_TEST_EQ(
            std::size_t(std::distance(std::begin(c2), result.in2)), c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result =
            hpx::ranges::mismatch(range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.in1)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.in2)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::ranges::mismatch(policy,
            range(iterator(begin1), iterator(end1)),
            base_range(std::begin(c2), std::end(c2)));

        // verify values
        HPX_TEST_EQ(std::size_t(std::distance(begin1, result.in1)), c1.size());
        HPX_TEST_EQ(
            std::size_t(std::distance(std::begin(c2), result.in2)), c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::ranges::mismatch(policy,
            range(iterator(begin1), iterator(end1)),
            base_range(std::begin(c2), std::end(c2)));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.in1)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.in2)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto f =
            hpx::ranges::mismatch(p, range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)));
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(std::size_t(std::distance(begin1, result.in1)), c1.size());
        HPX_TEST_EQ(
            std::size_t(std::distance(std::begin(c2), result.in2)), c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto f =
            hpx::ranges::mismatch(p, range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)));
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.in1)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.in2)),
            changed_idx);
    }
}

template <typename IteratorTag>
void test_mismatch_binary1()
{
    using namespace hpx::execution;

    test_mismatch_binary1(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_mismatch_binary1(seq, IteratorTag());
        test_mismatch_binary1(par, IteratorTag());
        test_mismatch_binary1(par_unseq, IteratorTag());

        test_mismatch_binary1_async(seq(task), IteratorTag());
        test_mismatch_binary1_async(par(task), IteratorTag());
    }
}

void mismatch_binary_test1()
{
    test_mismatch_binary1<std::random_access_iterator_tag>();
    test_mismatch_binary1<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_mismatch_binary2(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result =
            hpx::ranges::mismatch(range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)),
                std::equal_to<std::size_t>());

        // verify values
        HPX_TEST_EQ(std::size_t(std::distance(begin1, result.in1)), c1.size());
        HPX_TEST_EQ(
            std::size_t(std::distance(std::begin(c2), result.in2)), c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result =
            hpx::ranges::mismatch(range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)),
                std::equal_to<std::size_t>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.in1)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.in2)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::ranges::mismatch(policy,
            range(iterator(begin1), iterator(end1)),
            base_range(std::begin(c2), std::end(c2)),
            std::equal_to<std::size_t>());

        // verify values
        HPX_TEST_EQ(std::size_t(std::distance(begin1, result.in1)), c1.size());
        HPX_TEST_EQ(
            std::size_t(std::distance(std::begin(c2), result.in2)), c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::ranges::mismatch(policy,
            range(iterator(begin1), iterator(end1)),
            base_range(std::begin(c2), std::end(c2)),
            std::equal_to<std::size_t>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.in1)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.in2)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary2_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto f =
            hpx::ranges::mismatch(p, range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)),
                std::equal_to<std::size_t>());
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(std::size_t(std::distance(begin1, result.in1)), c1.size());
        HPX_TEST_EQ(
            std::size_t(std::distance(std::begin(c2), result.in2)), c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto f =
            hpx::ranges::mismatch(p, range(iterator(begin1), iterator(end1)),
                base_range(std::begin(c2), std::end(c2)),
                std::equal_to<std::size_t>());
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.in1)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.in2)),
            changed_idx);
    }
}

template <typename IteratorTag>
void test_mismatch_binary2()
{
    using namespace hpx::execution;

    test_mismatch_binary2(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_mismatch_binary2(seq, IteratorTag());
        test_mismatch_binary2(par, IteratorTag());
        test_mismatch_binary2(par_unseq, IteratorTag());

        test_mismatch_binary2_async(seq(task), IteratorTag());
        test_mismatch_binary2_async(par(task), IteratorTag());
    }
}

void mismatch_binary_test2()
{
    test_mismatch_binary2<std::random_access_iterator_tag>();
    test_mismatch_binary2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_mismatch_binary_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    try
    {
        hpx::ranges::mismatch(
            range(iterator(std::begin(c1)), iterator(std::end(c1))),
            base_range(std::begin(c2), std::end(c2)),
            [](std::size_t, std::size_t) {
                return throw std::runtime_error("test"), true;
            });

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
void test_mismatch_binary_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    try
    {
        hpx::ranges::mismatch(policy,
            range(iterator(std::begin(c1)), iterator(std::end(c1))),
            base_range(std::begin(c2), std::end(c2)),
            [](std::size_t, std::size_t) {
                return throw std::runtime_error("test"), true;
            });

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
void test_mismatch_binary_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::mismatch(p,
            range(iterator(std::begin(c1)), iterator(std::end(c1))),
            base_range(std::begin(c2), std::end(c2)),
            [](std::size_t, std::size_t) {
                return throw std::runtime_error("test"), true;
            });
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
void test_mismatch_binary_exception()
{
    using namespace hpx::execution;

    test_mismatch_binary_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_mismatch_binary_exception(seq, IteratorTag());
        test_mismatch_binary_exception(par, IteratorTag());

        test_mismatch_binary_exception_async(seq(task), IteratorTag());
        test_mismatch_binary_exception_async(par(task), IteratorTag());
    }
}

void mismatch_binary_exception_test()
{
    test_mismatch_binary_exception<std::random_access_iterator_tag>();
    test_mismatch_binary_exception<std::forward_iterator_tag>();
}

/////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_mismatch_binary_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::mismatch(policy,
            range(iterator(std::begin(c1)), iterator(std::end(c1))),
            base_range(std::begin(c2), std::end(c2)),
            [](std::size_t, std::size_t) {
                return throw std::bad_alloc(), true;
            });

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
void test_mismatch_binary_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef hpx::util::iterator_range<base_iterator> base_range;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    typedef hpx::util::iterator_range<iterator> range;

    std::vector<std::size_t> c1(10007);
    std::vector<std::size_t> c2(c1.size());

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::mismatch(p,
            range(iterator(std::begin(c1)), iterator(std::end(c1))),
            base_range(std::begin(c2), std::end(c2)),
            [](std::size_t, std::size_t) {
                return throw std::bad_alloc(), true;
            });
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
void test_mismatch_binary_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_mismatch_binary_bad_alloc(seq, IteratorTag());
        test_mismatch_binary_bad_alloc(par, IteratorTag());

        test_mismatch_binary_bad_alloc_async(seq(task), IteratorTag());
        test_mismatch_binary_bad_alloc_async(par(task), IteratorTag());
    }
}

void mismatch_binary_bad_alloc_test()
{
    test_mismatch_binary_bad_alloc<std::random_access_iterator_tag>();
    test_mismatch_binary_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    mismatch_binary_test1();
    mismatch_binary_test2();
    mismatch_binary_exception_test();
    mismatch_binary_bad_alloc_test();
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
