//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/cache.hpp>
#include <hpx/modules/testing.hpp>

#include <functional>
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
// The entries are inserted back to back, many of them within the same tick of
// the steady clock. The cache has to evict them in the order of insertion
// nevertheless.
void test_fifo_insert()
{
    using entry_type = hpx::util::cache::entries::fifo_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type>;

    cache_type c(3);

    for (data const* d = &cache_entries[0]; d->key != nullptr; ++d)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
    }

    // the three entries inserted last are left
    std::string value;
    HPX_TEST(!c.get_entry("white", value));
    HPX_TEST(!c.get_entry("yellow", value));
    HPX_TEST(!c.get_entry("green", value));
    HPX_TEST(c.get_entry("blue", value));
    HPX_TEST(c.get_entry("magenta", value));
    HPX_TEST(c.get_entry("black", value));
}

void test_lifo_insert()
{
    using entry_type = hpx::util::cache::entries::fifo_entry<std::string>;
    using cache_type = hpx::util::cache::local_cache<std::string, entry_type,
        std::greater<entry_type>>;

    cache_type c(3);

    for (data const* d = &cache_entries[0]; d->key != nullptr; ++d)
    {
        HPX_TEST(c.insert(d->key, d->value));
        HPX_TEST_LTE(c.size(), static_cast<cache_type::size_type>(3));
    }

    // the two entries inserted first are left, plus the one inserted last
    std::string value;
    HPX_TEST(c.get_entry("white", value));
    HPX_TEST(c.get_entry("yellow", value));
    HPX_TEST(!c.get_entry("green", value));
    HPX_TEST(!c.get_entry("blue", value));
    HPX_TEST(!c.get_entry("magenta", value));
    HPX_TEST(c.get_entry("black", value));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_fifo_insert();
    test_lifo_insert();

    return hpx::util::report_errors();
}
