//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/testing.hpp>

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {

    // helper type that counts the number of live instances, used to verify
    // that clear()/the destructor properly release all nodes
    struct counted
    {
        static int instances;

        explicit counted(int const v) noexcept
          : value(v)
        {
            ++instances;
        }

        counted(counted const& other) noexcept
          : value(other.value)
        {
            ++instances;
        }

        counted(counted&& other) noexcept
          : value(other.value)
        {
            ++instances;
        }

        ~counted()
        {
            --instances;
        }

        int value;
    };

    int counted::instances = 0;

    // move-only type wrapping a std::unique_ptr<int>, used to exercise the
    // T&& overloads of push_front()/push_back()
    struct move_only
    {
        explicit move_only(int v)
          : value(std::make_unique<int>(v))
        {
        }

        move_only(move_only const&) = delete;
        move_only& operator=(move_only const&) = delete;

        move_only(move_only&&) = default;
        move_only& operator=(move_only&&) = default;

        std::unique_ptr<int> value;
    };
}    // namespace

static_assert(!std::is_copy_constructible_v<hpx::detail::forward_list<int>>);
static_assert(!std::is_move_constructible_v<hpx::detail::forward_list<int>>);
static_assert(!std::is_copy_assignable_v<hpx::detail::forward_list<int>>);
static_assert(!std::is_move_assignable_v<hpx::detail::forward_list<int>>);

void test_default_constructed()
{
    hpx::detail::forward_list<int> list;

    HPX_TEST(list.empty());

    HPX_TEST_THROW(list.front(), std::runtime_error);

    hpx::detail::forward_list<int> const& clist = list;
    HPX_TEST_THROW(clist.front(), std::runtime_error);

    HPX_TEST_THROW(list.pop_front(), std::runtime_error);
}

void test_push_pop_front_single()
{
    hpx::detail::forward_list<int> list;
    HPX_TEST(list.empty());

    list.push_front(42);
    HPX_TEST(!list.empty());
    HPX_TEST_EQ(list.front(), 42);

    list.pop_front();
    HPX_TEST(list.empty());
}

void test_push_pop_front_multiple_lifo()
{
    hpx::detail::forward_list<int> list;

    list.push_front(1);
    list.push_front(2);
    list.push_front(3);

    HPX_TEST_EQ(list.front(), 3);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 2);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_push_front_lvalue_and_rvalue()
{
    hpx::detail::forward_list<int> list;

    int value = 1;
    list.push_front(value);    // lvalue overload
    list.push_front(2);        // rvalue overload

    HPX_TEST_EQ(list.front(), 2);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_push_front_move_only()
{
    hpx::detail::forward_list<move_only> list;

    list.push_front(move_only(1));
    list.push_front(move_only(2));

    HPX_TEST_EQ(*list.front().value, 2);
    list.pop_front();
    HPX_TEST_EQ(*list.front().value, 1);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_push_back_empty()
{
    hpx::detail::forward_list<int> list;

    list.push_back(1);

    HPX_TEST(!list.empty());
    HPX_TEST_EQ(list.front(), 1);
}

void test_push_back_multiple_fifo()
{
    hpx::detail::forward_list<int> list;

    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 2);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 3);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_push_back_lvalue_and_rvalue()
{
    hpx::detail::forward_list<int> list;

    int value = 1;
    list.push_back(value);    // lvalue overload
    list.push_back(2);        // rvalue overload

    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 2);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_push_back_move_only()
{
    hpx::detail::forward_list<move_only> list;

    list.push_back(move_only(1));
    list.push_back(move_only(2));

    HPX_TEST_EQ(*list.front().value, 1);
    list.pop_front();
    HPX_TEST_EQ(*list.front().value, 2);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_mixed_push_front_push_back()
{
    hpx::detail::forward_list<int> list;

    list.push_front(2);
    list.push_back(3);
    list.push_front(1);
    list.push_back(4);

    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 2);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 3);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 4);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_reverse_empty()
{
    hpx::detail::forward_list<int> list;

    list.reverse();

    HPX_TEST(list.empty());
}

void test_reverse_single_element()
{
    hpx::detail::forward_list<int> list;
    list.push_front(1);

    list.reverse();

    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_reverse_multiple_elements()
{
    hpx::detail::forward_list<int> list;

    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    list.reverse();

    HPX_TEST_EQ(list.front(), 3);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 2);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_reverse_twice_restores_order()
{
    hpx::detail::forward_list<int> list;

    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    list.reverse();
    list.reverse();

    HPX_TEST_EQ(list.front(), 1);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 2);
    list.pop_front();
    HPX_TEST_EQ(list.front(), 3);
    list.pop_front();
    HPX_TEST(list.empty());
}

void test_clear_empty()
{
    hpx::detail::forward_list<int> list;

    list.clear();

    HPX_TEST(list.empty());
}

void test_clear_populated()
{
    HPX_TEST_EQ(counted::instances, 0);
    {
        hpx::detail::forward_list<counted> list;
        list.push_back(counted(1));
        list.push_back(counted(2));
        list.push_back(counted(3));

        HPX_TEST_EQ(counted::instances, 3);

        list.clear();

        HPX_TEST(list.empty());
        HPX_TEST_EQ(counted::instances, 0);
    }
    HPX_TEST_EQ(counted::instances, 0);
}

void test_destructor_releases_nodes()
{
    HPX_TEST_EQ(counted::instances, 0);
    {
        hpx::detail::forward_list<counted> list;
        list.push_back(counted(1));
        list.push_back(counted(2));
        list.push_back(counted(3));
        list.push_front(counted(0));

        HPX_TEST_EQ(counted::instances, 4);
    }
    HPX_TEST_EQ(counted::instances, 0);
}

void test_const_correctness()
{
    hpx::detail::forward_list<int> list;
    list.push_front(42);

    hpx::detail::forward_list<int> const& clist = list;

    static_assert(std::is_same_v<decltype(clist.front()), int const&>);
    HPX_TEST_EQ(clist.front(), 42);
}

int main()
{
    test_default_constructed();

    test_push_pop_front_single();
    test_push_pop_front_multiple_lifo();
    test_push_front_lvalue_and_rvalue();
    test_push_front_move_only();

    test_push_back_empty();
    test_push_back_multiple_fifo();
    test_push_back_lvalue_and_rvalue();
    test_push_back_move_only();
    test_mixed_push_front_push_back();

    test_reverse_empty();
    test_reverse_single_element();
    test_reverse_multiple_elements();
    test_reverse_twice_restores_order();

    test_clear_empty();
    test_clear_populated();
    test_destructor_releases_nodes();

    test_const_correctness();

    return hpx::util::report_errors();
}
