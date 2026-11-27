//  Copyright (c) 2017 Jeff Trull
//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
auto seed = std::random_device{}();
std::mt19937 _rand(seed);

struct random_fill
{
    random_fill() = default;
    random_fill(int rand_base, int range)
      : gen(_rand())
      , base(static_cast<unsigned int>(rand_base))
      , dist(-(range / 2), range / 2)
    {
    }

    int operator()()
    {
        // add a random offset to rand_base modulo 2^32: rand_base +/- the
        // range may not fit into an int, which would make the range of the
        // distribution invalid
        return static_cast<int>(base + static_cast<unsigned int>(dist(gen)));
    }

    std::mt19937 gen;
    unsigned int base = 0;
    std::uniform_int_distribution<> dist;
};

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename DataType>
void test_merge_stable(ExPolicy policy, DataType, int rand_base)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef typename std::pair<DataType, int> ElemType;

    using hpx::get;

    std::size_t const size1 = 1000007, size2 = 960202;
    std::vector<ElemType> src1(size1), src2(size2), dest(size1 + size2);

    int no = 0;
    auto rf = random_fill(rand_base, 6);
    std::generate(std::begin(src1), std::end(src1),
        [&no, &rf]() -> std::pair<int, int> { return {rf(), no++}; });
    rf = random_fill(rand_base, 8);
    std::generate(std::begin(src2), std::end(src2),
        [&no, &rf]() -> std::pair<int, int> { return {rf(), no++}; });
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    hpx::ranges::merge(
        policy, std::begin(src1), std::end(src1), std::begin(src2),
        std::end(src2), std::begin(dest),
        [](DataType const& a, DataType const& b) -> bool { return a < b; },
        [](ElemType const& elem) -> DataType const& {
            // This is projection.
            return elem.first;
        },
        [](ElemType const& elem) -> DataType const& {
            // This is projection.
            return elem.first;
        });

    bool stable = true;
    int check_count = 0;
    for (auto i = 1u; i < size1 + size2; ++i)
    {
        if (dest[i - 1].first == dest[i].first)
        {
            ++check_count;
            if (dest[i - 1].second > dest[i].second)
                stable = false;
        }
    }

    bool test_is_meaningful = check_count >= 100;

    HPX_TEST(test_is_meaningful);
    HPX_TEST(stable);
}

int hpx_main()
{
    using ElemType = std::tuple<int, char>;

    // these two vectors are sorted by the first value of each tuple
    std::vector<ElemType> a1{std::make_tuple(1, 'a'), std::make_tuple(2, 'b'),
        std::make_tuple(3, 'a'), std::make_tuple(3, 'b'),
        std::make_tuple(4, 'a'), std::make_tuple(5, 'a'),
        std::make_tuple(5, 'b')};
    std::vector<ElemType> a2{std::make_tuple(0, 'c'), std::make_tuple(3, 'c'),
        std::make_tuple(4, 'c'), std::make_tuple(5, 'c')};

    std::vector<ElemType> result(a1.size() + a2.size());
    std::vector<ElemType> solution(a1.size() + a2.size());

    // I expect a stable merge to order {3, 'a'} and {3, 'b'} before {3, 'c'}
    // because they come from the first sequence
    hpx::ranges::merge(hpx::execution::par, a1.begin(), a1.end(), a2.begin(),
        a2.end(), result.begin(), [](ElemType const& a, ElemType const& b) {
            return std::get<0>(a) < std::get<0>(b);
        });
    std::merge(a1.begin(), a1.end(), a2.begin(), a2.end(), solution.begin(),
        [](ElemType const& a, ElemType const& b) {
            return std::get<0>(a) < std::get<0>(b);
        });

    HPX_TEST(result == solution);

    // Expect {3, 'c'}, {3, 'a'}, {3, 'b'} in order.
    hpx::ranges::merge(hpx::execution::par, a2.begin(), a2.end(), a1.begin(),
        a1.end(), result.begin(), [](ElemType const& a, ElemType const& b) {
            return std::get<0>(a) < std::get<0>(b);
        });
    std::merge(a2.begin(), a2.end(), a1.begin(), a1.end(), solution.begin(),
        [](ElemType const& a, ElemType const& b) {
            return std::get<0>(a) < std::get<0>(b);
        });

    HPX_TEST(result == solution);

    // Do modulus operation for avoiding overflow in ramdom_fill. (#2954)
    std::uniform_int_distribution<> dis(0, 9999);
    int rand_base = dis(_rand);

    using namespace hpx::execution;

    test_merge_stable(seq, int(), rand_base);
    test_merge_stable(par, int(), rand_base);
    test_merge_stable(par_unseq, int(), rand_base);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // By default this should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=1"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
