////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/agas_base/server/component_namespace.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace hpx::agas {

    naming::gid_type bootstrap_component_namespace_gid()
    {
        return naming::gid_type(agas::component_ns_msb, agas::component_ns_lsb);
    }

    hpx::id_type bootstrap_component_namespace_id()
    {
        return hpx::id_type(agas::component_ns_msb, agas::component_ns_lsb,
            hpx::id_type::management_type::unmanaged);
    }
}    // namespace hpx::agas

namespace hpx::agas::server {

    void component_namespace::register_server_instance(
        char const* servicename, error_code& ec)
    {
        // now register this AGAS instance with AGAS :-P
        instance_name_ = agas::service_name;
        instance_name_ += servicename;
        instance_name_ += agas::server::component_namespace_service_name;

        // register a gid (not the id) to avoid AGAS holding a reference to this
        // component
        agas::register_name(
            launch::sync, instance_name_, get_unmanaged_id().get_gid(), ec);
    }

    void component_namespace::unregister_server_instance(error_code& ec)
    {
        agas::unregister_name(launch::sync, instance_name_, ec);
        this->base_type::finalize();
    }

    void component_namespace::finalize()
    {
        if (!instance_name_.empty())
        {
            error_code ec(throwmode::lightweight);
            agas::unregister_name(launch::sync, instance_name_, ec);
        }
    }

    components::component_type component_namespace::bind_prefix(
        std::string const& key, std::uint32_t prefix)
    {    // {{{ bind_prefix implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.bind_prefix_.time_,
            counter_data_.bind_prefix_.enabled_);
        counter_data_.increment_bind_prefix_count();

        std::unique_lock<mutex_type> l(mutex_);

        auto cit = component_ids_.find(key);
        auto const cend = component_ids_.end();

        // This is the first request, so we use the type counter, and then
        // increment it.
        if (component_ids_.find(key) == cend)
        {
            if (HPX_UNLIKELY(!util::insert_checked(
                    component_ids_.emplace(key, type_counter), cit)))
            {
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::lock_error,
                    "component_namespace::bind_prefix",
                    "component id table insertion failed due to a locking "
                    "error or memory corruption");
            }

            // If the insertion succeeded, we need to increment the type
            // counter.
            ++type_counter;
        }

        factory_table_type::iterator fit = factories_.find(cit->second),
                                     fend = factories_.end();

        if (fit != fend)
        {
            prefixes_type& prefixes = fit->second;
            prefixes_type::iterator pit = prefixes.find(prefix);

            if (pit != prefixes.end())
            {
                // Duplicate type registration for this locality.
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::duplicate_component_id,
                    "component_namespace::bind_prefix",
                    "component id is already registered for the given "
                    "locality, key({1}), prefix({2}), ctype({3})",
                    key, prefix, cit->second);
            }

            fit->second.insert(prefix);

            // First registration for this locality, we still return no_success to
            // convey the fact that another locality already registered this
            // component type.
            LAGAS_(info).format(
                "component_namespace::bind_prefix, key({1}), prefix({2}), "
                "ctype({3}), response(no_success)",
                key, prefix, cit->second);

            return cit->second;
        }

        // Instead of creating a temporary and then inserting it, we insert
        // an empty set, then put the prefix into said set. This should
        // prevent a copy, though most compilers should be able to optimize
        // this without our help.
        if (HPX_UNLIKELY(!util::insert_checked(
                factories_.emplace(cit->second, prefixes_type()), fit)))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::lock_error,
                "component_namespace::bind_prefix",
                "factory table insertion failed due to a locking "
                "error or memory corruption");
        }

        fit->second.insert(prefix);

        LAGAS_(info).format(
            "component_namespace::bind_prefix, key({1}), prefix({2}), "
            "ctype({3})",
            key, prefix, cit->second);

        return cit->second;
    }    // }}}

    components::component_type component_namespace::bind_name(
        std::string const& key)
    {    // {{{ bind_name implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.bind_name_.time_, counter_data_.bind_name_.enabled_);
        counter_data_.increment_bind_name_count();

        std::unique_lock<mutex_type> l(mutex_);

        auto it = component_ids_.find(key);
        auto const end = component_ids_.end();

        // If the name is not in the table, register it (this is only done so
        // we can implement a backwards compatible get_component_id).
        if (it == end)
        {
            if (HPX_UNLIKELY(!util::insert_checked(
                    component_ids_.emplace(key, type_counter), it)))
            {
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::lock_error,
                    "component_namespace::bind_name",
                    "component id table insertion failed due to a locking "
                    "error or memory corruption");
            }

            // If the insertion succeeded, we need to increment the type
            // counter.
            ++type_counter;
        }

        LAGAS_(info).format(
            "component_namespace::bind_name, key({1}), ctype({2})", key,
            it->second);

        return it->second;
    }    // }}}

    std::vector<std::uint32_t> component_namespace::resolve_id(
        components::component_type key)
    {    // {{{ resolve_id implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.resolve_id_.time_,
            counter_data_.resolve_id_.enabled_);
        counter_data_.increment_resolve_id_count();

        // If the requested component type is a derived type, use only its derived
        // part for the lookup.
        if (key != components::get_base_type(key))
            key = components::get_derived_type(key);

        std::lock_guard<mutex_type> l(mutex_);

        factory_table_type::const_iterator it = factories_.find(key),
                                           end = factories_.end();

        // REVIEW: Should we differentiate between these two cases? Should we
        // throw an exception if it->second.empty()? It should be impossible.
        if (it == end || it->second.empty())
        {
            LAGAS_(info).format(
                "component_namespace::resolve_id, key({1}), localities(0)",
                key);

            return std::vector<std::uint32_t>();
        }

        else
        {
            prefixes_type const& prefixes = it->second;

            std::vector<std::uint32_t> p;
            p.assign(prefixes.cbegin(), prefixes.cend());

            LAGAS_(info).format("component_namespace::resolve_id, "
                                "key({1}), localities({2})",
                key, prefixes.size());

            return p;
        }
    }    // }}}

    bool component_namespace::unbind(std::string const& key)
    {    // {{{ unbind implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.unbind_name_.time_,
            counter_data_.unbind_name_.enabled_);
        counter_data_.increment_unbind_name_count();

        std::lock_guard<mutex_type> l(mutex_);

        auto const it = component_ids_.find(key);

        // REVIEW: Should this be an error?
        if (it == component_ids_.end())
        {
            LAGAS_(info).format("component_namespace::unbind, key({1}), "
                                "response(no_success)",
                key);

            return false;
        }

        // REVIEW: If there are no localities with this type, should we throw
        // an exception here?
        factories_.erase(it->second);
        component_ids_.erase(it);

        LAGAS_(info).format("component_namespace::unbind, key({1})", key);

        return true;
    }    // }}}

    // TODO: catch exceptions
    void component_namespace::iterate_types(
        iterate_types_function_type const& f)
    {    // {{{ iterate implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.iterate_types_.time_,
            counter_data_.iterate_types_.enabled_);
        counter_data_.increment_iterate_types_count();

        using value_type = std::pair<typename component_id_table_type::key_type,
            typename component_id_table_type::mapped_type>;
        std::vector<value_type> types;
        {
            std::lock_guard<mutex_type> l(mutex_);

            types.assign(component_ids_.cbegin(), component_ids_.cend());
        }

        for (auto&& type : types)
        {
            f(type.first, type.second);
        }

        LAGAS_(info).format("component_namespace::iterate_types");
    }    // }}}

    static std::string get_component_name(
        component_namespace::component_id_table_type const& m,
        components::component_type t)
    {
        if (t < to_int(components::component_enum_type::last))
            return components::get_component_type_name(t);

        for (auto const& c : m)
        {
            if (c.second == t)
                return c.first;
        }
        return "";
    }

    std::string component_namespace::get_component_type_name(
        components::component_type t)
    {    // {{{ get_component_type_name implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.get_component_type_name_.time_,
            counter_data_.get_component_type_name_.enabled_);
        counter_data_.increment_get_component_type_name_count();

        std::lock_guard<mutex_type> l(mutex_);

        std::string result;

        if (t == to_int(hpx::components::component_enum_type::invalid))
        {
            result = "component_invalid";
        }
        else if (components::get_derived_type(t) == 0)
        {
            result = get_component_name(component_ids_, t);
        }
        else if (components::get_derived_type(t) != 0)
        {
            result = get_component_name(
                component_ids_, components::get_derived_type(t));
            result += "/";
            result += get_component_name(
                component_ids_, components::get_base_type(t));
        }

        if (result.empty())
        {
            LAGAS_(info).format("component_namespace::get_component_"
                                "typename, key({1}/{2}), "
                                "response(no_success)",
                int(components::get_derived_type(t)),
                int(components::get_base_type(t)));

            return result;
        }

        LAGAS_(info).format(
            "component_namespace::get_component_typename, key({1}/{2}), "
            "response({3})",
            int(components::get_derived_type(t)),
            int(components::get_base_type(t)), result);

        return result;
    }    // }}}

    std::uint32_t component_namespace::get_num_localities(
        components::component_type key)
    {    // {{{ get_num_localities implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.num_localities_.time_,
            counter_data_.num_localities_.enabled_);
        counter_data_.increment_num_localities_count();

        // If the requested component type is a derived type, use only its derived
        // part for the lookup.
        if (key != components::get_base_type(key))
            key = components::get_derived_type(key);

        std::lock_guard<mutex_type> l(mutex_);

        factory_table_type::const_iterator it = factories_.find(key),
                                           end = factories_.end();
        if (it == end)
        {
            LAGAS_(info).format(
                "component_namespace::get_num_localities, key({1}), "
                "localities(0)",
                key);

            return std::uint32_t(0);
        }

        std::uint32_t num_localities =
            static_cast<std::uint32_t>(it->second.size());

        LAGAS_(info).format(
            "component_namespace::get_num_localities, key({1}), "
            "localities({2})",
            key, num_localities);

        return num_localities;
    }    // }}}

    // access current counter values
    std::int64_t component_namespace::counter_data::get_bind_prefix_count(
        bool reset)
    {
        return util::get_and_reset_value(bind_prefix_.count_, reset);
    }

    std::int64_t component_namespace::counter_data::get_bind_name_count(
        bool reset)
    {
        return util::get_and_reset_value(bind_name_.count_, reset);
    }

    std::int64_t component_namespace::counter_data::get_resolve_id_count(
        bool reset)
    {
        return util::get_and_reset_value(resolve_id_.count_, reset);
    }

    std::int64_t component_namespace::counter_data::get_unbind_name_count(
        bool reset)
    {
        return util::get_and_reset_value(unbind_name_.count_, reset);
    }

    std::int64_t component_namespace::counter_data::get_iterate_types_count(
        bool reset)
    {
        return util::get_and_reset_value(iterate_types_.count_, reset);
    }

    std::int64_t
    component_namespace::counter_data ::get_component_type_name_count(
        bool reset)
    {
        return util::get_and_reset_value(
            get_component_type_name_.count_, reset);
    }

    std::int64_t component_namespace::counter_data::get_num_localities_count(
        bool reset)
    {
        return util::get_and_reset_value(num_localities_.count_, reset);
    }

    std::int64_t component_namespace::counter_data::get_overall_count(
        bool reset)
    {
        return util::get_and_reset_value(bind_prefix_.count_, reset) +
            util::get_and_reset_value(bind_name_.count_, reset) +
            util::get_and_reset_value(resolve_id_.count_, reset) +
            util::get_and_reset_value(unbind_name_.count_, reset) +
            util::get_and_reset_value(iterate_types_.count_, reset) +
            util::get_and_reset_value(get_component_type_name_.count_, reset) +
            util::get_and_reset_value(num_localities_.count_, reset);
    }

    void component_namespace::counter_data::enable_all()
    {
        bind_prefix_.enabled_ = true;
        bind_name_.enabled_ = true;
        resolve_id_.enabled_ = true;
        unbind_name_.enabled_ = true;
        iterate_types_.enabled_ = true;
        get_component_type_name_.enabled_ = true;
        num_localities_.enabled_ = true;
    }

    // access execution time counters
    std::int64_t component_namespace::counter_data::get_bind_prefix_time(
        bool reset)
    {
        return util::get_and_reset_value(bind_prefix_.time_, reset);
    }

    std::int64_t component_namespace::counter_data::get_bind_name_time(
        bool reset)
    {
        return util::get_and_reset_value(bind_name_.time_, reset);
    }

    std::int64_t component_namespace::counter_data::get_resolve_id_time(
        bool reset)
    {
        return util::get_and_reset_value(resolve_id_.time_, reset);
    }

    std::int64_t component_namespace::counter_data::get_unbind_name_time(
        bool reset)
    {
        return util::get_and_reset_value(unbind_name_.time_, reset);
    }

    std::int64_t component_namespace::counter_data::get_iterate_types_time(
        bool reset)
    {
        return util::get_and_reset_value(iterate_types_.time_, reset);
    }

    std::int64_t
    component_namespace::counter_data ::get_component_type_name_time(bool reset)
    {
        return util::get_and_reset_value(get_component_type_name_.time_, reset);
    }

    std::int64_t component_namespace::counter_data::get_num_localities_time(
        bool reset)
    {
        return util::get_and_reset_value(num_localities_.time_, reset);
    }

    std::int64_t component_namespace::counter_data::get_overall_time(bool reset)
    {
        return util::get_and_reset_value(bind_prefix_.time_, reset) +
            util::get_and_reset_value(bind_name_.time_, reset) +
            util::get_and_reset_value(resolve_id_.time_, reset) +
            util::get_and_reset_value(unbind_name_.time_, reset) +
            util::get_and_reset_value(iterate_types_.time_, reset) +
            util::get_and_reset_value(get_component_type_name_.time_, reset) +
            util::get_and_reset_value(num_localities_.time_, reset);
    }

    // increment counter values
    void component_namespace::counter_data::increment_bind_prefix_count()
    {
        if (bind_prefix_.enabled_)
        {
            ++bind_prefix_.count_;
        }
    }

    void component_namespace::counter_data::increment_bind_name_count()
    {
        if (bind_name_.enabled_)
        {
            ++bind_name_.count_;
        }
    }

    void component_namespace::counter_data::increment_resolve_id_count()
    {
        if (resolve_id_.enabled_)
        {
            ++resolve_id_.count_;
        }
    }

    void component_namespace::counter_data::increment_unbind_name_count()
    {
        if (unbind_name_.enabled_)
        {
            ++unbind_name_.count_;
        }
    }

    void component_namespace::counter_data::increment_iterate_types_count()
    {
        if (iterate_types_.enabled_)
        {
            ++iterate_types_.count_;
        }
    }

    void
    component_namespace::counter_data::increment_get_component_type_name_count()
    {
        if (get_component_type_name_.enabled_)
        {
            ++get_component_type_name_.count_;
        }
    }

    void component_namespace::counter_data::increment_num_localities_count()
    {
        if (num_localities_.enabled_)
        {
            ++num_localities_.count_;
        }
    }
}    // namespace hpx::agas::server
