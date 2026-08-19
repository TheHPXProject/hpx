//  Copyright (c) 2023 Johan511
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is based on https://github.com/Johan511/ByteLock

#pragma once

#include <hpx/assert.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <iterator>
#include <random>
#include <thread>
#include <utility>
#include <vector>

using increments_ty = std::vector<std::pair<std::size_t, std::size_t>>;

std::random_device rnd_device;
std::mt19937 mersenne_engine{rnd_device()};

/*
  thread execution times of each run
*/
namespace hpx::ranged_lock::test::util {
    template <typename RangeEndGen>
    increments_ty get_increment_ranges(
        std::size_t const num_ranges, std::size_t const len, RangeEndGen&& f)
    {
        increments_ty ranges;
        ranges.resize(num_ranges);

        for (std::size_t i = 0; i != num_ranges; i++)
        {
            std::size_t x, y;

            x = mersenne_engine() % len;
            y = f(x, len);

            std::size_t begin = (std::min)(x, y);
            std::size_t end = (std::max)(x, y);

            ranges[i] = std::make_pair(begin, end);
        }

        return ranges;
    }

    template <typename T, typename Modifier>
    std::vector<T> get_final_vector(std::vector<T>&& init_vector,
        increments_ty increments, Modifier&& modifier)
    {
        auto const for_each_unary_func = [&modifier, &init_vector](
                                             auto const& p) {
            std::size_t begin = p.first;
            std::size_t end = p.second;

            modifier(init_vector, begin, end);
        };

        std::for_each(
            increments.begin(), increments.end(), for_each_unary_func);

        return std::move(init_vector);
    }

    /*
    1) Generates increment num_threads * num_incr_per_thread valid ranges
    2) Spawns num_threads threads and assigns all equal amount of work
    3) Checks if result is valid

    NOTE : Critical Section should take care of obtaining the lock
            passed as first parameter
    */
    template <typename RangeMutex, typename RangeEndGen,
        typename CriticalSection>
    void test_lock_once(std::size_t const len, std::size_t const num_threads,
        std::size_t const num_incr_per_thread, RangeEndGen&& range_end_gen,
        CriticalSection&& critical_section)
    {
        std::vector<hpx::thread> threads;
        std::vector<std::size_t> v(len);

        increments_ty increments =
            get_increment_ranges(num_incr_per_thread * num_threads, len,
                std::forward<RangeEndGen>(range_end_gen));

        RangeMutex rmtx;

        for (std::size_t i = 0; i != num_threads; i++)
        {
            increments_ty::iterator start_iter =
                increments.begin() + (i * num_incr_per_thread);

            increments_ty::iterator end_iter =
                increments.begin() + ((i + 1) * num_incr_per_thread);

            threads.emplace_back(
                [&rmtx, &v, &critical_section, start_iter, end_iter]() {
                    increments_ty::iterator it = start_iter;
                    for (; it != end_iter; ++it)
                    {
                        std::size_t begin = it->first;
                        std::size_t end = it->second;

                        std::size_t lockId = rmtx.lock(begin, end);

                        critical_section(v, begin, end);

                        rmtx.unlock(lockId);
                    }
                });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        std::vector<std::size_t> expectedVector = get_final_vector(
            std::vector<std::size_t>(len), std::move(increments),
            std::forward<CriticalSection>(critical_section));

        HPX_ASSERT(v == expectedVector);
    }

    template <typename Lock, typename RangeEndGen, typename CriticalSection>
    void test_lock_n_times(std::size_t const n, std::size_t const len,
        std::size_t const num_threads, std::size_t const num_incr_per_thread,
        RangeEndGen&& range_end_gen, CriticalSection&& critical_section)
    {
        for (std::size_t i = 0; i != n; i++)
        {
            test_lock_once<Lock>(len, num_threads, num_incr_per_thread,
                std::forward<RangeEndGen>(range_end_gen),
                std::forward<CriticalSection>(critical_section));
        }
    }

}    // namespace hpx::ranged_lock::test::util
