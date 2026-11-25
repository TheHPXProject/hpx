//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Data large enough to go into a zero-copy chunk must still arrive when the
// archive has a filter (e.g. compression): the filter applies to the whole
// stream, so the reader reads that data from the filtered stream.

#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

// A filter that passes the data through unchanged.
struct identity_filter : hpx::serialization::binary_filter
{
    void set_max_length(std::size_t const size) override
    {
        buffer_.reserve(size);
    }

    void save(void const* src, std::size_t const src_count) override
    {
        char const* begin = static_cast<char const*>(src);
        buffer_.insert(buffer_.end(), begin, begin + src_count);
    }

    bool flush(
        void* dst, std::size_t const dst_count, std::size_t& written) override
    {
        // like a compressor that needs more room than the input (snappy):
        // asks for a larger container first
        if (needs_room_ && dst_count < 2 * (buffer_.size() - current_))
        {
            written = 0;
            return false;
        }
        written = (std::min) (dst_count, buffer_.size() - current_);
        std::memcpy(dst, buffer_.data() + current_, written);
        current_ += written;
        return current_ == buffer_.size();
    }

    std::size_t init_data(void const* buffer, std::size_t const size,
        std::size_t const buffer_size) override
    {
        char const* begin = static_cast<char const*>(buffer);
        buffer_.assign(begin, begin + size);
        current_ = 0;
        return buffer_size;
    }

    void load(void* dst, std::size_t const dst_count) override
    {
        HPX_TEST(current_ + dst_count <= buffer_.size());
        std::memcpy(dst, buffer_.data() + current_, dst_count);
        current_ += dst_count;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }

    HPX_SERIALIZATION_POLYMORPHIC(identity_filter, override);

    void needs_room(bool const value) noexcept
    {
        needs_room_ = value;
    }

private:
    std::vector<char> buffer_;
    std::size_t current_ = 0;
    bool needs_room_ = false;
};

int main()
{
    using hpx::serialization::archive_flags;

    // larger than the zero-copy threshold given to the archive below
    std::vector<std::uint64_t> data(4096);
    std::iota(data.begin(), data.end(), std::uint64_t(42));

    for (int const kind : {0, 1, 2})
    {
        // without filter, with filter, with a filter needing more room
        bool const with_filter = kind != 0;

        std::vector<char> buffer;
        std::vector<hpx::serialization::serialization_chunk> chunks;
        identity_filter filter;
        filter.needs_room(kind == 2);

        std::uint32_t const flags = static_cast<std::uint32_t>(
            std::endian::native == std::endian::big ?
                archive_flags::endian_big | archive_flags::enable_compression :
                archive_flags::endian_little |
                    archive_flags::enable_compression);

        std::size_t size = 0;
        {
            hpx::serialization::output_archive oarchive(
                buffer, flags, &chunks, with_filter ? &filter : nullptr, 128);
            oarchive << data;
            oarchive.flush();
            size = oarchive.bytes_written();
        }

        hpx::serialization::input_archive iarchive(buffer, size, &chunks);
        std::vector<std::uint64_t> received;
        iarchive >> received;

        HPX_TEST(received == data);

        if (with_filter)
        {
            // the filtered data replaces the zero-copy chunk: the container
            // holds all of it (the identity filter keeps the size), nothing
            // more
            HPX_TEST_EQ(buffer.size(), size);
            HPX_TEST_EQ(chunks.size(), std::size_t(1));
        }
    }

    return hpx::util::report_errors();
}
