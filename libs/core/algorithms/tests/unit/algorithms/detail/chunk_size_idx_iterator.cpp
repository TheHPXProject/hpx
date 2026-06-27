//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  Copyright David Abrahams 2001-2004.
//  Copyright (c) Jeremy Siek 2001-2003.
//  Copyright (c) Thomas Witt 2002.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/iterator_support/tests/iterator_tests.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <list>
#include <numeric>
#include <utility>
#include <vector>

// define operators for the type returned by the chunk_size_iterator to make
// the tests compile
template <typename Iter>
bool operator==(hpx::tuple<Iter, std::size_t, std::size_t> const& lhs,
    hpx::tuple<Iter, std::size_t> const& rhs)
{
    return hpx::get<0>(lhs) == hpx::get<0>(rhs) &&
        hpx::get<1>(lhs) == hpx::get<1>(rhs) &&
        hpx::get<2>(lhs) == hpx::get<2>(rhs);
}

// Special tests for RandomAccess Iterator.
template <typename Iterator, typename Value>
void category_test(
    Iterator start, Iterator finish, Value, std::random_access_iterator_tag)
{
    using difference_type =
        typename std::iterator_traits<Iterator>::difference_type;

    difference_type distance = std::distance(start, finish);

    // Pick a random position internal to the range
    difference_type offset = (unsigned) rand() % distance;

    HPX_TEST(offset >= 0);

    Iterator internal = start;
    std::advance(internal, offset);

    // Try some binary searches on the range to show that it's ordered
    HPX_TEST(std::binary_search(start, finish, *internal));

    std::pair<Iterator, Iterator> xy(
        std::equal_range(start, finish, *internal));
    Iterator x = xy.first, y = xy.second;

    HPX_TEST(std::distance(x, y) == 1);

    // Show that values outside the range can't be found
    HPX_TEST(!std::binary_search(start, std::prev(finish), *finish));

    // Do the generic random_access_iterator_test
    std::vector<typename Iterator::value_type> v;
    for (auto z = start; z != finish; ++z)
    {
        v.push_back(*z);
    }

    // Note that this test requires a that the first argument is
    // dereferenceable /and/ a valid iterator prior to the first argument
    tests::random_access_iterator_test(
        start, static_cast<int>(v.size()), v.begin());
}

// Special tests for bidirectional Iterators
template <typename Iterator, typename Value>
void category_test(
    Iterator start, Iterator, Value v1, std::bidirectional_iterator_tag)
{
    Iterator next = start;
    Value v2 = *++next;

    // Note that this test requires a that the first argument is
    // dereferenceable /and/ a valid iterator prior to the first argument
    tests::bidirectional_iterator_test(start, v1, v2);
}

template <typename Iterator, typename Value>
void test_aux(
    Iterator start, Iterator finish, Value v1, std::ptrdiff_t chunk_size)
{
    using category = typename Iterator::iterator_category;

    // If it's a RandomAccessIterator we can do a few delicate tests
    category_test(start, finish, v1, category());

    // Okay, brute force...
    for (Iterator p = start; p != finish && std::next(p) != finish; ++p)
    {
        auto c = *p;
        HPX_TEST(hpx::get<0>(c) + chunk_size == hpx::get<0>(*std::next(p)));
    }

    //// prove that a reference can be formed to these values
    //using value = typename Iterator::value_type;
    //value const* q = &*start;
    //(void) q;    // suppress unused variable warning
}

template <typename Iterator>
using chunk_size_idx_iterator =
    hpx::parallel::util::detail::chunk_size_idx_iterator<Iterator>;

template <typename Integer>
void test_integer(
    Integer start, Integer end, std::size_t chunk_size, std::size_t count)
{
    static_assert(std::random_access_iterator<chunk_size_idx_iterator<Integer>>,
        "chunk_size_idx_iterator<Integer> must be random access");

    auto first = chunk_size_idx_iterator<Integer>(start, chunk_size, count);
    auto last = chunk_size_idx_iterator<Integer>(end, chunk_size, count, count);
    test_aux(first, last, start, chunk_size);
}

template <typename Iterator,
    typename Tag = std::iterator_traits<Iterator>::iterator_category>
struct iterator_test;

template <typename Iterator>
struct iterator_test<Iterator, std::random_access_iterator_tag>
{
    constexpr static bool value =
        std::random_access_iterator<chunk_size_idx_iterator<Iterator>>;
};

template <typename Iterator>
struct iterator_test<Iterator, std::bidirectional_iterator_tag>
{
    constexpr static bool value =
        std::bidirectional_iterator<chunk_size_idx_iterator<Iterator>>;
};

template <typename Container>
void test_container(std::size_t chunk_size)
{
    Container c(2 + (unsigned) rand() % 1673);
    std::size_t count = c.size();

    std::iota(c.begin(), c.end(), 0);

    using iterator = typename Container::iterator;

    static_assert(iterator_test<iterator>::value,
        "chunk_size_iterator<iterator> must of the correct category");

    iterator const start = c.begin();

    // back off by 1 to leave room for dereferenceable value at the end
    iterator finish = start;
    std::advance(finish, count - 1);

    auto first =
        chunk_size_idx_iterator<iterator>(start, chunk_size, count - 1);
    auto last = chunk_size_idx_iterator<iterator>(
        finish, chunk_size, count - 1, count - 1);

    using category = typename iterator::iterator_category;
    category_test(first, last, *first, category());
}

int main(int, char*[])
{
    // Test the built-in integer types.
    test_integer<int>(0, 20, 5, 20);
    test_integer<int>(0, 17, 5, 17);
    test_integer<int>(0, 20, 4, 20);
    test_integer<int>(0, 17, 4, 17);

    test_integer<unsigned int>(0, 20, 5, 20);
    test_integer<unsigned int>(0, 17, 5, 17);
    test_integer<unsigned int>(0, 20, 6, 20);
    test_integer<unsigned int>(0, 17, 6, 17);

    test_container<std::vector<int>>(5);
    test_container<std::list<int>>(5);

    test_container<std::vector<unsigned int>>(4);
    test_container<std::list<unsigned int>>(6);

    return hpx::util::report_errors();
}
