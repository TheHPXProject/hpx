//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tuning.hpp>
#include <hpx/parcel_tuning/transfers.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

namespace hpx::tuning::detail {

    // the data of a transfer of the given size: a pattern the receiver
    // checks, the same for both localities
    std::vector<std::uint8_t> const& transfer_pattern(std::size_t const size)
    {
        static std::mutex mtx;
        static std::map<std::size_t, std::vector<std::uint8_t>> patterns;

        std::lock_guard<std::mutex> l(mtx);
        auto it = patterns.find(size);
        if (it == patterns.end())
        {
            std::vector<std::uint8_t> v(size);
            for (std::size_t i = 0; i != size; ++i)
            {
                v[i] = static_cast<std::uint8_t>((i * 131 + 7) & 0xff);
            }
            it = patterns.emplace(size, HPX_MOVE(v)).first;
        }
        return it->second;
    }

    bool matches_pattern(
        std::vector<std::uint8_t> const& v, std::size_t const size)
    {
        if (v.size() != size)
        {
            return false;
        }
        std::vector<std::uint8_t> const& expected = transfer_pattern(size);
        return size == 0 || std::memcmp(v.data(), expected.data(), size) == 0;
    }

    // get: the remote locality returns a vector of the size
    std::vector<std::uint8_t> tuning_transfer_get(std::size_t const size)
    {
        return transfer_pattern(size);
    }

    // set: a vector sent as an argument, checked where it arrives
    bool tuning_transfer_set(
        std::vector<std::uint8_t> const& v, std::size_t const size)
    {
        return matches_pattern(v, size);
    }
}    // namespace hpx::tuning::detail

HPX_PLAIN_ACTION(
    hpx::tuning::detail::tuning_transfer_get, hpx_tuning_transfer_get_action)
HPX_PLAIN_ACTION(
    hpx::tuning::detail::tuning_transfer_set, hpx_tuning_transfer_set_action)

namespace hpx::tuning {

    std::vector<std::size_t> default_transfer_sizes()
    {
        std::vector<std::size_t> sizes;
        for (std::size_t size = 1024; size <= (std::size_t(64) << 20);
            size *= 2)
        {
            sizes.push_back(size);
        }
        return sizes;
    }

    namespace {

        std::uint64_t conduit_chunks()
        {
            parcelset::device_conduit const* conduit =
                parcelset::get_device_conduit();
            if (conduit == nullptr)
            {
                return 0;
            }
            parcelset::device_conduit_statistics const s =
                conduit->statistics();
            return s.chunks_sent + s.chunks_received;
        }

        // the rounds of one size and direction: their times in microseconds,
        // and the rounds whose data arrived wrong
        template <typename Round>
        std::vector<double> run_rounds(
            Round&& round, transfer_options const& options, std::size_t& errors)
        {
            for (std::size_t i = 0; i != options.warmup; ++i)
            {
                if (!round())
                {
                    ++errors;
                }
            }

            std::vector<double> times;
            times.reserve(options.rounds);
            auto const start = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i != options.rounds; ++i)
            {
                auto const t0 = std::chrono::steady_clock::now();
                if (!round())
                {
                    ++errors;
                }
                auto const t1 = std::chrono::steady_clock::now();
                times.push_back(
                    std::chrono::duration<double, std::micro>(t1 - t0).count());

                if (times.size() >= options.min_rounds &&
                    std::chrono::duration<double>(t1 - start).count() >=
                        options.limit_s)
                {
                    break;
                }
            }
            return times;
        }

        // sends vectors to the destination until the conduit moves them, so
        // that its check whether it reaches the destination is done before
        // the measurements; gives up after a few seconds (the conduit
        // doesn't move host chunks, or not to this destination)
        void prime_conduit(hpx::id_type const& remote, std::size_t const size)
        {
            if (parcelset::get_device_conduit() == nullptr)
            {
                return;
            }
            std::vector<std::uint8_t> const& data =
                detail::transfer_pattern(size);
            auto const deadline =
                std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (std::chrono::steady_clock::now() < deadline)
            {
                std::uint64_t const before = conduit_chunks();
                hpx_tuning_transfer_set_action()(remote, data, size);
                if (conduit_chunks() != before)
                {
                    return;
                }
            }
        }
    }    // namespace

    std::vector<transfer_point> measure_transfers(
        hpx::id_type const& remote, transfer_options const& options)
    {
        std::vector<std::size_t> const sizes =
            options.sizes.empty() ? default_transfer_sizes() : options.sizes;

        std::size_t largest = 0;
        for (std::size_t const size : sizes)
        {
            largest = (std::max) (largest, size);
        }
        prime_conduit(remote, (std::min) (largest, std::size_t(1) << 20));

        std::vector<transfer_point> points;
        points.reserve(sizes.size());
        for (std::size_t const size : sizes)
        {
            transfer_point p;
            p.size = size;

            std::uint64_t const chunks_before = conduit_chunks();

            std::vector<double> const get_times = run_rounds(
                [&]() {
                    return detail::matches_pattern(
                        hpx_tuning_transfer_get_action()(remote, size), size);
                },
                options, p.errors);

            std::vector<std::uint8_t> const& data =
                detail::transfer_pattern(size);
            std::vector<double> const set_times = run_rounds(
                [&]() {
                    return hpx_tuning_transfer_set_action()(remote, data, size);
                },
                options, p.errors);

            std::size_t const rounds =
                2 * options.warmup + get_times.size() + set_times.size();
            p.conduit_chunks_per_round =
                static_cast<double>(conduit_chunks() - chunks_before) /
                static_cast<double>(rounds);
            p.get_us = summarize(get_times);
            p.set_us = summarize(set_times);
            if (options.progress)
                options.progress(p);
            points.push_back(p);
        }
        return points;
    }
}    // namespace hpx::tuning

#endif
