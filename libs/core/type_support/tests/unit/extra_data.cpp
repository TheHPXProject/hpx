//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace {

    // the order in which the items were destroyed and reset
    std::vector<int> destroyed;
    std::vector<int> reset_items;

    template <int N>
    struct item
    {
        item() = default;
        item(item const&) = delete;
        item(item&&) = delete;
        item& operator=(item const&) = delete;
        item& operator=(item&&) = delete;

        ~item()
        {
            destroyed.push_back(N);
        }

        int value = 0;
    };

    // a type with a larger alignment than the table of the objects
    struct alignas(64) aligned_item
    {
        char value = 0;
    };
}    // namespace

template <int N>
struct hpx::util::extra_data_helper<item<N>>
{
    static extra_data_id_type id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }

    static void reset(item<N>* p) noexcept
    {
        p->value = 0;
        reset_items.push_back(N);
    }
};

template <>
struct hpx::util::extra_data_helper<aligned_item>
{
    static extra_data_id_type id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }

    static void reset(aligned_item*) noexcept {}
};

namespace {

    using hpx::util::extra_data;
    using hpx::util::extra_data_id;

    void test_ids()
    {
        // one id for each type, the same on each call
        auto const id0 = extra_data_id<item<0>>();
        auto const id1 = extra_data_id<item<1>>();
        HPX_TEST_NEQ(id0, id1);
        HPX_TEST_EQ(extra_data_id<item<0>>(), id0);
        HPX_TEST(hpx::util::detail::extra_data_id_count() > id1);
    }

    void test_get()
    {
        extra_data data;
        HPX_TEST(data.try_get<item<0>>() == nullptr);

        item<0>& a = data.get<item<0>>();
        a.value = 42;
        HPX_TEST(data.try_get<item<0>>() == &a);
        HPX_TEST_EQ(data.get<item<0>>().value, 42);

        // other types are independent
        HPX_TEST(data.try_get<item<1>>() == nullptr);
        item<1>& b = data.get<item<1>>();
        b.value = 4242;
        HPX_TEST(data.try_get<item<1>>() == &b);
        HPX_TEST(data.try_get<item<0>>() == &a);
        HPX_TEST_EQ(a.value, 42);

        // a type whose id is larger than those known when the items were
        // added grows the table: the items keep their addresses
        item<7>& c = data.get<item<7>>();
        HPX_TEST(data.try_get<item<0>>() == &a);
        HPX_TEST(data.try_get<item<1>>() == &b);
        HPX_TEST(data.try_get<item<7>>() == &c);
    }

    void test_reset()
    {
        extra_data data;
        data.get<item<0>>().value = 1;
        data.get<item<1>>().value = 2;

        reset_items.clear();
        data.reset();
        HPX_TEST_EQ(data.get<item<0>>().value, 0);
        HPX_TEST_EQ(data.get<item<1>>().value, 0);

        // in the reverse order of their ids
        HPX_TEST_EQ(reset_items.size(), std::size_t(2));
        HPX_TEST_EQ(reset_items[0], 1);
        HPX_TEST_EQ(reset_items[1], 0);
    }

    void test_move()
    {
        extra_data data;
        item<0>& a = data.get<item<0>>();
        item<1>& b = data.get<item<1>>();

        // moving keeps the addresses of the items
        extra_data moved(std::move(data));
        HPX_TEST(moved.try_get<item<0>>() == &a);
        HPX_TEST(moved.try_get<item<1>>() == &b);
        // NOLINTNEXTLINE(bugprone-use-after-move)
        HPX_TEST(data.try_get<item<0>>() == nullptr);

        // the moved-from object can be used again
        data.get<item<2>>();
        HPX_TEST(data.try_get<item<2>>() != nullptr);

        // move assignment destroys the items of the target first
        destroyed.clear();
        data = std::move(moved);
        HPX_TEST_EQ(destroyed.size(), std::size_t(1));
        HPX_TEST_EQ(destroyed[0], 2);
        HPX_TEST(data.try_get<item<0>>() == &a);
        HPX_TEST(data.try_get<item<1>>() == &b);
    }

    // the first object is allocated together with the table, the others
    // separately: both are aligned
    void test_alignment()
    {
        {
            extra_data data;
            aligned_item& a = data.get<aligned_item>();
            HPX_TEST_EQ(reinterpret_cast<std::uintptr_t>(&a) % 64, 0u);
        }
        {
            extra_data data;
            data.get<item<0>>();
            aligned_item& a = data.get<aligned_item>();
            HPX_TEST_EQ(reinterpret_cast<std::uintptr_t>(&a) % 64, 0u);
        }
    }

    void test_destruction_order()
    {
        destroyed.clear();
        {
            extra_data data;
            data.get<item<0>>();
            data.get<item<2>>();
            data.get<item<1>>();
        }

        // in the reverse order of their ids
        HPX_TEST_EQ(destroyed.size(), std::size_t(3));
        HPX_TEST_EQ(destroyed[0], 2);
        HPX_TEST_EQ(destroyed[1], 1);
        HPX_TEST_EQ(destroyed[2], 0);
    }
}    // namespace

int main()
{
    test_ids();
    test_get();
    test_reset();
    test_move();
    test_alignment();
    test_destruction_order();

    return hpx::util::report_errors();
}
