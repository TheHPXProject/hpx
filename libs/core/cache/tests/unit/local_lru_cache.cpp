//  Copyright (c) 2008-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/cache.hpp>
#include <hpx/modules/testing.hpp>

#include <string>

///////////////////////////////////////////////////////////////////////////////
struct data
{
    constexpr data(char const* const k, char const* const v) noexcept
      : key(k)
      , value(v)
    {
    }

    char const* const key;
    char const* const value;
};

data cache_entries[] = {data("white", "255,255,255"),
    data("yellow", "255,255,0"), data("green", "0,255,0"),
    data("blue", "0,0,255"), data("magenta", "255,0,255"),
    data("black", "0,0,0"), data(nullptr, nullptr)};

///////////////////////////////////////////////////////////////////////////////
void test_lru_insert()
{
    using entry_type = hpx::util::cache::entries::lru_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type>;

    cache_type c(3);

    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.capacity());

    // insert all items into the cache
    for (data const* d = &cache_entries[0]; d->key != nullptr; ++d)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
    }

    // there should be 3 items in the cache
    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());
}

///////////////////////////////////////////////////////////////////////////////
void test_lru_insert_with_touch()
{
    using entry_type = hpx::util::cache::entries::lru_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type>;

    cache_type c(3);

    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.capacity());

    // insert 3 items into the cache
    int i = 0;
    data* d = &cache_entries[0];

    for (/**/; i < 3 && d->key != nullptr; ++d, ++i)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
    }

    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());

    // now touch the first item
    std::string white;
    HPX_TEST(c.get_entry("white", white));
    HPX_TEST_EQ(white, "255,255,255");

    // add two more items
    for (i = 0; i < 2 && d->key != nullptr; ++d, ++i)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());
    }

    // there should be 3 items in the cache, and white should be there as well
    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());
    HPX_TEST(c.holds_key("white"));
}

///////////////////////////////////////////////////////////////////////////////
void test_lru_clear()
{
    using entry_type = hpx::util::cache::entries::lru_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type>;

    cache_type c(3);

    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.capacity());

    // insert all items into the cache
    for (data const* d = &cache_entries[0]; d->key != nullptr; ++d)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
    }

    c.clear();

    // there should be no items in the cache
    HPX_TEST_EQ(static_cast<cache_type::size_type>(0), c.size());
}

///////////////////////////////////////////////////////////////////////////////
struct erase_func
{
    explicit erase_func(std::string const& key)
      : key_(key)
    {
    }

    template <typename Entry>
    bool operator()(Entry const& e) const
    {
        return key_ == e.first;
    }

    std::string key_;
};

void test_lru_erase_one()
{
    using entry_type = hpx::util::cache::entries::lru_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type>;

    cache_type c(3);

    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.capacity());

    // insert all items into the cache
    for (data const* d = &cache_entries[0]; d->key != nullptr; ++d)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
    }

    entry_type blue;
    HPX_TEST(c.get_entry("blue", blue));

    c.erase(erase_func("blue"));

    // there should be 2 items in the cache
    HPX_TEST(!c.get_entry("blue", blue));
    HPX_TEST_EQ(static_cast<cache_type::size_type>(2), c.size());
}

///////////////////////////////////////////////////////////////////////////////
void test_lru_update()
{
    using entry_type = hpx::util::cache::entries::lru_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type>;

    cache_type c(4);    // this time we can hold 4 items

    HPX_TEST_EQ(static_cast<cache_type::size_type>(4), c.capacity());

    // insert 3 items into the cache
    int i = 0;
    data* d = &cache_entries[0];

    for (/**/; i < 3 && d->key != nullptr; ++d, ++i)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
    }

    // there should be 3 items in the cache
    HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());

    // now update some items
    HPX_TEST(c.update("black", "255,0,0"));    // isn't in the cache
    HPX_TEST_EQ(static_cast<cache_type::size_type>(4), c.size());

    HPX_TEST(c.update("yellow", "255,0,0"));
    HPX_TEST_EQ(static_cast<cache_type::size_type>(4), c.size());

    std::string yellow;
    HPX_TEST(c.get_entry("yellow", yellow));
    HPX_TEST_EQ(yellow, "255,0,0");
}

///////////////////////////////////////////////////////////////////////////////
void test_lru_reserve()
{
    using entry_type = hpx::util::cache::entries::lru_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type>;

    {
        cache_type c(10);    // this time we can hold 10 items

        HPX_TEST_EQ(static_cast<cache_type::size_type>(10), c.capacity());

        // insert 3 items into the cache
        int i = 0;
        data* d = &cache_entries[0];
        for (/**/; i < 3 && d->key != nullptr; ++d, ++i)
        {
            HPX_TEST(c.insert(d->key, d->value));
            HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
        }

        // there should be 3 items in the cache
        HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());

        // shrink the capacity but not below number of entries in the cache
        HPX_TEST(c.reserve(5));
        HPX_TEST_EQ(static_cast<cache_type::size_type>(5), c.capacity());

        // nothing needed to be evicted
        HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());
        HPX_TEST(c.holds_key("white"));
        HPX_TEST(c.holds_key("yellow"));
        HPX_TEST(c.holds_key("green"));
    }

    {
        cache_type c(10);

        HPX_TEST_EQ(static_cast<cache_type::size_type>(10), c.capacity());
        HPX_TEST_EQ(static_cast<cache_type::size_type>(0), c.size());

        // shrinking an empty cache must succeed
        HPX_TEST(c.reserve(5));
        HPX_TEST_EQ(static_cast<cache_type::size_type>(5), c.capacity());
        HPX_TEST_EQ(static_cast<cache_type::size_type>(0), c.size());
    }

    {
        cache_type c(0);    // 0 means: no limit

        HPX_TEST_EQ(static_cast<cache_type::size_type>(0), c.capacity());

        // insert 5 items into the cache
        int i = 0;
        data* d = &cache_entries[0];
        for (/**/; i < 5 && d->key != nullptr; ++d, ++i)
        {
            HPX_TEST(c.insert(d->key, d->value));
            HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(5));
        }

        // there should be 5 items in the cache
        HPX_TEST_EQ(static_cast<cache_type::size_type>(5), c.size());

        HPX_TEST(c.reserve(2));
        HPX_TEST_EQ(static_cast<cache_type::size_type>(2), c.capacity());
        HPX_TEST_EQ(static_cast<cache_type::size_type>(2), c.size());
    }

    {
        cache_type c(5);

        HPX_TEST_EQ(static_cast<cache_type::size_type>(5), c.capacity());

        // insert 3 items into the cache
        int i = 0;
        data* d = &cache_entries[0];
        for (/**/; i < 3 && d->key != nullptr; ++d, ++i)
        {
            HPX_TEST(c.insert(d->key, d->value));
            HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(5));
        }

        // there should be 3 items in the cache
        HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());

        // reserve(0) removes the limit, it must not evict anything
        HPX_TEST(c.reserve(0));
        HPX_TEST_EQ(static_cast<cache_type::size_type>(0), c.capacity());
        HPX_TEST_EQ(static_cast<cache_type::size_type>(3), c.size());
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_lru_free_space_order()
{
    using entry_type = hpx::util::cache::entries::lru_entry<int>;
    using cache_type = hpx::util::cache::local_cache<int, entry_type>;

    // shrinking evicts the least recently used entries, in order
    {
        cache_type c(5);

        // keys 0..4, 0 is the least recently used
        for (int i = 0; i != 5; ++i)
        {
            HPX_TEST(c.insert(i, i));
        }
        HPX_TEST_EQ(static_cast<cache_type::size_type>(5), c.size());

        // shrink the cache to 2 entries and check the return value
        HPX_TEST(c.reserve(2));
        // check that the size is 2
        HPX_TEST_EQ(static_cast<cache_type::size_type>(2), c.size());
        // check that keys 3 and 4 are still present
        HPX_TEST(c.holds_key(3));
        HPX_TEST(c.holds_key(4));
        // check that keys 0, 1 and 2 are gone
        HPX_TEST(!c.holds_key(0));
        HPX_TEST(!c.holds_key(1));
        HPX_TEST(!c.holds_key(2));
    }

    // an entry that was touched recently survives
    {
        cache_type c(5);

        for (int i = 0; i != 5; ++i)
        {
            HPX_TEST(c.insert(i, i));
        }

        // touch key 0 (see test_lru_insert_with_touch)
        int value = 0;
        HPX_TEST(c.get_entry(0, value));
        // shrink the cache to 2 entries
        HPX_TEST(c.reserve(2));
        // 0 was touched last and 4 inserted last, so they are the most recently
        // used ones and survive
        HPX_TEST_EQ(static_cast<cache_type::size_type>(2), c.size());
        HPX_TEST(c.holds_key(0));
        HPX_TEST(c.holds_key(4));
        HPX_TEST(!c.holds_key(1));
        HPX_TEST(!c.holds_key(2));
        HPX_TEST(!c.holds_key(3));
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_lru_insert_oversize_entry()
{
    using entry_type = hpx::util::cache::entries::size_entry<int>;
    using cache_type = hpx::util::cache::local_cache<int, entry_type>;

    cache_type c(3);

    // insert key 0 with an entry of size 1 and check that it succeeds
    HPX_TEST(c.insert(0, entry_type(0, 1)));

    // an entry larger than the whole cache can never fit: this used to
    // dereference an invalid iterator
    // insert key 1 with an entry of size 5 and check that it fails
    HPX_TEST(!c.insert(1, entry_type(1, 5)));
    // (don't check what remains in the cache)
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_lru_insert();
    test_lru_insert_with_touch();
    test_lru_clear();
    test_lru_erase_one();
    test_lru_update();
    test_lru_reserve();
    test_lru_free_space_order();
    test_lru_insert_oversize_entry();

    return hpx::util::report_errors();
}
