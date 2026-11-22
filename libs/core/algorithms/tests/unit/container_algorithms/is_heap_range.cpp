//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
struct user_defined_type
{
    user_defined_type() = default;
    user_defined_type(int rand_no)
      : val(rand_no)
    {
    }

    bool operator<(user_defined_type const& t) const
    {
        if (this->name < t.name)
            return true;
        else if (this->name > t.name)
            return false;
        else
            return this->val < t.val;
    }

    user_defined_type const& operator++()
    {
        static std::vector<std::string> const name_list = {
            "ABB", "ABC", "ACB", "BCA", "CAA", "CAAA", "CAAB"};
        name = name_list[std::rand() % name_list.size()];
        ++val;
        return *this;
    }

    std::string name;
    int val;
};

////////////////////////////////////////////////////////////////////////////
template <typename DataType>
void test_is_heap(DataType)
{
    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size);
    std::iota(std::begin(c), std::end(c), DataType(std::rand()));

    auto heap_end_iter = std::next(std::begin(c), std::rand() % c.size());
    std::make_heap(std::begin(c), heap_end_iter);

    bool result = hpx::ranges::is_heap(c);
    bool solution = std::is_heap(std::begin(c), std::end(c));

    HPX_TEST_EQ(result, solution);
}

template <typename ExPolicy, typename DataType>
void test_is_heap(ExPolicy&& policy, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size);
    std::iota(std::begin(c), std::end(c), DataType(std::rand()));

    auto heap_end_iter = std::next(std::begin(c), std::rand() % c.size());
    std::make_heap(std::begin(c), heap_end_iter);

    bool result = hpx::ranges::is_heap(policy, c);
    bool solution = std::is_heap(std::begin(c), std::end(c));

    HPX_TEST_EQ(result, solution);
}

template <typename ExPolicy, typename DataType>
void test_is_heap_async(ExPolicy&& policy, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size);
    std::iota(std::begin(c), std::end(c), DataType(std::rand()));

    auto heap_end_iter = std::next(std::begin(c), std::rand() % c.size());
    std::make_heap(std::begin(c), heap_end_iter);

    auto f = hpx::ranges::is_heap(policy, c);
    bool result = f.get();
    bool solution = std::is_heap(std::begin(c), std::end(c));

    HPX_TEST_EQ(result, solution);
}

template <typename Sentinel, typename... ExPolicy>
void test_is_heap_sent(ExPolicy&&... policy)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator =
        test::test_iterator<base_iterator, std::random_access_iterator_tag>;

    std::size_t const size = 10007;
    std::vector<int> c(size);
    std::iota(std::begin(c), std::end(c), std::rand());

    // a heap that covers all, part, or none of the range
    for (std::size_t heap_size :
        {size, std::size_t(std::rand()) % size, std::size_t(0)})
    {
        std::make_heap(std::begin(c), std::next(std::begin(c), heap_size));

        bool result = hpx::ranges::is_heap(policy..., iterator(std::begin(c)),
            Sentinel(iterator(std::end(c))));
        HPX_TEST_EQ(result, std::is_heap(std::begin(c), std::end(c)));
    }
}

template <typename DataType>
void test_is_heap()
{
    using namespace hpx::execution;

    test_is_heap(DataType());

    test_is_heap(seq, DataType());
    test_is_heap(par, DataType());
    test_is_heap(par_unseq, DataType());

    test_is_heap_async(seq(task), DataType());
    test_is_heap_async(par(task), DataType());
}

void test_is_heap()
{
    test_is_heap<int>();
    test_is_heap<user_defined_type>();

    using namespace hpx::execution;
    using iterator = test::test_iterator<std::vector<int>::iterator,
        std::random_access_iterator_tag>;
    using sized_sentinel = test::sized_sentinel_from_iterator<iterator>;
    test_is_heap_sent<test::sentinel_from_iterator<iterator>>();
    test_is_heap_sent<sized_sentinel>(seq);
    test_is_heap_sent<sized_sentinel>(par);
    test_is_heap_sent<sized_sentinel>(par_unseq);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test_is_heap();
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
