//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  This code is based on boost::iterators::counting_iterator
// (C) Copyright David Abrahams 2001.

#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iterator_tests.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <list>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename T>
struct signed_assert_nonnegative
{
    static void test(T x)
    {
        HPX_TEST(x >= 0);
    }
};

template <typename T>
struct unsigned_assert_nonnegative
{
    static void test(T) {}
};

template <typename T>
struct assert_nonnegative
  : std::conditional<std::numeric_limits<T>::is_signed,
        signed_assert_nonnegative<T>, unsigned_assert_nonnegative<T>>::type
{
};

// Special tests for RandomAccess CountingIterators.
template <typename CountingIterator, typename Value>
void category_test(CountingIterator start, CountingIterator finish, Value,
    std::random_access_iterator_tag)
{
    using difference_type =
        typename std::iterator_traits<CountingIterator>::difference_type;

    difference_type distance = std::distance(start, finish);

    // Pick a random position internal to the range
    std::uniform_int_distribution<difference_type> dist(0, distance - 1);
    difference_type offset = dist(gen);

    HPX_TEST(offset >= 0);

    CountingIterator internal = start;
    std::advance(internal, offset);

    // Try some binary searches on the range to show that it's ordered
    HPX_TEST(std::binary_search(start, finish, *internal));

    std::pair<CountingIterator, CountingIterator> xy(
        std::equal_range(start, finish, *internal));
    CountingIterator x = xy.first, y = xy.second;

    HPX_TEST(std::distance(x, y) == 1);

// disable warning: unary minus operator applied to unsigned type, result still unsigned
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 4146)
#endif
    // Show that values outside the range can't be found
    HPX_TEST(!std::binary_search(start, std::prev(finish), *finish));
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

    // Do the generic random_access_iterator_test
    using value_type = typename CountingIterator::value_type;

    std::vector<value_type> v;
    for (value_type z = *start; !(z == *finish); ++z)
    {
        v.push_back(z);
    }

    // Note that this test requires a that the first argument is
    // dereferenceable /and/ a valid iterator prior to the first argument
    tests::random_access_iterator_test(
        start, static_cast<int>(v.size()), v.begin());
}

// Special tests for bidirectional CountingIterators
template <typename CountingIterator, typename Value>
void category_test(
    CountingIterator start, Value v1, std::bidirectional_iterator_tag)
{
    Value v2 = v1;
    ++v2;

    // Note that this test requires a that the first argument is
    // dereferenceable /and/ a valid iterator prior to the first argument
    tests::bidirectional_iterator_test(start, v1, v2);
}

template <typename CountingIterator, typename Value>
void category_test(CountingIterator start, CountingIterator finish, Value v1,
    std::forward_iterator_tag)
{
    Value v2 = v1;
    ++v2;
    if (finish != start && finish != std::next(start))
    {
        tests::forward_readable_iterator_test(start, finish, v1, v2);
    }
}

template <typename CountingIterator, typename Value>
void test_aux(CountingIterator start, CountingIterator finish, Value v1)
{
    typedef typename CountingIterator::iterator_category category;

    // If it's a RandomAccessIterator we can do a few delicate tests
    category_test(start, finish, v1, category());

    // Okay, brute force...
    for (CountingIterator p = start; p != finish && std::next(p) != finish; ++p)
    {
        auto c = *p;
        HPX_TEST(++c == *std::next(p));
    }

    //// prove that a reference can be formed to these values
    //using value = typename CountingIterator::value_type;
    //value const* q = &*start;
    //(void) q;    // suppress unused variable warning
}

template <typename Incrementable>
void test(Incrementable start, Incrementable finish)
{
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 4146)
#endif
    test_aux(hpx::util::counting_iterator(start),
        hpx::util::counting_iterator(finish), start);
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
}

template <typename Integer>
void test_integer(Integer* = nullptr)    // default arg works around MSVC bug
{
    Integer start = Integer(0);
    Integer finish = Integer(120);
    test(start, finish);
}

template <typename Integer, typename Category, typename Difference>
void test_integer3(Integer* = nullptr, Category* = nullptr,
    Difference* = nullptr)    // default arg works around MSVC bug
{
    Integer start = Integer(0);
    Integer finish = Integer(120);
    using iterator =
        hpx::util::counting_iterator<Integer, Category, Difference>;
    test_aux(iterator(start), iterator(finish), start);
}

template <typename Container>
void test_container(
    Container* = nullptr)    // default arg works around MSVC bug
{
    std::uniform_int_distribution<unsigned> dis(3, 1673);
    Container c(dis(gen));

    typename Container::iterator const start = c.begin();

    // back off by 1 to leave room for dereferenceable value at the end
    typename Container::iterator finish = start;
    std::advance(finish, c.size() - 1);

    test(start, finish);

    using const_iterator = typename Container::const_iterator;
    test(const_iterator(start), const_iterator(finish));
}

class my_int1
{
public:
    my_int1() = default;
    explicit my_int1(int x)
      : m_int(x)
    {
    }

    my_int1& operator++()
    {
        ++m_int;
        return *this;
    }

    bool operator==(my_int1 const& x) const
    {
        return m_int == x.m_int;
    }

private:
    int m_int;
};

class my_int2
{
public:
    using value_type = int;
    using pointer = void;
    using reference = void;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    my_int2() = default;
    explicit my_int2(int x)
      : m_int(x)
    {
    }

    my_int2& operator++()
    {
        ++m_int;
        return *this;
    }
    my_int2 operator++(int)
    {
        my_int2 tmp = *this;
        ++m_int;
        return tmp;
    }
    my_int2& operator--()
    {
        --m_int;
        return *this;
    }
    my_int2 operator--(int)
    {
        my_int2 tmp = *this;
        --m_int;
        return tmp;
    }
    bool operator==(my_int2 const& x) const
    {
        return m_int == x.m_int;
    }
    int operator*() const
    {
        return m_int;
    }

private:
    int m_int;
};

class my_int3
{
public:
    using value_type = int;
    using pointer = void;
    using reference = void;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    my_int3() = default;
    explicit my_int3(int x)
      : m_int(x)
    {
    }
    my_int3& operator++()
    {
        ++m_int;
        return *this;
    }
    my_int3 operator++(int)
    {
        my_int3 tmp = *this;
        ++m_int;
        return tmp;
    }
    my_int3& operator+=(std::ptrdiff_t n)
    {
        m_int += static_cast<int>(n);
        return *this;
    }
    std::ptrdiff_t operator-(my_int3 const& x) const
    {
        return m_int - x.m_int;
    }
    my_int3& operator--()
    {
        --m_int;
        return *this;
    }
    my_int3 operator--(int)
    {
        my_int3 tmp = *this;
        --m_int;
        return tmp;
    }
    bool operator==(my_int3 const& x) const
    {
        return m_int == x.m_int;
    }
    bool operator!=(my_int3 const& x) const
    {
        return m_int != x.m_int;
    }
    bool operator<(my_int3 const& x) const
    {
        return m_int < x.m_int;
    }
    int operator*() const
    {
        return m_int;
    }

private:
    int m_int;
};

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
        gen.seed(seed);
    }
    std::cout << "using seed: " << seed << std::endl;

    // Test the built-in integer types.
    test_integer<char>();
    test_integer<unsigned char>();
    test_integer<signed char>();
    test_integer<wchar_t>();
    test_integer<short>();
    test_integer<unsigned short>();
    test_integer<int>();
    test_integer<unsigned int>();
    test_integer<long>();
    test_integer<unsigned long>();

    // Test user-defined type.
    test_integer3<my_int1, std::forward_iterator_tag, int>();
    test_integer3<long, std::random_access_iterator_tag, int>();
    test_integer<my_int2>();
    test_integer<my_int3>();

    // Some tests on container iterators, to prove we handle a few different categories
    test_container<std::vector<int>>();
    test_container<std::list<int>>();

    // Also prove that we can handle raw pointers.
    int array[2000];
    test(hpx::util::counting_iterator(array),
        hpx::util::counting_iterator(array + 2000 - 1));

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

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
