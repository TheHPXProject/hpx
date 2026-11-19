//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/serialization/detail/pointer.hpp>
#include <hpx/serialization/input_archive.hpp>
#include <hpx/serialization/output_archive.hpp>

#include <cstdint>
#include <map>
#include <utility>

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable
    // across shared libraries.
    extra_data_id_type extra_data_helper<
        serialization::detail::input_pointer_tracker>::id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }

    extra_data_id_type extra_data_helper<
        serialization::detail::output_pointer_tracker>::id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }

    void
    extra_data_helper<serialization::detail::output_pointer_tracker>::reset(
        serialization::detail::output_pointer_tracker* data)
    {
        data->clear();
    }
}    // namespace hpx::util

namespace hpx::serialization {

    void register_pointer(
        input_archive& ar, std::uint64_t pos, detail::ptr_helper_ptr helper)
    {
        auto& tracker = ar.get_extra_data<detail::input_pointer_tracker>();
        HPX_ASSERT(tracker.find(pos) == tracker.end());

        tracker.emplace(std::make_pair(pos, HPX_MOVE(helper)));
    }

    detail::ptr_helper& tracked_pointer(input_archive& ar, std::uint64_t pos)
    {
        auto& tracker = ar.get_extra_data<detail::input_pointer_tracker>();

        auto const it = tracker.find(pos);
        HPX_ASSERT(it != tracker.end());

        return *it->second;
    }

    std::uint64_t track_pointer(output_archive& ar, void const* pos)
    {
        auto& tracker = ar.get_extra_data<detail::output_pointer_tracker>();

        auto const it = tracker.find(pos);
        if (it == tracker.end())
        {
            tracker.emplace(std::make_pair(pos, ar.bytes_written()));
            return static_cast<std::uint64_t>(-1);
        }
        return it->second;
    }
}    // namespace hpx::serialization
