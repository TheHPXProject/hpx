//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2023 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/agas_base/server/symbol_namespace.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    naming::gid_type bootstrap_symbol_namespace_gid()
    {
        return naming::gid_type(agas::symbol_ns_msb, agas::symbol_ns_lsb);
    }

    hpx::id_type bootstrap_symbol_namespace_id()
    {
        return {agas::symbol_ns_msb, agas::symbol_ns_lsb,
            hpx::id_type::management_type::unmanaged};
    }
}    // namespace hpx::agas

namespace hpx::agas::server {

    void symbol_namespace::register_server_instance(
        char const* servicename, std::uint32_t locality_id, error_code& ec)
    {
        // set locality_id for this component
        if (locality_id == naming::invalid_locality_id)
            locality_id = 0;    // if not given, we're on the root

        this->base_type::set_locality_id(locality_id);

        // now register this AGAS instance with AGAS :-P
        instance_name_ = agas::service_name;
        instance_name_ += servicename;
        instance_name_ += agas::server::symbol_namespace_service_name;

        // register a gid (not the id) to avoid AGAS holding a reference to this
        // component
        agas::register_name(
            launch::sync, instance_name_, get_unmanaged_id().get_gid(), ec);
    }

    void symbol_namespace::unregister_server_instance(error_code& ec) const
    {
        agas::unregister_name(launch::sync, instance_name_, ec);
        this->base_type::finalize();
    }

    void symbol_namespace::finalize() const
    {
        if (!instance_name_.empty())
        {
            error_code ec(throwmode::lightweight);
            agas::unregister_name(launch::sync, instance_name_, ec);
        }
    }

    bool symbol_namespace::bind(
        std::string const& key, naming::gid_type const& gid_)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.bind_.time_, counter_data_.bind_.enabled_);
        counter_data_.increment_bind_count();

        std::unique_lock<mutex_type> l(mutex_);

        naming::gid_type gid = gid_;

        auto const it = gids_.find(key);
        if (auto const end = gids_.end(); it != end)
        {
            std::int64_t const credits =
                naming::detail::get_credit_from_gid(gid);
            naming::gid_type raw_gid = *(it->second);

            naming::detail::strip_internal_bits_from_gid(raw_gid);
            naming::detail::strip_internal_bits_from_gid(gid);

            // increase reference count
            if (raw_gid == gid)
            {
                // REVIEW: do we need to add the credit of the argument to the
                // table?
                naming::detail::add_credit_to_gid(*(it->second), credits);

                l.unlock();

                LAGAS_(info).format(
                    "symbol_namespace::bind, key({1}), gid({2}), "
                    "old_credit({3}), new_credit({4})",
                    key, gid,
                    naming::detail::get_credit_from_gid(*(it->second)),
                    naming::detail::get_credit_from_gid(*(it->second)) +
                        credits);

                return true;
            }

            if (LAGAS_ENABLED(info))
            {
                naming::detail::add_credit_to_gid(gid, credits);

                l.unlock();
                LAGAS_(info).format(
                    "symbol_namespace::bind, key({1}), gid({2}), "
                    "response(no_success)",
                    key, gid);
            }

            return false;
        }

        if (HPX_UNLIKELY(!util::insert_checked(
                gids_.emplace(key, std::make_shared<naming::gid_type>(gid)))))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::lock_error,
                "symbol_namespace::bind",
                "GID table insertion failed due to a locking error or "
                "memory corruption");
        }

        // handle registered events
        if (auto const [first, last] = on_event_data_.equal_range(key);
            first != last)
        {
            std::vector<hpx::id_type> lcos;

            auto iter = first;
            while (iter != last)
            {
                lcos.push_back(iter->second);
                ++iter;
            }

            on_event_data_.erase(first, last);

            // notify all LCOS which were registered with this name
            for (hpx::id_type const& id : lcos)
            {
                // re-locate the entry in the GID table for each LCO anew, as we
                // need to unlock the mutex protecting the table for each
                // iteration below
                auto gid_it = gids_.find(key);
                if (gid_it == gids_.end())
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "symbol_namespace::bind",
                        "unable to re-locate the entry in the GID table");
                }

                {
                    // hold on to the gid while the map is unlocked
                    std::shared_ptr<naming::gid_type> current_gid =
                        gid_it->second;

                    unlock_guard<std::unique_lock<mutex_type>> ul(l);

                    // split the credit as the receiving end will expect to keep
                    // the object alive
                    naming::gid_type new_gid =
                        naming::detail::split_gid_if_needed(
                            hpx::launch::sync, *current_gid);

                    // trigger the lco
                    set_lco_value(id, new_gid);

                    LAGAS_(info).format(
                        "symbol_namespace::bind, notify: key({1}), "
                        "stored_gid({2}), new_gid({3})",
                        key, *current_gid, new_gid);
                }
            }
        }

        l.unlock();

        LAGAS_(info).format(
            "symbol_namespace::bind, key({1}), gid({2})", key, gid);

        return true;
    }

    naming::gid_type symbol_namespace::resolve(std::string const& key)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.resolve_.time_, counter_data_.resolve_.enabled_);
        counter_data_.increment_resolve_count();

        std::unique_lock<mutex_type> l(mutex_);

        auto const it = gids_.find(key);
        if (auto const end = gids_.end(); it == end)
        {
            l.unlock();

            LAGAS_(info).format(
                "symbol_namespace::resolve, key({1}), response(no_success)",
                key);

            return naming::invalid_gid;
        }

        // hold on to gid before unlocking the map
        std::shared_ptr<naming::gid_type> const current_gid(it->second);

        l.unlock();

        naming::gid_type gid = naming::detail::split_gid_if_needed(
            hpx::launch::sync, *current_gid);

        LAGAS_(info).format("symbol_namespace::resolve, key({1}), "
                            "stored_gid({2}), gid({3})",
            key, *current_gid, gid);

        return gid;
    }

    naming::gid_type symbol_namespace::unbind(std::string const& key)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.unbind_.time_, counter_data_.unbind_.enabled_);
        counter_data_.increment_unbind_count();

        std::unique_lock<mutex_type> l(mutex_);

        auto const it = gids_.find(key);
        if (auto const end = gids_.end(); it == end)
        {
            l.unlock();

            LAGAS_(info).format(
                "symbol_namespace::unbind, key({1}), response(no_success)",
                key);

            return naming::invalid_gid;
        }

        naming::gid_type gid = *(it->second);

        gids_.erase(it);

        l.unlock();

        LAGAS_(info).format(
            "symbol_namespace::unbind, key({1}), gid({2})", key, gid);

        return gid;
    }

    // TODO: catch exceptions
    symbol_namespace::iterate_names_return_type symbol_namespace::iterate(
        std::string const& pattern)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.iterate_names_.time_,
            counter_data_.iterate_names_.enabled_);
        counter_data_.increment_iterate_names_count();

        std::map<std::string, naming::gid_type> found;

        if (pattern.find_first_of("*?[]") != std::string::npos)
        {
            std::string const str_rx(util::regex_from_pattern(pattern, throws));
            std::regex const rx(str_rx);

            std::unique_lock<mutex_type> l(mutex_);
            for (auto& gid : gids_)
            {
                if (!std::regex_match(gid.first, rx))
                    continue;

                // hold on to entry while map is unlocked
                std::shared_ptr<naming::gid_type> current_gid(gid.second);
                unlock_guard<std::unique_lock<mutex_type>> ul(l);

                found[gid.first] = naming::detail::split_gid_if_needed(
                    hpx::launch::sync, *current_gid);
            }
        }
        else
        {
            std::unique_lock<mutex_type> l(mutex_);
            for (auto& gid : gids_)
            {
                if (!pattern.empty() && pattern != gid.first)
                    continue;

                // hold on to entry while map is unlocked
                std::shared_ptr<naming::gid_type> current_gid(gid.second);
                unlock_guard<std::unique_lock<mutex_type>> ul(l);

                found[gid.first] = naming::detail::split_gid_if_needed(
                    hpx::launch::sync, *current_gid);
            }
        }

        LAGAS_(info).format("symbol_namespace::iterate");

        return found;
    }

    bool symbol_namespace::on_event(std::string const& name,
        bool call_for_past_events, hpx::id_type const& lco)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.on_event_.time_, counter_data_.on_event_.enabled_);
        counter_data_.increment_on_event_count();

        std::unique_lock<mutex_type> l(mutex_);

        bool handled = false;

        if (call_for_past_events)
        {
            if (auto const it = gids_.find(name); it != gids_.end())
            {
                // split the credit as the receiving end will expect to keep the
                // object alive
                {
                    // hold on to entry while map is unlocked
                    std::shared_ptr<naming::gid_type> const current_gid(
                        it->second);

                    unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    naming::gid_type new_gid =
                        naming::detail::split_gid_if_needed(
                            hpx::launch::sync, *current_gid);

                    // trigger the lco
                    handled = true;

                    // trigger LCO as name is already bound to an id
                    set_lco_value(lco, new_gid);

                    LAGAS_(info).format(
                        "symbol_namespace::on_event, notify: key({1}), "
                        "stored_gid({2}), new_gid({3})",
                        name, *current_gid, new_gid);
                }
            }
        }

        if (!handled)
        {
            [[maybe_unused]] auto const it = on_event_data_.emplace(name, lco);

            // This overload of insert always returns the iterator pointing
            // to the inserted value. It should never point to end
            HPX_ASSERT_LOCKED(l, it != on_event_data_.end());
        }

        l.unlock();

        LAGAS_(info).format("symbol_namespace::on_event: name({1})", name);

        return true;
    }

    // access current counter values
    std::int64_t symbol_namespace::counter_data::get_bind_count(bool reset)
    {
        return util::get_and_reset_value(bind_.count_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_resolve_count(bool reset)
    {
        return util::get_and_reset_value(resolve_.count_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_unbind_count(bool reset)
    {
        return util::get_and_reset_value(unbind_.count_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_iterate_names_count(
        bool reset)
    {
        return util::get_and_reset_value(iterate_names_.count_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_on_event_count(bool reset)
    {
        return util::get_and_reset_value(on_event_.count_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_overall_count(bool reset)
    {
        return util::get_and_reset_value(bind_.count_, reset) +
            util::get_and_reset_value(resolve_.count_, reset) +
            util::get_and_reset_value(unbind_.count_, reset) +
            util::get_and_reset_value(iterate_names_.count_, reset) +
            util::get_and_reset_value(on_event_.count_, reset);
    }

    void symbol_namespace::counter_data::enable_all()
    {
        bind_.enabled_ = true;
        resolve_.enabled_ = true;
        unbind_.enabled_ = true;
        iterate_names_.enabled_ = true;
        on_event_.enabled_ = true;
    }

    // access execution time counters
    std::int64_t symbol_namespace::counter_data::get_bind_time(bool reset)
    {
        return util::get_and_reset_value(bind_.time_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_resolve_time(bool reset)
    {
        return util::get_and_reset_value(resolve_.time_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_unbind_time(bool reset)
    {
        return util::get_and_reset_value(unbind_.time_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_iterate_names_time(
        bool reset)
    {
        return util::get_and_reset_value(iterate_names_.time_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_on_event_time(bool reset)
    {
        return util::get_and_reset_value(on_event_.time_, reset);
    }

    std::int64_t symbol_namespace::counter_data::get_overall_time(bool reset)
    {
        return util::get_and_reset_value(bind_.time_, reset) +
            util::get_and_reset_value(resolve_.time_, reset) +
            util::get_and_reset_value(unbind_.time_, reset) +
            util::get_and_reset_value(iterate_names_.time_, reset) +
            util::get_and_reset_value(on_event_.time_, reset);
    }

    // increment counter values
    void symbol_namespace::counter_data::increment_bind_count()
    {
        if (bind_.enabled_)
        {
            ++bind_.count_;
        }
    }

    void symbol_namespace::counter_data::increment_resolve_count()
    {
        if (resolve_.enabled_)
        {
            ++resolve_.count_;
        }
    }

    void symbol_namespace::counter_data::increment_unbind_count()
    {
        if (unbind_.enabled_)
        {
            ++unbind_.count_;
        }
    }

    void symbol_namespace::counter_data::increment_iterate_names_count()
    {
        if (iterate_names_.enabled_)
        {
            ++iterate_names_.count_;
        }
    }

    void symbol_namespace::counter_data::increment_on_event_count()
    {
        if (on_event_.enabled_)
        {
            ++on_event_.count_;
        }
    }
}    // namespace hpx::agas::server
