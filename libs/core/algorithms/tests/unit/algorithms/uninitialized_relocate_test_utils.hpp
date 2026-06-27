//  Copyright (c) 2023 Isidoros Tsaousis-Seiras
//  Copyright (c) 2025 Jatin Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shared test infrastructure for the uninitialized_relocate family of
// algorithm tests. Extracted to avoid duplication across
// uninitialized_relocate.cpp, uninitialized_relocaten.cpp,
// uninitialized_relocate_backward.cpp and their sender variants.

#pragma once

#include <hpx/modules/testing.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <mutex>
#include <set>
#include <utility>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
constexpr int N = 500;    // number of objects to construct
constexpr int M = 200;    // number of objects to relocate
constexpr int K = 100;    // number of objects to relocate before throwing

static_assert(N > M);
static_assert(M > K);

using hpx::experimental::is_trivially_relocatable_v;

// ---------------------------------------------------------------------------
// Thread-safe bookkeeping helpers
// ---------------------------------------------------------------------------
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline std::mutex& get_test_mutex()
{
    static std::mutex m;
    return m;
}

template <typename F>
void simple_mutex_operation(F&& f)
{
    std::lock_guard<std::mutex> lk(get_test_mutex());
    f();
}

// ---------------------------------------------------------------------------
// counted_struct - tracks constructions, moves, and destructions
// ---------------------------------------------------------------------------
enum relocation_category
{
    trivially_relocatable,
    non_trivially_relocatable,
    non_trivially_relocatable_throwing
};

template <relocation_category c, bool overlapping_test = false>
struct counted_struct
{
    static std::set<counted_struct<c, overlapping_test>*> made;
    static std::atomic<int> moved;
    static std::atomic<int> destroyed;
    int data;

    explicit counted_struct(int data)
      : data(data)
    {
        // Check that we are not constructing an object on top of another
        simple_mutex_operation([&]() {
            HPX_TEST(!made.count(this));
            made.insert(this);
        });
    }

    counted_struct(counted_struct&& other) noexcept(
        c != non_trivially_relocatable_throwing)
      : data(other.data)
    {
        if constexpr (c == non_trivially_relocatable_throwing)
        {
            if (moved++ == K - 1)
            {
                throw 42;
            }
        }
        else
        {
            moved++;
        }

        // Check that we are not constructing an object on top of another
        simple_mutex_operation([&]() {
            // Unless we are testing overlapping relocation
            // we should not be move-constructing an object on top of another
            if constexpr (!overlapping_test)
            {
                HPX_TEST(!made.count(this));
            }
            made.insert(this);
        });
    }

    ~counted_struct()
    {
        destroyed++;

        // Check that the object was constructed
        // and not already destroyed
        simple_mutex_operation([&]() {
            HPX_TEST(made.count(this));
            made.erase(this);
        });
    }

    // making sure the address is never directly accessed
    friend void operator&(counted_struct) = delete;
};

template <relocation_category c, bool overlapping_test>
std::set<counted_struct<c, overlapping_test>*>
    counted_struct<c, overlapping_test>::made;

template <relocation_category c, bool overlapping_test>
std::atomic<int> counted_struct<c, overlapping_test>::moved = 0;

template <relocation_category c, bool overlapping_test>
std::atomic<int> counted_struct<c, overlapping_test>::destroyed = 0;

// ---------------------------------------------------------------------------
// Type aliases
// ---------------------------------------------------------------------------

// Non overlapping relocation testing mechanisms
using trivially_relocatable_struct = counted_struct<trivially_relocatable>;
HPX_DECLARE_TRIVIALLY_RELOCATABLE(trivially_relocatable_struct);

using non_trivially_relocatable_struct =
    counted_struct<non_trivially_relocatable>;

using non_trivially_relocatable_struct_throwing =
    counted_struct<non_trivially_relocatable_throwing>;

// Overlapping relocation testing mechanisms
using trivially_relocatable_struct_overlapping =
    counted_struct<trivially_relocatable, true>;
HPX_DECLARE_TRIVIALLY_RELOCATABLE(trivially_relocatable_struct_overlapping);

using non_trivially_relocatable_struct_overlapping =
    counted_struct<non_trivially_relocatable, true>;

using non_trivially_relocatable_struct_throwing_overlapping =
    counted_struct<non_trivially_relocatable_throwing, true>;

// Check that the correct types are trivially relocatable
static_assert(is_trivially_relocatable_v<trivially_relocatable_struct>);
static_assert(
    is_trivially_relocatable_v<trivially_relocatable_struct_overlapping>);

static_assert(!is_trivially_relocatable_v<non_trivially_relocatable_struct>);
static_assert(
    !is_trivially_relocatable_v<non_trivially_relocatable_struct_overlapping>);

static_assert(
    !is_trivially_relocatable_v<non_trivially_relocatable_struct_throwing>);
static_assert(!is_trivially_relocatable_v<
    non_trivially_relocatable_struct_throwing_overlapping>);

// ---------------------------------------------------------------------------
// clear() - resets all static state between tests
// ---------------------------------------------------------------------------
inline void clear()
{
    trivially_relocatable_struct::moved = 0;
    trivially_relocatable_struct::destroyed = 0;
    trivially_relocatable_struct::made.clear();

    trivially_relocatable_struct_overlapping::moved = 0;
    trivially_relocatable_struct_overlapping::destroyed = 0;
    trivially_relocatable_struct_overlapping::made.clear();

    non_trivially_relocatable_struct::moved = 0;
    non_trivially_relocatable_struct::destroyed = 0;
    non_trivially_relocatable_struct::made.clear();

    non_trivially_relocatable_struct_overlapping::moved = 0;
    non_trivially_relocatable_struct_overlapping::destroyed = 0;
    non_trivially_relocatable_struct_overlapping::made.clear();

    non_trivially_relocatable_struct_throwing::moved = 0;
    non_trivially_relocatable_struct_throwing::destroyed = 0;
    non_trivially_relocatable_struct_throwing::made.clear();

    non_trivially_relocatable_struct_throwing_overlapping::moved = 0;
    non_trivially_relocatable_struct_throwing_overlapping::destroyed = 0;
    non_trivially_relocatable_struct_throwing_overlapping::made.clear();
}

// ---------------------------------------------------------------------------
// setup<T>() - allocates two buffers for non-overlapping tests
// ---------------------------------------------------------------------------
template <typename T>
std::pair<T*, T*> setup()
{
    clear();

    void* mem1 = std::malloc(N * sizeof(T));
    void* mem2 = std::malloc(N * sizeof(T));

    HPX_TEST(mem1 && mem2);

    T* ptr1 = static_cast<T*>(mem1);
    T* ptr2 = static_cast<T*>(mem2);

    HPX_TEST(T::made.size() == 0);
    HPX_TEST(T::moved == 0);
    HPX_TEST(T::destroyed == 0);

    for (int i = 0; i < N; i++)
    {
        hpx::construct_at(ptr1 + i, i);
    }

    // fill ptr2 with 0 after M
    std::fill(static_cast<std::byte*>(mem2) + M * sizeof(T),
        static_cast<std::byte*>(mem2) + N * sizeof(T), std::byte{0});

    // N objects constructed
    HPX_TEST(T::made.size() == N);

    return {ptr1, ptr2};
}

// ---------------------------------------------------------------------------
// setup_single<T>() - allocates one buffer for overlapping tests
// ---------------------------------------------------------------------------
template <typename T>
T* setup_single()
{
    clear();

    void* mem = std::malloc(N * sizeof(T));

    HPX_TEST(mem);

    T* ptr = static_cast<T*>(mem);

    HPX_TEST(T::made.size() == 0);
    HPX_TEST(T::moved == 0);
    HPX_TEST(T::destroyed == 0);

    for (int i = 0; i < N; i++)
    {
        hpx::construct_at(ptr + i, i);
    }

    // N objects constructed
    HPX_TEST(T::made.size() == N);

    return ptr;
}
