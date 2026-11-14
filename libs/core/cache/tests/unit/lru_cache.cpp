//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/cache.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <utility>

using cache_type = hpx::util::cache::lru_cache<int, int>;

///////////////////////////////////////////////////////////////////////////////
void test_insert_evicts_least_recently_used()
{
    cache_type c(3);

    for (int i = 0; i != 3; ++i)
    {
        HPX_TEST(c.insert(i, i));
    }
    HPX_TEST(!c.insert(0, 10));    // already in the cache

    // touch key 0, key 1 is the least recently used one now
    int value = -1;
    HPX_TEST(c.get_entry(0, value));
    HPX_TEST_EQ(value, 0);

    HPX_TEST(c.insert(3, 3));
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(3));
    HPX_TEST(c.holds_key(0));
    HPX_TEST(!c.holds_key(1));
    HPX_TEST(c.holds_key(2));
    HPX_TEST(c.holds_key(3));
}

///////////////////////////////////////////////////////////////////////////////
void test_erase_keeps_size()
{
    cache_type c(3);

    for (int i = 0; i != 3; ++i)
    {
        HPX_TEST(c.insert(i, i));
    }

    // erasing an entry frees its space
    auto const is_key_1 = [](std::pair<int, int> const& p) {
        return p.first == 1;
    };
    HPX_TEST_EQ(c.erase(is_key_1), static_cast<std::size_t>(1));
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(2));

    // so the next insertion does not evict anything
    HPX_TEST(c.insert(3, 3));
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(3));
    HPX_TEST(c.holds_key(0));
    HPX_TEST(c.holds_key(2));
    HPX_TEST(c.holds_key(3));

    // erasing all entries, then shrinking the cache, leaves it empty
    auto const all = [](std::pair<int, int> const&) { return true; };
    HPX_TEST_EQ(c.erase(all), static_cast<std::size_t>(3));
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(0));
    c.reserve(1);
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(0));

    // the full capacity is available again
    c.reserve(3);
    for (int i = 0; i != 3; ++i)
    {
        HPX_TEST(c.insert(i, i));
    }
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(3));
    HPX_TEST(c.holds_key(0));
    HPX_TEST(c.holds_key(1));
    HPX_TEST(c.holds_key(2));
}

///////////////////////////////////////////////////////////////////////////////
void test_unlimited_size()
{
    // a maximum size of zero means no size limitation
    cache_type c;
    HPX_TEST_EQ(c.capacity(), static_cast<std::size_t>(0));

    for (int i = 0; i != 100; ++i)
    {
        HPX_TEST(c.insert(i, i));
    }
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(100));

    // limiting the size evicts the least recently used entries
    c.reserve(10);
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(10));
    HPX_TEST(!c.holds_key(89));
    HPX_TEST(c.holds_key(90));
    HPX_TEST(c.holds_key(99));

    // removing the limit again does not evict anything
    c.reserve(0);
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(10));
    HPX_TEST(c.insert(100, 100));
    HPX_TEST_EQ(c.size(), static_cast<std::size_t>(11));
}

///////////////////////////////////////////////////////////////////////////////
void test_update_if()
{
    cache_type c(3);
    HPX_TEST(c.insert(0, 0));

    // the update goes ahead if the callable returns true
    HPX_TEST(c.update_if(0, 10, [](int, int) { return true; }));
    int value = -1;
    HPX_TEST(c.get_entry(0, value));
    HPX_TEST_EQ(value, 10);

    // the update does not succeed if the callable returns false
    HPX_TEST(!c.update_if(0, 20, [](int, int) { return false; }));
    HPX_TEST(c.get_entry(0, value));
    HPX_TEST_EQ(value, 10);

    // a missing entry is inserted without asking the callable
    HPX_TEST(c.update_if(1, 1, [](int, int) { return false; }));
    HPX_TEST(c.get_entry(1, value));
    HPX_TEST_EQ(value, 1);
}

///////////////////////////////////////////////////////////////////////////////
void test_get_entry()
{
    cache_type c(3);
    HPX_TEST(c.insert(5, 50));

    int key = -1;
    int value = -1;
    HPX_TEST(c.get_entry(5, key, value));
    HPX_TEST_EQ(key, 5);
    HPX_TEST_EQ(value, 50);

    value = -1;
    HPX_TEST(c.get_entry(5, value));
    HPX_TEST_EQ(value, 50);

    HPX_TEST(!c.get_entry(6, value));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_insert_evicts_least_recently_used();
    test_erase_keeps_size();
    test_unlimited_size();
    test_update_if();
    test_get_entry();

    return hpx::util::report_errors();
}
