//  Copyright (c) 2025 Zhengnan Hua
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/type_support.hpp>

#include "uninitialized_relocate_test_utils.hpp"

using hpx::experimental::uninitialized_relocate;

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

template <typename LnPolicy, typename ExPolicy>
void test(LnPolicy ln_policy, ExPolicy&& ex_policy)
{
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {    // Non-overlapping trivially relocatable
        auto [ptr1, ptr2] = setup<trivially_relocatable_struct>();

        for (int i = 0; i < N; i++)
        {
            // Check that the objects were constructed in the first place
            HPX_TEST(trivially_relocatable_struct::made.count(ptr1 + i));
        }

        // relocate M objects to ptr2
        tt::sync_wait(ex::just(ptr1, ptr1 + M, ptr2) |
            uninitialized_relocate(ex_policy.on(exec)));

        // bookkeeping
        // Artificially add and remove the objects to the set that are where
        // relocated because the relocation is trivial
        for (int i = 0; i < M; i++)
        {
            trivially_relocatable_struct::made.erase(ptr1 + i);
            trivially_relocatable_struct::made.insert(ptr2 + i);
        }

        // No move constructor or destructor should be called
        HPX_TEST(trivially_relocatable_struct::moved == 0);
        HPX_TEST(trivially_relocatable_struct::destroyed == 0);

        // Objects not touched
        for (int i = M; i < N; i++)
        {
            HPX_TEST(ptr1[i].data == i);
        }

        // Objects moved from ptr1 to ptr2
        for (int i = 0; i < M; i++)
        {
            HPX_TEST(ptr2[i].data == i);
        }

        // make sure the memory beyond M is untouched
        for (std::byte* p = reinterpret_cast<std::byte*>(ptr2 + M);
            p < reinterpret_cast<std::byte*>(ptr2 + N); p++)
        {
            HPX_TEST(*p == std::byte{0});
        }

        // From our perspective, the objects in ptr1 are destroyed
        std::destroy(ptr1 + M, ptr1 + N);
        std::destroy(ptr2, ptr2 + M);

        HPX_TEST(trivially_relocatable_struct::made.empty());

        std::free(ptr1);
        std::free(ptr2);
    }
    {    // Non-overlapping non-trivially relocatable
        auto [ptr1, ptr2] = setup<non_trivially_relocatable_struct>();

        // relocate them to ptr2
        tt::sync_wait(ex::just(ptr1, ptr1 + M, ptr2) |
            uninitialized_relocate(ex_policy.on(exec)));

        // M move constructors were called and M destructors
        HPX_TEST(non_trivially_relocatable_struct::moved == M);
        HPX_TEST(non_trivially_relocatable_struct::destroyed == M);

        // Objects not touched
        for (int i = M; i < N; i++)
        {
            HPX_TEST(ptr1[i].data == i);
        }

        // Objects moved from ptr1 to ptr2
        for (int i = 0; i < M; i++)
        {
            HPX_TEST(ptr2[i].data == i);
        }

        // make sure the memory beyond M is untouched
        for (std::byte* p = reinterpret_cast<std::byte*>(ptr2 + M);
            p < reinterpret_cast<std::byte*>(ptr2 + N); p++)
        {
            HPX_TEST(*p == std::byte{0});
        }

        std::destroy(ptr1 + M, ptr1 + N);
        std::destroy(ptr2, ptr2 + M);

        HPX_TEST(non_trivially_relocatable_struct::made.empty());

        std::free(ptr1);
        std::free(ptr2);
    }
    {    // Non-overlapping non-trivially relocatable throwing
        auto [ptr1, ptr2] = setup<non_trivially_relocatable_struct_throwing>();

        // relocate M objects to ptr2
        try
        {
            tt::sync_wait(ex::just(ptr1, ptr1 + M, ptr2) |
                uninitialized_relocate(ex_policy.on(exec)));
            HPX_UNREACHABLE;    // should have thrown
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {
        }

        // If the order is sequenced we can guarantee that no moving
        // occurs after the exception is thrown
        if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
        {
            // K move constructors were called, and then the last one throws
            HPX_TEST(non_trivially_relocatable_struct_throwing::moved == K);

            // K - 1 objects where destroyed after being moved-from
            // and then M destructors were called: K on the old range and
            // M - K on the new range
            HPX_TEST(non_trivially_relocatable_struct_throwing::destroyed ==
                K - 1 + M);
        }

        // After the exception is caught, all the objects in the old and the new
        // range are destroyed, so M objects are destroyed on each range
        // In the case of parallel execution, the number of moves done before stopping
        // is arbitrary, since the exception in one thread is not guaranteed to stop
        // all the other threads.
        HPX_TEST(
            non_trivially_relocatable_struct_throwing::made.size() == N - M);

        // The objects in the ends of ptr1 are still valid
        for (int i = M; i < N; i++)
        {
            HPX_TEST(ptr1[i].data == i);
        }

        // make sure the memory beyond M is untouched
        for (std::byte* p = reinterpret_cast<std::byte*>(ptr2 + M);
            p < reinterpret_cast<std::byte*>(ptr2 + N); p++)
        {
            HPX_TEST(*p == std::byte{0});
        }

        std::destroy(ptr1 + M, ptr1 + N);

        HPX_TEST(non_trivially_relocatable_struct_throwing::made.empty());

        std::free(ptr1);
        std::free(ptr2);
    }
    clear();

    return;
}

template <typename LnPolicy, typename ExPolicy>
void test_overlapping(LnPolicy ln_policy, ExPolicy&& ex_policy)
{
    constexpr int offset = 4;

    // relocating M objects `offset` positions forward
    static_assert(M + offset <= N);

    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {    // Overlapping trivially-relocatable
        auto ptr = setup_single<trivially_relocatable_struct_overlapping>();

        // Destroy the objects that will be overwritten for bookkeeping
        std::destroy(ptr, ptr + offset);
        HPX_TEST(trivially_relocatable_struct_overlapping::destroyed == offset);

        // relocate M objects `offset` positions backwards
        tt::sync_wait(ex::just(ptr + offset, ptr + M + offset, ptr) |
            uninitialized_relocate(ex_policy.on(exec)));

        // Artificially remove the objects from the set for bookkeeping
        for (int i = offset; i < M + offset; i++)
        {
            trivially_relocatable_struct_overlapping::made.erase(ptr + i);
        }    // and add the objects that were relocated
        for (int i = 0; i < M; i++)
        {
            trivially_relocatable_struct_overlapping::made.insert(ptr + i);
        }

        // No move constructor or destructor should be called
        // because the objects are trivially relocatable
        HPX_TEST(trivially_relocatable_struct_overlapping::moved == 0);
        HPX_TEST(trivially_relocatable_struct_overlapping::destroyed == offset);

        // Objects relocated backwards
        for (int i = 0; i < M; i++)
        {
            HPX_TEST(ptr[i].data == i + offset);
        }

        // Objects not touched
        for (int i = M + offset; i < N; i++)
        {
            HPX_TEST(ptr[i].data == i);
        }

        // Destroy objects within their lifetime
        // from our perspective objects in the range [M, M + offset) are destroyed
        std::destroy(ptr, ptr + M);
        std::destroy(ptr + M + offset, ptr + N);

        HPX_TEST(trivially_relocatable_struct_overlapping::made.empty());

        std::free(ptr);
    }
    {    // Overlapping non-trivially relocatable
        auto ptr = setup_single<non_trivially_relocatable_struct_overlapping>();

        // Destroy the objects that will be overwritten for bookkeeping purposes
        std::destroy(ptr, ptr + offset);
        HPX_TEST(
            non_trivially_relocatable_struct_overlapping::destroyed == offset);

        // relocate backwards them to ptr2
        tt::sync_wait(ex::just(ptr + offset, ptr + M + offset, ptr) |
            uninitialized_relocate(ex_policy.on(exec)));

        // M move constructors were called and M destructors + prior destructors
        HPX_TEST(non_trivially_relocatable_struct_overlapping::moved == M);
        HPX_TEST(non_trivially_relocatable_struct_overlapping::destroyed ==
            M + offset);

        // Objects relocated forwards
        for (int i = 0; i < M; i++)
        {
            HPX_TEST(ptr[i].data == i + offset);
        }

        // Objects not touched
        for (int i = M + offset; i < N; i++)
        {
            HPX_TEST(ptr[i].data == i);
        }

        // Destroy objects within their lifetime
        // objects in the range [M, M + offset) are destroyed
        std::destroy(ptr, ptr + M);
        std::destroy(ptr + M + offset, ptr + N);

        HPX_TEST(non_trivially_relocatable_struct_overlapping::made.empty());

        std::free(ptr);
    }
    {    // Overlapping non-trivially relocatable throwing
        auto ptr = setup_single<
            non_trivially_relocatable_struct_throwing_overlapping>();

        // Destroy the objects that will be overwritten for bookkeeping purposes
        std::destroy(ptr, ptr + offset);
        HPX_TEST(
            non_trivially_relocatable_struct_throwing_overlapping::destroyed ==
            offset);

        // relocate them backwards
        try
        {
            tt::sync_wait(ex::just(ptr + offset, ptr + M + offset, ptr) |
                uninitialized_relocate(ex_policy.on(exec)));
            HPX_UNREACHABLE;    // should have thrown
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {
        }

        // If the order is sequenced we can guarantee that no moving
        // occurs after the exception is thrown
        if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
        {
            // K move constructors were called, and then the last one throws
            HPX_TEST(
                non_trivially_relocatable_struct_throwing_overlapping::moved ==
                K);

            // K - 1 objects where destroyed after being moved-from
            // and then M destructors were called: K on the new range and
            // M - K on the old range
            // + offset from prior the relocation
            HPX_TEST(non_trivially_relocatable_struct_throwing_overlapping::
                         destroyed == K - 1 + M + offset);
        }

        // The objects in the end of ptr1 are still valid
        for (int i = M + offset; i < N; i++)
        {
            HPX_TEST(ptr[i].data == i);
        }

        // Destroy objects within their lifetime
        std::destroy(ptr + M + offset, ptr + N);

        HPX_TEST(non_trivially_relocatable_struct_throwing_overlapping::made
                .empty());

        std::free(ptr);
    }
    clear();

    return;
}

int hpx_main()
{
    using namespace hpx::execution;

    test(hpx::launch::sync, seq(task));
    test(hpx::launch::sync, unseq(task));
    test(hpx::launch::async, par(task));
    test(hpx::launch::async, par_unseq(task));

    test_overlapping(hpx::launch::sync, seq(task));
    test_overlapping(hpx::launch::sync, unseq(task));
    test_overlapping(hpx::launch::async, par(task));
    test_overlapping(hpx::launch::async, par_unseq(task));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init(hpx_main, argc, argv);
    return hpx::util::report_errors();
}
