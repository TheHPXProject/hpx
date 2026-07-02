//  Copyright (c) 2014 Thomas Heller
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
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/modules/parcelset_base.hpp>

#include <hpx/parcelset/connection_cache.hpp>
#include <hpx/parcelset/detail/call_for_each.hpp>
#include <hpx/parcelset/detail/parcel_await.hpp>
#include <hpx/parcelset/encode_parcels.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename ConnectionHandler>
    struct connection_handler_traits;

    HPX_CXX_EXPORT template <typename ConnectionHandler>
    class HPX_EXPORT parcelport_impl : public parcelport
    {
        using connection = typename connection_handler_traits<
            ConnectionHandler>::connection_type;

    public:
        static char const* connection_handler_type()
        {
            return connection_handler_traits<ConnectionHandler>::type();
        }

        static std::size_t thread_pool_size(
            util::runtime_configuration const& ini)
        {
            std::string key("hpx.parcel.");
            key += connection_handler_type();

            return hpx::util::get_entry_as<std::size_t>(
                ini, key + ".io_pool_size", 2);
        }

        static char const* pool_name()
        {
            return connection_handler_traits<ConnectionHandler>::pool_name();
        }

        static char const* pool_name_postfix()
        {
            return connection_handler_traits<
                ConnectionHandler>::pool_name_postfix();
        }

        static std::size_t max_connections(
            util::runtime_configuration const& ini)
        {
            std::string key("hpx.parcel.");
            key += connection_handler_type();

            return hpx::util::get_entry_as<std::size_t>(
                ini, key + ".max_connections", HPX_PARCEL_MAX_CONNECTIONS);
        }

        static std::size_t max_connections_per_loc(
            util::runtime_configuration const& ini)
        {
            std::string key("hpx.parcel.");
            key += connection_handler_type();

            return hpx::util::get_entry_as<std::size_t>(ini,
                key + ".max_connections_per_locality",
                HPX_PARCEL_MAX_CONNECTIONS_PER_LOCALITY);
        }

        static std::size_t zero_copy_serialization_threshold(
            util::runtime_configuration const& ini)
        {
            std::string key("hpx.parcel.");
            key += connection_handler_type();

            return hpx::util::get_entry_as<std::size_t>(ini,
                key + ".zero_copy_serialization_threshold",
                HPX_ZERO_COPY_SERIALIZATION_THRESHOLD);
        }

        static std::size_t max_background_threads(
            util::runtime_configuration const& ini)
        {
            std::string key("hpx.parcel.");
            key += connection_handler_type();

            return hpx::util::get_entry_as<std::size_t>(ini,
                key + ".max_background_threads",
                (std::numeric_limits<std::size_t>::max)());
        }

    public:
        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)

        /// Construct the parcelport on the given locality.
        parcelport_impl(util::runtime_configuration const& ini,
            locality const& here,
            threads::policies::callback_notifier const& notifier)
          : parcelport(ini, here, connection_handler_type(),
                zero_copy_serialization_threshold(ini))
          , io_service_pool_(thread_pool_size(ini), notifier, pool_name(),
                pool_name_postfix())
          , connection_cache_(
                max_connections(ini), max_connections_per_loc(ini))
          , archive_flags_(0)
          , operations_in_flight_(0)
          , num_thread_(0)
          , max_background_thread_(max_background_threads(ini))
        {
            std::string const endian_out =
                get_config_entry("hpx.parcel.endian_out",
                    endian::native == endian::big ? "big" : "little");
            if (endian_out == "little")
            {
                archive_flags_ = archive_flags_ |
                    serialization::archive_flags::endian_little;
            }
            else if (endian_out == "big")
            {
                archive_flags_ =
                    archive_flags_ | serialization::archive_flags::endian_big;
            }
            else
            {
                HPX_ASSERT(endian_out == "little" || endian_out == "big");
            }

            if (!this->allow_array_optimizations())
            {
                archive_flags_ = archive_flags_ |
                    serialization::archive_flags::disable_array_optimization;
                archive_flags_ = archive_flags_ |
                    serialization::archive_flags::disable_data_chunking;
                archive_flags_ = archive_flags_ |
                    serialization::archive_flags::disable_receive_data_chunking;
            }
            else if (!this->allow_zero_copy_optimizations())
            {
                archive_flags_ = archive_flags_ |
                    serialization::archive_flags::disable_data_chunking;
                archive_flags_ = archive_flags_ |
                    serialization::archive_flags::disable_receive_data_chunking;
            }
            else if (!this->allow_zero_copy_receive_optimizations())
            {
                archive_flags_ = archive_flags_ |
                    serialization::archive_flags::disable_receive_data_chunking;
            }
        }

        parcelport_impl(parcelport_impl const&) = delete;
        parcelport_impl(parcelport_impl&&) = delete;
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        parcelport_impl& operator=(parcelport_impl const&) = delete;
        parcelport_impl& operator=(parcelport_impl&&) = delete;

        ~parcelport_impl() override
        {
            connection_cache_.clear();
        }

        bool can_bootstrap() const override
        {
            return connection_handler_traits<
                ConnectionHandler>::send_early_parcel::value;
        }

        bool run(bool const blocking = true) override
        {
            io_service_pool_.run(false);    // start pool

            bool const success = connection_handler().do_run();

            if (success && blocking)
                io_service_pool_.join();

            return success;
        }

        void flush_parcels() override
        {
            // We suspend our thread, which will make progress on the network
            hpx::execution_base::this_thread::yield(
                "parcelport_impl::flush_parcels");

            // make sure no more work is pending, wait for service pool to get
            // empty
            hpx::util::yield_while(
                [this]() {
                    return operations_in_flight_ != 0 ||
                        get_pending_parcels_count(false) != 0;
                },
                "parcelport_impl::flush_parcels");
        }

        void stop(bool const blocking = true) override
        {
            flush_parcels();

            if (blocking)
            {
                connection_cache_.shutdown();
                connection_handler().do_stop();
                io_service_pool_.wait();
                io_service_pool_.stop();
                io_service_pool_.join();
                connection_cache_.clear();
                io_service_pool_.clear();
            }
            else
            {
                io_service_pool_.stop();
            }
        }

    public:
        void put_parcel(
            locality const& dest, parcel p, write_handler_type f) override
        {
            HPX_ASSERT(dest.type() == type());

            // We create a shared pointer of the parcels_await object since it
            // needs to be kept alive as long as there are futures not ready
            // or GIDs to be split. This is necessary to preserve the identity
            // of the 'this' pointer.
            detail::parcel_await_apply(HPX_MOVE(p), HPX_MOVE(f), archive_flags_,
                [this, dest](parcel&& p, write_handler_type&& f) {
                    if (connection_handler_traits<
                            ConnectionHandler>::send_immediate_parcels::value &&
                        can_send_immediate_impl())
                    {
                        send_immediate_impl(dest, &f, &p, 1);
                    }
                    else
                    {
                        // enqueue the outgoing parcel ...
                        enqueue_parcel(dest, HPX_MOVE(p), HPX_MOVE(f));
                        get_connection_and_send_parcels(dest);
                    }
                });
        }

        void put_parcels(locality const& dest, std::vector<parcel> parcels,
            std::vector<write_handler_type> handlers) override
        {
            if (parcels.size() != handlers.size())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "parcelport::put_parcels",
                    "mismatched number of parcels and handlers");
            }

#if defined(HPX_DEBUG)
            // make sure all parcels go to the same locality
            HPX_ASSERT(dest.type() == type());
            for (std::size_t i = 1; i != parcels.size(); ++i)
            {
                HPX_ASSERT(parcels[0].destination_locality() ==    //-V767
                    parcels[i].destination_locality());
            }
#endif
            // We create a shared pointer of the parcels_await object since it
            // needs to be kept alive as long as there are futures not ready
            // or GIDs to be split. This is necessary to preserve the identity
            // of the 'this' pointer.
            detail::parcels_await_apply(HPX_MOVE(parcels), HPX_MOVE(handlers),
                archive_flags_,
                [this, dest](std::vector<parcel>&& parcels,
                    std::vector<write_handler_type>&& handlers) {
                    if (connection_handler_traits<
                            ConnectionHandler>::send_immediate_parcels::value &&
                        can_send_immediate_impl())
                    {
                        send_immediate_impl(dest, handlers.data(),
                            parcels.data(), parcels.size());
                    }
                    else
                    {
                        enqueue_parcels(
                            dest, HPX_MOVE(parcels), HPX_MOVE(handlers));
                        get_connection_and_send_parcels(dest);
                    }
                });
        }

        void send_early_parcel(locality const& dest, parcel p) override
        {
            send_early_parcel_impl(dest, HPX_MOVE(p));
        }

        util::io_service_pool* get_thread_pool(char const* name) override
        {
            if (0 == std::strcmp(name, io_service_pool_.get_name()))
                return &io_service_pool_;
            return nullptr;
        }

        bool do_background_work(std::size_t const num_thread,
            parcelport_background_mode const mode) override
        {
            trigger_pending_work();
            return do_background_work_impl(num_thread, mode);
        }

        /// support enable_shared_from_this
        std::shared_ptr<parcelport_impl> shared_from_this()
        {
            return std::static_pointer_cast<parcelport_impl>(
                parcelset::parcelport::shared_from_this());
        }

        std::shared_ptr<parcelport_impl const> shared_from_this() const
        {
            return std::static_pointer_cast<parcelport_impl const>(
                parcelset::parcelport::shared_from_this());
        }

    protected:
        template <typename F>
        bool reschedule_on_thread(
            F&& f, threads::thread_schedule_state state, char const* funcname)
        {
            error_code ec(throwmode::lightweight);
            hpx::threads::thread_init_data data(
                hpx::threads::make_thread_function_nullary(HPX_FORWARD(F, f)),
                funcname, threads::thread_priority::normal,
                threads::thread_schedule_hint(
                    static_cast<std::int16_t>(get_next_num_thread())),
                threads::thread_stacksize::default_, state, true);

            auto const id = hpx::threads::register_thread(data, ec);
            if (!ec)
                return false;

            if (state == threads::thread_schedule_state::suspended)
            {
                threads::set_thread_state(id.noref(),
                    std::chrono::milliseconds(100),
                    threads::thread_schedule_state::pending,
                    threads::thread_restart_state::signaled,
                    threads::thread_priority::boost, true, ec);
            }

            return true;
        }

    public:
        // Cache specific functionality
        void remove_from_connection_cache_delayed(locality const& loc)
        {
            if (operations_in_flight_ != 0)
            {
                if (!connection_handler().reschedule_on_thread(
                        util::deferred_call(
                            &parcelport_impl::remove_from_connection_cache,
                            this, loc),
                        threads::thread_schedule_state::pending,
                        "remove_from_connection_cache_delayed"))
                {
                    return;
                }
            }

            connection_cache_.clear(loc);
        }

        void remove_from_connection_cache(locality const& loc) override
        {
            connection_handler().reschedule_on_thread(
                util::deferred_call(
                    &parcelport_impl::remove_from_connection_cache_delayed,
                    this, loc),
                threads::thread_schedule_state::pending,
                "remove_from_connection_cache_delayed");
        }

        /// Return the name of this locality
        std::string get_locality_name() const override
        {
            return connection_handler().get_locality_name();
        }

        ////////////////////////////////////////////////////////////////////////
        // Return the given connection cache statistic
        std::int64_t get_connection_cache_statistics(
            connection_cache_statistics_type const t, bool reset) override
        {
            switch (t)
            {
            case connection_cache_insertions:
                return connection_cache_.get_cache_insertions(reset);

            case connection_cache_evictions:
                return connection_cache_.get_cache_evictions(reset);

            case connection_cache_hits:
                return connection_cache_.get_cache_hits(reset);

            case connection_cache_misses:
                return connection_cache_.get_cache_misses(reset);

            case connection_cache_reclaims:
                return connection_cache_.get_cache_reclaims(reset);

            case connection_cache_reservation_failures:
                return connection_cache_.get_reservation_failures(reset);

            case connection_cache_num_connections:
                return connection_cache_.get_num_connections();

            case connection_cache_max_connections:
                return connection_cache_.get_max_connections();

            default:
                break;
            }

            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "parcelport_impl::get_connection_cache_statistics",
                "invalid connection cache statistics type");
        }

    private:
        ConnectionHandler& connection_handler()
        {
            return static_cast<ConnectionHandler&>(*this);
        }

        ConnectionHandler const& connection_handler() const
        {
            return static_cast<ConnectionHandler const&>(*this);
        }

        void send_early_parcel_impl(
            [[maybe_unused]] locality const& dest, [[maybe_unused]] parcel p)
        {
            if constexpr (connection_handler_traits<
                              ConnectionHandler>::send_early_parcel::value)
            {
                put_parcel(dest, HPX_MOVE(p),
                    &parcelport::early_pending_parcel_handler);
            }
            else
            {
                HPX_THROW_EXCEPTION(hpx::error::network_error,
                    "send_early_parcel",
                    "This parcelport does not support sending early parcels");
            }
        }

        bool do_background_work_impl([[maybe_unused]] std::size_t num_thread,
            [[maybe_unused]] parcelport_background_mode mode)
        {
            if constexpr (connection_handler_traits<
                              ConnectionHandler>::do_background_work::value)
            {
                return connection_handler().background_work(num_thread, mode);
            }
            else
            {
                return false;
            }
        }

        bool can_send_immediate_impl()
        {
            if constexpr (connection_handler_traits<
                              ConnectionHandler>::send_immediate_parcels::value)
            {
                return connection_handler().can_send_immediate();
            }
            else
            {
                return false;
            }
        }

    protected:
        void send_immediate_impl_connectionless(locality const& dest_,
            write_handler_type* fs, parcel* ps, std::size_t num_parcels)
        {
            static_assert(connection_handler_traits<
                              ConnectionHandler>::is_connectionless::value &&
                    connection_handler_traits<
                        ConnectionHandler>::send_immediate_parcels::value,
                "This parcelport is not suitable for the connectionless "
                "version of send_immediate!");

            std::vector<parcel> parcels;
            std::vector<write_handler_type> handlers;
            if (fs == nullptr)
            {
                HPX_ASSERT(ps == nullptr);
                HPX_ASSERT(num_parcels == 0u);

                if (!dequeue_parcels(dest_, parcels, handlers))
                {
                    return;
                }

                ps = parcels.data();
                fs = handlers.data();
                num_parcels = parcels.size();
                HPX_ASSERT(parcels.size() == handlers.size());
            }

            // encode the parcels
            typename ConnectionHandler::sender_type::parcel_buffer_type buffer;
            std::size_t const encoded_parcels =
                encode_parcels(*this, ps, num_parcels, buffer, archive_flags_,
                    get_max_outbound_message_size());

            typename ConnectionHandler::sender_type::callback_fn_type
                callback_fn = detail::call_for_each(
                    detail::call_for_each::handlers_type(
                        std::make_move_iterator(fs),
                        std::make_move_iterator(fs + encoded_parcels)),
                    detail::call_for_each::parcels_type(
                        std::make_move_iterator(ps),
                        std::make_move_iterator(ps + encoded_parcels)));

            if (connection_handler().send_immediate(
                    this, dest_, HPX_MOVE(buffer), HPX_MOVE(callback_fn)))
            {
                // we don't propagate errors for now
            }

            if (num_parcels != encoded_parcels && fs != nullptr)
            {
                std::vector<parcel> overflow_parcels(
                    std::make_move_iterator(ps + encoded_parcels),
                    std::make_move_iterator(ps + num_parcels));
                std::vector<write_handler_type> overflow_handlers(
                    std::make_move_iterator(fs + encoded_parcels),
                    std::make_move_iterator(fs + num_parcels));
                enqueue_parcels(dest_, HPX_MOVE(overflow_parcels),
                    HPX_MOVE(overflow_handlers));
            }
        }

        void send_immediate_impl_connection(locality const& dest_,
            write_handler_type* fs, parcel* ps, std::size_t num_parcels)
        {
            static_assert(!connection_handler_traits<
                              ConnectionHandler>::is_connectionless::value &&
                    connection_handler_traits<
                        ConnectionHandler>::send_immediate_parcels::value,
                "This parcelport is not suitable for the connection-oriented "
                "version of send_immediate!");
            // First try to get a connection ...
            std::uint64_t addr;
            connection* sender =
                connection_handler().get_connection(dest_, addr);

            // If we couldn't get one ... enqueue the parcel and move on
            std::size_t encoded_parcels = 0;
            std::vector<parcel> parcels;
            std::vector<write_handler_type> handlers;
            if (sender != nullptr)
            {
                if (fs == nullptr)
                {
                    HPX_ASSERT(ps == nullptr);
                    HPX_ASSERT(num_parcels == 0u);

                    if (!dequeue_parcels(dest_, parcels, handlers))
                    {
                        // Give this connection back to the connection
                        // handler as we couldn't dequeue parcels.
                        connection_handler().reclaim_connection(sender);
                        return;
                    }

                    ps = parcels.data();     //-V506
                    fs = handlers.data();    //-V506
                    num_parcels = parcels.size();
                    HPX_ASSERT(parcels.size() == handlers.size());
                }

                // encode the parcels
                auto encoded_buffer = sender->get_new_buffer();
                encoded_parcels =
                    encode_parcels(*this, ps, num_parcels, encoded_buffer,
                        archive_flags_, get_max_outbound_message_size());

                using handler_type = detail::call_for_each;

                if (sender->parcelport_->async_write(
                        handler_type(
                            handler_type::handlers_type(
                                std::make_move_iterator(fs),
                                std::make_move_iterator(fs + encoded_parcels)),
                            handler_type::parcels_type(
                                std::make_move_iterator(ps),
                                std::make_move_iterator(ps + encoded_parcels))),
                        sender, addr, encoded_buffer))
                {
                    // we don't propagate errors for now
                }
            }
            if (num_parcels != encoded_parcels && fs != nullptr)
            {
                std::vector<parcel> overflow_parcels(
                    std::make_move_iterator(ps + encoded_parcels),
                    std::make_move_iterator(ps + num_parcels));
                std::vector<write_handler_type> overflow_handlers(
                    std::make_move_iterator(fs + encoded_parcels),
                    std::make_move_iterator(fs + num_parcels));
                enqueue_parcels(dest_, HPX_MOVE(overflow_parcels),
                    HPX_MOVE(overflow_handlers));
            }
        }

        void send_immediate_impl([[maybe_unused]] locality const& dest_,
            [[maybe_unused]] write_handler_type* fs,
            [[maybe_unused]] parcel* ps,
            [[maybe_unused]] std::size_t const num_parcels)
        {
            if constexpr (connection_handler_traits<
                              ConnectionHandler>::is_connectionless::value)
            {
                send_immediate_impl_connectionless(dest_, fs, ps, num_parcels);
            }
            else if constexpr (connection_handler_traits<ConnectionHandler>::
                                   send_immediate_parcels::value)
            {
                send_immediate_impl_connection(dest_, fs, ps, num_parcels);
            }
            else
            {
                HPX_ASSERT(false);
            }
        }

    private:
        ///////////////////////////////////////////////////////////////////////
        std::shared_ptr<connection> get_connection(
            locality const& l, bool /* force */, error_code& ec)
        {
            // Request new connection from connection cache.
            std::shared_ptr<connection> sender_connection;

            if (connection_handler_traits<
                    ConnectionHandler>::send_immediate_parcels::value &&
                can_send_immediate_impl())
            {
                std::terminate();
            }
            else
            {
                // Get a connection or reserve space for a new connection.
                if (!connection_cache_.get_or_reserve(l, sender_connection))
                {
                    // If no slot is available it's not a problem as the parcel
                    // will be sent out whenever the next connection is returned
                    // to the cache.
                    if (&ec != &throws)
                        ec = make_success_code();
                    return sender_connection;
                }
            }

            // Check if we need to create the new connection.
            if (!sender_connection)
            {
                return connection_handler().create_connection(l, ec);
            }

            if (&ec != &throws)
                ec = make_success_code();

            return sender_connection;
        }

        ///////////////////////////////////////////////////////////////////////
        void enqueue_parcel(
            locality const& locality_id, parcel&& p, write_handler_type&& f)
        {
            using mapped_type = pending_parcels_map::mapped_type;

            std::unique_lock const l(mtx_);

            [[maybe_unused]] util::ignore_while_checking il(&l);

            mapped_type& e = pending_parcels_[locality_id];
            hpx::get<0>(e).push_back(HPX_MOVE(p));
            hpx::get<1>(e).push_back(HPX_MOVE(f));

            ++num_parcel_destinations_;
            if (!parcel_destinations_.insert(locality_id).second)
            {
                --num_parcel_destinations_;
            }
        }

        void enqueue_parcels(locality const& locality_id,
            std::vector<parcel>&& parcels,
            std::vector<write_handler_type>&& handlers)
        {
            using mapped_type = pending_parcels_map::mapped_type;

            std::unique_lock const l(mtx_);

            [[maybe_unused]] util::ignore_while_checking il(&l);

            HPX_ASSERT(parcels.size() == handlers.size());

            mapped_type& e = pending_parcels_[locality_id];
            if (hpx::get<0>(e).empty())
            {
                HPX_ASSERT(hpx::get<1>(e).empty());
                std::swap(hpx::get<0>(e), parcels);
                std::swap(hpx::get<1>(e), handlers);
            }
            else
            {
                HPX_ASSERT(hpx::get<0>(e).size() == hpx::get<1>(e).size());
                std::size_t const new_size =
                    hpx::get<0>(e).size() + parcels.size();
                hpx::get<0>(e).reserve(new_size);

                std::move(parcels.begin(), parcels.end(),
                    std::back_inserter(hpx::get<0>(e)));
                hpx::get<1>(e).reserve(new_size);
                std::move(handlers.begin(), handlers.end(),
                    std::back_inserter(hpx::get<1>(e)));
            }

            ++num_parcel_destinations_;
            if (!parcel_destinations_.insert(locality_id).second)
            {
                --num_parcel_destinations_;
            }
        }

        bool dequeue_parcels(locality const& locality_id,
            std::vector<parcel>& parcels,
            std::vector<write_handler_type>& handlers)
        {
            std::unique_lock const l(mtx_, std::try_to_lock);
            if (!l.owns_lock())
                return false;

            auto const it = pending_parcels_.find(locality_id);

            // do nothing if parcels have already been picked up by another
            // thread
            if (it != pending_parcels_.end() &&
                !hpx::get<0>(it->second).empty())
            {
                HPX_ASSERT(it->first == locality_id);
                HPX_ASSERT(handlers.empty());
                HPX_ASSERT(handlers.size() == parcels.size());
                std::swap(parcels, hpx::get<0>(it->second));
                HPX_ASSERT(hpx::get<0>(it->second).empty());
                std::swap(handlers, hpx::get<1>(it->second));
                HPX_ASSERT(handlers.size() == parcels.size());

                HPX_ASSERT(!handlers.empty());
            }
            else
            {
                HPX_ASSERT(hpx::get<1>(it->second).empty());
                return false;
            }

            parcel_destinations_.erase(locality_id);

            HPX_ASSERT(
                0 != num_parcel_destinations_.load(std::memory_order_relaxed));
            --num_parcel_destinations_;

            return true;
        }

    protected:
        bool dequeue_parcel(
            locality& dest, parcel& p, write_handler_type& handler)
        {
            std::unique_lock const l(mtx_, std::try_to_lock);
            if (!l.owns_lock())
                return false;

            for (auto& pending : pending_parcels_)
            {
                auto& parcels = hpx::get<0>(pending.second);
                if (!parcels.empty())
                {
                    auto& handlers = hpx::get<1>(pending.second);
                    dest = pending.first;
                    p = HPX_MOVE(parcels.back());
                    parcels.pop_back();
                    handler = HPX_MOVE(handlers.back());
                    handlers.pop_back();

                    if (parcels.empty())
                    {
                        pending_parcels_.erase(dest);
                    }
                    return true;
                }
            }
            return false;
        }

        bool trigger_pending_work()
        {
            if (0 == num_parcel_destinations_.load(std::memory_order_relaxed))
                return true;

            std::vector<locality> destinations;

            {
                std::unique_lock const l(mtx_, std::try_to_lock);
                if (!l.owns_lock())
                    return true;

                if (parcel_destinations_.empty())
                    return true;

                destinations.reserve(parcel_destinations_.size());
                for (locality const& loc : parcel_destinations_)
                {
                    destinations.push_back(loc);
                }
            }

            // Create new HPX threads which send the parcels that are still
            // pending.
            for (locality const& loc : destinations)
            {
                get_connection_and_send_parcels(loc);
            }

            return true;
        }

    private:
        ///////////////////////////////////////////////////////////////////////
        void get_connection_and_send_parcels(
            locality const& locality_id, bool /* background */ = false)
        {
            if (connection_handler_traits<
                    ConnectionHandler>::send_immediate_parcels::value &&
                can_send_immediate_impl())
            {
                send_immediate_impl(locality_id, nullptr, nullptr, 0);
                return;
            }

            // If one of the sending threads are in suspended state, we need to
            // force a new connection to avoid deadlocks.
            constexpr bool force_connection = true;

            error_code ec;
            std::shared_ptr<connection> sender_connection =
                get_connection(locality_id, force_connection, ec);

            if (!sender_connection)
            {
                // We can safely return if no connection is available at this
                // point. As soon as a connection becomes available it checks
                // for pending parcels and sends those out.
                return;
            }

            // repeat until no more parcels are to be sent
            std::vector<parcel> parcels;
            std::vector<write_handler_type> handlers;

            if (!dequeue_parcels(locality_id, parcels, handlers))
            {
                // Give this connection back to the cache as we couldn't dequeue
                // parcels.
                connection_cache_.reclaim(locality_id, sender_connection);
            }
            else
            {
                // send parcels if they didn't get sent by another connection
                send_pending_parcels(locality_id, sender_connection,
                    HPX_MOVE(parcels), HPX_MOVE(handlers));
            }
        }

        void send_pending_parcels_trampoline(std::error_code const& ec,
            locality const& locality_id,
            std::shared_ptr<connection> sender_connection)
        {
            HPX_ASSERT(operations_in_flight_ != 0);
            --operations_in_flight_;

#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
            sender_connection->set_state(connection::state_scheduled_thread);
#endif
            if (!ec)
            {
                // Give this connection back to the cache as it's not
                // needed anymore.
                connection_cache_.reclaim(locality_id, sender_connection);
            }
            else
            {
                // remove this connection from cache
                connection_cache_.clear(locality_id, sender_connection);
            }

            {
                std::lock_guard l(mtx_);

                // HPX_ASSERT(locality_id == sender_connection->destination());
                if (auto const it = pending_parcels_.find(locality_id);
                    it == pending_parcels_.end() ||
                    hpx::get<0>(it->second).empty())
                {
                    return;
                }
            }

            // Create a new HPX thread which sends parcels that are still
            // pending.
            get_connection_and_send_parcels(locality_id);
        }

        void send_pending_parcels(parcelset::locality const& parcel_locality_id,
            std::shared_ptr<connection> sender_connection,
            std::vector<parcel>&& parcels,                 //-V826
            std::vector<write_handler_type>&& handlers)    //-V826
        {
#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
            sender_connection->set_state(connection::state_send_pending);
#endif

#if defined(HPX_DEBUG)
            // verify the connection points to the right destination
            // HPX_ASSERT(parcel_locality_id == sender_connection->destination());
            sender_connection->verify_(parcel_locality_id);
#endif
            // encode the parcels
            std::size_t const num_parcels = encode_parcels(*this,
                parcels.data(), parcels.size(), sender_connection->buffer_,
                archive_flags_, this->get_max_outbound_message_size());

            using hpx::parcelset::detail::call_for_each;
            if (num_parcels == parcels.size())
            {
                ++operations_in_flight_;

                // send all the parcels
                sender_connection->async_write(
                    call_for_each(HPX_MOVE(handlers), HPX_MOVE(parcels)),
                    hpx::bind_front(
                        &parcelport_impl::send_pending_parcels_trampoline,
                        this));
            }
            else
            {
                ++operations_in_flight_;

                // NOLINTNEXTLINE(bugprone-use-after-move)
                HPX_ASSERT(num_parcels < parcels.size());

                std::vector<write_handler_type> handled_handlers;
                handled_handlers.reserve(num_parcels);

                std::move(handlers.begin(), handlers.begin() + num_parcels,
                    std::back_inserter(handled_handlers));

                std::vector<parcel> handled_parcels;
                handled_parcels.reserve(num_parcels);

                std::move(parcels.begin(), parcels.begin() + num_parcels,
                    std::back_inserter(handled_parcels));

                // send only part of the parcels
                sender_connection->async_write(
                    call_for_each(
                        HPX_MOVE(handled_handlers), HPX_MOVE(handled_parcels)),
                    hpx::bind_front(
                        &parcelport_impl::send_pending_parcels_trampoline,
                        this));

                // give back unhandled parcels
                parcels.erase(parcels.begin(), parcels.begin() + num_parcels);
                handlers.erase(
                    handlers.begin(), handlers.begin() + num_parcels);

                enqueue_parcels(
                    parcel_locality_id, HPX_MOVE(parcels), HPX_MOVE(handlers));
            }

            // We yield here for a short amount of time to give another HPX
            // thread the chance to put a subsequent parcel which leads to a
            // more effective parcel buffering
            hpx::execution_base::this_thread::yield();
        }

    public:
        std::size_t get_next_num_thread()
        {
            return ++num_thread_ % max_background_thread_;
        }

    protected:
        /// The pool of io_service objects used to perform asynchronous operations.
        util::io_service_pool io_service_pool_;

        /// The connection cache for sending connections
        util::connection_cache<connection, locality> connection_cache_;

        using mutex_type = hpx::spinlock;

        int archive_flags_;
        hpx::util::atomic_count operations_in_flight_;

        std::atomic<std::size_t> num_thread_;
        std::size_t const max_background_thread_;
    };
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
