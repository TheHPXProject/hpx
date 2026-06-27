//  Copyright (c)      2014 Thomas Heller
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    /// The \a parcelhandler is the representation of the parcelset inside a
    /// locality. It is built on top of a single parcelport. Several
    /// parcel-handlers may be connected to a single parcelport.
    HPX_CXX_EXPORT class HPX_EXPORT parcelhandler
    {
    public:
        parcelhandler(parcelhandler const&) = delete;
        parcelhandler(parcelhandler&&) = delete;
        parcelhandler& operator=(parcelhandler const&) = delete;
        parcelhandler& operator=(parcelhandler&&) = delete;

    private:
        // make sure the parcel has been properly initialized
        static void init_parcel(parcel& p);

        using mutex_type = hpx::spinlock;

    public:
        using handler_key_type = std::pair<locality, std::string>;
        using message_handler_map = std::map<handler_key_type,
            std::shared_ptr<policies::message_handler>>;

        using read_handler_type = parcelport::read_handler_type;
        using write_handler_type = parcelport::write_handler_type;

        /// Construct a new \a parcelhandler.
        explicit parcelhandler(util::runtime_configuration const& cfg);

        ~parcelhandler();

        void set_notification_policies(util::runtime_configuration const& cfg,
            threads::threadmanager* tm,
            threads::policies::callback_notifier const& notifier);

        std::shared_ptr<parcelport> get_bootstrap_parcelport() const;

        void initialize();

        void flush_parcels() const;

        /// \brief Stop all parcel ports associated with this parcelhandler
        void stop(bool blocking = true);

        /// \brief do background work in the parcel layer
        ///
        /// \returns Whether any work has been performed
        bool do_background_work(std::size_t num_thread = 0,
            bool stop_buffering = false,
            parcelport_background_mode mode = parcelport_background_mode::all);

        /// Return the list of all remote localities supporting the given
        /// component type
        ///
        /// \param locality_ids [in/out] The reference to a vector of id_types
        ///                 filled by the function.
        /// \param type     [in] The type of the component which needs to exist
        ///                 on the returned localities.
        /// \param ec       [in/out]
        ///
        /// \returns The function returns \a true if there is at least one
        ///          remote locality known by AGAS
        ///          (!prefixes.empty()).
        bool get_raw_remote_localities(
            std::vector<naming::gid_type>& locality_ids,
            components::component_type type = to_int(
                hpx::components::component_enum_type::invalid),
            error_code& ec = throws) const;

        /// Return the list of all localities supporting the given
        /// component type
        ///
        /// \param locality_ids [in/out] The reference to a vector of id_types
        ///                 filled by the function.
        /// \param type     [in] The type of the component which needs to exist
        ///                 on the returned localities.
        /// \param ec       [in/out]
        ///
        /// \returns The function returns \a true if there is at least one
        ///          locality known by AGAS
        ///          (!prefixes.empty()).
        bool get_raw_localities(std::vector<naming::gid_type>& locality_ids,
            components::component_type type, error_code& ec = throws) const;

        /// A parcel is submitted for transport at the source locality site to
        /// the parcel set of the locality with the put-parcel command
        ///
        /// \note The function \a sync_put_parcel() is synchronous, it blocks
        ///       until the parcel has been sent by the underlying \a
        ///       parcelport.
        ///
        /// \param p        [in, out] A reference to the parcel to send. The
        ///                 function does not return before the parcel has been
        ///                 transmitted. The parcel \a p will be modified in
        ///                 place, as it will get set the resolved destination
        ///                 address and parcel id (if not already set).
        void sync_put_parcel(parcel p);

        /// A parcel is submitted for transport at the source locality site to
        /// the parcel set of the locality with the put-parcel command
        //
        /// \note The function \a put_parcel() is asynchronous, the provided
        /// function or function object gets invoked on completion of the send
        /// operation or on any error.
        ///
        /// \param p        [in, out] A reference to the parcel to send. The
        ///                 parcel \a p will be modified in place, as it will
        ///                 get set the resolved destination address and parcel
        ///                 id (if not already set).
        /// \param f        [in] A function object to be invoked on successful
        ///                 completion or on errors. The signature of this
        ///                 function object is expected to be:
        ///
        /// \code
        ///     void f (std::error_code const& err, std::size_t );
        /// \endcode
        ///
        ///                 where \a err is the status code of the operation and
        ///                       \a size is the number of successfully
        ///                              transferred bytes.
        void put_parcel(parcel p, write_handler_type f);

        /// This put_parcel() function overload is asynchronous, but no
        /// callback is provided by the user.
        ///
        /// \note   The function \a put_parcel() is asynchronous.
        ///
        /// \param p        [in, out] A reference to the parcel to send. The
        ///                 parcel \a p will be modified in place, as it will
        ///                 get set the resolved destination address and parcel
        ///                 id (if not already set).
        void put_parcel(parcel p);

        /// A parcel is submitted for transport at the source locality site to
        /// the parcel set of the locality with the put-parcel command
        //
        /// \note The function \a put_parcel() is asynchronous, the provided
        /// function or function object gets invoked on completion of the send
        /// operation or on any error.
        ///
        /// \param parcels   [in] The parcels to send.
        /// \param funcs     [in] The function objects to be invoked on
        ///                 successful completion or on errors. The signature
        ///                 of these function object are expected to be:
        ///
        /// \code
        ///     void f (std::error_code const& err, std::size_t );
        /// \endcode
        ///
        ///                 where \a err is the status code of the operation and
        ///                       \a size is the number of successfully
        ///                              transferred bytes.
        void put_parcels(
            std::vector<parcel> parcels, std::vector<write_handler_type> funcs);

        /// This put_parcel() function overload is asynchronous, but no
        /// callback is provided by the user.
        ///
        /// \note   The function \a put_parcel() is asynchronous.
        ///
        /// \param parcels  [in, out] A reference to the parcel to send. The
        ///                 parcel \a p will be modified in place, as it will
        ///                 get set the resolved destination address and parcel
        ///                 id (if not already set).
        void put_parcels(std::vector<parcel> parcels);

        /// \brief Factory function used in serialization to create a given
        /// locality endpoint
        locality create_locality(std::string const& name) const
        {
            return find_parcelport(name)->create_locality();
        }

        /// Return the name of this locality as retrieved from the active
        /// parcelport
        std::string get_locality_name() const;

        /// \brief Make sure the specified locality is not held by any
        /// connection caches anymore
        void remove_from_connection_cache(
            naming::gid_type const& gid, endpoints_type const& endpoints) const;

        /// \brief return the endpoints associated with this parcelhandler
        /// \returns all connection information for the enabled parcelports
        endpoints_type const& endpoints() const
        {
            return endpoints_;
        }

        void enable_alternative_parcelports()
        {
            use_alternative_parcelports_.store(true);
        }

        void disable_alternative_parcelports()
        {
            use_alternative_parcelports_.store(false);
        }

        /// Return the reference to an existing io_service
        util::io_service_pool* get_thread_pool(char const* name) const;

        ///////////////////////////////////////////////////////////////////////
        policies::message_handler* get_message_handler(char const* action,
            char const* message_handler_type, std::size_t num_messages,
            std::size_t interval, locality const& loc, error_code& ec = throws);

        ///////////////////////////////////////////////////////////////////////
        // Performance counter data

        // number of parcels routed
        std::int64_t get_parcel_routed_count(bool reset);

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        // number of parcels sent
        std::int64_t get_parcel_send_count(
            std::string const& pp_type, bool reset) const;

        // number of messages sent
        std::int64_t get_message_send_count(
            std::string const& pp_type, bool reset) const;

        // number of parcels received
        std::int64_t get_parcel_receive_count(
            std::string const& pp_type, bool reset) const;

        // number of messages received
        std::int64_t get_message_receive_count(
            std::string const& pp_type, bool reset) const;

        // the total time it took for all sends, from async_write to the
        // completion handler (nanoseconds)
        std::int64_t get_sending_time(
            std::string const& pp_type, bool reset) const;

        // the total time it took for all receives, from async_read to the
        // completion handler (nanoseconds)
        std::int64_t get_receiving_time(
            std::string const& pp_type, bool reset) const;

        // the total time it took for all sender-side serialization operations
        // (nanoseconds)
        std::int64_t get_sending_serialization_time(
            std::string const& pp_type, bool reset) const;

        // the total time it took for all receiver-side serialization
        // operations (nanoseconds)
        std::int64_t get_receiving_serialization_time(
            std::string const& pp_type, bool reset) const;

        // total data sent (bytes)
        std::int64_t get_data_sent(
            std::string const& pp_type, bool reset) const;

        // total data (uncompressed) sent (bytes)
        std::int64_t get_raw_data_sent(
            std::string const& pp_type, bool reset) const;

        // total data received (bytes)
        std::int64_t get_data_received(
            std::string const& pp_type, bool reset) const;

        // total data (uncompressed) received (bytes)
        std::int64_t get_raw_data_received(
            std::string const& pp_type, bool reset) const;

        std::int64_t get_buffer_allocate_time_sent(
            std::string const& pp_type, bool reset) const;

        std::int64_t get_buffer_allocate_time_received(
            std::string const& pp_type, bool reset) const;

        // total zero-copy chunks sent
        std::int64_t get_zchunks_send_count(
            std::string const& pp_type, bool reset) const;

        // total zero-copy chunks received
        std::int64_t get_zchunks_recv_count(
            std::string const& pp_type, bool reset) const;

        // the maximum number of zero-copy chunks per message sent
        std::int64_t get_zchunks_send_per_msg_count_max(
            std::string const& pp_type, bool reset) const;

        // the maximum number of zero-copy chunks per message received
        std::int64_t get_zchunks_recv_per_msg_count_max(
            std::string const& pp_type, bool reset) const;

        // the size of zero-copy chunks per message sent
        std::int64_t get_zchunks_send_size(
            std::string const& pp_type, bool reset) const;

        // the size of zero-copy chunks per message received
        std::int64_t get_zchunks_recv_size(
            std::string const& pp_type, bool reset) const;

        // the maximum size of zero-copy chunks per message sent
        std::int64_t get_zchunks_send_size_max(
            std::string const& pp_type, bool reset) const;

        // the maximum size of zero-copy chunks per message received
        std::int64_t get_zchunks_recv_size_max(
            std::string const& pp_type, bool reset) const;
#endif
#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
        // same as above, just separated data for each action
        // number of parcels sent
        std::int64_t get_action_parcel_send_count(std::string const& pp_type,
            std::string const& action, bool reset) const;

        // number of parcels received
        std::int64_t get_action_parcel_receive_count(std::string const& pp_type,
            std::string const& action, bool reset) const;

        // the total time it took for all sender-side serialization operations
        // (nanoseconds)
        std::int64_t get_action_sending_serialization_time(
            std::string const& pp_type, std::string const& action,
            bool reset) const;

        // the total time it took for all receiver-side serialization
        // operations (nanoseconds)
        std::int64_t get_action_receiving_serialization_time(
            std::string const& pp_type, std::string const& action,
            bool reset) const;

        // total data sent (bytes)
        std::int64_t get_action_data_sent(std::string const& pp_type,
            std::string const& action, bool reset) const;

        // total data received (bytes)
        std::int64_t get_action_data_received(std::string const& pp_type,
            std::string const& action, bool reset) const;
#endif

        //
        std::int64_t get_connection_cache_statistics(std::string const& pp_type,
            parcelport::connection_cache_statistics_type stat_type, bool) const;

        void list_parcelports(std::ostringstream& strm) const;
        void list_parcelport(std::ostringstream& strm,
            std::string const& ppname, int priority, bool bootstrap) const;

        void put_parcel_impl(parcel&& p, write_handler_type&& f);
        void put_parcels_impl(std::vector<parcel>&& parcels,
            std::vector<write_handler_type>&& handlers);

        // manage default exception handler
        void invoke_write_handler(
            std::error_code const& ec, parcel const& p) const;

        write_handler_type set_write_handler(write_handler_type f);

        bool enum_parcelports(
            hpx::move_only_function<bool(std::string const&)> const& f) const;

        std::int64_t get_incoming_queue_length(bool /*reset*/) const
        {
            return 0;
        }

        std::int64_t get_outgoing_queue_length(bool reset) const;

    protected:
        std::pair<std::shared_ptr<parcelport>, locality>
        find_appropriate_destination(naming::gid_type const& dest_gid);
        static locality find_endpoint(
            endpoints_type const& eps, std::string const& name);

    private:
        int get_priority(std::string const& name) const;

        parcelport* find_parcelport(
            std::string const& type, error_code& = throws) const;

        /// \brief Attach the given parcel port to this handler
        void attach_parcelport(std::shared_ptr<parcelport> const& pp);

        /// the parcelport this handler is associated with
        using pports_type =
            std::map<int, std::shared_ptr<parcelport>, std::greater<int>>;
        pports_type pports_;

        std::map<std::string, int> priority_;

        /// the endpoints corresponding to the parcel-ports
        endpoints_type endpoints_;

        /// the thread-manager to use (optional)
        threads::threadmanager* tm_;

        /// Allow to use alternative parcel-ports (this is enabled only after
        /// the runtime systems of all localities are guaranteed to have
        /// reached a certain state).
        std::atomic<bool> use_alternative_parcelports_;
        std::atomic<bool> enable_parcel_handling_;

        /// Store message handlers for actions
        mutex_type handlers_mtx_;
        message_handler_map handlers_;
        bool const load_message_handlers_;

        /// Count number of (outbound) parcels routed
        std::atomic<std::int64_t> count_routed_;

        /// global exception handler for unhandled exceptions thrown from the
        /// parcel layer
        mutable mutex_type mtx_;
        write_handler_type write_handler_;

        /// cache whether networking has been enabled
        bool is_networking_enabled_;

    public:
        bool is_networking_enabled() const
        {
            return is_networking_enabled_;
        }

        static std::vector<plugins::parcelport_factory_base*>&
        get_parcelport_factories();

        static void init(
            int* argc, char*** argv, util::command_line_handling& cfg);
        static void init(hpx::resource::partitioner& rp);
    };

    HPX_CXX_EXPORT HPX_EXPORT std::vector<std::string>
    load_runtime_configuration();
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
