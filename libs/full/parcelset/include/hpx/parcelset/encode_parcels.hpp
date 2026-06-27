//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011-2015 Thomas Heller
//  Copyright (c) 2007 Richard D Guidry Jr
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011 Katelyn Kufahl
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/parcelset/parcel.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#if ASIO_HAS_BOOST_THROW_EXCEPTION != 0
#include <boost/exception/exception.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    namespace detail {

#if defined(HPX_HAVE_LOGGING)
        ///////////////////////////////////////////////////////////////////
        constexpr char to_digit(int number) noexcept
        {
            char const number_tmp = static_cast<char>(number);
            if (number >= 0 && number <= 9)
            {
                return static_cast<char>(number_tmp + '0');
            }
            return static_cast<char>(number_tmp - 10 + 'A');
        }

        constexpr void convert_byte(
            std::uint8_t b, char* buffer, char const* /* end */) noexcept
        {
            *buffer++ = to_digit((b & 0xF0) >> 4);
            *buffer = to_digit(b & 0x0F);
        }

        template <typename Buffer>
        std::string binary_archive_content(Buffer const& buffer)
        {
            std::string result;
            if (LPT_ENABLED(debug))
            {
                result.reserve(buffer.data_.size() * 2 + 1);
                for (std::uint8_t const byte : buffer.data_)
                {
                    char b[3] = {0};
                    convert_byte(byte, &b[0], &b[3]);
                    result += b;
                }
            }
            return result;
        }
#endif

        template <typename Buffer>
        void encode_finalize(Buffer& buffer, std::size_t arg_size)
        {
            buffer.size_ = buffer.data_.size();
            buffer.data_size_ = arg_size;

#if defined(HPX_HAVE_LOGGING)
            LPT_(debug) << binary_archive_content(buffer);
#endif

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            parcelset::data_point& data = buffer.data_point_;
            data.bytes_ = buffer.data_.size();
            data.raw_bytes_ = arg_size;
#endif
            // prepare chunk data for transmission, the transmission_chunks data
            // first holds all zero-copy, then all non-zero-copy chunk infos
            using transmission_chunk_type =
                typename Buffer::transmission_chunk_type;
            using count_chunks_type = typename Buffer::count_chunks_type;

            std::vector<transmission_chunk_type>& chunks =
                buffer.transmission_chunks_;

            chunks.clear();
            chunks.reserve(buffer.chunks_.size());

            std::size_t index = 0;
            for (serialization::serialization_chunk& c : buffer.chunks_)
            {
                if (c.type_ == serialization::chunk_type::chunk_type_pointer ||
                    c.type_ ==
                        serialization::chunk_type::chunk_type_const_pointer)
                {
                    chunks.push_back(transmission_chunk_type(index, c.size_));
                }
                ++index;
            }

            buffer.num_chunks_ =
                count_chunks_type(static_cast<std::uint32_t>(chunks.size()),
                    static_cast<std::uint32_t>(
                        buffer.chunks_.size() - chunks.size()));
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            data.num_zchunks_ += chunks.size();
            data.num_zchunks_per_msg_max_ =
                (std::max) (data.num_zchunks_per_msg_max_,
                    static_cast<std::int64_t>(chunks.size()));
            for (auto& chunk : chunks)
            {
                data.size_zchunks_total_ += chunk.second;
                data.size_zchunks_max_ = (std::max) (data.size_zchunks_max_,
                    static_cast<std::int64_t>(chunk.second));
            }
#endif

            if (!chunks.empty())
            {
                // the remaining number of chunks are non-zero-copy
                for (serialization::serialization_chunk& c : buffer.chunks_)
                {
                    if (c.type_ == serialization::chunk_type::chunk_type_index)
                    {
                        chunks.push_back(
                            transmission_chunk_type(c.data_.index_, c.size_));
                    }
                }
            }
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename Buffer>
    std::size_t encode_parcels(parcelport& pp, parcel const* ps,
        std::size_t num_parcels, Buffer& buffer, int archive_flags_,
        std::uint64_t max_outbound_size)
    {
        HPX_ASSERT(buffer.data_.empty());

        // collect argument sizes from parcels
        std::size_t arg_size = 0;
        std::size_t parcels_sent = 0;
        std::size_t parcels_size = 1;

        if (num_parcels != static_cast<std::size_t>(-1))
        {
            arg_size = sizeof(std::int64_t);
            parcels_size = num_parcels;
        }

        // guard against serialization errors
        try
        {
            try
            {
                std::unique_ptr<serialization::binary_filter> const filter(
                    ps[0].get_serialization_filter());

                int archive_flags = archive_flags_;
                if (filter)
                {
                    archive_flags = archive_flags |
                        static_cast<int>(
                            serialization::archive_flags::enable_compression);
                }

                // preallocate data
                std::size_t num_chunks = 0;
                for (/**/; parcels_sent != parcels_size; ++parcels_sent)
                {
                    if (arg_size >= max_outbound_size)
                        break;
                    arg_size += ps[parcels_sent].size();
                    num_chunks += ps[parcels_sent].num_chunks();
                }

                buffer.data_.reserve(arg_size);
                buffer.chunks_.reserve(num_chunks);

                // mark start of serialization
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
                hpx::chrono::high_resolution_timer const timer;
#endif
                {
                    // Serialize the data
                    if (filter)
                    {
                        filter->set_max_length(buffer.data_.capacity());
                    }

                    serialization::output_archive archive(buffer.data_,
                        archive_flags, &buffer.chunks_, filter.get(),
                        pp.get_zero_copy_serialization_threshold());

                    if (num_parcels != static_cast<std::size_t>(-1))
                        archive << parcels_sent;    //-V128

                    for (std::size_t i = 0; i != parcels_sent; ++i)
                    {
#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        std::size_t const archive_pos = archive.current_pos();
                        std::int64_t const serialize_time =
                            timer.elapsed_nanoseconds();
#endif
                        LPT_(debug) << ps[i];

                        auto split_gids_map = ps[i].move_split_gids();
                        if (!split_gids_map.empty())
                        {
                            auto& split_gids = archive.get_extra_data<
                                serialization::detail::preprocess_gid_types>();
                            split_gids.set_split_gids(HPX_MOVE(split_gids_map));
                        }

                        archive << ps[i];

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        parcelset::data_point action_data;
                        action_data.bytes_ =
                            archive.current_pos() - archive_pos;
                        action_data.serialization_time_ =
                            timer.elapsed_nanoseconds() - serialize_time;
                        action_data.num_parcels_ = 1;
                        pp.add_sent_data(ps[i].get_action_name(), action_data);
#endif
                    }
                    archive.flush();
                    arg_size = archive.bytes_written();
                }

                // store the time required for serialization
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
                buffer.data_point_.serialization_time_ =
                    timer.elapsed_nanoseconds();
#endif
            }
            catch (hpx::exception const& e)
            {
                LPT_(fatal).format(
                    "encode_parcels: caught hpx::exception: {}", e.what());
                hpx::report_error(std::current_exception());
                return 0;
            }
            catch (std::system_error const& e)
            {
                LPT_(fatal).format(
                    "encode_parcels: caught std::system_error: {}", e.what());
                hpx::report_error(std::current_exception());
                return 0;
            }
#if ASIO_HAS_BOOST_THROW_EXCEPTION != 0
            catch (boost::exception const&)
            {
                LPT_(fatal).format("encode_parcels: caught boost::exception");
                hpx::report_error(std::current_exception());
                return 0;
            }
#endif
            catch (std::exception const& e)
            {
                // We have to repackage all exceptions thrown by the
                // serialization library as otherwise we will loose the e.what()
                // description of the problem, due to slicing.
                hpx::throw_with_info(
                    hpx::exception(hpx::error::serialization_error, e.what()));
            }
        }
        catch (...)
        {
            LPT_(fatal).format("encode_parcels: caught unknown exception");
            hpx::report_error(std::current_exception());
            return 0;
        }

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        buffer.data_point_.num_parcels_ = parcels_sent;
#endif
        detail::encode_finalize(buffer, arg_size);

        return parcels_sent;
    }
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
