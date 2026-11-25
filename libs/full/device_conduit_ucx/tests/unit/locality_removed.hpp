//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The data and the actions of the locality_removed test, used by the test
// (the console) and by the worker localities it launches.

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <vector>

// data whose zero-copy chunk is marked as device memory: the device conduit
// moves it (the archives never touch the data of such chunks), host memory
// stands in for device memory, which UCX moves as well
struct device_like
{
    std::vector<char> data;

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        ar << data.size();
        ar.save_binary_chunk(
            hpx::serialization::detail::fundamental_types::none, data.data(),
            data.size(), hpx::serialization::chunk_memory::device);
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        std::size_t size = 0;
        ar >> size;
        data.resize(size);
        if (ar.next_chunk_from_transport())
        {
            ar.load_binary_chunk(
                hpx::serialization::detail::fundamental_types::none,
                data.data(), size, true,
                hpx::serialization::chunk_memory::device);
        }
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

inline std::vector<char> pattern(std::size_t const size, char const first)
{
    std::vector<char> v(size);
    for (std::size_t i = 0; i != size; ++i)
    {
        v[i] = static_cast<char>(first + static_cast<char>(i % 61));
    }
    return v;
}

// whether the device data arrived intact
inline bool check_device_data(device_like const& d, char const first)
{
    return d.data == pattern(d.data.size(), first);
}
HPX_PLAIN_ACTION(check_device_data, check_device_data_action)

// device data that isn't waited for (the sends of the conduit are all that
// is left of it once the parcel has left)
inline void put_device_data(device_like const&) {}
HPX_PLAIN_ACTION(put_device_data, put_device_data_action)

// host data, moved by the conduit as well (hpx.parcel.conduit_host_chunks)
inline bool check_host_data(std::vector<char> const& d, char const first)
{
    return d == pattern(d.size(), first);
}
HPX_PLAIN_ACTION(check_host_data, check_host_data_action)

inline void put_host_data(std::vector<char> const&) {}
HPX_PLAIN_ACTION(put_host_data, put_host_data_action)

inline bool worker_is_running()
{
    return hpx::is_running();
}
HPX_PLAIN_ACTION(worker_is_running, worker_is_running_action)

// the worker leaves on its own (hpx::disconnect, not from the action itself)
inline void leave()
{
    hpx::post([] { hpx::disconnect(); });
}
HPX_PLAIN_ACTION(leave, leave_action)

#endif
