//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <utility>

namespace hpx::serialization::detail {

    // A buffer a serialize() function hands to the archive as a zero-copy
    // chunk although the serialized object doesn't own that memory, e.g. a
    // host copy of device memory. 'owner' keeps the memory alive.
    HPX_CXX_CORE_EXPORT struct keep_alive_buffer
    {
        std::shared_ptr<void> owner;
        void const* data = nullptr;
        std::size_t size = 0;
    };

    // Buffers by the address of the object they were created for.
    HPX_CXX_CORE_EXPORT using keep_alive_map =
        std::map<void const*, keep_alive_buffer>;

    // This class carries keep-alive buffers from the preprocessing pass of a
    // parcel to its real serialization pass. The preprocessing pass creates
    // them (and may register futures for their contents), the parcel holds
    // them until it has been sent, and the real pass finds them again by the
    // address of the serialized object. The owner of the preprocessing
    // archive creates it before serializing, to say that it does this (and
    // waits for the futures before the real pass); serialization creates no
    // buffers for archives without it.
    HPX_CXX_CORE_EXPORT class preprocess_keep_alive
    {
    public:
        preprocess_keep_alive() = default;

        void add(void const* key, keep_alive_buffer buffer)
        {
            buffers_.insert_or_assign(key, HPX_MOVE(buffer));
        }

        [[nodiscard]] keep_alive_buffer const* find(
            void const* key) const noexcept
        {
            auto const it = buffers_.find(key);
            return it != buffers_.end() ? &it->second : nullptr;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return buffers_.empty();
        }

        // hand the buffers to the parcel after preprocessing; none are left
        // here (the archive may preprocess the next parcel)
        [[nodiscard]] keep_alive_map move_buffers() noexcept
        {
            return std::exchange(buffers_, keep_alive_map{});
        }

        // make the buffers of a parcel available to its serialization; the
        // parcel keeps its own references
        void add_buffers(keep_alive_map const& buffers)
        {
            for (auto const& [key, buffer] : buffers)
            {
                buffers_.insert_or_assign(key, buffer);
            }
        }

    private:
        keep_alive_map buffers_;
    };
}    // namespace hpx::serialization::detail

// This is explicitly instantiated to ensure that the id is stable across shared
// libraries. Resetting the archive keeps the buffers: preprocessing a parcel
// may take several rounds, and each round must find the buffers of the
// previous one.
template <>
struct hpx::util::extra_data_helper<
    hpx::serialization::detail::preprocess_keep_alive>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static constexpr void reset(
        serialization::detail::preprocess_keep_alive*) noexcept
    {
    }
};
