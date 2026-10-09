//  Copyright (c) 2016 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// verify #2338 is fixed (Possible race in sliding semaphore)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

// ----------------------------------------------------------------------------
double message_double(double d)
{
    return d;
}
HPX_PLAIN_ACTION(message_double)

// ----------------------------------------------------------------------------
int main()
{
    // use the first remote locality to bounce messages, if possible
    hpx::id_type here = hpx::find_here();

    hpx::id_type there = here;
    std::vector<hpx::id_type> localities = hpx::find_remote_localities();
    if (!localities.empty())
        there = localities[0];

    std::size_t parcel_count = 0;
    std::size_t loop = 10000;
    std::size_t window_size = 1;
    std::size_t skip = 50;

    std::atomic<std::size_t> signal_count(0);

    hpx::sliding_semaphore sem(static_cast<std::int64_t>(window_size), 0);
    message_double_action msg;
    std::vector<hpx::future<void>> pending;
    pending.reserve((loop * window_size) + skip);

    for (std::size_t i = 0; i < (loop * window_size) + skip; ++i)
    {
        // launch a message to the remote node
        pending.push_back(hpx::async(msg, there, 3.5)
                .then(hpx::launch::sync,
                    // when the message completes, increment our semaphore count
                    // so that N are always in flight
                    [&, parcel_count](hpx::future<double>&&) -> void {
                        ++signal_count;
                        sem.signal(static_cast<std::int64_t>(parcel_count));
                    }));

        //
        ++parcel_count;

        //
        sem.wait(static_cast<std::int64_t>(parcel_count));
    }

    sem.wait(static_cast<std::int64_t>(parcel_count + window_size - 1));

    // Signals may arrive out of order, so the semaphore's high-water mark
    // does not guarantee that every callback has finished using our state.
    hpx::wait_all(pending);

    HPX_TEST_EQ(signal_count, (loop * window_size) + skip);

    return hpx::util::report_errors();
}
#endif
