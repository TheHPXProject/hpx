//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2011 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/async.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/itt_notify.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <vector>

using hpx::program_options::options_description;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::find_here;

using hpx::id_type;

using hpx::async;
using hpx::future;

using hpx::chrono::high_resolution_timer;

using hpx::cout;

#define N 100

///////////////////////////////////////////////////////////////////////////////
// we use globals here to prevent the delay from being optimized away
double global_scratch = 0;
double global_init[N] = {0};
std::uint64_t num_iterations = 0;

std::size_t k1 = 0;
std::size_t k2 = 0;

namespace test {
    struct local_spinlock
    {
    private:
        std::atomic<bool> v_;

        ///////////////////////////////////////////////////////////////////////////
        static void yield(std::size_t k)
        {
            if (k < k1)
            {
            }
            if (k < k2)
            {
                HPX_SMT_PAUSE;
            }
            else if (hpx::threads::get_self_ptr())
            {
                hpx::this_thread::suspend();
            }
            else
            {
#if defined(HPX_WINDOWS)
                Sleep(0);
#else
                // g++ -Wextra warns on {} or {0}
                struct timespec rqtp = {0, 0};

                // POSIX says that timespec has tv_sec and tv_nsec
                // But it doesn't guarantee order or placement

                rqtp.tv_sec = 0;
                rqtp.tv_nsec = 1000;

                nanosleep(&rqtp, nullptr);
#endif
            }
        }

    public:
        local_spinlock()
          : v_(0)
        {
            HPX_ITT_SYNC_CREATE(this, "test::local_spinlock", "");
        }

        local_spinlock(local_spinlock const&) = delete;
        local_spinlock& operator=(local_spinlock const&) = delete;

        ~local_spinlock()
        {
            HPX_ITT_SYNC_DESTROY(this);
        }

        void lock()
        {
            HPX_ITT_SYNC_PREPARE(this);

            do
            {
                hpx::util::yield_while<true>(
                    [this]() noexcept { return is_locked(); },
                    "test::local_spinlock::lock");
            } while (!acquire_lock());

            HPX_ITT_SYNC_ACQUIRED(this);
            hpx::util::register_lock(this);
        }

        bool try_lock()
        {
            HPX_ITT_SYNC_PREPARE(this);

            bool r = acquire_lock();    //-V707

            if (r)
            {
                HPX_ITT_SYNC_ACQUIRED(this);
                hpx::util::register_lock(this);
                return true;
            }

            HPX_ITT_SYNC_CANCEL(this);
            return false;
        }

        void unlock()
        {
            HPX_ITT_SYNC_RELEASING(this);

            relinquish_lock();

            HPX_ITT_SYNC_RELEASED(this);
            hpx::util::unregister_lock(this);
        }

    private:
        // returns whether the mutex has been acquired
        HPX_FORCEINLINE bool acquire_lock()
        {
            return !v_.exchange(true, std::memory_order_acquire);
        }

        // relinquish lock
        HPX_FORCEINLINE void relinquish_lock()
        {
            v_.store(false, std::memory_order_release);
        }

        HPX_FORCEINLINE bool is_locked() const
        {
            return v_.load(std::memory_order_relaxed);
        }
    };
}    // namespace test

test::local_spinlock mtx[N];

///////////////////////////////////////////////////////////////////////////////
double null_function(std::size_t i)
{
    double d = 0.;
    std::size_t idx = i % N;
    {
        std::lock_guard<test::local_spinlock> l(mtx[idx]);
        d = global_init[idx];
    }
    for (std::uint64_t j = 0.; j < num_iterations; ++j)
    {
        d += 1. / (2. * static_cast<double>(j) + 1.);
    }
    {
        std::lock_guard<test::local_spinlock> l(mtx[idx]);
        global_init[idx] = d;
    }
    return d;
}

HPX_PLAIN_ACTION(null_function, null_action)

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    {
        num_iterations = vm["delay-iterations"].as<std::uint64_t>();

        std::uint64_t const count = vm["futures"].as<std::uint64_t>();

        k1 = vm["k1"].as<std::size_t>();
        k2 = vm["k2"].as<std::size_t>();

        id_type const here = find_here();

        if (HPX_UNLIKELY(0 == count))
            throw std::logic_error("error: count of 0 futures specified\n");

        std::vector<future<double>> futures;

        futures.reserve(count);

        // start the clock

        /*
        k1 = 1;
        k2 = 2;
        for(; k1 < 256; k1 = k1 * 2)*/
        {
            //for(; k2 < 2056; k2 = k2 * 2)
            {
                high_resolution_timer walltime;
                for (std::uint64_t i = 0; i < count; ++i)
                    futures.push_back(async<null_action>(here, i));

                hpx::wait_each(
                    hpx::unwrapping([](double r) { global_scratch += r; }),
                    futures);

                // stop the clock
                double const duration = walltime.elapsed();

                if (vm.count("csv"))
                    hpx::util::format_to(
                        cout, "{3},{4},{2}\n", count, duration, k1, k2)
                        << std::flush;
                else
                    hpx::util::format_to(cout,
                        "invoked {1} futures in {2} seconds "
                        "(k1 = {3}, k2 = {4})\n",
                        count, duration, k1, k2)
                        << std::flush;
                hpx::util::print_cdash_timing("Spinlock1", duration);
            }
        }
    }

    hpx::finalize();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options.
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("futures",
        value<std::uint64_t>()->default_value(500000),
        "number of futures to invoke")

        ("delay-iterations", value<std::uint64_t>()->default_value(0),
            "number of iterations in the delay loop")

            ("k1", value<std::size_t>()->default_value(32), "")

                ("k2", value<std::size_t>()->default_value(256), "")

                    ("csv", "output results as csv (format: count,duration)");

    // Initialize and run HPX.
    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::init(argc, argv, init_args);
}
#endif
