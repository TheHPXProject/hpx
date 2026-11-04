//  Copyright (c) 2023 Johan511

//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

using hpx::parallel::util::unseq_first_n, hpx::parallel::util::unseq2_first_n;

template <typename T>
void test_unseq_first_n1_dispatch2(std::size_t length, std::size_t first_index)
{
    first_index = first_index % length;

    std::vector<T> v(length, static_cast<T>(false));
    std::size_t i = 0;

    std::for_each(v.begin(), v.end(), [&](T& t) {
        if (i == first_index)
            t = 1;
        else if (i > first_index)
            t = gen() % 2;
        else
            t = 0;
        i++;
    });

    auto f = [](T t) { return t; };

    auto iter_test = hpx::parallel::util::unseq_first_n(
        v.begin(), static_cast<T>(length), f);

    auto iter_known = v.begin() + first_index;

    HPX_TEST(iter_test == iter_known);
}

void test_unseq_first_n1_dispatch1()
{
    // length is used as a modulus, so it must not be 0
    test_unseq_first_n1_dispatch2<int>(gen() % 10007 + 1, gen());
}

template <typename T>
void test_unseq_first_n2_dispatch2(std::size_t length, std::size_t first_index)
{
    first_index = first_index % length;
    std::vector<T> v1(length, static_cast<T>(false));
    std::vector<T> v2(length, static_cast<T>(false));

    std::size_t idx = 0;

    while (idx != length)
    {
        if (idx == first_index)
        {
            v1[idx] = 1;
            v2[idx] = 1;
        }
        else if (idx > first_index)
        {
            v1[idx] = gen() % 2;
            v2[idx] = gen() % 2;
        }
        else
        {
            v1[idx] = 0;
            v2[idx] = 0;
        }
        idx++;
    }

    auto f = [](T t1, T t2) { return t1 && t2; };

    auto iter_pair_test = hpx::parallel::util::unseq2_first_n(
        v1.begin(), v2.begin(), static_cast<T>(length), f);

    auto iter_pair_value =
        std::make_pair(v1.begin() + first_index, v2.begin() + first_index);

    HPX_TEST(iter_pair_test == iter_pair_value);
}

void test_unseq_first_n2_dispatch1()
{
    // length is used as a modulus, so it must not be 0
    test_unseq_first_n2_dispatch2<int>(gen() % 10007 + 1, gen());
}

int main(int, char*[])
{
    std::cout << "using seed: " << seed << std::endl;

    test_unseq_first_n1_dispatch1();    //  Predicate takes single argument
    test_unseq_first_n2_dispatch1();    //  Predicate takes two arguments

    return hpx::util::report_errors();
}
