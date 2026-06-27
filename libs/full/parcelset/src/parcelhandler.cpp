//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013-2014 Thomas Heller
//  Copyright (c) 2007      Richard D Guidry Jr
//  Copyright (c) 2011      Bryce Lelbach & Katelyn Kufahl
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/io_service.hpp>

#include <hpx/modules/logging.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/parcelset/init_parcelports.hpp>
#include <hpx/parcelset/message_handler_fwd.hpp>
#include <hpx/parcelset/parcelhandler.hpp>

#include <asio/error.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#if defined(HPX_HAVE_PARCEL_PROFILING)
#include <chrono>
#endif

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    void dijkstra_make_black();    // forward declaration only
}    // namespace hpx::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    ///////////////////////////////////////////////////////////////////////////
    // A parcel is submitted for transport at the source locality site to
    // the parcel set of the locality with the put-parcel command
    // This function is synchronous.
    void parcelhandler::sync_put_parcel(parcel p)    //-V669
    {
        hpx::promise<void> promise;
        future<void> sent_future = promise.get_future();
        put_parcel(
            HPX_MOVE(p), [&promise](std::error_code const&, parcel const&) {
                promise.set_value();
            });               // schedule parcel send
        sent_future.get();    // wait for the parcel to be sent
    }

    parcelhandler::parcelhandler(util::runtime_configuration const& cfg)
      : tm_(nullptr)
      , use_alternative_parcelports_(false)
      , enable_parcel_handling_(true)
      , handlers_mtx_("parcelhandler::handlers_mtx")
      , load_message_handlers_(
            util::get_entry_as<int>(cfg, "hpx.parcel.message_handlers", 0) != 0)
      , count_routed_(0)
      , mtx_("parcelhandler::mtx")
      , write_handler_(&default_write_handler)
#if defined(HPX_HAVE_NETWORKING)
      , is_networking_enabled_(cfg.enable_networking())
#else
      , is_networking_enabled_(false)
#endif
    {
        LPROGRESS_;
    }

    parcelhandler::~parcelhandler() = default;

    void parcelhandler::set_notification_policies(
        util::runtime_configuration const& cfg, threads::threadmanager* tm,
        threads::policies::callback_notifier const& notifier)
    {
        is_networking_enabled_ = hpx::is_networking_enabled();
        tm_ = tm;

        if (is_networking_enabled_ &&
            cfg.get_entry("hpx.parcel.enable", "1") != "0")
        {
            for (plugins::parcelport_factory_base* factory :
                get_parcelport_factories())
            {
                std::shared_ptr<parcelport> pp(factory->create(cfg, notifier));
                attach_parcelport(pp);
            }
        }
    }

    std::shared_ptr<parcelport> parcelhandler::get_bootstrap_parcelport() const
    {
        std::string const cfgkey("hpx.parcel.bootstrap");
        if (!pports_.empty())
        {
            auto const it =
                pports_.find(get_priority(get_config_entry(cfgkey, "tcp")));
            if (it != pports_.end() && it->first > 0)
            {
                return it->second;
            }
        }

        for (auto const& pp : pports_)
        {
            if (pp.first > 0 && pp.second->can_bootstrap())
            {
                return pp.second;
            }
        }

        if (hpx::is_networking_enabled())
        {
            std::cerr << "Could not find usable bootstrap parcelport.\n";
            std::cerr << hpx::util::format(
                "Preconfigured bootstrap parcelport: '{}'\n",
                get_config_entry(cfgkey, "tcp"));

            if (pports_.empty())
            {
                std::cerr << "No available parcelports\n";
            }
            else
            {
                std::cerr << "List of available parcelports:\n";
                for (auto const& pp : pports_)
                {
                    std::cerr << hpx::util::format(
                        "  {}, priority: {}, can bootstrap: {}\n",
                        pp.second->type(), pp.first,
                        pp.second->can_bootstrap());
                }
                std::cerr << "\n";
            }

            // terminate this locality as there is nothing else we can do
            std::terminate();
        }

        return {};
    }

    void parcelhandler::initialize()
    {
        exception_list exceptions;
        std::vector<int> failed_pps;
        for (pports_type::value_type& pp : pports_)
        {
            // protect against exceptions thrown by a parcelport during
            // initialization
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    if (pp.second != get_bootstrap_parcelport())
                    {
                        if (pp.first > 0)
                            pp.second->run(false);
                    }

                    // early parcel handling is finished, normal operation is
                    // about to start
                    if (pp.first > 0)
                    {
                        pp.second->initialized();
                    }
                },
                [&](std::exception_ptr&& e) {
                    exceptions.add(HPX_MOVE(e));
                    failed_pps.push_back(pp.first);
                });
        }

        // handle exceptions
        if (exceptions.size() != 0)
        {
            if (failed_pps.size() == pports_.size())
            {
                std::cerr << hpx::util::format(
                    "all parcelports failed initializing on locality {}, "
                    "exiting:\n{}\n",
                    agas::get_locality_id(), exceptions.get_message());
                std::terminate();
            }
            else
            {
                std::cerr << hpx::util::format(
                    "warning: the following errors were detected while "
                    "initializing parcelports on locality {}:\n{}\n",
                    agas::get_locality_id(), exceptions.get_message());
            }

            // clean up parcelports that have failed initialization
            std::cerr << "the following parcelports will be disabled:\n";
            for (int pp : failed_pps)
            {
                auto it = pports_.find(pp);
                if (it != pports_.end())
                {
                    std::cerr << "  " << (*it).second->type() << "\n";
                    (*it).second->stop();
                    pports_.erase(it);
                }
            }
            std::cerr << "\n";
        }
    }

    void parcelhandler::list_parcelport(std::ostringstream& strm,
        std::string const& ppname, int priority, bool bootstrap) const
    {
        hpx::util::format_to(strm, "parcel port: {}", ppname);

        std::string const cfgkey("hpx.parcel." + ppname + ".enable");
        bool const enabled =
            hpx::util::from_string<int>(get_config_entry(cfgkey, "0"), 0);
        strm << (enabled ? ", enabled" : ", not enabled");

        if (bootstrap)
            strm << ", bootstrap";

        hpx::util::format_to(strm, ", priority {}\n", priority);
    }

    // list available parcel ports
    void parcelhandler::list_parcelports(std::ostringstream& strm) const
    {
        for (pports_type::value_type const& pp : pports_)
        {
            list_parcelport(strm, pp.second->type(), pp.second->priority(),
                pp.second == get_bootstrap_parcelport());
        }
        strm << '\n';
    }

    write_handler_type parcelhandler::set_write_handler(write_handler_type f)
    {
        std::lock_guard<mutex_type> l(mtx_);
        std::swap(f, write_handler_);
        return f;
    }

    bool parcelhandler::enum_parcelports(
        hpx::move_only_function<bool(std::string const&)> const& f) const
    {
        for (pports_type::value_type const& pp : pports_)
        {
            if (pp.first > 0 && !f(pp.second->type()))
            {
                return false;
            }
        }
        return true;
    }

    int parcelhandler::get_priority(std::string const& name) const
    {
        auto const it = priority_.find(name);
        if (it == priority_.end())
            return 0;
        return it->second;
    }

    parcelport* parcelhandler::find_parcelport(
        std::string const& type, error_code&) const
    {
        int const priority = get_priority(type);
        if (priority <= 0)
            return nullptr;
        HPX_ASSERT(pports_.find(priority) != pports_.end());
        return pports_.find(priority)->second.get();    // -V783
    }

    void parcelhandler::attach_parcelport(std::shared_ptr<parcelport> const& pp)
    {
        if (!hpx::is_networking_enabled() || !pp)
        {
            return;
        }

        // add the new parcelport to the list of parcel-ports we care about
        int priority = pp->priority();
        std::string const cfgkey(
            std::string("hpx.parcel.") + pp->type() + ".enable");
        if (get_config_entry(cfgkey, "0") != "1")
        {
            priority = -priority;
        }
        pports_[priority] = pp;
        priority_[pp->type()] = priority;

        // add the endpoint of the new parcelport
        HPX_ASSERT(pp->type() == pp->here().type());
        if (priority > 0)
            endpoints_[pp->type()] = pp->here();
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Make sure the specified locality is not held by any
    /// connection caches anymore
    void parcelhandler::remove_from_connection_cache(
        naming::gid_type const& gid, endpoints_type const& endpoints) const
    {
        for (endpoints_type::value_type const& loc : endpoints)
        {
            for (pports_type::value_type const& pp : pports_)
            {
                if (std::string(pp.second->type()) == loc.second.type())
                {
                    pp.second->remove_from_connection_cache(loc.second);
                }
            }
        }

        agas::remove_resolved_locality(gid);
    }

    ///////////////////////////////////////////////////////////////////////////
    bool parcelhandler::do_background_work(std::size_t num_thread,
        bool stop_buffering, parcelport_background_mode mode)
    {
        bool did_some_work = false;
        if (!is_networking_enabled_)
        {
            return did_some_work;
        }

        LPT_(debug).format(
            "parcelhandler::do_background_work: thread {}, mode {}", num_thread,
            get_parcelport_background_mode_name(mode));

        // flush all parcel buffers
        if (0 == num_thread &&
            (mode & parcelport_background_mode::flush_buffers))
        {
            std::unique_lock<mutex_type> l(handlers_mtx_, std::try_to_lock);

            if (l.owns_lock())
            {
                using parcelset::policies::message_handler;
                constexpr message_handler::flush_mode flush_mode =
                    message_handler::flush_mode_background_work;

                auto const end = handlers_.end();
                for (auto it = handlers_.begin(); it != end; ++it)
                {
                    if ((*it).second)
                    {
                        std::shared_ptr<policies::message_handler> const p(
                            (*it).second);
                        unlock_guard<std::unique_lock<mutex_type>> ul(l);
                        did_some_work = p->flush(flush_mode, stop_buffering) ||
                            did_some_work;
                    }
                }
            }
        }

        // make sure all pending parcels are being handled
        for (pports_type::value_type const& pp : pports_)
        {
            if (pp.first > 0)
            {
                did_some_work =
                    pp.second->do_background_work(num_thread, mode) ||
                    did_some_work;
            }
        }

        return did_some_work;
    }

    void parcelhandler::flush_parcels() const
    {
        if (is_networking_enabled_)
        {
            // now flush all parcel ports to be shut down
            for (pports_type::value_type const& pp : pports_)
            {
                if (pp.first > 0)
                {
                    pp.second->flush_parcels();
                }
            }
        }
    }

    void parcelhandler::stop(bool blocking)
    {
        // now stop all parcel ports
        for (pports_type::value_type const& pp : pports_)
        {
            if (pp.first > 0)
            {
                pp.second->stop(blocking);
            }
        }

        // release all message handlers
        handlers_.clear();
    }

    bool parcelhandler::get_raw_remote_localities(
        std::vector<naming::gid_type>& locality_ids,
        components::component_type type, error_code& ec) const
    {
        std::vector<naming::gid_type> allprefixes;
        bool const result = get_raw_localities(allprefixes, type, ec);
        if (ec || !result)
            return false;

        std::remove_copy(allprefixes.begin(), allprefixes.end(),
            std::back_inserter(locality_ids), agas::get_locality());

        return !locality_ids.empty();
    }

    bool parcelhandler::get_raw_localities(
        std::vector<naming::gid_type>& locality_ids,
        components::component_type type, error_code&) const
    {
        std::vector<std::uint32_t> const ids = agas::get_all_locality_ids(type);

        locality_ids.clear();
        locality_ids.reserve(ids.size());
        for (auto const& id : ids)
        {
            locality_ids.emplace_back(naming::get_gid_from_locality_id(id));
        }

        return !locality_ids.empty();
    }

    std::pair<std::shared_ptr<parcelport>, locality>
    parcelhandler::find_appropriate_destination(
        naming::gid_type const& dest_gid)
    {
        endpoints_type const& dest_endpoints = agas::resolve_locality(dest_gid);

        for (pports_type::value_type& pp : pports_)
        {
            if (pp.first > 0)
            {
                if (locality const& dest =
                        find_endpoint(dest_endpoints, pp.second->type());
                    dest &&
                    pp.second->can_connect(dest, use_alternative_parcelports_))
                {
                    return std::make_pair(pp.second, dest);
                }
            }
        }

        std::ostringstream strm;
        strm << "target locality: " << dest_gid << "\n";
        strm << "available destination endpoints:\n" << dest_endpoints << "\n";
        strm << "available parcelports:\n";
        for (auto const& pp : pports_)
        {
            list_parcelport(strm, pp.second->type(), pp.second->priority(),
                pp.second == get_bootstrap_parcelport());
            strm << "\t [" << pp.second->here() << "]\n";
        }

        HPX_THROW_EXCEPTION(hpx::error::network_error,
            "parcelhandler::find_appropriate_destination",
            "The locality gid cannot be resolved to a valid endpoint. "
            "No valid parcelport configured. Detailed information:\n{}",
            strm.str());
    }

    locality parcelhandler::find_endpoint(
        endpoints_type const& eps, std::string const& name)
    {
        auto const it = eps.find(name);
        if (it != eps.end())
            return it->second;
        return {};
    }

    // Return the reference to an existing io_service
    util::io_service_pool* parcelhandler::get_thread_pool(
        char const* name) const
    {
        util::io_service_pool* result = nullptr;
        for (pports_type::value_type const& pp : pports_)
        {
            result = pp.second->get_thread_pool(name);
            if (result)
                return result;
        }
        return result;
    }

    namespace detail {
        void parcel_sent_handler(
            parcelhandler::write_handler_type const& f,    //-V669
            std::error_code const& ec, parcelset::parcel const& p)
        {
            // inform termination detection of a sent message
            if (!p.does_termination_detection())
            {
                hpx::detail::dijkstra_make_black();
            }

            // invoke the original handler
            f(ec, p);

            HPX_TRACING_MARK_EVENT("send_parcel");

#if defined(HPX_HAVE_PARCEL_PROFILING)
            hpx::tracing::send_parcel(
                p.parcel_id().get_lsb(), p.size(), p.destination_locality_id());
#endif
        }
    }    // namespace detail

    void parcelhandler::put_parcel(parcelset::parcel p)
    {
        LPT_(debug).format(
            "parcelhandler::put_parcel: submitted: {}", p.parcel_id());

        // inform termination detection of a sent message
        if (!p.does_termination_detection())
        {
            hpx::detail::dijkstra_make_black();
        }

        auto handler = [this](std::error_code const& ec,
                           parcelset::parcel const& p) -> void {
            invoke_write_handler(ec, p);

            LPT_(debug).format(
                "parcelhandler::put_parcel: handled: {}", p.parcel_id());
        };

        put_parcel_impl(HPX_MOVE(p), HPX_MOVE(handler));
    }

    void parcelhandler::put_parcel(parcelset::parcel p, write_handler_type f)
    {
        LPT_(debug).format(
            "parcelhandler::put_parcel (with handler): submitted: {}",
            p.parcel_id());

        // inform termination detection of a sent message
        if (!p.does_termination_detection())
        {
            hpx::detail::dijkstra_make_black();
        }

        auto handler = [this, f = HPX_MOVE(f)](std::error_code const& ec,
                           parcelset::parcel const& p) -> void {
            invoke_write_handler(ec, p);
            f(ec, p);

            LPT_(debug).format(
                "parcelhandler::put_parcel: handled: {}", p.parcel_id());
        };

        put_parcel_impl(HPX_MOVE(p), HPX_MOVE(handler));
    }

    void parcelhandler::put_parcel_impl(parcel&& p, write_handler_type&& f)
    {
        HPX_ASSERT(is_networking_enabled_);

        naming::gid_type const& gid = p.destination();
        naming::address& addr = p.addr();

        // During bootstrap this is handled separately (see
        // addressing_service::resolve_locality.

        // if this isn't an HPX thread, the stack space check will return false
        if (!this_thread::has_sufficient_stack_space() &&
            hpx::threads::threadmanager_is(hpx::state::running))
        {
            LPT_(debug).format("parcelhandler::put_parcel_impl: reschedule: {}",
                p.parcel_id());

            {
                // reschedule request as an HPX thread to avoid hangs
                void (parcelhandler::*put_parcel_ptr)(
                    parcel, write_handler_type) = &parcelhandler::put_parcel;

                threads::thread_init_data data(
                    threads::make_thread_function_nullary(
                        put_parcel_ptr, this, HPX_MOVE(p), HPX_MOVE(f)),
                    "parcelhandler::put_parcel",
                    threads::thread_priority::boost,
                    threads::thread_schedule_hint(),
                    threads::thread_stacksize::medium,
                    threads::thread_schedule_state::pending, true);
                threads::register_thread(data);
                return;
            }
        }

        // properly initialize parcel
        init_parcel(p);

        bool resolved_locally = true;

        if (!addr)
        {
            resolved_locally = agas::resolve_local(gid, addr);
        }

        write_handler_type wrapped_f =
            hpx::bind_front(&detail::parcel_sent_handler, HPX_MOVE(f));

        // If we were able to resolve the address(es) locally we send the
        // parcel directly to the destination.
        if (resolved_locally)
        {
            // dispatch to the message handler which is associated with the
            // encapsulated action
            using destination_pair =
                std::pair<std::shared_ptr<parcelport>, locality>;
            destination_pair const dest =
                find_appropriate_destination(addr.locality_);

            if (load_message_handlers_ && !hpx::is_stopped_or_shutting_down())
            {
                if (policies::message_handler* mh =
                        p.get_message_handler(dest.second))
                {
                    mh->put_parcel(
                        dest.second, HPX_MOVE(p), HPX_MOVE(wrapped_f));
                    return;
                }
            }

            dest.first->put_parcel(
                dest.second, HPX_MOVE(p), HPX_MOVE(wrapped_f));
            return;
        }

        // At least one of the addresses is locally unknown, route the parcel
        // to the AGAS managing the destination.
        ++count_routed_;

        agas::route(HPX_MOVE(p), HPX_MOVE(wrapped_f));
    }

    void parcelhandler::put_parcels(std::vector<parcel> parcels)
    {
        LPT_(debug).format("parcelhandler::put_parcels: submitted: {} parcels",
            parcels.size());

        // inform termination detection of a sent message
        for (auto const& p : parcels)
        {
            if (!p.does_termination_detection())
            {
                hpx::detail::dijkstra_make_black();
                break;
            }
        }

        std::vector<write_handler_type> handlers(parcels.size(),
            [this](std::error_code const& ec, parcel const& p) -> void {
                invoke_write_handler(ec, p);
                LPT_(debug).format(
                    "parcelhandler::put_parcels: handled: {}", p.parcel_id());
            });

        put_parcels_impl(HPX_MOVE(parcels), HPX_MOVE(handlers));
    }

    void parcelhandler::put_parcels(
        std::vector<parcel> parcels, std::vector<write_handler_type> funcs)
    {
        LPT_(debug).format(
            "parcelhandler::put_parcels (with handlers): submitted: {} parcels",
            parcels.size());

        // inform termination detection of a sent message
        for (auto const& p : parcels)
        {
            if (!p.does_termination_detection())
            {
                hpx::detail::dijkstra_make_black();
                break;
            }
        }

        std::vector<write_handler_type> handlers;

        handlers.reserve(parcels.size());
        for (std::size_t i = 0; i != parcels.size(); ++i)
        {
            handlers.emplace_back([this, f = HPX_MOVE(funcs[i])](
                                      std::error_code const& ec,
                                      parcel const& p) -> void {
                invoke_write_handler(ec, p);
                f(ec, p);

                LPT_(debug).format(
                    "parcelhandler::put_parcels: handled: {}", p.parcel_id());
            });
        }

        put_parcels_impl(HPX_MOVE(parcels), HPX_MOVE(handlers));
    }

    void parcelhandler::put_parcels_impl(std::vector<parcel>&& parcels,
        std::vector<write_handler_type>&& handlers)
    {
        HPX_ASSERT(is_networking_enabled_);

        if (parcels.size() != handlers.size())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "parcelhandler::put_parcels",
                "mismatched number of parcels and handlers");
        }

        // if this isn't an HPX thread, the stack space check will return false
        if (!this_thread::has_sufficient_stack_space() &&
            hpx::threads::threadmanager_is(hpx::state::running))
        {
            LPT_(debug).format(
                "parcelhandler::put_parcels_impl: rescheduling: {}",
                parcels.size());

            // reschedule request as an HPX thread to avoid hangs
            void (parcelhandler::*put_parcels_ptr)(std::vector<parcel>,
                std::vector<write_handler_type>) = &parcelhandler::put_parcels;

            threads::thread_init_data data(
                threads::make_thread_function_nullary(put_parcels_ptr, this,
                    HPX_MOVE(parcels), HPX_MOVE(handlers)),
                "parcelhandler::put_parcels", threads::thread_priority::boost,
                threads::thread_schedule_hint(),
                threads::thread_stacksize::medium,
                threads::thread_schedule_state::pending, true);
            threads::register_thread(data);
            return;
        }

        // partition parcels depending on whether their destination can be
        // resolved locally
        std::size_t const num_parcels = parcels.size();

        std::vector<parcel> resolved_parcels;
        resolved_parcels.reserve(num_parcels);
        std::vector<write_handler_type> resolved_handlers;
        resolved_handlers.reserve(num_parcels);

        using destination_pair =
            std::pair<std::shared_ptr<parcelport>, locality>;

        destination_pair resolved_dest;

        std::vector<parcel> nonresolved_parcels;
        std::vector<write_handler_type> nonresolved_handlers;

        nonresolved_parcels.reserve(num_parcels);
        nonresolved_handlers.reserve(num_parcels);

        naming::gid_type dest_locality;
        for (std::size_t i = 0; i != num_parcels; ++i)
        {
            parcel& p = parcels[i];

            // properly initialize parcel
            init_parcel(p);

            bool resolved_locally = true;
            naming::address& addr = p.addr();

            if (!addr)
            {
                resolved_locally = agas::resolve_local(p.destination(), addr);
            }

            write_handler_type f = hpx::bind_front(
                &detail::parcel_sent_handler, HPX_MOVE(handlers[i]));

            // make sure all parcels go to the same locality
            if (i == 0)
            {
                dest_locality = p.destination_locality();
            }
            else if (dest_locality != p.destination_locality())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "parcelhandler::put_parcels",
                    "mismatched destinations, all parcels are expected to "
                    "target the same locality");
            }

            // If we were able to resolve the address(es) locally we would send
            // the parcel directly to the destination.
            if (resolved_locally)
            {
                // dispatch to the message handler which is associated with the
                // encapsulated action
                destination_pair dest =
                    find_appropriate_destination(addr.locality_);

                if (load_message_handlers_)
                {
                    if (policies::message_handler* mh =
                            p.get_message_handler(dest.second))
                    {
                        mh->put_parcel(dest.second, HPX_MOVE(p), HPX_MOVE(f));
                        continue;
                    }
                }

                resolved_parcels.emplace_back(HPX_MOVE(p));
                resolved_handlers.emplace_back(HPX_MOVE(f));
                if (!resolved_dest.second)
                {
                    resolved_dest = dest;
                }
                else
                {
                    HPX_ASSERT(resolved_dest == dest);
                }
            }
            else
            {
                nonresolved_parcels.emplace_back(HPX_MOVE(p));
                nonresolved_handlers.emplace_back(HPX_MOVE(f));
            }
        }

        // handle parcel that have been locally resolved
        if (!resolved_parcels.empty())
        {
            HPX_ASSERT(!!resolved_dest.first && !!resolved_dest.second);
            resolved_dest.first->put_parcels(resolved_dest.second,
                HPX_MOVE(resolved_parcels), HPX_MOVE(resolved_handlers));
        }

        // At least one of the addresses is locally unknown, route the
        // parcel to the AGAS managing the destination.
        for (std::size_t i = 0; i != nonresolved_parcels.size(); ++i)
        {
            ++count_routed_;
            agas::route(HPX_MOVE(nonresolved_parcels[i]),
                HPX_MOVE(nonresolved_handlers[i]));
        }
    }

    void parcelhandler::invoke_write_handler(
        std::error_code const& ec, parcel const& p) const
    {
        write_handler_type const f = write_handler_;
        f(ec, p);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t parcelhandler::get_outgoing_queue_length(bool reset) const
    {
        std::int64_t parcel_count = 0;
        for (pports_type::value_type const& pp : pports_)
        {
            parcel_count += pp.second->get_pending_parcels_count(reset);
        }
        return parcel_count;
    }

    ///////////////////////////////////////////////////////////////////////////
    // default callback for put_parcel
    void default_write_handler(std::error_code const& ec, parcel const& p)
    {
        if (ec)
        {
            // If we are in a stopped state, ignore some errors
            if (hpx::is_stopped_or_shutting_down())
            {
                using ::asio::error::make_error_code;
                if (ec == make_error_code(::asio::error::connection_aborted) ||
                    ec == make_error_code(::asio::error::connection_reset) ||
                    ec == make_error_code(::asio::error::broken_pipe) ||
                    ec == make_error_code(::asio::error::not_connected) ||
                    ec == make_error_code(::asio::error::eof))
                {
                    return;
                }
            }
            else if (hpx::tolerate_node_faults())
            {
                if (ec ==
                    ::asio::error::make_error_code(
                        ::asio::error::connection_reset))
                {
                    return;
                }
            }

            // all unhandled exceptions terminate the whole application
            std::exception_ptr const exception = hpx::detail::get_exception(
                hpx::exception(ec), "default_write_handler", __FILE__, __LINE__,
                parcelset::dump_parcel(p));

            hpx::report_error(exception);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    policies::message_handler* parcelhandler::get_message_handler(
        char const* action, char const* message_handler_type,
        std::size_t num_messages, std::size_t interval, locality const& loc,
        error_code& ec)
    {
        if (!is_networking_enabled_)
        {
            return nullptr;
        }

        std::unique_lock<mutex_type> l(handlers_mtx_);

        handler_key_type key(loc, action);
        auto it = handlers_.find(key);
        if (it == handlers_.end())
        {
            std::shared_ptr<policies::message_handler> p;

            {
                // Just ignore the handlers_mtx_ while checking. We need to hold
                // the lock here to avoid multiple registrations that happens
                // right now in the parcel coalescing plugin
                [[maybe_unused]] hpx::util::ignore_while_checking const il(&l);

                p.reset(hpx::create_message_handler(message_handler_type,
                    action, find_parcelport(loc.type()), num_messages, interval,
                    ec));
            }

            it = handlers_.find(key);
            if (it != handlers_.end())
            {
                // if some other thread has created the entry in the meantime
                l.unlock();
                if (&ec != &throws)
                {
                    if ((*it).second)
                        ec = make_success_code();
                    else
                        ec = make_error_code(
                            hpx::error::bad_parameter, throwmode::lightweight);
                }
                return (*it).second.get();
            }

            if (ec || !p)
            {
                // insert an empty entry into the map to avoid trying to create
                // this handler again
                p.reset();
                std::pair<message_handler_map::iterator, bool> const r =
                    handlers_.emplace(key, p);

                l.unlock();
                if (!r.second)
                {
                    HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                        "parcelhandler::get_message_handler",
                        "could not store empty message handler");
                    return nullptr;
                }
                return nullptr;    // no message handler available
            }

            std::pair<message_handler_map::iterator, bool> const r =
                handlers_.emplace(key, p);

            l.unlock();
            if (!r.second)
            {
                HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                    "parcelhandler::get_message_handler",
                    "could not store newly created message handler");
                return nullptr;
            }
            it = r.first;
        }
        else if (!(*it).second)
        {
            l.unlock();
            if (&ec != &throws)
            {
                ec = make_error_code(
                    hpx::error::bad_parameter, throwmode::lightweight);
            }
            else
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "parcelhandler::get_message_handler",
                    "couldn't find an appropriate message handler");
            }
            return nullptr;    // no message handler available
        }

        if (&ec != &throws)
            ec = make_success_code();

        return (*it).second.get();
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string parcelhandler::get_locality_name() const
    {
        for (pports_type::value_type const& pp : pports_)
        {
            if (pp.first > 0)
            {
                std::string name = pp.second->get_locality_name();
                if (!name.empty())
                    return name;
            }
        }
        return "<unknown>";
    }

    ///////////////////////////////////////////////////////////////////////////
    // Performance counter data

    // number of parcels routed
    std::int64_t parcelhandler::get_parcel_routed_count(bool reset)
    {
        return util::get_and_reset_value(count_routed_, reset);
    }

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
    // number of parcels sent
    std::int64_t parcelhandler::get_parcel_send_count(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_parcel_send_count(reset) : 0;
    }

    // number of messages sent
    std::int64_t parcelhandler::get_message_send_count(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_message_send_count(reset) : 0;
    }

    // number of parcels received
    std::int64_t parcelhandler::get_parcel_receive_count(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_parcel_receive_count(reset) : 0;
    }

    // number of messages received
    std::int64_t parcelhandler::get_message_receive_count(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_message_receive_count(reset) : 0;
    }

    // the total time it took for all sends, from async_write to the
    // completion handler (nanoseconds)
    std::int64_t parcelhandler::get_sending_time(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_sending_time(reset) : 0;
    }

    // the total time it took for all receives, from async_read to the
    // completion handler (nanoseconds)
    std::int64_t parcelhandler::get_receiving_time(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_receiving_time(reset) : 0;
    }

    // the total time it took for all sender-side serialization operations
    // (nanoseconds)
    std::int64_t parcelhandler::get_sending_serialization_time(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_sending_serialization_time(reset) : 0;
    }

    // the total time it took for all receiver-side serialization
    // operations (nanoseconds)
    std::int64_t parcelhandler::get_receiving_serialization_time(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_receiving_serialization_time(reset) : 0;
    }

    // total data sent (bytes)
    std::int64_t parcelhandler::get_data_sent(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_data_sent(reset) : 0;
    }

    // total data (uncompressed) sent (bytes)
    std::int64_t parcelhandler::get_raw_data_sent(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_raw_data_sent(reset) : 0;
    }

    // total data received (bytes)
    std::int64_t parcelhandler::get_data_received(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_data_received(reset) : 0;
    }

    // total data (uncompressed) received (bytes)
    std::int64_t parcelhandler::get_raw_data_received(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_raw_data_received(reset) : 0;
    }

    std::int64_t parcelhandler::get_buffer_allocate_time_sent(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_buffer_allocate_time_sent(reset) : 0;
    }
    std::int64_t parcelhandler::get_buffer_allocate_time_received(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_buffer_allocate_time_received(reset) : 0;
    }

    // total zero-copy chunks sent
    std::int64_t parcelhandler::get_zchunks_send_count(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_send_count(reset) : 0;
    }

    // total zero-copy chunks received
    std::int64_t parcelhandler::get_zchunks_recv_count(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_recv_count(reset) : 0;
    }

    // the maximum number of zero-copy chunks per message sent
    std::int64_t parcelhandler::get_zchunks_send_per_msg_count_max(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_send_per_msg_count_max(reset) : 0;
    }

    // the maximum number of zero-copy chunks per message received
    std::int64_t parcelhandler::get_zchunks_recv_per_msg_count_max(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_recv_per_msg_count_max(reset) : 0;
    }

    // the size of zero-copy chunks per message sent
    std::int64_t parcelhandler::get_zchunks_send_size(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_send_size(reset) : 0;
    }

    // the size of zero-copy chunks per message received
    std::int64_t parcelhandler::get_zchunks_recv_size(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_recv_size(reset) : 0;
    }

    // the maximum size of zero-copy chunks per message sent
    std::int64_t parcelhandler::get_zchunks_send_size_max(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_send_size_max(reset) : 0;
    }

    // the maximum size of zero-copy chunks per message received
    std::int64_t parcelhandler::get_zchunks_recv_size_max(
        std::string const& pp_type, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_zchunks_recv_size_max(reset) : 0;
    }

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
    // same as above, just separated data for each action
    // number of parcels sent
    std::int64_t parcelhandler::get_action_parcel_send_count(
        std::string const& pp_type, std::string const& action, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_action_parcel_send_count(action, reset) : 0;
    }

    // number of parcels received
    std::int64_t parcelhandler::get_action_parcel_receive_count(
        std::string const& pp_type, std::string const& action, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_action_parcel_receive_count(action, reset) : 0;
    }

    // the total time it took for all sender-side serialization operations
    // (nanoseconds)
    std::int64_t parcelhandler::get_action_sending_serialization_time(
        std::string const& pp_type, std::string const& action, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_action_sending_serialization_time(action, reset) :
                    0;
    }

    // the total time it took for all receiver-side serialization
    // operations (nanoseconds)
    std::int64_t parcelhandler::get_action_receiving_serialization_time(
        std::string const& pp_type, std::string const& action, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_action_receiving_serialization_time(action, reset) :
                    0;
    }

    // total data sent (bytes)
    std::int64_t parcelhandler::get_action_data_sent(
        std::string const& pp_type, std::string const& action, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_action_data_sent(action, reset) : 0;
    }

    // total data received (bytes)
    std::int64_t parcelhandler::get_action_data_received(
        std::string const& pp_type, std::string const& action, bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_action_data_received(action, reset) : 0;
    }
#endif
#endif
    // connection stack statistics
    std::int64_t parcelhandler::get_connection_cache_statistics(
        std::string const& pp_type,
        parcelport::connection_cache_statistics_type stat_type,
        bool reset) const
    {
        error_code ec(throwmode::lightweight);
        parcelport* pp = find_parcelport(pp_type, ec);
        return pp ? pp->get_connection_cache_statistics(stat_type, reset) : 0;
    }

    std::vector<plugins::parcelport_factory_base*>&
    parcelhandler::get_parcelport_factories()
    {
        auto& factories = plugins::get_parcelport_factories();
        if (factories.empty() && hpx::is_networking_enabled())
        {
            init_static_parcelport_factories(factories);
        }
        return factories;
    }

    void parcelhandler::init(
        int* argc, char*** argv, util::command_line_handling& cfg)
    {
        HPX_ASSERT(hpx::is_networking_enabled());

        for (plugins::parcelport_factory_base* factory :
            get_parcelport_factories())
        {
            factory->init(argc, argv, cfg);
        }
    }

    void parcelhandler::init(hpx::resource::partitioner& rp)
    {
        HPX_ASSERT(hpx::is_networking_enabled());

        for (plugins::parcelport_factory_base* factory :
            get_parcelport_factories())
        {
            factory->init(rp);
        }
    }

    std::vector<std::string> load_runtime_configuration()
    {
        std::vector<std::string> ini_defs;

        ini_defs.emplace_back("[hpx.parcel]");
        ini_defs.emplace_back(
            "address = ${HPX_PARCEL_SERVER_ADDRESS:" HPX_INITIAL_IP_ADDRESS
            "}");
        ini_defs.emplace_back(
            "port = ${HPX_PARCEL_SERVER_PORT:" HPX_PP_STRINGIZE(
                HPX_INITIAL_IP_PORT) "}");
        ini_defs.emplace_back(
            "bootstrap = ${HPX_PARCEL_BOOTSTRAP:" HPX_PARCEL_BOOTSTRAP "}");
        ini_defs.emplace_back(
            "max_connections = ${HPX_PARCEL_MAX_CONNECTIONS:" HPX_PP_STRINGIZE(
                HPX_PARCEL_MAX_CONNECTIONS) "}");
        ini_defs.emplace_back(
            "max_connections_per_locality = "
            "${HPX_PARCEL_MAX_CONNECTIONS_PER_LOCALITY:" HPX_PP_STRINGIZE(
                HPX_PARCEL_MAX_CONNECTIONS_PER_LOCALITY) "}");
        ini_defs.emplace_back("max_message_size = "
                              "${HPX_PARCEL_MAX_MESSAGE_SIZE:" HPX_PP_STRINGIZE(
                                  HPX_PARCEL_MAX_MESSAGE_SIZE) "}");
        ini_defs.emplace_back(
            "max_outbound_message_size = "
            "${HPX_PARCEL_MAX_OUTBOUND_MESSAGE_SIZE:" HPX_PP_STRINGIZE(
                HPX_PARCEL_MAX_OUTBOUND_MESSAGE_SIZE) "}");
        ini_defs.emplace_back(endian::native == endian::big ?
                "endian_out = ${HPX_PARCEL_ENDIAN_OUT:big}" :
                "endian_out = ${HPX_PARCEL_ENDIAN_OUT:little}");
        ini_defs.emplace_back(
            "array_optimization = ${HPX_PARCEL_ARRAY_OPTIMIZATION:1}");
        ini_defs.emplace_back(
            "zero_copy_optimization = ${HPX_PARCEL_ZERO_COPY_OPTIMIZATION:"
            "$[hpx.parcel.array_optimization]}");
        ini_defs.emplace_back("zero_copy_receive_optimization = "
                              "${HPX_PARCEL_ZERO_COPY_RECEIVE_OPTIMIZATION:"
                              "$[hpx.parcel.zero_copy_optimization]}");
        ini_defs.emplace_back(
            "async_serialization = ${HPX_PARCEL_ASYNC_SERIALIZATION:1}");
#if defined(HPX_HAVE_PARCEL_COALESCING)
        ini_defs.emplace_back(
            "message_handlers = ${HPX_PARCEL_MESSAGE_HANDLERS:1}");
#else
        ini_defs.emplace_back(
            "message_handlers = ${HPX_PARCEL_MESSAGE_HANDLERS:0}");
#endif
        ini_defs.emplace_back(
            "zero_copy_serialization_threshold = "
            "${HPX_PARCEL_ZERO_COPY_SERIALIZATION_THRESHOLD:" HPX_PP_STRINGIZE(
                HPX_ZERO_COPY_SERIALIZATION_THRESHOLD) "}");
        ini_defs.emplace_back("max_background_threads = "
                              "${HPX_PARCEL_MAX_BACKGROUND_THREADS:-1}");

        for (plugins::parcelport_factory_base* f :
            parcelhandler::get_parcelport_factories())
        {
            f->get_plugin_info(ini_defs);
        }
        return ini_defs;
    }

    ///////////////////////////////////////////////////////////////////////////
    void parcelhandler::init_parcel(parcel& p)
    {
        // ensure the source locality id is set (if no component id is given)
        if (!p.source_id())
        {
            p.set_source_id(hpx::id_type(agas::get_locality(),
                hpx::id_type::management_type::unmanaged));
        }

#if defined(HPX_HAVE_PARCEL_PROFILING)
        // set the current local time for this locality
        p.set_start_time(hpx::chrono::high_resolution_timer::now());

        if (!p.parcel_id())
        {
            error_code ec(throwmode::lightweight);    // ignore all errors
            std::uint32_t locality_id = agas::get_locality_id(ec);
            p.parcel_id() = parcelset::parcel::generate_unique_id(locality_id);
        }
#endif
    }
}    // namespace hpx::parcelset

#endif
