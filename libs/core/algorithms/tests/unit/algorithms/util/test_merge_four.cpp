//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstdint>
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

void test1()
{
    typedef typename std::vector<std::uint64_t>::iterator iter_t;
    typedef range<iter_t> range_t;
    typedef std::less<std::uint64_t> compare;

    std::vector<std::uint64_t> A, B, C, D, X;
    range_t R[4];
    compare comp;
    range_t RA(A.begin(), A.end());
    range_t RB(B.begin(), B.end());
    range_t RC(C.begin(), C.end());
    range_t RD(D.begin(), D.end());
    range_t RX(X.begin(), X.end());

    // 0 ranges
    RX = full_merge4(RX, R, 0, comp);
    HPX_TEST(RX.size() == 0);

    // 4 empty ranges
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    HPX_TEST(
        RA.size() == 0 && RB.size() == 0 && RC.size() == 0 && RD.size() == 0);
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 0);

    // 1 range filled && 3 empty
    X.resize(10, 0);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(i);
    RD = range_t(D.begin(), D.end());
    RX = range_t(X.begin(), X.end());
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    HPX_TEST(RD.size() == 10);
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 10);
    for (std::uint32_t i = 0; i < X.size(); ++i)
        HPX_TEST(X[i] == i);

    // Two ranges
    D.clear();
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i * 2 + 1);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(static_cast<std::uint64_t>(i * 2));
    X.resize(20, 0);
    RA = range_t(A.begin(), A.end());
    RD = range_t(D.begin(), D.end());
    RX = range_t(X.begin(), X.end());

    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    HPX_TEST(RA.size() == 10 && RD.size() == 10);
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 20);
    for (std::uint32_t i = 0; i < X.size(); ++i)
        HPX_TEST(X[i] == i);

    // Three ranges
    A.clear();
    D.clear();
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i * 3 + 2);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i * 3 + 1);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(static_cast<std::uint64_t>(i * 3));
    X.resize(30, 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range_t(X.begin(), X.end());
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 30);
    for (std::uint32_t i = 0; i < X.size(); ++i)
        HPX_TEST(X[i] == i);

    // Four ranges
    A.clear();
    B.clear();
    C.clear();
    D.clear();
    X.clear();
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i * 4 + 3);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i * 4 + 2);
    for (std::uint32_t i = 0; i < 10; ++i)
        C.push_back(i * 4 + 1);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(static_cast<std::uint64_t>(i * 4));

    X.resize(40, 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range_t(X.begin(), X.end());
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < X.size(); ++i)
        HPX_TEST(X[i] == i);

    // Four ranges sorted
    A.clear();
    B.clear();
    C.clear();
    D.clear();
    X.clear();
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i + 10);
    for (std::uint32_t i = 0; i < 10; ++i)
        C.push_back(i + 20);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(i + 30);

    X.resize(40, 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range_t(X.begin(), X.end());
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < X.size(); ++i)
        HPX_TEST(X[i] == i);

    // Four ranges sorted
    A.clear();
    B.clear();
    C.clear();
    D.clear();
    X.clear();
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i + 30);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i + 20);
    for (std::uint32_t i = 0; i < 10; ++i)
        C.push_back(i + 10);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(i);

    X.resize(40, 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range_t(X.begin(), X.end());
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < X.size(); ++i)
        HPX_TEST(X[i] == i);
}

void test2()
{
    typedef typename std::vector<xk>::iterator iter_t;
    typedef range<iter_t> range_t;
    typedef std::less<xk> compare;

    std::vector<xk> A, B, C, D, X;
    range_t R[4];
    compare comp;

    for (std::uint32_t i = 0; i < 10; ++i)
    {
        A.emplace_back(i, 0);
        B.emplace_back(i, 1);
        C.emplace_back(i, 2);
        D.emplace_back(i, 3);
    }

    X.resize(40);
    range_t RA(A.begin(), A.end());
    range_t RB(B.begin(), B.end());
    range_t RC(C.begin(), C.end());
    range_t RD(D.begin(), D.end());
    range_t RX(X.begin(), X.end());

    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < X.size(); ++i)
        HPX_TEST(X[i].num == i / 4 && X[i].tail == i % 4);
}

void test3()
{
    typedef typename std::vector<std::uint64_t>::iterator iter_t;
    typedef range<iter_t> range_t;
    typedef std::less<std::uint64_t> compare;

    std::uint64_t X[40];
    for (int i = 0; i < 40; ++i)
        X[i] = 0;
    std::vector<std::uint64_t> A, B, C, D;
    range_t R[4];
    compare comp;
    range_t RA(A.begin(), A.end());
    range_t RB(B.begin(), B.end());
    range_t RC(C.begin(), C.end());
    range_t RD(D.begin(), D.end());
    range<std::uint64_t*> RX(&X[0], &X[39]);

    // 0 ranges
    RX = uninit_full_merge4(RX, R, 0, comp);
    HPX_TEST(RX.size() == 0);

    // 4 empty ranges
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    HPX_TEST(
        RA.size() == 0 && RB.size() == 0 && RC.size() == 0 && RD.size() == 0);
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 0);

    // 1 range filled && 3 empty
    //X.resize ( 10, 0 );
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(i);
    RD = range_t(D.begin(), D.end());
    RX = range<std::uint64_t*>(&X[0], &X[9]);
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    HPX_TEST(RD.size() == 10);
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 10);
    for (std::uint32_t i = 0; i < RX.size(); ++i)
        HPX_TEST(X[i] == i);

    // Two ranges
    D.clear();
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i * 2 + 1);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(static_cast<std::uint64_t>(i * 2));
    //X.resize ( 20, 0);
    RA = range_t(A.begin(), A.end());
    RD = range_t(D.begin(), D.end());
    RX = range<std::uint64_t*>(&X[0], &X[19]);

    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    HPX_TEST(RA.size() == 10 && RD.size() == 10);
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 20);
    for (std::uint32_t i = 0; i < RX.size(); ++i)
        HPX_TEST(X[i] == i);

    // Three ranges
    A.clear();
    D.clear();
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i * 3 + 2);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i * 3 + 1);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(static_cast<std::uint64_t>(i * 3));
    //X.resize ( 30 , 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range<std::uint64_t*>(&X[0], &X[29]);
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 30);
    for (std::uint32_t i = 0; i < RX.size(); ++i)
        HPX_TEST(X[i] == i);

    // Four ranges
    A.clear();
    B.clear();
    C.clear();
    D.clear();
    //X.clear() ;
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i * 4 + 3);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i * 4 + 2);
    for (std::uint32_t i = 0; i < 10; ++i)
        C.push_back(i * 4 + 1);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(static_cast<std::uint64_t>(i * 4));

    //X.resize ( 40 , 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range<std::uint64_t*>(&X[0], &X[39]);
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < RX.size(); ++i)
        HPX_TEST(X[i] == i);

    // Four ranges sorted
    A.clear();
    B.clear();
    C.clear();
    D.clear();
    //X.clear() ;
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i + 10);
    for (std::uint32_t i = 0; i < 10; ++i)
        C.push_back(i + 20);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(i + 30);

    //X.resize ( 40 , 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range<std::uint64_t*>(&X[0], &X[39]);
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < RX.size(); ++i)
        HPX_TEST(X[i] == i);

    // Four ranges sorted
    A.clear();
    B.clear();
    C.clear();
    D.clear();
    //X.clear() ;
    for (std::uint32_t i = 0; i < 10; ++i)
        A.push_back(i + 30);
    for (std::uint32_t i = 0; i < 10; ++i)
        B.push_back(i + 20);
    for (std::uint32_t i = 0; i < 10; ++i)
        C.push_back(i + 10);
    for (std::uint32_t i = 0; i < 10; ++i)
        D.push_back(i);

    //X.resize ( 40 , 0);
    RA = range_t(A.begin(), A.end());
    RB = range_t(B.begin(), B.end());
    RC = range_t(C.begin(), C.end());
    RD = range_t(D.begin(), D.end());
    RX = range<std::uint64_t*>(&X[0], &X[39]);
    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < RX.size(); ++i)
        HPX_TEST(X[i] == i);
}

void test4()
{
    typedef typename std::vector<xk>::iterator iter_t;
    typedef range<iter_t> range_t;
    typedef std::less<xk> compare;

    xk X[40];
    std::vector<xk> A, B, C, D;
    range_t R[4];
    compare comp;

    for (std::uint32_t i = 0; i < 10; ++i)
    {
        A.emplace_back(i, 0);
        B.emplace_back(i, 1);
        C.emplace_back(i, 2);
        D.emplace_back(i, 3);
    }
    for (int i = 0; i < 40; ++i)
        X[i] = 0;
    range_t RA(A.begin(), A.end());
    range_t RB(B.begin(), B.end());
    range_t RC(C.begin(), C.end());
    range_t RD(D.begin(), D.end());
    range<xk*> RX(&X[0], &X[39]);

    R[0] = RA;
    R[1] = RB;
    R[2] = RC;
    R[3] = RD;
    RX = uninit_full_merge4(RX, R, 4, comp);
    HPX_TEST(RX.size() == 40);
    for (std::uint32_t i = 0; i < RX.size(); ++i)
        HPX_TEST(X[i].num == i / 4 && X[i].tail == i % 4);
}

int main(int, char*[])
{
    test1();
    test2();
    test3();
    test4();

    return hpx::util::report_errors();
}
