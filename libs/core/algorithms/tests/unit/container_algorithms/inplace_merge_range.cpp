//  Copyright (c) 2017-2018 Taeguk Kwon
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
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 _gen(seed);

////////////////////////////////////////////////////////////////////////////
struct user_defined_type
{
    user_defined_type() = default;
    user_defined_type(int rand_no)
      : val(rand_no)
      , name(name_list[_gen() % name_list.size()])
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

    bool operator>(user_defined_type const& t) const
    {
        if (this->name > t.name)
            return true;
        else if (this->name < t.name)
            return false;
        else
            return this->val > t.val;
    }

    bool operator==(user_defined_type const& t) const
    {
        return this->name == t.name && this->val == t.val;
    }

    user_defined_type operator+(int val) const
    {
        user_defined_type t(*this);
        t.val += val;
        return t;
    }

    static std::vector<std::string> const name_list;

    int val;
    std::string name;
};

std::vector<std::string> const user_defined_type::name_list{
    "ABB", "ABC", "ACB", "BASE", "CAA", "CAAA", "CAAB"};

struct random_fill
{
    random_fill(std::size_t rand_base, std::size_t range)
      : gen(std::rand())
      , dist(static_cast<int>(rand_base - range / 2),
            static_cast<int>(rand_base + range / 2))
    {
    }

    int operator()()
    {
        return dist(gen);
    }

    std::mt19937 gen;
    std::uniform_int_distribution<> dist;
};

////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename DataType>
void test_inplace_merge(ExPolicy policy, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol;

    auto res_first = std::begin(res);
    auto res_middle = res_first + left_size;
    auto res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(0, 6));
    std::generate(res_middle, res_last, random_fill(0, 8));
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    sol = res;
    auto sol_first = std::begin(sol);
    auto sol_middle = sol_first + left_size;
    auto sol_last = std::end(sol);

    auto result =
        hpx::ranges::inplace_merge(policy, res, res_middle, std::less<>());
    std::inplace_merge(sol_first, sol_middle, sol_last);

    HPX_TEST(result == res_last);

    bool equality = test::equal(res_first, res_last, sol_first, sol_last);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename DataType>
void test_inplace_merge_async(ExPolicy policy, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol;

    auto res_first = std::begin(res);
    auto res_middle = res_first + left_size;
    auto res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(0, 6));
    std::generate(res_middle, res_last, random_fill(0, 8));
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    sol = res;
    auto sol_first = std::begin(sol);
    auto sol_middle = sol_first + left_size;
    auto sol_last = std::end(sol);

    auto f = hpx::ranges::inplace_merge(policy, res, res_middle, std::less<>());
    auto result = f.get();
    std::inplace_merge(sol_first, sol_middle, sol_last);

    HPX_TEST(result == res_last);

    bool equality =
        test::equal(res_first, res_last, std::begin(sol), std::end(sol));

    HPX_TEST(equality);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_inplace_merge_stable(
    ExPolicy&& policy, IteratorTag, DataType, unsigned int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::pair<DataType, int> ElemType;
    typedef typename std::vector<ElemType>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<ElemType> res(left_size + right_size);

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    int no = 0;
    auto rf = random_fill(rand_base, 6);
    std::generate(res_first, res_middle,
        [&no, &rf]() -> std::pair<int, int> { return {rf(), no++}; });
    rf = random_fill(rand_base, 8);
    std::generate(res_middle, res_last,
        [&no, &rf]() -> std::pair<int, int> { return {rf(), no++}; });
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    hpx::ranges::inplace_merge(
        policy, iterator(res_first), iterator(res_middle), iterator(res_last),
        [](DataType const& a, DataType const& b) -> bool { return a < b; },
        [](ElemType const& elem) -> DataType const& {
            // This is projection.
            return elem.first;
        });

    bool stable = true;
    int check_count = 0;
    for (std::size_t i = 1u; i < left_size + right_size; ++i)
    {
        if (res[i - 1].first == res[i].first)
        {
            ++check_count;
            if (res[i - 1].second > res[i].second)
                stable = false;
        }
    }

    bool test_is_meaningful = check_count >= 100;

    HPX_TEST(test_is_meaningful);
    HPX_TEST(stable);
}

///////////////////////////////////////////////////////////////////////////////
template <typename DataType>
void test_inplace_merge()
{
    using namespace hpx::execution;

    test_inplace_merge(seq, DataType());
    test_inplace_merge(par, DataType());
    test_inplace_merge(par_unseq, DataType());

    test_inplace_merge_async(seq(task), DataType());
    test_inplace_merge_async(par(task), DataType());
}

void test_inplace_merge()
{
    test_inplace_merge<int>();
    test_inplace_merge<user_defined_type>();
}

template <typename IteratorTag, typename DataType>
void test_inplace_merge_stable()
{
    ////////// Test cases for checking whether the algorithm is stable.
    using namespace hpx::execution;

    unsigned int rand_base = _gen();

    ////////// Test cases for checking whether the algorithm is stable.
    test_inplace_merge_stable(seq, IteratorTag(), int(), rand_base);
    test_inplace_merge_stable(par, IteratorTag(), int(), rand_base);
    test_inplace_merge_stable(par_unseq, IteratorTag(), int(), rand_base);
    test_inplace_merge_stable(
        seq, IteratorTag(), user_defined_type(), rand_base);
    test_inplace_merge_stable(
        par, IteratorTag(), user_defined_type(), rand_base);
    test_inplace_merge_stable(
        par_unseq, IteratorTag(), user_defined_type(), rand_base);
}

void test_inplace_merge_stable()
{
    test_inplace_merge_stable<std::random_access_iterator_tag, int>();
    test_inplace_merge_stable<std::random_access_iterator_tag,
        user_defined_type>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType>
void test_inplace_merge_etc(IteratorTag, DataType, unsigned int rand_base)
{
    typedef typename std::vector<DataType>::iterator base_iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol, org;

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(rand_base, 6));
    std::generate(res_middle, res_last, random_fill(rand_base, 8));
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    org = sol = res;
    base_iterator sol_first = std::begin(sol);
    base_iterator sol_middle = sol_first + left_size;
    base_iterator sol_last = std::end(sol);

    // Test projection.
    {
        typedef test::test_iterator<base_iterator, IteratorTag> iterator;

        sol = res = org;

        DataType val;
        hpx::ranges::inplace_merge(
            iterator(res_first), iterator(res_middle), iterator(res_last),
            [](DataType const& a, DataType const& b) -> bool { return a < b; },
            [&val](DataType const&) -> DataType& {
                // This is projection.
                return val;
            });

        // The container must not be changed.
        bool equality = test::equal(res_first, res_last, sol_first, sol_last);

        HPX_TEST(equality);
    }
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_inplace_merge_etc(
    ExPolicy&& policy, IteratorTag, DataType, unsigned int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::vector<DataType>::iterator base_iterator;

    std::size_t const left_size = 300007, right_size = 123456;
    std::vector<DataType> res(left_size + right_size), sol, org;

    base_iterator res_first = std::begin(res);
    base_iterator res_middle = res_first + left_size;
    base_iterator res_last = std::end(res);

    std::generate(res_first, res_middle, random_fill(rand_base, 6));
    std::generate(res_middle, res_last, random_fill(rand_base, 8));
    std::sort(res_first, res_middle);
    std::sort(res_middle, res_last);

    org = sol = res;
    base_iterator sol_first = std::begin(sol);
    base_iterator sol_middle = sol_first + left_size;
    base_iterator sol_last = std::end(sol);

    // Test projection.
    {
        typedef test::test_iterator<base_iterator, IteratorTag> iterator;

        sol = res = org;

        DataType val;
        hpx::ranges::inplace_merge(
            policy, iterator(res_first), iterator(res_middle),
            iterator(res_last),
            [](DataType const& a, DataType const& b) -> bool { return a < b; },
            [&val](DataType const&) -> DataType& {
                // This is projection.
                return val;
            });

        // The container must not be changed.
        bool equality = test::equal(res_first, res_last, sol_first, sol_last);

        HPX_TEST(equality);
    }
}

template <typename IteratorTag, typename DataType>
void test_inplace_merge_etc()
{
    using namespace hpx::execution;

    unsigned int rand_base = _gen();

    ////////// Another test cases for justifying the implementation.
    test_inplace_merge_etc(IteratorTag(), DataType(), rand_base);
    test_inplace_merge_etc(seq, IteratorTag(), DataType(), rand_base);
    test_inplace_merge_etc(par, IteratorTag(), DataType(), rand_base);
    test_inplace_merge_etc(par_unseq, IteratorTag(), DataType(), rand_base);
}

void test_inplace_merge_etc()
{
    test_inplace_merge_stable<std::random_access_iterator_tag, int>();
    test_inplace_merge_stable<std::random_access_iterator_tag,
        user_defined_type>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
        _gen.seed(seed);
    }
    std::cout << "using seed: " << seed << std::endl;

    test_inplace_merge();
    test_inplace_merge_etc();
    test_inplace_merge_stable();

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
