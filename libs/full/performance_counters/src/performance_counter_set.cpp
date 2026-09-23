//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/performance_counter.hpp>
#include <hpx/performance_counters/performance_counter_set.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    performance_counter_set::performance_counter_set(
        std::string const& name, bool reset)
      : invocation_count_(0)
      , print_counters_locally_(false)
    {
        add_counters(name, reset);
    }

    performance_counter_set::performance_counter_set(
        std::vector<std::string> const& names, bool reset)
      : invocation_count_(0)
      , print_counters_locally_(false)
    {
        add_counters(names, reset);
    }

    void performance_counter_set::release()
    {
        std::vector<hpx::id_type> ids;

        {
            std::lock_guard<mutex_type> l(mtx_);
            infos_.clear();
            known_names_.clear();
            std::swap(ids_, ids);
        }
    }

    std::size_t performance_counter_set::size() const
    {
        std::lock_guard<mutex_type> l(mtx_);
        return ids_.size();
    }

    std::vector<counter_info> performance_counter_set::get_counter_infos() const
    {
        std::lock_guard<mutex_type> l(mtx_);
        return infos_;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool performance_counter_set::find_counter(
        counter_info const& info, bool reset, error_code& ec)
    {
        // keep only local counters if requested
        if (print_counters_locally_)
        {
            counter_path_elements p;
            get_counter_path_elements(info.fullname_, p, ec);
            if (ec)
                return false;

            if (p.parentinstanceindex_ != hpx::get_locality_id())
            {
                if (&ec != &throws)
                    ec = make_success_code();
                return true;
            }
        }

        // Discovery may be re-run for the same name pattern after the set
        // has already been populated, e.g. to pick up counters that were
        // registered only after this set was first filled (see #4627).
        // Skip counters that are already part of this set instead of
        // adding a duplicate entry. known_names_ mirrors the full names
        // already stored in infos_, so this lookup is O(log N) instead of
        // the O(N) linear scan a plain search over infos_ would need.
        {
            std::lock_guard<mutex_type> l(mtx_);
            auto it = known_names_.find(info.fullname_);
            if (it != known_names_.end())
            {
                // The same counter can be discovered once without reset
                // (e.g. via --hpx:print-counter) and again with reset
                // requested (e.g. via --hpx:print-counter-reset), either
                // because it matches both name lists or because a
                // wild-card pattern used by both expands to it. Promote
                // the existing entry's reset_ flag in place instead of
                // silently dropping the request, otherwise
                // --hpx:print-counter-reset would not reset it.
                if (reset)
                    reset_[it->second] = 1;

                if (&ec != &throws)
                    ec = make_success_code();
                return true;
            }
        }

        hpx::id_type id = get_counter(info.fullname_, ec);
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "performance_counter_set::find_counter",
                "unknown performance counter: '{1}' ({2})", info.fullname_,
                ec.get_message());
            return false;
        }

        {
            std::unique_lock<mutex_type> l(mtx_);

            // Another thread may have discovered and added the same
            // counter while get_counter() above was resolving 'id'.
            // Re-check under the lock so a concurrent caller cannot slip
            // a duplicate entry past the unlocked check above.
            auto it = known_names_.find(info.fullname_);
            if (it != known_names_.end())
            {
                if (reset)
                    reset_[it->second] = 1;

                if (&ec != &throws)
                    ec = make_success_code();
                return true;
            }

            known_names_.emplace(info.fullname_, infos_.size());
            infos_.push_back(info);
            ids_.push_back(HPX_MOVE(id));
            reset_.push_back(reset ? 1 : 0);
        }

        if (&ec != &throws)
            ec = make_success_code();

        return true;
    }

    namespace detail {

        // A wild-card pattern (e.g. /apex/*) that legitimately matches no
        // counter type yet, because its provider has not registered one at
        // this point (see #4627), is reported by discover_counter_type() as
        // counter_type_unknown -- the very same status used for a
        // genuinely unknown, non-wild-card counter name. Only the latter is
        // a real error: treating the former as one here would stop callers
        // such as query_counters::refresh_counters() from ever caching the
        // registry generation for such a pattern, forcing them to re-run
        // discovery on every single evaluation for as long as the counter
        // stays unregistered.
        bool is_benign_empty_wildcard_match(
            std::string const& pattern, counter_status status)
        {
            return status == counter_status::counter_type_unknown &&
                pattern.find_first_of("*?[]") != std::string::npos;
        }
    }    // namespace detail

    void performance_counter_set::add_counters(
        std::string const& name, bool reset, error_code& ec)
    {
        using placeholders::_1;
        using placeholders::_2;

        discover_counter_func func = hpx::bind(
            &performance_counter_set::find_counter, this, _1, reset, _2);

        // do INI expansion on counter name
        std::string n(name);
        util::expand(n);

        // find matching counter types
        error_code discover_ec(throwmode::lightweight);
        counter_status const status = discover_counter_type(
            n, HPX_MOVE(func), discover_counters_mode::full, discover_ec);

        if (discover_ec && !detail::is_benign_empty_wildcard_match(n, status))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "performance_counter_set::add_counters",
                "failed to discover counters matching '{1}' ({2})", n,
                discover_ec.get_message());
            return;
        }

        HPX_ASSERT(ids_.size() == infos_.size());

        if (&ec != &throws)
            ec = make_success_code();
    }

    void performance_counter_set::add_counters(
        std::vector<std::string> const& names, bool reset, error_code& ec)
    {
        using placeholders::_1;
        using placeholders::_2;

        discover_counter_func func = hpx::bind(
            &performance_counter_set::find_counter, this, _1, reset, _2);

        for (std::string const& name : names)
        {
            // do INI expansion on counter name
            std::string n(name);
            util::expand(n);

            // find matching counter types
            error_code discover_ec(throwmode::lightweight);
            counter_status const status = discover_counter_type(
                n, func, discover_counters_mode::full, discover_ec);

            if (discover_ec &&
                !detail::is_benign_empty_wildcard_match(n, status))
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "performance_counter_set::add_counters",
                    "failed to discover counters matching '{1}' ({2})", n,
                    discover_ec.get_message());
                return;
            }
        }

        HPX_ASSERT(ids_.size() == infos_.size());

        if (&ec != &throws)
            ec = make_success_code();
    }

    ///////////////////////////////////////////////////////////////////////////
    std::vector<hpx::future<bool>> performance_counter_set::start()
    {
        std::vector<hpx::id_type> ids;

        {
            std::unique_lock<mutex_type> l(mtx_);
            ids = ids_;
        }

        std::vector<hpx::future<bool>> v;
        v.reserve(ids.size());

        // start all performance counters
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            performance_counters::performance_counter c(ids[i]);
            v.emplace_back(c.start());
        }

        return v;
    }

    bool performance_counter_set::start(launch::sync_policy, error_code& ec)
    {
        try
        {
            auto v = hpx::unwrap(start());
            return std::all_of(
                v.begin(), v.end(), [](bool val) { return val; });
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "performance_counter_set::start");
            return false;
        }
    }

    std::vector<hpx::future<bool>> performance_counter_set::start(
        std::size_t first)
    {
        std::vector<hpx::id_type> ids;

        {
            std::unique_lock<mutex_type> l(mtx_);
            if (first < ids_.size())
            {
                ids.assign(ids_.begin() + static_cast<std::ptrdiff_t>(first),
                    ids_.end());
            }
        }

        std::vector<hpx::future<bool>> v;
        v.reserve(ids.size());

        // start only the counters at indices [first, size())
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            performance_counters::performance_counter c(ids[i]);
            v.emplace_back(c.start());
        }

        return v;
    }

    bool performance_counter_set::start(
        launch::sync_policy, std::size_t first, error_code& ec)
    {
        try
        {
            auto v = hpx::unwrap(start(first));
            return std::all_of(
                v.begin(), v.end(), [](bool val) { return val; });
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "performance_counter_set::start");
            return false;
        }
    }

    std::vector<hpx::future<bool>> performance_counter_set::stop()
    {
        std::vector<hpx::id_type> ids;

        {
            std::unique_lock<mutex_type> l(mtx_);
            ids = ids_;
        }

        std::vector<hpx::future<bool>> v;
        v.reserve(ids.size());

        // stop all performance counters
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            performance_counters::performance_counter c(ids[i]);
            v.emplace_back(c.stop());
        }

        return v;
    }

    bool performance_counter_set::stop(launch::sync_policy, error_code& ec)
    {
        try
        {
            auto v = hpx::unwrap(stop());
            return std::all_of(
                v.begin(), v.end(), [](bool val) { return val; });
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "performance_counter_set::stop");
            return false;
        }
    }

    std::vector<hpx::future<void>> performance_counter_set::reset()
    {
        std::vector<hpx::id_type> ids;

        {
            std::unique_lock<mutex_type> l(mtx_);
            ids = ids_;
        }

        std::vector<hpx::future<void>> v;
        v.reserve(ids.size());

        // reset all performance counters
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            performance_counters::performance_counter c(ids[i]);
            v.emplace_back(c.reset());
        }

        return v;
    }

    void performance_counter_set::reset(launch::sync_policy, error_code& ec)
    {
        try
        {
            hpx::unwrap(reset());
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "performance_counter_set::reset");
        }
    }

    std::vector<hpx::future<void>> performance_counter_set::reinit(bool reset)
    {
        std::vector<hpx::id_type> ids;

        {
            std::unique_lock<mutex_type> l(mtx_);
            ids = ids_;
        }

        std::vector<hpx::future<void>> v;
        v.reserve(ids.size());

        // re-initialize all performance counters
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            performance_counters::performance_counter c(ids[i]);
            v.emplace_back(c.reinit(reset));
        }

        return v;
    }

    void performance_counter_set::reinit(
        launch::sync_policy, bool reset, error_code& ec)
    {
        try
        {
            hpx::unwrap(reinit(reset));
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "performance_counter_set::reinit");
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    std::vector<hpx::future<counter_value>>
    performance_counter_set::get_counter_values(bool reset) const
    {
        std::vector<hpx::id_type> ids;
        std::vector<counter_info> infos;
        std::vector<std::uint8_t> resets;

        {
            std::unique_lock<mutex_type> l(mtx_);
            ids = ids_;
            infos = infos_;
            resets = reset_;
            ++invocation_count_;
        }

        std::vector<hpx::future<counter_value>> v;
        v.reserve(ids.size());

        // reset all performance counters
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            if (infos[i].type_ == counter_type::histogram ||
                infos[i].type_ == counter_type::raw_values)
            {
                continue;
            }

            performance_counters::performance_counter c(ids[i]);
            v.emplace_back(c.get_counter_value(reset || resets[i]));
        }

        return v;
    }

    std::vector<counter_value> performance_counter_set::get_counter_values(
        launch::sync_policy, bool reset, error_code& ec) const
    {
        try
        {
            return hpx::unwrap(get_counter_values(reset));
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(
                ec, e, "performance_counter_set::get_counter_values");
            return std::vector<counter_value>();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    std::vector<hpx::future<counter_values_array>>
    performance_counter_set::get_counter_values_array(bool reset) const
    {
        std::vector<hpx::id_type> ids;
        std::vector<counter_info> infos;
        std::vector<std::uint8_t> resets;

        {
            std::unique_lock<mutex_type> l(mtx_);
            ids = ids_;
            infos = infos_;
            resets = reset_;
            ++invocation_count_;
        }

        std::vector<hpx::future<counter_values_array>> v;
        v.reserve(ids.size());

        // reset all performance counters
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            if (infos[i].type_ != counter_type::histogram &&
                infos[i].type_ != counter_type::raw_values)
            {
                continue;
            }

            performance_counters::performance_counter c(ids[i]);
            v.emplace_back(c.get_counter_values_array(reset || resets[i]));
        }

        return v;
    }

    std::vector<counter_values_array>
    performance_counter_set::get_counter_values_array(
        launch::sync_policy, bool reset, error_code& ec) const
    {
        try
        {
            return hpx::unwrap(get_counter_values_array(reset));
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(
                ec, e, "performance_counter_set::get_counter_values_aray");
            return std::vector<counter_values_array>();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    std::size_t performance_counter_set::get_invocation_count() const
    {
        std::unique_lock<mutex_type> l(mtx_);
        return invocation_count_;
    }
}    // namespace hpx::performance_counters
