//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

using iterator_range = hpx::util::iterator_range<int*>;
using unsized_iterator_range =
    hpx::util::iterator_range<int*, std::unreachable_sentinel_t>;
using vector_type = std::vector<int>;
using vector_iterator_range =
    hpx::util::iterator_range<std::ranges::iterator_t<vector_type>>;
using span_type = std::span<int>;
using span_iterator_range =
    hpx::util::iterator_range<std::ranges::iterator_t<span_type>>;

struct throwing_range
{
    int* begin() noexcept(false);
    int* end() noexcept(false);
};

template <typename T>
concept has_size = requires(T const& range) { range.size(); };

static_assert(std::ranges::borrowed_range<iterator_range>);
static_assert(std::ranges::sized_range<iterator_range>);
static_assert(std::ranges::borrowed_range<unsized_iterator_range>);
static_assert(!std::ranges::sized_range<unsized_iterator_range>);
static_assert(has_size<iterator_range>);
static_assert(!has_size<unsized_iterator_range>);
static_assert(std::is_constructible_v<vector_iterator_range, vector_type&>);
static_assert(
    !std::is_constructible_v<vector_iterator_range, std::vector<long>&>);
static_assert(!std::is_constructible_v<vector_iterator_range, vector_type>);
static_assert(std::is_constructible_v<span_iterator_range, span_type>);
static_assert(std::is_constructible_v<iterator_range, throwing_range&>);
static_assert(!noexcept(iterator_range(std::declval<throwing_range&>())));

///////////////////////////////////////////////////////////////////////////////
void array_range()
{
    int r[3] = {0, 1, 2};
    HPX_TEST(hpx::util::begin(r) == &r[0]);
    HPX_TEST(hpx::util::end(r) == &r[3]);

    int const cr[3] = {0, 1, 2};
    HPX_TEST(hpx::util::begin(cr) == &cr[0]);
    HPX_TEST(hpx::util::end(cr) == &cr[3]);
    HPX_TEST_EQ(hpx::util::size(cr), 3u);
    HPX_TEST_EQ(hpx::util::empty(cr), false);
}

///////////////////////////////////////////////////////////////////////////////
struct member
{
    int x;

    int* begin()
    {
        return &x;
    }

    int const* begin() const
    {
        return &x;
    }

    int* end()
    {
        return &x + 1;
    }

    int const* end() const
    {
        return &x + 1;
    }
};

void member_range()
{
    member r = member();
    HPX_TEST(hpx::util::begin(r) == &r.x);
    HPX_TEST(hpx::util::end(r) == &r.x + 1);

    member const cr = member();
    HPX_TEST(hpx::util::begin(cr) == &cr.x);
    HPX_TEST(hpx::util::end(cr) == &cr.x + 1);
    HPX_TEST_EQ(hpx::util::size(cr), 1u);
    HPX_TEST_EQ(hpx::util::empty(cr), false);
}

///////////////////////////////////////////////////////////////////////////////
namespace adl {
    struct free
    {
        int x;
    };

    int* begin(free& r)
    {
        return &r.x;
    }

    int const* begin(free const& r)
    {
        return &r.x;
    }

    int* end(free& r)
    {
        return &r.x + 1;
    }

    int const* end(free const& r)
    {
        return &r.x + 1;
    }
}    // namespace adl

void adl_range()
{
    adl::free r = adl::free();
    HPX_TEST(hpx::util::begin(r) == &r.x);
    HPX_TEST(hpx::util::end(r) == &r.x + 1);

    adl::free const cr = adl::free();
    HPX_TEST(hpx::util::begin(cr) == &cr.x);
    HPX_TEST(hpx::util::end(cr) == &cr.x + 1);
    HPX_TEST_EQ(hpx::util::size(cr), 1u);
    HPX_TEST_EQ(hpx::util::empty(cr), false);
}

///////////////////////////////////////////////////////////////////////////////
void vector_range()
{
    std::vector<int> r(3);
    HPX_TEST(hpx::util::begin(r) == r.begin());
    HPX_TEST(hpx::util::end(r) == r.end());

    std::vector<int> cr(3);
    HPX_TEST(hpx::util::begin(cr) == cr.begin());
    HPX_TEST(hpx::util::end(cr) == cr.end());
    HPX_TEST_EQ(hpx::util::size(cr), 3u);
    HPX_TEST_EQ(hpx::util::empty(cr), false);
}

///////////////////////////////////////////////////////////////////////////////
void counting_range()
{
    // a counting_iterator reports a difference_type wide enough to hold the
    // difference of any two of its values, which can be wider than
    // std::size_t, while hpx::util::size() hands out a count
    using iterator = hpx::util::counting_iterator<unsigned int>;
    static_assert(
        sizeof(std::iter_difference_t<iterator>) >= sizeof(unsigned int));

    hpx::util::counting_shape const r(3u);
    static_assert(std::is_same_v<decltype(hpx::util::size(r)), std::size_t>);

    HPX_TEST(hpx::util::begin(r) == iterator(0u));
    HPX_TEST(hpx::util::end(r) == iterator(3u));
    HPX_TEST_EQ(hpx::util::size(r), 3u);
    HPX_TEST_EQ(hpx::util::empty(r), false);

    hpx::util::counting_shape const cr(2u, 7u);
    HPX_TEST_EQ(hpx::util::size(cr), 5u);

    hpx::util::counting_shape const er(0u);
    HPX_TEST_EQ(hpx::util::size(er), 0u);
    HPX_TEST_EQ(hpx::util::empty(er), true);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        array_range();
        member_range();
        adl_range();
        vector_range();
        counting_range();
    }

    return hpx::util::report_errors();
}
