//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2014-2021 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/modules/parcelset_base.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT std::vector<serialization::serialization_chunk>
    decode_chunks(parcel_buffer& buffer);

    HPX_CXX_EXPORT HPX_EXPORT std::vector<serialization::serialization_chunk>
    decode_chunks_zero_copy(parcel_buffer& buffer);

    HPX_CXX_EXPORT HPX_EXPORT void handle_received_parcels(
        std::vector<parcelset::parcel>&& deferred_parcels,
        std::size_t num_thread = -1);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT using add_received_data_cb = hpx::move_only_function<void(
        char const*, parcelset::data_point const&)>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT std::vector<parcelset::parcel>
    decode_message_with_chunks(add_received_data_cb&& add_received_data,
        parcel_buffer buffer, std::size_t parcel_count,
        std::vector<serialization::serialization_chunk>& chunks,
        std::size_t num_thread = -1);

    HPX_CXX_EXPORT HPX_EXPORT std::vector<parcelset::parcel> decode_message(
        add_received_data_cb&& add_received_data, parcel_buffer buffer,
        std::size_t parcel_count, std::size_t num_thread = -1);

    HPX_CXX_EXPORT template <typename Parcelport>
    std::vector<parcelset::parcel> decode_parcel(
        [[maybe_unused]] Parcelport& parcelport, parcel_buffer&& buffer,
        std::size_t const num_thread = -1)
    {
        return decode_message(
#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
            [&](char const* action_name, parcelset::data_point const& data) {
                parcelport.add_received_data(action_name, data);
            },
#else
            nullptr,
#endif
            HPX_MOVE(buffer), 1, num_thread);
    }

    HPX_CXX_EXPORT template <typename Parcelport>
    std::vector<parcelset::parcel> decode_parcels(
        [[maybe_unused]] Parcelport& parcelport, parcel_buffer&& buffer,
        std::size_t const num_thread = -1)
    {
        return decode_message(
#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
            [&](char const* action_name, parcelset::data_point const& data) {
                parcelport.add_received_data(action_name, data);
            },
#else
            nullptr,
#endif
            HPX_MOVE(buffer), 0, num_thread);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT std::vector<parcelset::parcel>
    decode_message_with_chunks_zero_copy(
        add_received_data_cb&& add_received_data, parcel_buffer& buffer,
        std::size_t parcel_count,
        std::vector<serialization::serialization_chunk>& chunks,
        std::size_t num_thread = -1);

    HPX_CXX_EXPORT HPX_EXPORT std::vector<parcelset::parcel>
    decode_message_zero_copy(add_received_data_cb&& add_received_data,
        parcel_buffer& buffer, std::size_t parcel_count,
        std::size_t num_thread = -1);

    HPX_CXX_EXPORT template <typename Parcelport>
    std::vector<parcelset::parcel> decode_parcel_zero_copy(
        [[maybe_unused]] Parcelport& parcelport, parcel_buffer& buffer,
        std::size_t const num_thread = -1)
    {
        return decode_message_zero_copy(
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            [&](char const* action_name, parcelset::data_point const& data) {
                parcelport.add_received_data(action_name, data);
            },
#else
            nullptr,
#endif
            buffer, 1, num_thread);
    }

    HPX_CXX_EXPORT template <typename Parcelport>
    std::vector<parcelset::parcel> decode_parcels_zero_copy(
        [[maybe_unused]] Parcelport& parcelport, parcel_buffer& buffer,
        std::size_t const num_thread = -1)
    {
        return decode_message_zero_copy(
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            [&](char const* action_name, parcelset::data_point const& data) {
                parcelport.add_received_data(action_name, data);
            },
#else
            nullptr,
#endif
            buffer, 0, num_thread);
    }
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
