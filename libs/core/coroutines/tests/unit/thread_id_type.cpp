//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test focuses on the reference counting behavior of
// hpx::threads::thread_id_ref's move-from-thread_id constructor (which now
// takes an additional `add_ref` flag) and on hpx::threads::keep_alive_thread_id,
// a helper type that optionally keeps a thread_id alive without necessarily
// adding to its reference count.

#include <hpx/config.hpp>
#include <hpx/coroutines/thread_id_type.hpp>
#include <hpx/modules/testing.hpp>

#include <type_traits>
#include <utility>

namespace {

    // A minimal thread_data_reference_counting implementation that allows
    // tests to observe both the current reference count and whether
    // destroy_thread() was invoked, without requiring a full thread_data /
    // HPX runtime instance.
    struct test_thread_data : hpx::threads::detail::thread_data_reference_counting
    {
        explicit test_thread_data(hpx::threads::thread_id_addref const addref =
                                       hpx::threads::thread_id_addref::yes)
          : hpx::threads::detail::thread_data_reference_counting(addref)
        {
        }

        void destroy_thread() override
        {
            destroyed = true;
        }

        [[nodiscard]] long use_count() const noexcept
        {
            return count_;
        }

        bool destroyed = false;
    };
}    // namespace

///////////////////////////////////////////////////////////////////////////
// hpx::threads::thread_id_ref(thread_id&&, bool add_ref = true)
void test_thread_id_ref_move_from_thread_id_default_add_ref()
{
    test_thread_data data;    // starts with a use count of 1
    HPX_TEST_EQ(data.use_count(), 1L);

    {
        hpx::threads::thread_id tid(&data);
        HPX_TEST(tid);

        // default add_ref == true increments the reference count
        hpx::threads::thread_id_ref const ref(HPX_MOVE(tid));
        HPX_TEST(!tid);    // moved-from thread_id was reset
        HPX_TEST(ref);
        HPX_TEST_EQ(data.use_count(), 2L);
    }

    // leaving scope releases the extra reference again
    HPX_TEST_EQ(data.use_count(), 1L);
    HPX_TEST(!data.destroyed);
}

void test_thread_id_ref_move_from_thread_id_no_add_ref()
{
    test_thread_data data;    // starts with a use count of 1
    HPX_TEST_EQ(data.use_count(), 1L);

    {
        hpx::threads::thread_id tid(&data);

        // add_ref == false does not touch the reference count on
        // construction...
        hpx::threads::thread_id_ref const ref(HPX_MOVE(tid), false);
        HPX_TEST(ref);
        HPX_TEST_EQ(data.use_count(), 1L);
    }

    // ...but a plain thread_id_ref unconditionally releases on destruction,
    // which (as it never added a reference) now under-counts and triggers
    // destruction. This demonstrates why keep_alive_thread_id (tested below)
    // needs special handling for the add_ref == false case.
    HPX_TEST_EQ(data.use_count(), 0L);
    HPX_TEST(data.destroyed);
}

///////////////////////////////////////////////////////////////////////////
// hpx::threads::keep_alive_thread_id
void test_keep_alive_thread_id_with_add_ref()
{
    test_thread_data data;    // simulate an already-alive object, use count 1
    HPX_TEST_EQ(data.use_count(), 1L);

    {
        hpx::threads::thread_id tid(&data);
        hpx::threads::keep_alive_thread_id const kept(HPX_MOVE(tid), true);

        HPX_TEST(kept.add_ref);
        HPX_TEST(kept.id);
        HPX_TEST_EQ(data.use_count(), 2L);
    }

    // the temporary extra reference is released again, count returns to the
    // original value; the object must not have been destroyed
    HPX_TEST_EQ(data.use_count(), 1L);
    HPX_TEST(!data.destroyed);
}

void test_keep_alive_thread_id_without_add_ref()
{
    test_thread_data data;    // simulate an object already kept alive
                              // elsewhere (e.g. by the scheduler), use count 1
    HPX_TEST_EQ(data.use_count(), 1L);

    {
        hpx::threads::thread_id tid(&data);
        hpx::threads::keep_alive_thread_id const kept(HPX_MOVE(tid), false);

        HPX_TEST(!kept.add_ref);
        HPX_TEST(kept.id);

        // constructing with add_ref == false must not increment the count
        HPX_TEST_EQ(data.use_count(), 1L);
    }

    // destroying the keep_alive_thread_id must not decrement the count
    // either (it detaches the underlying pointer instead of releasing it),
    // so the object must still be alive with an unchanged reference count
    HPX_TEST_EQ(data.use_count(), 1L);
    HPX_TEST(!data.destroyed);
}

void test_keep_alive_thread_id_move_construction()
{
    test_thread_data data;
    HPX_TEST_EQ(data.use_count(), 1L);

    hpx::threads::thread_id tid(&data);
    hpx::threads::keep_alive_thread_id kept1(HPX_MOVE(tid), true);
    HPX_TEST_EQ(data.use_count(), 2L);

    hpx::threads::keep_alive_thread_id const kept2(HPX_MOVE(kept1));

    // moving must not change the observed reference count
    HPX_TEST_EQ(data.use_count(), 2L);
    HPX_TEST(!kept1.id);    // moved-from thread_id_ref is now empty
    HPX_TEST(kept2.id);
    HPX_TEST(kept2.add_ref);

    // kept1 (moved-from, holds no reference) destructs first without effect,
    // then kept2 releases the single held reference
    {
        [[maybe_unused]] auto discard = HPX_MOVE(kept1);
    }
    HPX_TEST_EQ(data.use_count(), 2L);
}

void test_keep_alive_thread_id_move_assignment()
{
    test_thread_data data;
    HPX_TEST_EQ(data.use_count(), 1L);

    hpx::threads::thread_id tid(&data);
    hpx::threads::keep_alive_thread_id kept1(HPX_MOVE(tid), false);
    HPX_TEST_EQ(data.use_count(), 1L);

    hpx::threads::keep_alive_thread_id kept2(hpx::threads::thread_id(), true);
    HPX_TEST(!kept2.id);

    kept2 = HPX_MOVE(kept1);

    HPX_TEST(!kept2.add_ref);
    HPX_TEST(kept2.id);
    HPX_TEST_EQ(data.use_count(), 1L);
}

///////////////////////////////////////////////////////////////////////////
void test_keep_alive_thread_id_is_move_only()
{
    static_assert(!std::is_copy_constructible_v<hpx::threads::keep_alive_thread_id>,
        "keep_alive_thread_id must not be copy constructible");
    static_assert(!std::is_copy_assignable_v<hpx::threads::keep_alive_thread_id>,
        "keep_alive_thread_id must not be copy assignable");
    static_assert(std::is_move_constructible_v<hpx::threads::keep_alive_thread_id>,
        "keep_alive_thread_id must be move constructible");
    static_assert(std::is_move_assignable_v<hpx::threads::keep_alive_thread_id>,
        "keep_alive_thread_id must be move assignable");

    HPX_TEST(true);
}

int main()
{
    test_thread_id_ref_move_from_thread_id_default_add_ref();
    test_thread_id_ref_move_from_thread_id_no_add_ref();

    test_keep_alive_thread_id_with_add_ref();
    test_keep_alive_thread_id_without_add_ref();
    test_keep_alive_thread_id_move_construction();
    test_keep_alive_thread_id_move_assignment();
    test_keep_alive_thread_id_is_move_only();

    return hpx::util::report_errors();
}