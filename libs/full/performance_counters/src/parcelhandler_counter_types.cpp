//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset.hpp>

#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/manage_counter_type.hpp>
#include <hpx/performance_counters/parcelhandler_counter_types.hpp>

#include <cstdint>
#include <string>

namespace hpx::performance_counters {

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
    ///////////////////////////////////////////////////////////////////////////
    namespace {

        void register_parcelhandler_counter_types(
            parcelset::parcelhandler& ph, std::string const& pp_type)
        {
            using placeholders::_1;
            using placeholders::_2;

            using parcelset::parcelhandler;
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
            hpx::function<std::int64_t(std::string const&, bool)>
                num_parcel_sends(hpx::bind_front(
                    &parcelhandler::get_action_parcel_send_count, &ph,
                    pp_type));
            hpx::function<std::int64_t(std::string const&, bool)>
                num_parcel_receives(hpx::bind_front(
                    &parcelhandler::get_action_parcel_receive_count, &ph,
                    pp_type));
#else
            hpx::function<std::int64_t(bool)> num_parcel_sends(hpx::bind_front(
                &parcelhandler::get_parcel_send_count, &ph, pp_type));
            hpx::function<std::int64_t(bool)> num_parcel_receives(
                hpx::bind_front(
                    &parcelhandler::get_parcel_receive_count, &ph, pp_type));
#endif

            hpx::function<std::int64_t(bool)> num_message_sends(hpx::bind_front(
                &parcelhandler::get_message_send_count, &ph, pp_type));
            hpx::function<std::int64_t(bool)> num_message_receives(
                hpx::bind_front(
                    &parcelhandler::get_message_receive_count, &ph, pp_type));

            hpx::function<std::int64_t(bool)> sending_time(hpx::bind_front(
                &parcelhandler::get_sending_time, &ph, pp_type));
            hpx::function<std::int64_t(bool)> receiving_time(hpx::bind_front(
                &parcelhandler::get_receiving_time, &ph, pp_type));

#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
            hpx::function<std::int64_t(std::string const&, bool)>
                sending_serialization_time(hpx::bind_front(
                    &parcelhandler::get_action_sending_serialization_time, &ph,
                    pp_type));
            hpx::function<std::int64_t(std::string const&, bool)>
                receiving_serialization_time(hpx::bind_front(
                    &parcelhandler::get_action_receiving_serialization_time,
                    &ph, pp_type));
#else
            hpx::function<std::int64_t(bool)> sending_serialization_time(
                hpx::bind_front(&parcelhandler::get_sending_serialization_time,
                    &ph, pp_type));
            hpx::function<std::int64_t(bool)> receiving_serialization_time(
                hpx::bind_front(
                    &parcelhandler::get_receiving_serialization_time, &ph,
                    pp_type));
#endif

#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
            hpx::function<std::int64_t(std::string const&, bool)> data_sent(
                hpx::bind_front(
                    &parcelhandler::get_action_data_sent, &ph, pp_type));
            hpx::function<std::int64_t(std::string const&, bool)> data_received(
                hpx::bind_front(
                    &parcelhandler::get_action_data_received, &ph, pp_type));
#else
            hpx::function<std::int64_t(bool)> data_sent(
                hpx::bind_front(&parcelhandler::get_data_sent, &ph, pp_type));
            hpx::function<std::int64_t(bool)> data_received(hpx::bind_front(
                &parcelhandler::get_data_received, &ph, pp_type));
#endif

            hpx::function<std::int64_t(bool)> data_raw_sent(hpx::bind_front(
                &parcelhandler::get_raw_data_sent, &ph, pp_type));
            hpx::function<std::int64_t(bool)> data_raw_received(hpx::bind_front(
                &parcelhandler::get_raw_data_received, &ph, pp_type));

            hpx::function<std::int64_t(bool)> buffer_allocate_time_sent(
                hpx::bind_front(&parcelhandler::get_buffer_allocate_time_sent,
                    &ph, pp_type));
            hpx::function<std::int64_t(bool)> buffer_allocate_time_received(
                hpx::bind_front(
                    &parcelhandler::get_buffer_allocate_time_received, &ph,
                    pp_type));

            hpx::function<std::int64_t(bool)> num_zchunks_send(hpx::bind_front(
                &parcelhandler::get_zchunks_send_count, &ph, pp_type));
            hpx::function<std::int64_t(bool)> num_zchunks_recv(hpx::bind_front(
                &parcelhandler::get_zchunks_recv_count, &ph, pp_type));

            hpx::function<std::int64_t(bool)> num_zchunks_send_per_msg_max(
                hpx::bind_front(
                    &parcelhandler::get_zchunks_send_per_msg_count_max, &ph,
                    pp_type));
            hpx::function<std::int64_t(bool)> num_zchunks_recv_per_msg_max(
                hpx::bind_front(
                    &parcelhandler::get_zchunks_recv_per_msg_count_max, &ph,
                    pp_type));

            hpx::function<std::int64_t(bool)> size_zchunks_send(hpx::bind_front(
                &parcelhandler::get_zchunks_send_size, &ph, pp_type));
            hpx::function<std::int64_t(bool)> size_zchunks_recv(hpx::bind_front(
                &parcelhandler::get_zchunks_recv_size, &ph, pp_type));

            hpx::function<std::int64_t(bool)> size_zchunks_send_per_msg_max(
                hpx::bind_front(
                    &parcelhandler::get_zchunks_send_size_max, &ph, pp_type));
            hpx::function<std::int64_t(bool)> size_zchunks_recv_per_msg_max(
                hpx::bind_front(
                    &parcelhandler::get_zchunks_recv_size_max, &ph, pp_type));

            performance_counters::generic_counter_type_data const
                counter_types[] = {
                    {hpx::util::format("/parcels/count/{}/sent", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the number of parcels sent using the {} "
                            "connection type for the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        hpx::bind(&performance_counters::
                                      per_action_data_counter_creator,
                            _1, HPX_MOVE(num_parcel_sends), _2),
                        &performance_counters::
                            per_action_data_counter_discoverer,
#else
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_parcel_sends), _2),
                        &performance_counters::locality_counter_discoverer,
#endif
                        ""},
                    {hpx::util::format("/parcels/count/{}/received", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the number of parcels received using the "
                            "{} "
                            "connection type for the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        hpx::bind(&performance_counters::
                                      per_action_data_counter_creator,
                            _1, HPX_MOVE(num_parcel_receives), _2),
                        &performance_counters::
                            per_action_data_counter_discoverer,
#else
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_parcel_receives), _2),
                        &performance_counters::locality_counter_discoverer,
#endif
                        ""},
                    {hpx::util::format("/messages/count/{}/sent", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the number of messages sent using the {} "
                            "connection type for the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_message_sends), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format("/messages/count/{}/received", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the number of messages received using the "
                            "{} "
                            "connection type for the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_message_receives), _2),
                        &performance_counters::locality_counter_discoverer, ""},

                    {hpx::util::format("/data/time/{}/sent", pp_type),
                        performance_counters::counter_type::elapsed_time,
                        hpx::util::format(
                            "returns the total time between the start of each "
                            "asynchronous write and the invocation of the "
                            "write "
                            "callback using the {} connection type for the "
                            "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(sending_time), _2),
                        &performance_counters::locality_counter_discoverer,
                        "ns"},
                    {hpx::util::format("/data/time/{}/received", pp_type),
                        performance_counters::counter_type::elapsed_time,
                        hpx::util::format(
                            "returns the total time between the start of each "
                            "asynchronous read and the invocation of the read "
                            "callback using the {} connection type for the "
                            "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(receiving_time), _2),
                        &performance_counters::locality_counter_discoverer,
                        "ns"},
                    {hpx::util::format("/serialize/time/{}/sent", pp_type),
                        performance_counters::counter_type::elapsed_time,
                        hpx::util::format(
                            "returns the total time required to serialize all "
                            "sent "
                            "parcels using the {} connection type for the "
                            "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        hpx::bind(&performance_counters::
                                      per_action_data_counter_creator,
                            _1, HPX_MOVE(sending_serialization_time), _2),
                        &performance_counters::
                            per_action_data_counter_discoverer,
#else
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(sending_serialization_time), _2),
                        &performance_counters::locality_counter_discoverer,
#endif
                        "ns"},
                    {hpx::util::format("/serialize/time/{}/received", pp_type),
                        performance_counters::counter_type::elapsed_time,
                        hpx::util::format("returns the total time required to "
                                          "de-serialize all "
                                          "received parcels using the {} "
                                          "connection type for the "
                                          "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        hpx::bind(&performance_counters::
                                      per_action_data_counter_creator,
                            _1, HPX_MOVE(receiving_serialization_time), _2),
                        &performance_counters::
                            per_action_data_counter_discoverer,
#else
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(receiving_serialization_time), _2),
                        &performance_counters::locality_counter_discoverer,
#endif
                        "ns"},

                    {hpx::util::format("/data/count/{}/sent", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the amount of (uncompressed) parcel "
                            "argument "
                            "data sent using the {} connection type by the "
                            "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(data_raw_sent), _2),
                        &performance_counters::locality_counter_discoverer,
                        "bytes"},
                    {hpx::util::format("/data/count/{}/received", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the amount of (uncompressed) parcel "
                            "argument "
                            "data received using the {} connection type by the "
                            "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(data_raw_received), _2),
                        &performance_counters::locality_counter_discoverer,
                        "bytes"},
                    {hpx::util::format("/serialize/count/{}/sent", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the amount of parcel data (including "
                            "headers, "
                            "possibly compressed) sent using the {} connection "
                            "type by the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        hpx::bind(&performance_counters::
                                      per_action_data_counter_creator,
                            _1, HPX_MOVE(data_sent), _2),
                        &performance_counters::
                            per_action_data_counter_discoverer,
#else
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(data_sent), _2),
                        &performance_counters::locality_counter_discoverer,
#endif
                        "bytes"},
                    {hpx::util::format("/serialize/count/{}/received", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format("returns the amount of parcel data "
                                          "(including headers, "
                                          "possibly compressed) received using "
                                          "the {} connection "
                                          "type by the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
                        hpx::bind(&performance_counters::
                                      per_action_data_counter_creator,
                            _1, HPX_MOVE(data_received), _2),
                        &performance_counters::
                            per_action_data_counter_discoverer,
#else
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(data_received), _2),
                        &performance_counters::locality_counter_discoverer,
#endif
                        "bytes"},
                    {hpx::util::format(
                         "/parcels/time/{}/buffer_allocate/received", pp_type),
                        performance_counters::counter_type::elapsed_time,
                        hpx::util::format(
                            "returns the time needed to allocate the buffers "
                            "for "
                            "serializing using the {} connection type",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(buffer_allocate_time_received), _2),
                        &performance_counters::locality_counter_discoverer,
                        "ns"},
                    {hpx::util::format(
                         "/parcels/time/{}/buffer_allocate/sent", pp_type),
                        performance_counters::counter_type::elapsed_time,
                        hpx::util::format(
                            "returns the time needed to allocate the buffers "
                            "for "
                            "serializing using the {} connection type",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(buffer_allocate_time_sent), _2),
                        &performance_counters::locality_counter_discoverer,
                        "ns"},
                    {hpx::util::format(
                         "/parcelport/count/{}/zero_copy_chunks/sent", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the total number of zero-copy chunks sent "
                            "using the {} connection type for the referenced "
                            "locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_zchunks_send), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/zero_copy_chunks/received",
                         pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the total number of zero-copy chunks "
                            "received "
                            "using the {} connection type for the referenced "
                            "locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_zchunks_recv), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count-max/{}/zero_copy_chunks/sent",
                         pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the maximum number of zero-copy chunks "
                            "per "
                            "message sent using the {} connection type for the "
                            "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_zchunks_send_per_msg_max), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count-max/{}/zero_copy_chunks/received",
                         pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format("returns the maximum number of "
                                          "zero-copy chunks per "
                                          "message received using the {} "
                                          "connection type for the "
                                          "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(num_zchunks_recv_per_msg_max), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/size/{}/zero_copy_chunks/sent", pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format("returns the total size of zero-copy "
                                          "chunks sent using "
                                          "the {} connection type for the "
                                          "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(size_zchunks_send), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/size/{}/zero_copy_chunks/received",
                         pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the total size of zero-copy chunks "
                            "received "
                            "using the {} connection type for the referenced "
                            "locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(size_zchunks_recv), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/size-max/{}/zero_copy_chunks/sent",
                         pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the maximum size of zero-copy chunks sent "
                            "using the {} connection type for the referenced "
                            "locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(size_zchunks_send_per_msg_max), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/size-max/{}/zero_copy_chunks/received",
                         pp_type),
                        performance_counters::counter_type::
                            monotonically_increasing,
                        hpx::util::format(
                            "returns the maximum size of zero-copy chunks "
                            "received "
                            "using the {} connection type for the referenced "
                            "locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(size_zchunks_recv_per_msg_max), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                };

            performance_counters::install_counter_types(
                counter_types, std::size(counter_types));
        }

        ///////////////////////////////////////////////////////////////////////
        // register connection specific performance counters related to
        // connection caches
        void register_connection_cache_counter_types(
            parcelset::parcelhandler& ph, std::string const& pp_type)
        {
            using hpx::placeholders::_1;
            using hpx::placeholders::_2;

            using parcelset::parcelhandler;
            using parcelset::parcelport;

            hpx::function<std::int64_t(bool)> cache_insertions(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type, parcelport::connection_cache_insertions));
            hpx::function<std::int64_t(bool)> cache_evictions(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type, parcelport::connection_cache_evictions));
            hpx::function<std::int64_t(bool)> cache_hits(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type, parcelport::connection_cache_hits));
            hpx::function<std::int64_t(bool)> cache_misses(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type, parcelport::connection_cache_misses));
            hpx::function<std::int64_t(bool)> cache_reclaims(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type, parcelport::connection_cache_reclaims));
            hpx::function<std::int64_t(bool)> cache_reservation_failures(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type,
                    parcelport::connection_cache_reservation_failures));
            hpx::function<std::int64_t(bool)> cache_num_connections(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type,
                    parcelport::connection_cache_num_connections));
            hpx::function<std::int64_t(bool)> cache_max_connections(
                hpx::bind_front(&parcelhandler::get_connection_cache_statistics,
                    &ph, pp_type,
                    parcelport::connection_cache_max_connections));

            performance_counters::generic_counter_type_data const
                connection_cache_types[] = {
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-insertions", pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format(
                            "returns the number of cache insertions while "
                            "accessing the connection cache for the {} "
                            "connection "
                            "type on the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_insertions), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-evictions", pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format("returns the number of cache "
                                          "evictions while accessing "
                                          "the connection cache for the {} "
                                          "connection type on "
                                          "the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_evictions), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-hits", pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format("returns the number of cache hits "
                                          "while accessing the "
                                          "connection cache for the {} "
                                          "connection type on the "
                                          "referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_hits), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-misses", pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format("returns the number of cache misses "
                                          "while accessing "
                                          "the connection cache for the {} "
                                          "connection type on "
                                          "the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_misses), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-reclaims", pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format("returns the number of cache "
                                          "reclaims while accessing "
                                          "the connection cache for the {} "
                                          "connection type on "
                                          "the referenced locality",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_reclaims), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-reservation-failures",
                         pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format(
                            "returns the number of times a connection "
                            "reservation "
                            "failed because the {} connection cache was fully "
                            "checked out (pool saturation); parcels are "
                            "silently "
                            "deferred on each such event",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_reservation_failures), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-connections", pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format(
                            "returns the current number of connections tracked "
                            "by "
                            "the {} connection cache (in-use plus idle); "
                            "compare "
                            "with cache-max-connections to compute utilisation",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_num_connections), _2),
                        &performance_counters::locality_counter_discoverer, ""},
                    {hpx::util::format(
                         "/parcelport/count/{}/cache-max-connections", pp_type),
                        performance_counters::counter_type::raw,
                        hpx::util::format("returns the configured maximum "
                                          "number of connections "
                                          "for the {} connection cache "
                                          "(hpx.max_connections ini "
                                          "key); fixed at startup",
                            pp_type),
                        HPX_PERFORMANCE_COUNTER_V1,
                        hpx::bind(
                            &performance_counters::locality_raw_counter_creator,
                            _1, HPX_MOVE(cache_max_connections), _2),
                        &performance_counters::locality_counter_discoverer,
                        ""}};

            performance_counters::install_counter_types(
                connection_cache_types, std::size(connection_cache_types));
        }
    }    // namespace
#endif

    ///////////////////////////////////////////////////////////////////////////
    void register_parcelhandler_counter_types(
        [[maybe_unused]] parcelset::parcelhandler& ph)
    {
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        if (!ph.is_networking_enabled())
        {
            return;
        }

        // register connection specific counters
        ph.enum_parcelports([&](std::string const& type) -> bool {
            register_parcelhandler_counter_types(ph, type);
            register_connection_cache_counter_types(ph, type);
            return true;
        });

        using placeholders::_1;
        using placeholders::_2;

        using parcelset::parcelhandler;

        // register common counters
        hpx::function<std::int64_t(bool)> incoming_queue_length(
            hpx::bind_front(&parcelhandler::get_incoming_queue_length, &ph));
        hpx::function<std::int64_t(bool)> outgoing_queue_length(
            hpx::bind_front(&parcelhandler::get_outgoing_queue_length, &ph));
        hpx::function<std::int64_t(bool)> outgoing_routed_count(
            hpx::bind_front(&parcelhandler::get_parcel_routed_count, &ph));

        performance_counters::generic_counter_type_data const counter_types[] =
            {{"/parcelqueue/length/receive",
                 performance_counters::counter_type::raw,
                 "returns the number current length of the queue of "
                 "incoming "
                 "parcels",
                 HPX_PERFORMANCE_COUNTER_V1,
                 hpx::bind(&performance_counters::locality_raw_counter_creator,
                     _1, incoming_queue_length, _2),
                 &performance_counters::locality_counter_discoverer, ""},
                {"/parcelqueue/length/send",
                    performance_counters::counter_type::raw,
                    "returns the number current length of the queue of "
                    "outgoing parcels",
                    HPX_PERFORMANCE_COUNTER_V1,
                    hpx::bind(
                        &performance_counters::locality_raw_counter_creator, _1,
                        outgoing_queue_length, _2),
                    &performance_counters::locality_counter_discoverer, ""},
                {"/parcels/count/routed",
                    performance_counters::counter_type::
                        monotonically_increasing,
                    "returns the number of (outbound) parcel routed through "
                    "the responsible AGAS service",
                    HPX_PERFORMANCE_COUNTER_V1,
                    hpx::bind(
                        &performance_counters::locality_raw_counter_creator, _1,
                        outgoing_routed_count, _2),
                    &performance_counters::locality_counter_discoverer, ""}};

        performance_counters::install_counter_types(
            counter_types, std::size(counter_types));
#endif
    }
}    // namespace hpx::performance_counters

#endif
