//  copyright (c) 2020 Hartmut Kaiser
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
#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

// FIXME: Intel 15 currently can not compile this code. This needs to be fixed. See #1408
#if !(defined(HPX_INTEL_VERSION) && HPX_INTEL_VERSION == 1500)

////////////////////////////////////////////////////////////////////////////
void test_generate_sent()
{
    std::vector<std::size_t> c(200);
    std::iota(std::begin(c), std::end(c), std::rand());

    auto gen = []() { return std::size_t(10); };

    hpx::ranges::generate(
        std::begin(c), sentinel<std::size_t>{*(std::begin(c) + 100)}, gen);

    // verify values
    std::size_t count = 0;
    std::for_each(
        std::begin(c), std::begin(c) + 100, [&count](std::size_t v) -> void {
            HPX_TEST_EQ(v, std::size_t(10));
            ++count;
        });
    HPX_TEST_EQ(count, (size_t) 100);
}

template <typename ExPolicy>
void test_generate_sent(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(200);
    std::iota(std::begin(c), std::end(c), std::rand());

    auto gen = []() { return std::size_t(10); };

    hpx::ranges::generate(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*(std::begin(c) + 100)}),
        gen);

    // verify values
    std::size_t count = 0;
    std::for_each(
        std::begin(c), std::begin(c) + 100, [&count](std::size_t v) -> void {
            HPX_TEST_EQ(v, std::size_t(10));
            ++count;
        });
    HPX_TEST_EQ(count, (size_t) 100);
}

template <typename IteratorTag>
void test_generate(IteratorTag)
{
    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);

    auto gen = []() { return std::size_t(10); };

    hpx::ranges::generate(c, gen);

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c.base()), std::end(c.base()),
        [&count](std::size_t v) -> void {
            HPX_TEST_EQ(v, std::size_t(10));
            ++count;
        });
    HPX_TEST_EQ(count, c.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_generate(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);

    auto gen = []() { return std::size_t(10); };

    hpx::ranges::generate(policy, c, gen);

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c.base()), std::end(c.base()),
        [&count](std::size_t v) -> void {
            HPX_TEST_EQ(v, std::size_t(10));
            ++count;
        });
    HPX_TEST_EQ(count, c.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_generate_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);

    auto gen = []() { return std::size_t(10); };

    hpx::future<iterator> f = hpx::ranges::generate(p, c, gen);
    f.wait();

    std::size_t count = 0;
    std::for_each(std::begin(c.base()), std::end(c.base()),
        [&count](std::size_t v) -> void {
            HPX_TEST_EQ(v, std::size_t(10));
            ++count;
        });
    HPX_TEST_EQ(count, c.size());
}

template <typename IteratorTag>
void test_generate()
{
    using namespace hpx::execution;

    test_generate(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_generate(seq, IteratorTag());
        test_generate(par, IteratorTag());
        test_generate(par_unseq, IteratorTag());

        test_generate_async(seq(task), IteratorTag());
        test_generate_async(par(task), IteratorTag());
    }

    test_generate_sent();
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_generate_sent(seq);
        test_generate_sent(par);
        test_generate_sent(par_unseq);
    }
}

void generate_test()
{
    test_generate<std::random_access_iterator_tag>();
    test_generate<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_generate_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<std::size_t> c(10007);

    auto gen = []() { return std::size_t(10); };

    bool caught_exception = false;
    try
    {
        hpx::ranges::generate(
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            gen);
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
void test_generate_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<std::size_t> c(10007);

    auto gen = []() { return std::size_t(10); };

    bool caught_exception = false;
    try
    {
        hpx::ranges::generate(policy,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            gen);
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
void test_generate_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);

    auto gen = []() { return std::size_t(10); };

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::ranges::generate(p,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            gen);
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
void test_generate_exception()
{
    using namespace hpx::execution;

    test_generate_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_generate_exception(seq, IteratorTag());
        test_generate_exception(par, IteratorTag());

        test_generate_exception_async(seq(task), IteratorTag());
        test_generate_exception_async(par(task), IteratorTag());
    }
}

void generate_exception_test()
{
    test_generate_exception<std::random_access_iterator_tag>();
    test_generate_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_generate_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(100007);

    auto gen = []() { return 10; };

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::generate(policy,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            gen);
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
void test_generate_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);

    auto gen = []() { return std::size_t(10); };

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::ranges::generate(p,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            gen);
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
void test_generate_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_generate_bad_alloc(seq, IteratorTag());
        test_generate_bad_alloc(par, IteratorTag());

        test_generate_bad_alloc_async(seq(task), IteratorTag());
        test_generate_bad_alloc_async(par(task), IteratorTag());
    }
}

void generate_bad_alloc_test()
{
    test_generate_bad_alloc<std::random_access_iterator_tag>();
    test_generate_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    generate_test();
    generate_exception_test();
    generate_bad_alloc_test();
    return hpx::local::finalize();
}
#else
int hpx_main(hpx::program_options::variables_map& vm)
{
    return hpx::local::finalize();
}
#endif

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
