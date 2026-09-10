//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_HAVE_CXX26_REFLECTION)
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace reflect_test {

    std::int32_t increment(std::int32_t i)
    {
        return i + 1;
    }

    std::int32_t add(std::int32_t x, std::int32_t y)
    {
        return x + y;
    }

    hpx::id_type identity()
    {
        return hpx::find_here();
    }

    std::int32_t always_throws(std::int32_t)
    {
        throw std::runtime_error("reflect_test::always_throws");
    }

    // Receives result posted back from the remote locality.
    // Set by receive_result when the remote post completes.
    hpx::util::spinlock post_result_mutex;
    std::int32_t post_result = -1;

    void receive_result(std::int32_t i)
    {
        std::lock_guard<hpx::util::spinlock> l(post_result_mutex);
        post_result = i;
    }

    // Posts the incremented result back to the calling locality.
    // This gives hpx::post an observable side effect.
    void increment_and_post_back(hpx::id_type const& here, std::int32_t i)
    {
        hpx::post<^^receive_result>(here, i + 1);
    }

}    // namespace reflect_test

int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    hpx::id_type const here = hpx::find_here();

    for (hpx::id_type const& loc : localities)
    {
        // hpx::sync<^^func> -- returns result directly, no .get() needed
        {
            std::int32_t r =
                hpx::sync<^^reflect_test::increment>(loc, std::int32_t(41));
            HPX_TEST_EQ(r, std::int32_t(42));
        }

        // hpx::sync<^^func> with launch policy
        {
            std::int32_t r = hpx::sync<^^reflect_test::add>(
                hpx::launch::sync, loc, std::int32_t(20), std::int32_t(22));
            HPX_TEST_EQ(r, std::int32_t(42));
        }

        // hpx::post<^^func> -- verify remote execution via callback.
        // increment_and_post_back posts the result back to here;
        // receive_result stores it in post_result which we then assert.
        {
            reflect_test::post_result = -1;
            hpx::post<^^reflect_test::increment_and_post_back>(
                loc, here, std::int32_t(41));
            // Wait until the callback posts the result back.
            // Check outside the lock to avoid holding the mutex
            // while receive_result needs to acquire it.
            while (true)
            {
                std::int32_t r;
                {
                    std::lock_guard<hpx::util::spinlock> l(
                        reflect_test::post_result_mutex);
                    r = reflect_test::post_result;
                }
                if (r != -1)
                    break;
            }
            HPX_TEST_EQ(reflect_test::post_result, std::int32_t(42));
        }

        // hpx::dataflow<^^func> -- dependency-driven dispatch
        {
            hpx::future<std::int32_t> dep =
                hpx::make_ready_future(std::int32_t(41));
            hpx::future<std::int32_t> f =
                hpx::dataflow<^^reflect_test::increment>(loc, std::move(dep));
            HPX_TEST_EQ(f.get(), std::int32_t(42));
        }

        // hpx::async_continue<^^func> -- gid target
        {
            hpx::future<std::int32_t> f =
                hpx::async_continue<^^reflect_test::increment>(
                    [](hpx::future<std::int32_t> fut) { return fut.get(); },
                    loc, std::int32_t(41));
            HPX_TEST_EQ(f.get(), std::int32_t(42));
        }

        // Exception propagation -- hpx::sync<^^func> must rethrow
        {
            bool caught = false;
            try
            {
                hpx::sync<^^reflect_test::always_throws>(loc, std::int32_t(0));
            }
            catch (std::runtime_error const&)
            {
                caught = true;
            }
            HPX_TEST(caught);
        }

        // Exception propagation -- hpx::dataflow<^^func> must rethrow on .get()
        {
            bool caught = false;
            try
            {
                hpx::future<std::int32_t> dep =
                    hpx::make_ready_future(std::int32_t(0));
                hpx::future<std::int32_t> f =
                    hpx::dataflow<^^reflect_test::always_throws>(
                        loc, std::move(dep));
                f.get();
            }
            catch (std::runtime_error const&)
            {
                caught = true;
            }
            HPX_TEST(caught);
        }

        // Verify action executes on the correct locality
        {
            hpx::id_type r = hpx::sync<^^reflect_test::identity>(loc);
            HPX_TEST_EQ(r, loc);
        }
    }

    return hpx::util::report_errors();
}
#else
int main()
{
    return 0;
}
#endif    // !HPX_COMPUTE_DEVICE_CODE && HPX_HAVE_CXX26_REFLECTION
