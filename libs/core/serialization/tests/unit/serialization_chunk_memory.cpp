//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Zero-copy chunks record whether they refer to device memory: the sending
// side marks the chunks it hands over from device memory, the receiving side
// the chunks it receives into device memory. The archives never touch the
// data of such chunks, so host buffers stand in for device memory here.

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using hpx::serialization::chunk_memory;
using hpx::serialization::chunk_type;
using hpx::serialization::serialization_chunk;
namespace detail = hpx::serialization::detail;

constexpr std::size_t threshold = 128;
constexpr std::size_t size = 1024;

// A host chunk and a "device" chunk, the latter received into "device"
// memory as well.
struct two_chunks
{
    std::vector<char> host = std::vector<char>(size, 'h');
    std::vector<char> device = std::vector<char>(size, 'd');

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        ar.save_binary_chunk(
            detail::fundamental_types::none, host.data(), host.size());
        ar.save_binary_chunk(detail::fundamental_types::none, device.data(),
            device.size(), chunk_memory::device);
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        ar.load_binary_chunk(
            detail::fundamental_types::none, host.data(), host.size(), true);
        ar.load_binary_chunk(detail::fundamental_types::none, device.data(),
            device.size(), true, chunk_memory::device);
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

std::vector<serialization_chunk const*> pointer_chunks(
    std::vector<serialization_chunk> const& chunks)
{
    std::vector<serialization_chunk const*> result;
    for (auto const& c : chunks)
    {
        if (c.type_ != chunk_type::chunk_type_index)
            result.push_back(&c);
    }
    return result;
}

int main()
{
    // pointer chunks are host memory unless marked
    HPX_TEST(!hpx::serialization::create_pointer_chunk(nullptr, 1)
            .is_device_memory());
    HPX_TEST(hpx::serialization::create_pointer_chunk(
        static_cast<void*>(nullptr), 1, chunk_memory::device)
            .is_device_memory());

    two_chunks const out;
    std::vector<char> buffer;
    std::vector<serialization_chunk> chunks;
    {
        hpx::serialization::output_archive archive(
            buffer, 0U, &chunks, nullptr, threshold);
        archive << out;
    }

    // the sending side marks the chunk of device memory
    auto const sent = pointer_chunks(chunks);
    HPX_TEST_EQ(sent.size(), std::size_t(2));
    HPX_TEST(!sent[0]->is_device_memory());
    HPX_TEST(sent[1]->is_device_memory());
    HPX_TEST_EQ(sent[1]->data(), static_cast<void const*>(out.device.data()));

    // the receiving side gets the chunks without their memory (it isn't
    // sent) and with no address yet, as the parcelports decode them
    std::vector<serialization_chunk> received;
    for (auto const& c : chunks)
    {
        if (c.type_ == chunk_type::chunk_type_index)
            received.push_back(c);
        else
            received.push_back(
                hpx::serialization::create_pointer_chunk(nullptr, c.size()));
    }

    two_chunks in;
    {
        hpx::serialization::input_archive archive(
            buffer, buffer.size(), &received);
        archive >> in;
    }

    // the receiving side marks the chunk it receives into device memory and
    // places both chunks at the addresses the de-serialization gave
    auto const placed = pointer_chunks(received);
    HPX_TEST_EQ(placed.size(), std::size_t(2));
    HPX_TEST(!placed[0]->is_device_memory());
    HPX_TEST(placed[1]->is_device_memory());
    HPX_TEST_EQ(placed[0]->data(), static_cast<void const*>(in.host.data()));
    HPX_TEST_EQ(placed[1]->data(), static_cast<void const*>(in.device.data()));

    return hpx::util::report_errors();
}
