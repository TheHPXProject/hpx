//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c) 2007-2026 Hartmut Kaiser
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
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/parcelset_base/detail/data_point.hpp>
#include <hpx/parcelset_base/detail/gatherer.hpp>
#include <hpx/parcelset_base/detail/per_action_data_counter.hpp>
#include <hpx/parcelset_base/locality.hpp>
#include <hpx/parcelset_base/parcel_interface.hpp>
#include <hpx/parcelset_base/parcelset_base_fwd.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    /// The parcelport is the lowest possible representation of the parcel
    /// interface inside a locality. It provides the minimal functionality
    /// to send and to receive parcels.
    HPX_CXX_EXPORT class HPX_EXPORT parcelport
      : public std::enable_shared_from_this<parcelport>
    {
        parcelport(parcelport const&) = delete;
        parcelport(parcelport&&) = delete;
        parcelport& operator=(parcelport const&) = delete;
        parcelport& operator=(parcelport&&) = delete;

    public:
        using write_handler_type = parcel_write_handler_type;

        using read_handler_type = hpx::function<void(parcelport& pp,
            std::shared_ptr<std::vector<char>>, threads::thread_priority)>;

        /// Construct the parcelport on the given locality.
        parcelport(util::runtime_configuration const& ini, locality here,
            std::string const& type,
            std::size_t zero_copy_serialization_threshold);

        /// Virtual destructor
        virtual ~parcelport() = default;

        virtual bool can_bootstrap() const = 0;

        /// Access the parcelport priority (negative if disabled)
        int priority() const noexcept;

        /// Retrieve the type of the locality represented by this parcelport
        std::string const& type() const noexcept;

        /// This accessor returns a reference to the locality this parcelport
        /// is associated with.
        locality const& here() const noexcept;

        /// Return the threshold to use for deciding whether to zero-copy
        // serialize an entity
        std::size_t get_zero_copy_serialization_threshold() const noexcept;

        /// Start the parcelport I/O thread pool.
        ///
        /// \param blocking [in] If blocking is set to \a true the routine will
        ///                 not return before stop() has been called, otherwise
        ///                 the routine returns immediately.
        virtual bool run(bool blocking = true) = 0;

        /// Notify the parcelport that the parcel layer has been initialized
        /// globally. This function is being called after the early parcels have
        /// been processed and normal operation is about to start.
        virtual void initialized();

        virtual void flush_parcels() = 0;

        /// Stop the parcelport I/O thread pool.
        ///
        /// \param blocking [in] If blocking is set to \a false the routine will
        ///                 return immediately, otherwise it will wait for all
        ///                 worker threads to exit.
        virtual void stop(bool blocking = true) = 0;

        /// Check if this parcelport can connect to this locality
        ///
        /// The default is to return true if it can be used at bootstrap or alternative
        /// parcelports are enabled.
        virtual bool can_connect(
            locality const&, bool use_alternative_parcelport);

        /// Queues a parcel for transmission to another locality
        ///
        /// \note The function put_parcel() is asynchronous, the provided
        /// function or function object gets invoked on completion of the send
        /// operation or on any error.
        ///
        /// \param dest     [in] Where should the parcel be sent to
        /// \param p        [in] A reference to the parcel to send.
        /// \param f        [in] A function object to be invoked on successful
        ///                 completion or on errors. The signature of this
        ///                 function object is expected to be:
        ///
        /// \code
        ///      void handler(std::error_code const& err,
        ///                   std::size_t bytes_written);
        /// \endcode
        virtual void put_parcel(
            locality const& dest, parcel p, write_handler_type f) = 0;

        /// Queues a list of parcels for transmission to another locality
        ///
        /// \note The function put_parcels() is asynchronous, the provided
        /// functions or function objects get invoked on completion of the send
        /// operation or on any error.
        ///
        /// \param dest     [in] Where should the parcels be sent to
        /// \param parcels  [in] A reference to the list of parcels to send.
        /// \param handlers [in] A list of function objects to be invoked on
        ///                 successful completion or on errors. The signature of
        ///                 these function objects is expected to be:
        ///
        /// \code
        ///      void handler(std::error_code const& err,
        ///                   std::size_t bytes_written);
        /// \endcode
        virtual void put_parcels(locality const& dest,
            std::vector<parcel> parcels,
            std::vector<write_handler_type> handlers) = 0;

        /// Send an early parcel through the TCP parcelport
        ///
        /// \param dest     [in] Where should the parcels be sent to
        /// \param p        [in, out] A reference to the parcel to send. The
        ///                 parcel \a p will be modified in place, as it will
        ///                 get set the resolved destination address and parcel
        ///                 id (if not already set).
        virtual void send_early_parcel(locality const& dest, parcel p) = 0;

        /// Cache specific functionality
        virtual void remove_from_connection_cache(locality const& loc) = 0;

        /// Return the thread pool if the name matches
        virtual util::io_service_pool* get_thread_pool(char const* name) = 0;

        /// Return the given connection cache statistic
        enum connection_cache_statistics_type
        {
            connection_cache_insertions = 0,
            connection_cache_evictions = 1,
            connection_cache_hits = 2,
            connection_cache_misses = 3,
            connection_cache_reclaims = 4,
            // Number of get_or_reserve() calls that returned false because all
            // connection slots were checked out (pool saturation, distinct from
            // normal cache misses which do not cause parcel deferral).
            connection_cache_reservation_failures = 5,
            // Current number of connections tracked by the cache (in-use +
            // available). Use with connection_cache_max_connections to compute
            // pool utilisation.
            connection_cache_num_connections = 6,
            // Configured maximum total connections (hpx.max_connections ini
            // key). Fixed at startup; returned without reset semantics.
            connection_cache_max_connections = 7
        };

        // invoke pending background work
        virtual bool do_background_work(
            std::size_t num_thread, parcelport_background_mode mode) = 0;

        // retrieve performance counter value for given statistics type
        virtual std::int64_t get_connection_cache_statistics(
            connection_cache_statistics_type, bool reset) = 0;

        /// Return the name of this locality
        virtual std::string get_locality_name() const = 0;

        virtual locality create_locality() const = 0;

        virtual locality agas_locality(
            util::runtime_configuration const& ini) const = 0;

        /// Performance counter data
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        /// number of parcels sent
        std::int64_t get_parcel_send_count(bool reset);

        /// number of messages sent
        std::int64_t get_message_send_count(bool reset);

        /// number of parcels received
        std::int64_t get_parcel_receive_count(bool reset);

        /// number of messages received
        std::int64_t get_message_receive_count(bool reset);

        /// the total time it took for all sends, from async_write to the
        /// completion handler (nanoseconds)
        std::int64_t get_sending_time(bool reset);

        /// the total time it took for all receives, from async_read to the
        /// completion handler (nanoseconds)
        std::int64_t get_receiving_time(bool reset);

        /// the total time it took for all sender-side serialization operations
        /// (nanoseconds)
        std::int64_t get_sending_serialization_time(bool reset);

        /// the total time it took for all receiver-side serialization
        /// operations (nanoseconds)
        std::int64_t get_receiving_serialization_time(bool reset);

        /// total data sent (bytes)
        std::int64_t get_data_sent(bool reset);

        /// total data (uncompressed) sent (bytes)
        std::int64_t get_raw_data_sent(bool reset);

        /// total data received (bytes)
        std::int64_t get_data_received(bool reset);

        /// total data (uncompressed) received (bytes)
        std::int64_t get_raw_data_received(bool reset);

        std::int64_t get_buffer_allocate_time_sent(bool reset);
        std::int64_t get_buffer_allocate_time_received(bool reset);

        //// total zero-copy chunks sent
        std::int64_t get_zchunks_send_count(bool reset);

        //// total zero-copy chunks received
        std::int64_t get_zchunks_recv_count(bool reset);

        //// the maximum number of zero-copy chunks per message sent
        std::int64_t get_zchunks_send_per_msg_count_max(bool reset);

        //// the maximum number of zero-copy chunks per message received
        std::int64_t get_zchunks_recv_per_msg_count_max(bool reset);

        //// the size of zero-copy chunks per message sent
        std::int64_t get_zchunks_send_size(bool reset);

        //// the size of zero-copy chunks per message received
        std::int64_t get_zchunks_recv_size(bool reset);

        //// the maximum size of zero-copy chunks per message sent
        std::int64_t get_zchunks_send_size_max(bool reset);

        //// the maximum size of zero-copy chunks per message received
        std::int64_t get_zchunks_recv_size_max(bool reset);
#endif
#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
        // same as above, just separated data for each action
        // number of parcels sent
        std::int64_t get_action_parcel_send_count(
            std::string const&, bool reset);

        // number of parcels received
        std::int64_t get_action_parcel_receive_count(
            std::string const&, bool reset);

        // the total time it took for all sender-side serialization operations
        // (nanoseconds)
        std::int64_t get_action_sending_serialization_time(
            std::string const&, bool reset);

        // the total time it took for all receiver-side serialization
        // operations (nanoseconds)
        std::int64_t get_action_receiving_serialization_time(
            std::string const&, bool reset);

        // total data sent (bytes)
        std::int64_t get_action_data_sent(std::string const&, bool reset);

        // total data received (bytes)
        std::int64_t get_action_data_received(std::string const&, bool reset);
#endif
        std::int64_t get_pending_parcels_count(bool /*reset*/);

        ///////////////////////////////////////////////////////////////////////
        /// Update performance counter data
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        void add_received_data(parcelset::data_point const& data);

        void add_sent_data(parcelset::data_point const& data);
#endif
#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
        void add_received_data(
            char const* action, parcelset::data_point const& data);

        void add_sent_data(
            char const* action, parcelset::data_point const& data);
#endif

        /// Return the configured maximal allowed inbound message data
        /// size
        std::int64_t get_max_inbound_message_size() const noexcept;

        /// Return the configured maximal allowed outbound message data
        /// size
        std::int64_t get_max_outbound_message_size() const noexcept;

        /// Return whether it is allowed to apply array optimizations
        bool allow_array_optimizations() const noexcept;

        /// Return whether it is allowed to apply zero copy optimizations
        bool allow_zero_copy_optimizations() const noexcept;

        /// Return whether it is allowed to apply zero copy optimizations on the
        /// receiving end
        bool allow_zero_copy_receive_optimizations() const noexcept;

        bool async_serialization() const noexcept;

        // callback while bootstrap the parcel layer
        static void early_pending_parcel_handler(
            std::error_code const& ec, parcel const& p);

    protected:
        // mutex for all the member data
        mutable hpx::spinlock mtx_;

        using pending_parcels_destinations = std::set<locality>;
        std::atomic<std::uint32_t> num_parcel_destinations_;
        pending_parcels_destinations parcel_destinations_;

        // The cache for pending parcels
        using map_second_type =
            hpx::tuple<std::vector<parcel>, std::vector<write_handler_type>>;
        using pending_parcels_map = std::map<locality, map_second_type>;
        pending_parcels_map pending_parcels_;

        // The local locality
        locality here_;

        // The maximally allowed message size
        std::int64_t max_inbound_message_size_;
        std::int64_t max_outbound_message_size_;

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        // Overall parcel statistics
        parcelset::gatherer parcels_sent_;
        parcelset::gatherer parcels_received_;
#endif
#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
        // Per-action based parcel statistics
        detail::per_action_data_counter action_parcels_sent_;
        detail::per_action_data_counter action_parcels_received_;
#endif

        /// serialization is allowed to use array optimization
        bool allow_array_optimizations_;
        bool allow_zero_copy_optimizations_;
        bool allow_zero_copy_receive_optimizations_;

        /// async serialization of parcels
        bool async_serialization_;

        /// priority of the parcelport
        int priority_;
        std::string type_;

        std::size_t zero_copy_serialization_threshold_;
    };
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
