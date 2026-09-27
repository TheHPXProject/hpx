//  Copyright (c) 2018 Christopher Ogle
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

struct user_defined_type_1
{
    user_defined_type_1() = default;
    user_defined_type_1(int v)
      : val(v)
    {
    }
    unsigned int val;
};

struct user_defined_type_2
{
    user_defined_type_2() = default;
    user_defined_type_2(int v)
      : val(v)
    {
    }
    std::size_t val;
};

////////////////////////////////////////////////////////////////////////////

template <typename IteratorTag>
void test_search1(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    auto index = hpx::ranges::search(c, h);
    auto test_index = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_search1(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    auto index = test::subrange_begin(hpx::ranges::search(policy, c, h));
    auto test_index = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_search1_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    auto f = test::subrange_begin(hpx::ranges::search(p, c, h));
    f.wait();

    // create iterator at position of value to be found
    auto test_index = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_search1()
{
    using namespace hpx::execution;
    test_search1(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_search1(seq, IteratorTag());
        test_search1(par, IteratorTag());
        test_search1(par_unseq, IteratorTag());

        test_search1_async(seq(task), IteratorTag());
        test_search1_async(par(task), IteratorTag());
    }
}

void search_test1()
{
    test_search1<std::random_access_iterator_tag>();
    test_search1<std::forward_iterator_tag>();
}

template <typename ExPolicy, typename IteratorTag>
void test_search2(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1, 2};

    auto index = test::subrange_begin(hpx::ranges::search(policy, c, h));

    auto test_index = std::begin(c);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_search2_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1, 2};

    auto f = test::subrange_begin(hpx::ranges::search(p, c, h));
    f.wait();

    // create iterator at position of value to be found
    auto test_index = std::begin(c);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_search2()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_search2(seq, IteratorTag());
        test_search2(par, IteratorTag());
        test_search2(par_unseq, IteratorTag());

        test_search2_async(seq(task), IteratorTag());
        test_search2_async(par(task), IteratorTag());
    }
}

void search_test2()
{
    test_search2<std::random_access_iterator_tag>();
    test_search2<std::forward_iterator_tag>();
}

template <typename ExPolicy, typename IteratorTag>
void test_search3(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;
    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    auto index = test::subrange_begin(hpx::ranges::search(policy, c, h));

    auto test_index = std::begin(c);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_search3_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    // fill vector with random values above 6
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 7);
    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;
    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    // create only two partitions, splitting the desired sub sequence into
    // separate partitions.
    auto f = test::subrange_begin(hpx::ranges::search(p, c, h));
    f.wait();

    //create iterator at position of value to be found
    auto test_index = std::begin(c);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_search3()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_search3(seq, IteratorTag());
        test_search3(par, IteratorTag());
        test_search3(par_unseq, IteratorTag());

        test_search3_async(seq(task), IteratorTag());
        test_search3_async(par(task), IteratorTag());
    }
}

void search_test3()
{
    test_search3<std::random_access_iterator_tag>();
    test_search3<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_search4_sentinel(IteratorTag)
{
    // Using iter_sent for the iterator and sentinel types
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using sentinel = test::sentinel_from_iterator<iterator>;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::vector<std::size_t> h = {1, 2};

    auto op = [](std::size_t a, std::size_t b) { return !(a != b); };

    iterator index = hpx::ranges::search(iterator(std::begin(c)),
        sentinel(iterator(std::end(c))), iterator(std::begin(h)),
        sentinel(iterator(std::end(h))), op);

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search4_sentinel(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using sentinel = test::sized_sentinel_from_iterator<iterator>;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    //std::size_t h[] = {1, 2};
    std::vector<std::size_t> h = {1, 2};

    auto op = [](std::size_t a, std::size_t b) { return !(a != b); };

    iterator index = test::subrange_begin(hpx::ranges::search(policy,
        iterator(std::begin(c)), sentinel(iterator(std::end(c))),
        iterator(std::begin(h)), sentinel(iterator(std::end(h))), op));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search4(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    auto op = [](std::size_t a, std::size_t b) { return !(a != b); };

    auto index = test::subrange_begin(hpx::ranges::search(policy, c, h, op));

    auto test_index = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_search4_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector, provide custom predicate
    // for search
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    auto op = [](std::size_t a, std::size_t b) { return !(a != b); };

    auto f = test::subrange_begin(hpx::ranges::search(p, c, h, op));
    f.wait();

    // create iterator at position of value to be found
    auto test_index = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_search4()
{
    using namespace hpx::execution;
    test_search4_sentinel(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_search4_sentinel(seq, IteratorTag());
        test_search4_sentinel(par, IteratorTag());
        test_search4_sentinel(par_unseq, IteratorTag());

        test_search4(seq, IteratorTag());
        test_search4(par, IteratorTag());
        test_search4(par_unseq, IteratorTag());

        test_search4_async(seq(task), IteratorTag());
        test_search4_async(par(task), IteratorTag());
    }
}

void search_test4()
{
    test_search4<std::random_access_iterator_tag>();
    test_search4<std::forward_iterator_tag>();
}

template <typename ExPolicy, typename IteratorTag>
void test_search5(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<user_defined_type_1> c(10007);

    // fill vector with random values above 2
    std::for_each(std::begin(c), std::end(c),
        [](user_defined_type_1& ut1) { ut1.val = (std::rand() % 100) + 3; });

    c[c.size() / 2].val = 1;
    c[c.size() / 2 + 1].val = 2;

    user_defined_type_2 h[] = {user_defined_type_2(1), user_defined_type_2(2)};

    auto op = [](std::size_t a, std::size_t b) { return (a == b); };

    //Provide custom projections
    auto proj1 = [](user_defined_type_1 const& ut1) { return ut1.val; };

    auto proj2 = [](user_defined_type_2 const& ut2) { return ut2.val; };

    auto index = test::subrange_begin(
        hpx::ranges::search(policy, c, h, op, proj1, proj2));
    auto test_index = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_search5_async(ExPolicy p, IteratorTag)
{
    std::vector<user_defined_type_1> c(10007);
    // fill vector with random values above 2
    std::for_each(std::begin(c), std::end(c),
        [](user_defined_type_1& ut1) { ut1.val = (std::rand() % 100) + 3; });
    // create subsequence in middle of vector,
    c[c.size() / 2].val = 1;
    c[c.size() / 2 + 1].val = 2;

    user_defined_type_2 h[] = {user_defined_type_2(1), user_defined_type_2(2)};

    auto op = [](std::size_t a, std::size_t b) { return !(a != b); };

    //Provide custom projections
    auto proj1 = [](user_defined_type_1 const& ut1) { return ut1.val; };

    auto proj2 = [](user_defined_type_2 const& ut2) { return ut2.val; };

    auto f =
        test::subrange_begin(hpx::ranges::search(p, c, h, op, proj1, proj2));
    f.wait();

    // create iterator at position of value to be found
    auto test_index = std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_search5()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_search5(seq, IteratorTag());
        test_search5(par, IteratorTag());
        test_search5(par_unseq, IteratorTag());

        test_search5_async(seq(task), IteratorTag());
        test_search5_async(par(task), IteratorTag());
    }
}

void search_test5()
{
    test_search5<std::random_access_iterator_tag>();
    test_search5<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    search_test1();
    search_test2();
    search_test3();
    search_test4();
    search_test5();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
