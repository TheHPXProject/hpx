//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iterator>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
// Shared fixture helpers to reduce code duplication

std::pair<std::vector<std::size_t>, std::ptrdiff_t> get_search_n1_fixture()
{
    std::vector<std::size_t> c(10007);
    // fill with values above 2 so target value stands out
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // plant 3 consecutive 1s in the middle
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 1;
    c[c.size() / 2 + 2] = 1;
    return {c, static_cast<std::ptrdiff_t>(c.size() / 2)};
}

std::vector<std::size_t> get_search_n2_fixture()
{
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // do NOT plant the target value
    return c;
}

std::vector<std::size_t> get_search_n3_fixture()
{
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    return c;
}

std::pair<std::vector<std::size_t>, std::ptrdiff_t> get_search_n5_fixture()
{
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // plant 3 consecutive 1s at the very end
    c[c.size() - 3] = 1;
    c[c.size() - 2] = 1;
    c[c.size() - 1] = 1;
    return {c, static_cast<std::ptrdiff_t>(c.size() - 3)};
}

std::pair<std::vector<std::size_t>, std::ptrdiff_t> get_search_n6_fixture()
{
    std::vector<std::size_t> c(10007);
    // fill with background value 4
    std::fill(std::begin(c), std::end(c), static_cast<std::size_t>(4));
    // plant 0, 1, 2 in the middle
    c[c.size() / 2] = 0;
    c[c.size() / 2 + 1] = 1;
    c[c.size() / 2 + 2] = 2;
    return {c, static_cast<std::ptrdiff_t>(c.size() / 2)};
}

std::pair<std::vector<std::size_t>, std::ptrdiff_t> get_search_n7_fixture()
{
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // first match at size / 4
    c[c.size() / 4] = 1;
    c[c.size() / 4 + 1] = 1;
    c[c.size() / 4 + 2] = 1;
    // second match at size / 2
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 1;
    c[c.size() / 2 + 2] = 1;
    return {c, static_cast<std::ptrdiff_t>(c.size() / 4)};
}

///////////////////////////////////////////////////////////////////////////////
// test 1: match in the middle of the range
template <typename IteratorTag>
void test_search_n1_without_expolicy(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n1_fixture();
    std::vector<std::size_t>& c = fixture.first;

    iterator index = hpx::search_n(iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n1(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n1_fixture();
    std::vector<std::size_t>& c = fixture.first;

    iterator index = hpx::search_n(policy, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n1_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n1_fixture();
    std::vector<std::size_t>& c = fixture.first;

    hpx::future<iterator> f = hpx::search_n(p, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));
    f.wait();

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(f.get() == iterator(test_index));
}

template <typename IteratorTag>
void test_search_n1()
{
    using namespace hpx::execution;
    test_search_n1_without_expolicy(IteratorTag());

    test_search_n1(seq, IteratorTag());
    test_search_n1(par, IteratorTag());
    test_search_n1(par_unseq, IteratorTag());

    test_search_n1_async(seq(task), IteratorTag());
    test_search_n1_async(par(task), IteratorTag());
}

void search_n_test1()
{
    test_search_n1<std::random_access_iterator_tag>();
    test_search_n1<std::bidirectional_iterator_tag>();
    test_search_n1<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// test 2: value not present - must return last
template <typename IteratorTag>
void test_search_n2_without_expolicy(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = get_search_n2_fixture();

    iterator result = hpx::search_n(iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    HPX_TEST(result == iterator(std::end(c)));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n2(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = get_search_n2_fixture();

    iterator result = hpx::search_n(policy, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    HPX_TEST(result == iterator(std::end(c)));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n2_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = get_search_n2_fixture();

    hpx::future<iterator> f = hpx::search_n(p, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));
    f.wait();

    HPX_TEST(f.get() == iterator(std::end(c)));
}

template <typename IteratorTag>
void test_search_n2()
{
    using namespace hpx::execution;
    test_search_n2_without_expolicy(IteratorTag());

    test_search_n2(seq, IteratorTag());
    test_search_n2(par, IteratorTag());
    test_search_n2(par_unseq, IteratorTag());

    test_search_n2_async(seq(task), IteratorTag());
    test_search_n2_async(par(task), IteratorTag());
}

void search_n_test2()
{
    test_search_n2<std::random_access_iterator_tag>();
    test_search_n2<std::bidirectional_iterator_tag>();
    test_search_n2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// test 3: count == 0 - must return first (C++20 [alg.search] p14)
template <typename IteratorTag>
void test_search_n3_without_expolicy(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = get_search_n3_fixture();

    iterator result = hpx::search_n(iterator(std::begin(c)),
        iterator(std::end(c)), 0, static_cast<std::size_t>(1));

    HPX_TEST(result == iterator(std::begin(c)));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n3(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = get_search_n3_fixture();

    iterator result = hpx::search_n(policy, iterator(std::begin(c)),
        iterator(std::end(c)), 0, static_cast<std::size_t>(1));

    HPX_TEST(result == iterator(std::begin(c)));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n3_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = get_search_n3_fixture();

    hpx::future<iterator> f = hpx::search_n(p, iterator(std::begin(c)),
        iterator(std::end(c)), 0, static_cast<std::size_t>(1));
    f.wait();

    HPX_TEST(f.get() == iterator(std::begin(c)));
}

template <typename IteratorTag>
void test_search_n3()
{
    using namespace hpx::execution;
    test_search_n3_without_expolicy(IteratorTag());

    test_search_n3(seq, IteratorTag());
    test_search_n3(par, IteratorTag());
    test_search_n3(par_unseq, IteratorTag());

    test_search_n3_async(seq(task), IteratorTag());
    test_search_n3_async(par(task), IteratorTag());
}

void search_n_test3()
{
    test_search_n3<std::random_access_iterator_tag>();
    test_search_n3<std::bidirectional_iterator_tag>();
    test_search_n3<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// test 4: count > range length - must return last (C++20 [alg.search] p14)
template <typename IteratorTag>
void test_search_n4_without_expolicy(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // case 4a: all elements match, but count is greater
    {
        std::vector<std::size_t> c(5, static_cast<std::size_t>(1));
        iterator result = hpx::search_n(iterator(std::begin(c)),
            iterator(std::end(c)), 10, static_cast<std::size_t>(1));
        HPX_TEST(result == iterator(std::end(c)));
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n4(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    {
        std::vector<std::size_t> c(5, static_cast<std::size_t>(1));
        iterator result = hpx::search_n(policy, iterator(std::begin(c)),
            iterator(std::end(c)), 10, static_cast<std::size_t>(1));
        HPX_TEST(result == iterator(std::end(c)));
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n4_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    {
        std::vector<std::size_t> c(5, static_cast<std::size_t>(1));
        hpx::future<iterator> f = hpx::search_n(p, iterator(std::begin(c)),
            iterator(std::end(c)), 10, static_cast<std::size_t>(1));
        f.wait();
        HPX_TEST(f.get() == iterator(std::end(c)));
    }
}

template <typename IteratorTag>
void test_search_n4()
{
    using namespace hpx::execution;
    test_search_n4_without_expolicy(IteratorTag());

    test_search_n4(seq, IteratorTag());
    test_search_n4(par, IteratorTag());
    test_search_n4(par_unseq, IteratorTag());

    test_search_n4_async(seq(task), IteratorTag());
    test_search_n4_async(par(task), IteratorTag());
}

void search_n_test4()
{
    test_search_n4<std::random_access_iterator_tag>();
    test_search_n4<std::bidirectional_iterator_tag>();
    test_search_n4<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// test 5: match at the very end of the range (boundary condition)
template <typename IteratorTag>
void test_search_n5_without_expolicy(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n5_fixture();
    std::vector<std::size_t>& c = fixture.first;

    iterator index = hpx::search_n(iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n5(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n5_fixture();
    std::vector<std::size_t>& c = fixture.first;

    iterator index = hpx::search_n(policy, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n5_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n5_fixture();
    std::vector<std::size_t>& c = fixture.first;

    hpx::future<iterator> f = hpx::search_n(p, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));
    f.wait();

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(f.get() == iterator(test_index));
}

template <typename IteratorTag>
void test_search_n5()
{
    using namespace hpx::execution;
    test_search_n5_without_expolicy(IteratorTag());

    test_search_n5(seq, IteratorTag());
    test_search_n5(par, IteratorTag());
    test_search_n5(par_unseq, IteratorTag());

    test_search_n5_async(seq(task), IteratorTag());
    test_search_n5_async(par(task), IteratorTag());
}

void search_n_test5()
{
    test_search_n5<std::random_access_iterator_tag>();
    test_search_n5<std::bidirectional_iterator_tag>();
    test_search_n5<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// test 6: custom binary predicate (non-commutative: a < b)
template <typename IteratorTag>
void test_search_n6_without_expolicy(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n6_fixture();
    std::vector<std::size_t>& c = fixture.first;

    // predicate: order-sensitive. op(a, b) = a < b is invoked as
    // pred(*it, value), so it reads as elem < 3.
    auto op = [](std::size_t a, std::size_t b) { return a < b; };

    // search for 3 elements that are less than 3
    iterator index = hpx::search_n(iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(3), op);

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n6(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n6_fixture();
    std::vector<std::size_t>& c = fixture.first;

    // predicate: order-sensitive. op(a, b) = a < b is invoked as
    // pred(*it, value), so it reads as elem < 3.
    auto op = [](std::size_t a, std::size_t b) { return a < b; };

    iterator index = hpx::search_n(policy, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(3), op);

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n6_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n6_fixture();
    std::vector<std::size_t>& c = fixture.first;

    // predicate: order-sensitive. op(a, b) = a < b is invoked as
    // pred(*it, value), so it reads as elem < 3.
    auto op = [](std::size_t a, std::size_t b) { return a < b; };

    hpx::future<iterator> f = hpx::search_n(p, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(3), op);
    f.wait();

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(f.get() == iterator(test_index));
}

template <typename IteratorTag>
void test_search_n6()
{
    using namespace hpx::execution;
    test_search_n6_without_expolicy(IteratorTag());

    test_search_n6(seq, IteratorTag());
    test_search_n6(par, IteratorTag());
    test_search_n6(par_unseq, IteratorTag());

    test_search_n6_async(seq(task), IteratorTag());
    test_search_n6_async(par(task), IteratorTag());
}

void search_n_test6()
{
    test_search_n6<std::random_access_iterator_tag>();
    test_search_n6<std::bidirectional_iterator_tag>();
    test_search_n6<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// test 7: multiple matching windows - must return the first match
template <typename IteratorTag>
void test_search_n7_without_expolicy(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n7_fixture();
    std::vector<std::size_t>& c = fixture.first;

    iterator index = hpx::search_n(iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n7(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n7_fixture();
    std::vector<std::size_t>& c = fixture.first;

    iterator index = hpx::search_n(policy, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n7_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    auto fixture = get_search_n7_fixture();
    std::vector<std::size_t>& c = fixture.first;

    hpx::future<iterator> f = hpx::search_n(p, iterator(std::begin(c)),
        iterator(std::end(c)), 3, static_cast<std::size_t>(1));
    f.wait();

    base_iterator test_index = std::begin(c) + fixture.second;

    HPX_TEST(f.get() == iterator(test_index));
}

template <typename IteratorTag>
void test_search_n7()
{
    using namespace hpx::execution;
    test_search_n7_without_expolicy(IteratorTag());

    test_search_n7(seq, IteratorTag());
    test_search_n7(par, IteratorTag());
    test_search_n7(par_unseq, IteratorTag());

    test_search_n7_async(seq(task), IteratorTag());
    test_search_n7_async(par(task), IteratorTag());
}

void search_n_test7()
{
    test_search_n7<std::random_access_iterator_tag>();
    test_search_n7<std::bidirectional_iterator_tag>();
    test_search_n7<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// exception tests
template <typename ExPolicy, typename IteratorTag>
void test_search_n_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), static_cast<std::size_t>(2));

    bool caught_exception = false;
    try
    {
        hpx::search_n(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            3, static_cast<std::size_t>(1));
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
void test_search_n_async_exception(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), static_cast<std::size_t>(2));

    bool caught_exception = false;
    try
    {
        hpx::future<decorated_iterator> f = hpx::search_n(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            3, static_cast<std::size_t>(1));
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
}

template <typename IteratorTag>
void test_search_n_exception()
{
    using namespace hpx::execution;
    // vector_execution_policy would call std::terminate - not tested
    test_search_n_exception(seq, IteratorTag());
    test_search_n_exception(par, IteratorTag());

    test_search_n_async_exception(seq(task), IteratorTag());
    test_search_n_async_exception(par(task), IteratorTag());
}

void search_n_exception_test()
{
    test_search_n_exception<std::random_access_iterator_tag>();
    test_search_n_exception<std::bidirectional_iterator_tag>();
    test_search_n_exception<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// bad_alloc tests
template <typename ExPolicy, typename IteratorTag>
void test_search_n_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), static_cast<std::size_t>(2));

    bool caught_bad_alloc = false;
    try
    {
        hpx::search_n(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }),
            3, static_cast<std::size_t>(1));
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
void test_search_n_async_bad_alloc(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), static_cast<std::size_t>(2));

    bool caught_bad_alloc = false;
    try
    {
        hpx::future<decorated_iterator> f = hpx::search_n(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }),
            3, static_cast<std::size_t>(1));

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
}

template <typename IteratorTag>
void test_search_n_bad_alloc()
{
    using namespace hpx::execution;
    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_search_n_bad_alloc(seq, IteratorTag());
    test_search_n_bad_alloc(par, IteratorTag());

    test_search_n_async_bad_alloc(seq(task), IteratorTag());
    test_search_n_async_bad_alloc(par(task), IteratorTag());
}

void search_n_bad_alloc_test()
{
    test_search_n_bad_alloc<std::random_access_iterator_tag>();
    test_search_n_bad_alloc<std::bidirectional_iterator_tag>();
    test_search_n_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    search_n_test1();
    search_n_test2();
    search_n_test3();
    search_n_test4();
    search_n_test5();
    search_n_test6();
    search_n_test7();
    search_n_exception_test();
    search_n_bad_alloc_test();

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
