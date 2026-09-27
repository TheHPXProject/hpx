//  copyright (c) 2014 Grant Mercer
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
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, 10006);
std::uniform_int_distribution<> dist(0, 2);

template <typename IteratorTag>
void test_find_first_of_proj(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    int find_first_of_pos = dis(gen);
    int random_sub_seq_pos = dist(gen);

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), (gen() % 32768) + 19);
    std::size_t h[] = {1 + 65536, 7 + 65536, 18 + 65536, 3 + 65536};
    c[find_first_of_pos] = h[random_sub_seq_pos];    //-V108

    iterator index = hpx::ranges::find_first_of(
        iterator(std::begin(c)), iterator(std::end(c)), std::begin(h),
        std::end(h), std::equal_to<std::size_t>(),
        [](std::size_t x) { return x % 65536; },
        [](std::size_t x) { return x % 65536; });

    base_iterator test_index = std::begin(c) + find_first_of_pos;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find_first_of_proj(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    int find_first_of_pos = dis(gen);
    int random_sub_seq_pos = dist(gen);

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), (gen() % 32768) + 19);
    std::size_t h[] = {1 + 65536, 7 + 65536, 18 + 65536, 3 + 65536};
    c[find_first_of_pos] = h[random_sub_seq_pos];    //-V108

    iterator index = hpx::ranges::find_first_of(
        policy, iterator(std::begin(c)), iterator(std::end(c)), std::begin(h),
        std::end(h), std::equal_to<std::size_t>(),
        [](std::size_t x) { return x % 65536; },
        [](std::size_t x) { return x % 65536; });

    base_iterator test_index = std::begin(c) + find_first_of_pos;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find_first_of_async_proj(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    int find_first_of_pos = dis(gen);
    int random_sub_seq_pos = dist(gen);

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), (gen() % 32768) + 19);
    std::size_t h[] = {1 + 65536, 7 + 65536, 18 + 65536, 3 + 65536};
    c[find_first_of_pos] = h[random_sub_seq_pos];    //-V108

    hpx::future<iterator> f = hpx::ranges::find_first_of(
        p, iterator(std::begin(c)), iterator(std::end(c)), std::begin(h),
        std::end(h), std::equal_to<std::size_t>(),
        [](std::size_t x) { return x % 65536; },
        [](std::size_t x) { return x % 65536; });
    f.wait();

    // create iterator at position of value to be found
    base_iterator test_index = std::begin(c) + find_first_of_pos;

    HPX_TEST(f.get() == iterator(test_index));
}

template <typename IteratorTag>
void test_find_first_of()
{
    using namespace hpx::execution;

    test_find_first_of_proj(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_first_of_proj(seq, IteratorTag());
        test_find_first_of_proj(par, IteratorTag());
        test_find_first_of_proj(par_unseq, IteratorTag());

        test_find_first_of_async_proj(seq(task), IteratorTag());
        test_find_first_of_async_proj(par(task), IteratorTag());
    }
}

void find_first_of_test()
{
    test_find_first_of<std::random_access_iterator_tag>();
    test_find_first_of<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_find_first_of_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    std::size_t h[] = {1, 2};

    bool caught_exception = false;
    try
    {
        hpx::ranges::find_first_of(
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    std::size_t h[] = {1, 2};

    bool caught_exception = false;
    try
    {
        hpx::ranges::find_first_of(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    std::size_t h[] = {1, 2};

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<decorated_iterator> f = hpx::ranges::find_first_of(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_exception()
{
    using namespace hpx::execution;

    test_find_first_of_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_first_of_exception(seq, IteratorTag());
        test_find_first_of_exception(par, IteratorTag());

        test_find_first_of_exception_async(seq(task), IteratorTag());
        test_find_first_of_exception_async(par(task), IteratorTag());
    }
}

void find_first_of_exception_test()
{
    test_find_first_of_exception<std::random_access_iterator_tag>();
    test_find_first_of_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_find_first_of_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(100007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    std::size_t h[] = {1, 2};

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::find_first_of(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    std::size_t h[] = {1, 2};

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<decorated_iterator> f = hpx::ranges::find_first_of(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_first_of_bad_alloc(seq, IteratorTag());
        test_find_first_of_bad_alloc(par, IteratorTag());

        test_find_first_of_bad_alloc_async(seq(task), IteratorTag());
        test_find_first_of_bad_alloc_async(par(task), IteratorTag());
    }
}

void find_first_of_bad_alloc_test()
{
    test_find_first_of_bad_alloc<std::random_access_iterator_tag>();
    test_find_first_of_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    find_first_of_test();
    find_first_of_exception_test();
    find_first_of_bad_alloc_test();
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
