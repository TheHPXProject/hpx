//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

using namespace hpx::parallel::util;

// template <typename Iter, typename Sent>
// std::ostream& operator<<(std::ostream& out, range<Iter, Sent> R)
// {
//     out << "[ " << (R.last - R.first) << "] ";
//     if (not R.valid())
//         return out;
//     while (R.first != R.last)
//         out << (*(R.first++)) << " ";
//     out << std::endl;
//     return out;
// }

struct xk
{
    unsigned tail : 3;
    unsigned num : 24;

    xk(unsigned N = 0, unsigned T = 0)
      : tail(T)
      , num(N)
    {
    }

    bool operator<(xk A) const
    {
        return (unsigned) num < (unsigned) A.num;
    }
};

// std::ostream& operator<<(std::ostream& out, xk x)
// {
//     out << "[" << x.num << "-" << x.tail << "] ";
//     return out;
// }

// TEST OF  MERGE_LEVEL4
void test1()
{
    std::uint64_t X[10][10], Y[100];
    range<std::uint64_t*> RY(&Y[0], &Y[100]);

    for (std::uint32_t i = 0; i < 4; ++i)
    {
        for (std::uint32_t k = 0; k < 10; ++k)
            X[i][k] = i + k * 4;
    }
    for (std::uint32_t i = 0; i < 3; ++i)
    {
        for (std::uint32_t k = 0; k < 10; ++k)
            X[i + 4][k] = i + 40 + k * 3;
    }
    for (std::uint32_t i = 0; i < 3; ++i)
    {
        for (std::uint32_t k = 0; k < 10; ++k)
            X[i + 7][k] = i + 70 + k * 3;
    }

    for (std::uint32_t i = 0; i < 100; ++i)
        Y[i] = 1000;
    std::vector<range<std::uint64_t*>> V, Z;
    for (std::uint32_t i = 0; i < 10; ++i)
        V.emplace_back(&X[i][0], &X[i][10]);
    merge_level4(RY, V, Z, std::less<std::uint64_t>());

    for (std::uint32_t i = 0; i < 100; ++i)
        HPX_TEST(Y[i] == i);
    HPX_TEST(Z.size() == 3);
}

void test2()
{
    typedef typename std::vector<xk>::iterator iter_t;
    typedef range<iter_t> rng;
    typedef std::less<xk> compare;

    std::vector<xk> VA, VB;
    VB.resize(90);
    rng RB(VB.begin(), VB.end());

    for (std::uint32_t i = 0; i < 30; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(k, i / 10);
    }
    for (std::uint32_t i = 0; i < 30; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(k + 10, i / 10);
    }
    for (std::uint32_t i = 0; i < 30; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(k + 20, i / 10);
    }

    std::vector<rng> Vin, Vout;
    for (std::uint32_t i = 0; i < 9; ++i)
    {
        Vin.emplace_back(VA.begin() + (static_cast<std::ptrdiff_t>(i * 10)),
            VA.begin() + (static_cast<std::ptrdiff_t>((i + 1) * 10)));
    }
    compare comp;
    merge_level4(RB, Vin, Vout, comp);

    for (std::uint32_t i = 0; i < 90; ++i)
    {
        HPX_TEST(VB[i].num == i / 3 && VB[i].tail == i % 3);
    }
}

// TEST OF  UNINIT_MERGE_LEVEL4
void test3()
{
    std::uint64_t X[10][10], Y[100];
    range<std::uint64_t*> RY(&Y[0], &Y[100]);

    for (std::uint32_t i = 0; i < 4; ++i)
    {
        for (std::uint32_t k = 0; k < 10; ++k)
            X[i][k] = i + k * 4;
    }
    for (std::uint32_t i = 0; i < 3; ++i)
    {
        for (std::uint32_t k = 0; k < 10; ++k)
            X[i + 4][k] = i + 40 + k * 3;
    }
    for (std::uint32_t i = 0; i < 3; ++i)
    {
        for (std::uint32_t k = 0; k < 10; ++k)
            X[i + 7][k] = i + 70 + k * 3;
    }

    for (std::uint32_t i = 0; i < 100; ++i)
        Y[i] = 1000;
    std::vector<range<std::uint64_t*>> V, Z;
    for (std::uint32_t i = 0; i < 10; ++i)
        V.emplace_back(&X[i][0], &X[i][10]);
    uninit_merge_level4(RY, V, Z, std::less<std::uint64_t>());

    for (std::uint32_t i = 0; i < 100; ++i)
        HPX_TEST(Y[i] == i);
    HPX_TEST(Z.size() == 3);
}

void test4()
{
    typedef typename std::vector<xk>::iterator iter_t;
    typedef range<iter_t> rng;
    typedef std::less<xk> compare;

    std::vector<xk> VA;
    xk VB[90];
    range<xk*> RB(&VB[0], &VB[89]);

    for (std::uint32_t i = 0; i < 30; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(k, i / 10);
    }
    for (std::uint32_t i = 0; i < 30; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(k + 10, i / 10);
    }
    for (std::uint32_t i = 0; i < 30; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(k + 20, i / 10);
    }

    std::vector<rng> Vin;
    std::vector<range<xk*>> Vout;
    for (std::uint32_t i = 0; i < 9; ++i)
    {
        Vin.emplace_back(VA.begin() + (static_cast<std::ptrdiff_t>(i * 10)),
            VA.begin() + (static_cast<std::ptrdiff_t>((i + 1) * 10)));
    }
    compare comp;
    uninit_merge_level4(RB, Vin, Vout, comp);

    for (std::uint32_t i = 0; i < 90; ++i)
    {
        HPX_TEST(VB[i].num == i / 3 && VB[i].tail == i % 3);
    }
}

// TEST OF MERGE_VECTOR4
void test5()
{
    typedef typename std::vector<std::uint64_t>::iterator iter_t;
    typedef range<iter_t> rng;
    typedef std::less<std::uint64_t> compare;

    std::vector<std::uint64_t> X, Y;
    compare comp;
    Y.resize(100, 0);

    for (std::uint32_t i = 0; i < 10; ++i)
    {
        for (std::uint32_t k = 0; k < 10; ++k)
        {
            X.push_back(k * 10 + i);
        }
    }
    rng Rin(X.begin(), X.end());
    rng Rout(Y.begin(), Y.end());

    std::vector<rng> Vin, Vout;
    for (std::uint32_t i = 0; i < 10; ++i)
    {
        Vin.emplace_back(X.begin() + (static_cast<std::ptrdiff_t>(i * 10)),
            X.begin() + (static_cast<std::ptrdiff_t>((i + 1) * 10)));
    }
    rng RX(merge_vector4(Rin, Rout, Vin, Vout, comp));
    HPX_TEST(RX.size() == 100);

    for (std::uint32_t i = 0; i < Y.size(); ++i)
    {
        HPX_TEST(Y[i] == i);
    }
}

void test6()
{
    typedef typename std::vector<xk>::iterator iter_t;
    typedef range<iter_t> rng;
    typedef std::less<xk> compare;

    std::vector<xk> VA, VB;
    VB.resize(160);
    compare comp;
    for (std::uint32_t i = 0; i < 80; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(2 * k + 1, i / 10);
    }
    for (std::uint32_t i = 0; i < 80; ++i)
    {
        std::uint32_t k = i % 10;
        VA.emplace_back(2 * k, i / 10);
    }
    std::vector<rng> Vin, Vout;
    for (std::uint32_t i = 0; i < 16; ++i)
    {
        Vin.emplace_back(VA.begin() + (static_cast<std::ptrdiff_t>(i * 10)),
            VA.begin() + (static_cast<std::ptrdiff_t>((i + 1) * 10)));
    }
    rng RA(VA.begin(), VA.end());
    rng RB(VB.begin(), VB.end());

    rng RX(merge_vector4(RA, RB, Vin, Vout, comp));
    HPX_TEST(RX.size() == 160);

    for (std::uint32_t i = 0; i < VB.size(); ++i)
    {
        std::uint32_t K = i / 8;
        HPX_TEST(VB[i].num == K && VB[i].tail == (i % 8));
    }
}

int main(int, char*[])
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();

    return hpx::util::report_errors();
}
