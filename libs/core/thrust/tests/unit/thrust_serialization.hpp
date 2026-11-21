//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// What a parcelport does with the archives of a parcel, for
// thrust_serialization.cu: nvcc (13.4) aborts compiling the construction of
// an output_archive or input_archive, the host compiler creates them in
// thrust_serialization_archives.cpp.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace test {

    using save_type = std::function<void(hpx::serialization::output_archive&)>;
    using load_type = std::function<void(hpx::serialization::input_archive&)>;

    // archive flags: zero-copy chunks or not, and whether they may refer to
    // device memory
    std::uint32_t archive_flags(bool chunks, bool device_chunks = false);

    // what the sending side sends
    struct message
    {
        std::vector<char> data;
        std::vector<hpx::serialization::serialization_chunk> chunks;

        // the contents of the zero-copy chunks (copied to the host before
        // the sent object goes away), and the addresses they were sent from
        std::vector<std::vector<char>> chunk_data;
        std::vector<void const*> sources;

        // the buffers the preprocessing created for the zero-copy chunks
        std::size_t keep_alive = 0;
    };

    // Serializes with save(): after preprocessing passes if 'preprocess'
    // (until no futures are left to wait for, as parcel_await_apply does),
    // then into an archive with a chunk list.
    message send(std::uint32_t flags, bool preprocess, save_type const& save);

    // A preprocessing pass of an archive that doesn't keep buffers alive
    // (e.g. the size calculation of a checkpoint): whether save() registered
    // futures to wait for, and the size it calculated.
    struct preprocessed
    {
        bool futures = false;
        std::size_t size = 0;
    };
    preprocessed preprocess_only(std::uint32_t flags, save_type const& save);

    // what the receiving side did
    struct received
    {
        // where the networking layer placed the zero-copy chunks, and how
        // many of them the de-serialization said are device memory
        std::vector<void*> destinations;
        std::size_t device_destinations = 0;

        // the steps the de-serialization deferred until the data had arrived
        std::size_t steps = 0;
    };

    // De-serializes with load(). With zero-copy receive, load() chooses where
    // the chunk data goes, it's placed there afterwards, and the deferred
    // steps run then; otherwise the data has arrived before load() runs.
    // 'device_receive': the parcelport can receive into device memory.
    // 'receive_device': the GPU the receiving side asks for (as for a parcel
    // whose destination has a target), -1 for none.
    received receive(message& m, bool zero_copy_receive, load_type const& load,
        bool device_receive = false, int receive_device = -1);

    // what filtered_round_trip saw
    struct filtered
    {
        // whether both archives said they are filtered
        bool saved_filtered = false;
        bool loaded_filtered = false;

        // the chunks referring to device memory
        std::size_t device_chunks = 0;
    };

    // Serializes with save() into an archive with the given flags and a
    // filter that passes the data through unchanged (as a compressor does,
    // it reads and writes all data of the archive on the host), and
    // de-serializes it with load().
    filtered filtered_round_trip(
        std::uint32_t flags, save_type const& save, load_type const& load);
}    // namespace test
