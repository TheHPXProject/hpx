//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/performance_counter.hpp>
#include <hpx/performance_counters/query_counters.hpp>
#include <hpx/performance_counters/registry.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    query_counters::query_counters(std::vector<std::string> const& names,
        std::vector<std::string> const& reset_names, std::int64_t interval,
        std::string const& dest, std::string const& form,
        std::vector<std::string> const& shortnames, bool csv_header,
        bool print_counters_locally, bool counter_types)
      : names_(names)
      , reset_names_(reset_names)
      , counters_(print_counters_locally)
      , destination_(dest)
      , format_(form)
      , counter_shortnames_(shortnames)
      , csv_header_(csv_header)
      , print_counters_locally_(print_counters_locally)
      , counter_types_(counter_types)
      , started_(false)
      , last_known_generation_(0)
      , timer_(hpx::bind_front(&query_counters::evaluate, this_(), false),
            hpx::bind_front(&query_counters::terminate, this_()),
            interval * 1000, "query_counters", true)
    {
        // add counter prefix, if necessary
        for (std::string& name : names_)
        {
            performance_counters::ensure_counter_prefix(name);
        }
        for (std::string& name : reset_names_)
        {
            performance_counters::ensure_counter_prefix(name);
        }
    }

    query_counters::~query_counters()
    {
        counters_.release();
    }

    bool query_counters::find_counters()
    {
        // A wild-card pattern legitimately matching no counter type yet,
        // e.g. because its provider has not registered one at this point
        // (see #4627), must not abort start() with the default ec
        // (throws). This mirrors the lightweight handling
        // refresh_counters() already uses for the same reason.
        bool success = true;

        if (!names_.empty())
        {
            error_code ec(throwmode::lightweight);
            counters_.add_counters(names_, false, ec);
            if (ec)
            {
                success = false;
                LPCS_(debug).format(
                    "query_counters::find_counters: failed to discover "
                    "counters ({})",
                    ec.get_message());
            }
        }
        if (!reset_names_.empty())
        {
            error_code ec(throwmode::lightweight);
            counters_.add_counters(reset_names_, true, ec);
            if (ec)
            {
                success = false;
                LPCS_(debug).format(
                    "query_counters::find_counters: failed to discover "
                    "reset counters ({})",
                    ec.get_message());
            }
        }

        for (auto const& info : counters_.get_counter_infos())
        {
            std::string real_name =
                performance_counters::remove_counter_prefix(info.fullname_);
            hpx::tracing::create_counter(info.fullname_, real_name);
        }

        return success;
    }

    bool query_counters::refresh_counters()
    {
        // Snapshot the registry generation before running discovery. If
        // another counter type is registered concurrently while discovery
        // below is in flight, this snapshot will already be stale, but
        // that's fine: it simply means the next evaluate() will see a
        // generation mismatch again and refresh once more.
        std::uint64_t const current_generation =
            performance_counters::registry::instance().generation();

        // Re-discovery is opportunistic: a name pattern that legitimately
        // matches nothing new should not abort the pending evaluation.
        // Each call below gets its own lightweight error_code and is
        // logged at debug level rather than thrown, so a genuine failure
        // (for instance an AGAS resolution problem) stays visible without
        // aborting the pending evaluation. Discovery is still tracked as
        // failed in that case, below, so that the generation snapshot
        // taken above is not cached and the missed counter(s) are retried
        // on the next evaluation instead of being forgotten for good.
        std::size_t const size_before = counters_.size();
        bool success = true;

        if (!names_.empty())
        {
            error_code ec(throwmode::lightweight);
            counters_.add_counters(names_, false, ec);
            if (ec)
            {
                success = false;
                LPCS_(debug).format(
                    "query_counters::refresh_counters: failed to refresh "
                    "counters ({})",
                    ec.get_message());
            }
        }
        if (!reset_names_.empty())
        {
            error_code ec(throwmode::lightweight);
            counters_.add_counters(reset_names_, true, ec);
            if (ec)
            {
                success = false;
                LPCS_(debug).format(
                    "query_counters::refresh_counters: failed to refresh "
                    "reset counters ({})",
                    ec.get_message());
            }
        }

        std::vector<performance_counters::counter_info> const infos =
            counters_.get_counter_infos();
        if (infos.size() <= size_before)
        {
            // Only cache a generation that discovery fully covered. If
            // either add_counters() call above failed, storing
            // current_generation here would let a later periodic
            // evaluate() skip discovery while the requested counter is
            // still missing (see review discussion on #7562).
            if (success)
            {
                last_known_generation_.store(
                    current_generation, std::memory_order_relaxed);
            }
            return success;    // nothing new was discovered
        }

        // Only the newly discovered counters, at indices
        // [size_before, infos.size()), need to be started; the rest were
        // already started by an earlier call.
        error_code ec2(throwmode::lightweight);
        counters_.start(launch::sync, size_before, ec2);
        if (ec2)
        {
            success = false;
            LPCS_(debug).format(
                "query_counters::refresh_counters: failed to start newly "
                "discovered counters ({})",
                ec2.get_message());
        }

        for (std::size_t i = size_before; i != infos.size(); ++i)
        {
            std::string const real_name =
                performance_counters::remove_counter_prefix(infos[i].fullname_);
            hpx::tracing::create_counter(infos[i].fullname_, real_name);
        }

        // Only cache a generation that discovery, including starting any
        // newly found counters, fully covered end to end. If any step
        // above failed, storing current_generation here would let a
        // later periodic evaluate() skip re-discovery while the
        // requested counter is still missing or unstarted (see review
        // discussion on #7562).
        if (success)
        {
            last_known_generation_.store(
                current_generation, std::memory_order_relaxed);
        }

        return success;
    }

    void query_counters::start()
    {
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
        if (print_counters_locally_ && destination_ != "cout")
        {
            destination_ += "." + std::to_string(hpx::get_locality_id());
        }
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif

        // Snapshot the registry generation *before* find_counters() runs
        // below, not after: a counter type registered concurrently while
        // find_counters() is in flight must still be visible as "newer
        // than what we've observed" to the generation check in
        // evaluate_counters(), otherwise it could be acknowledged here
        // without ever having actually been examined by discovery.
        std::uint64_t const generation_before_discovery =
            performance_counters::registry::instance().generation();

        bool const discovered = find_counters();

        // Only cache the snapshot above when discovery fully succeeded,
        // for the same reason refresh_counters() only caches its own
        // snapshot on success: otherwise a partially failed discovery at
        // start() could be mistaken for "up to date" and never retried.
        if (discovered)
        {
            last_known_generation_.store(
                generation_before_discovery, std::memory_order_relaxed);
        }

        counters_.start(launch::sync);

        started_.store(true, std::memory_order_release);

        // this will invoke the evaluate function for the first time
        timer_.start();
    }

    void query_counters::stop_evaluating_counters(bool terminate)
    {
        timer_.stop(terminate);
        counters_.stop(launch::sync);
    }

    std::size_t query_counters::size() const
    {
        return counters_.size();
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace strings {

        constexpr char const* counter_type_short_names[] = {
            "counter_type::text",
            "counter_type::raw",
            "counter_type::monotonically_increasing",
            "counter_type::average_base",
            "counter_type::average_count",
            "counter_type::aggregated",
            "counter_type::average_timer",
            "counter_type::elapsed_time",
            "counter_type::histogram",
            "counter_type::raw_values",
        };
    }

    char const* get_counter_short_type_name(
        performance_counters::counter_type type)
    {
        if (type < performance_counters::counter_type::text ||
            type > performance_counters::counter_type::raw_values)
        {
            return "unknown";
        }
        return strings::counter_type_short_names[static_cast<int>(type)];
    }

    template <typename Stream>
    void query_counters::print_name_csv(Stream& out, std::string const& name)
    {
        std::string s = performance_counters::remove_counter_prefix(name);
        if (s.find_first_of(',') != std::string::npos)
            out << "\"" << s << "\"";
        else
            out << s;
    }

    template <typename Stream>
    void query_counters::print_value(Stream* out,
        performance_counters::counter_info const& info,
        performance_counters::counter_value const& value)
    {
        std::string const& name = info.fullname_;
        std::string const& uom = info.unit_of_measure_;

        error_code ec(throwmode::lightweight);    // do not throw
        double val = value.get_value<double>(ec);

        if (!ec)
        {
            std::string real_name =
                performance_counters::remove_counter_prefix(name);
            hpx::tracing::sample_counter(name, real_name, val);

            if (out == nullptr)
                return;

            print_name_csv(*out, name);
            *out << "," << value.count_ << ",";

            double const elapsed = static_cast<double>(value.time_) * 1e-9;
            *out << hpx::util::format("{:.6}", elapsed) << ",[s]," << val;
            if (!uom.empty())
                *out << ",[" << uom << "]";

            if (counter_types_)
            {
                if (uom.empty())
                    *out << ",[]";
                *out << "," << get_counter_short_type_name(info.type_);
            }
            *out << "\n";
        }
        else
        {
            if (out != nullptr)
                *out << "invalid\n";
        }
    }

    template <typename Stream>
    void query_counters::print_value(Stream* out,
        performance_counters::counter_info const& info,
        performance_counters::counter_values_array const& value)
    {
        if (out == nullptr)
            return;

        std::string const& name = info.fullname_;
        std::string const& uom = info.unit_of_measure_;

        error_code ec(throwmode::lightweight);    // do not throw

        print_name_csv(*out, name);
        *out << "," << value.count_ << ",";

        double const elapsed = static_cast<double>(value.time_) * 1e-9;
        *out << hpx::util::format("{:.6}", elapsed) << ",[s],";

        bool first = true;
        for (std::int64_t val : value.values_)
        {
            if (!first)
                *out << ':';
            first = false;
            *out << val;
        }

        if (!uom.empty())
            *out << ",[" << uom << "]";

        if (counter_types_)
        {
            if (uom.empty())
                *out << ",[]";
            *out << "," << get_counter_short_type_name(info.type_);
        }
        *out << "\n";
    }

    template <typename Stream>
    void query_counters::print_value_csv(Stream* out,
        [[maybe_unused]] performance_counters::counter_info const& info,
        performance_counters::counter_value const& value)
    {
        error_code ec(throwmode::lightweight);
        double val = value.get_value<double>(ec);

        if (!ec)
        {
            std::string real_name =
                performance_counters::remove_counter_prefix(info.fullname_);
            hpx::tracing::sample_counter(info.fullname_, real_name, val);
            if (out == nullptr)
                return;

            *out << val;
        }
        else
        {
            if (out != nullptr)
                *out << "invalid";
        }
    }

    template <typename Stream>
    void query_counters::print_value_csv(Stream* out,
        performance_counters::counter_info const& /* info */,
        performance_counters::counter_values_array const& value)
    {
        if (out == nullptr)
            return;

        bool first = true;
        for (std::int64_t val : value.values_)
        {
            if (!first)
                *out << ':';
            first = false;
            *out << val;
        }
    }

    template <typename Stream>
    void query_counters::print_name_csv_short(
        Stream& out, std::string const& name)
    {
        out << name;
    }

    template <typename Stream>
    void query_counters::print_headers(Stream& output,
        std::vector<performance_counters::counter_info> const& infos)
    {
        if (csv_header_)
        {
            if (format_ == "csv")
            {
                // first print raw value counters
                bool first = true;
                for (std::size_t i = 0; i != infos.size(); ++i)
                {
                    using performance_counters::counter_type;
                    if (infos[i].type_ != counter_type::raw &&
                        infos[i].type_ !=
                            counter_type::monotonically_increasing &&
                        infos[i].type_ != counter_type::aggregating &&
                        infos[i].type_ != counter_type::elapsed_time &&
                        infos[i].type_ != counter_type::average_count &&
                        infos[i].type_ != counter_type::average_timer)
                    {
                        continue;
                    }
                    if (!first)
                        output << ",";
                    first = false;
                    print_name_csv(output, infos[i].fullname_);
                }

                // now print array value counters
                for (std::size_t i = 0; i != infos.size(); ++i)
                {
                    if (infos[i].type_ !=
                            performance_counters::counter_type::histogram &&
                        infos[i].type_ !=
                            performance_counters::counter_type::raw_values)
                    {
                        continue;
                    }

                    if (!first)
                        output << ",";
                    first = false;
                    print_name_csv(output, infos[i].fullname_);
                }

                output << "\n";
            }
            else if (format_ == "csv-short")
            {
                // first print raw value counters
                bool first = true;
                for (std::size_t i = 0; i != counter_shortnames_.size(); ++i)
                {
                    using performance_counters::counter_type;
                    if (infos[i].type_ != counter_type::raw &&
                        infos[i].type_ !=
                            counter_type::monotonically_increasing &&
                        infos[i].type_ != counter_type::aggregating &&
                        infos[i].type_ != counter_type::elapsed_time &&
                        infos[i].type_ != counter_type::average_count &&
                        infos[i].type_ != counter_type::average_timer)
                    {
                        continue;
                    }
                    if (!first)
                        output << ",";
                    first = false;
                    print_name_csv_short(output, counter_shortnames_[i]);
                }

                // now print array value counters
                for (std::size_t i = 0; i != counter_shortnames_.size(); ++i)
                {
                    if (infos[i].type_ !=
                            performance_counters::counter_type::histogram &&
                        infos[i].type_ !=
                            performance_counters::counter_type::raw_values)
                    {
                        continue;
                    }

                    if (!first)
                        output << ",";
                    first = false;
                    print_name_csv_short(output, counter_shortnames_[i]);
                }

                output << "\n";
            }
            csv_header_ = false;
        }
    }

    template <typename Stream, typename Value>
    void query_counters::print_values(Stream* output,
        std::vector<Value>&& values, std::vector<std::size_t>&& indices,
        std::vector<performance_counters::counter_info> const& infos)
    {
        if (format_ == "csv" || format_ == "csv-short")
        {
            bool first = true;
            for (std::size_t i = 0; i != values.size(); ++i)
            {
                if (!first && output != nullptr)
                    *output << ",";
                first = false;
                print_value_csv(output, infos[i], values[i]);
            }
            if (output != nullptr)
                *output << "\n";
        }
        else
        {
            std::size_t idx = 0;
            for (std::size_t const i : indices)
            {
                print_value(output, infos[i], values[idx]);
                ++idx;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    bool query_counters::evaluate(bool force)
    {
        bool reset = false;
        if (get_config_entry("hpx.print_counter.reset", "0") == "1")
            reset = true;

        evaluate_counters(reset, nullptr, force);

        // Note: deliberately not forwarding evaluate_counters()'s return
        // value here. This function is only ever invoked as the periodic
        // callback driving interval_timer (see the constructor), which
        // treats a `false` return as "nothing more to do, stop
        // rescheduling for good" (interval_timer::evaluate()). Before
        // #4627, a counter set that matched nothing was assumed to never
        // match anything later, so tying the two together was harmless.
        // That assumption no longer holds: a wild-card pattern (e.g.
        // /apex/*) can legitimately match zero counters at first and gain
        // matches later, once its provider registers them (see
        // refresh_counters()). If this function forwarded a `false`
        // result from an early, empty evaluation, interval_timer would
        // mark itself terminated immediately, and every subsequent
        // non-forced evaluate_counters() call would then short-circuit on
        // its `timer_.is_terminated()` check before refresh_counters()
        // ever ran again, permanently hiding any counter registered
        // after that point. Always returning true keeps the periodic
        // timer alive; it is still stopped correctly, and only
        // intentionally, via stop_evaluating_counters(true) or runtime
        // shutdown, both of which call interval_timer::terminate()
        // directly rather than going through this return value.
        return true;
    }

    void query_counters::terminate() {}

    ///////////////////////////////////////////////////////////////////////////
    void query_counters::start_counters(error_code& ec)
    {
        if (!started_.load(std::memory_order_acquire))
        {
            // start has not been called yet
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "query_counters::start_counters",
                "The counters to be evaluated have not been initialized yet");
            return;
        }

        // Start the performance counters.
        counters_.start(launch::sync, ec);
    }

    void query_counters::stop_counters(error_code& ec)
    {
        if (!started_.load(std::memory_order_acquire))
        {
            // start has not been called yet
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "query_counters::stop_counters",
                "The counters to be evaluated have not been initialized yet");
            return;
        }

        // Stop the performance counters.
        counters_.stop(launch::sync, ec);
    }

    void query_counters::reset_counters(error_code& ec)
    {
        if (!started_.load(std::memory_order_acquire))
        {
            // start has not been called yet
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "query_counters::reset_counters",
                "The counters to be evaluated have not been initialized yet");
            return;
        }

        // Reset the performance counters.
        counters_.reset(launch::sync, ec);
    }

    void query_counters::reinit_counters(bool reset, error_code& ec)
    {
        if (!started_.load(std::memory_order_acquire))
        {
            // start has not been called yet
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "query_counters::reinit_counters",
                "The counters to be evaluated have not been initialized yet");
            return;
        }

        // Reset the performance counters.
        counters_.reinit(launch::sync, reset, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    bool query_counters::print_raw_counters(bool destination_is_cout,
        bool reset, bool no_output, char const* description,
        std::vector<performance_counters::counter_info> const& infos,
        error_code& ec)
    {
        // Query the performance counters.
        std::vector<std::size_t> indices;
        indices.reserve(infos.size());

        for (std::size_t i = 0; i != infos.size(); ++i)
        {
            if (infos[i].type_ ==
                    performance_counters::counter_type::histogram ||
                infos[i].type_ ==
                    performance_counters::counter_type::raw_values)
            {
                continue;
            }

            indices.push_back(i);
        }

        if (indices.empty())
            return false;

        std::ostringstream output;
        if (description && !no_output)
            output << description << std::endl;

        std::vector<performance_counters::counter_value> values =
            counters_.get_counter_values(launch::sync, reset, ec);

        HPX_ASSERT(values.size() == indices.size());

        // Output the performance counter value.
        if (!no_output)
            print_headers(output, infos);
        print_values(no_output ? nullptr : &output, HPX_MOVE(values),
            HPX_MOVE(indices), infos);

        if (!no_output)
        {
            if (destination_is_cout)
            {
                std::cout << output.str() << std::flush;
            }
            else
            {
                std::ofstream out(destination_.c_str(), std::ofstream::app);
                out << output.str();
            }
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool query_counters::print_array_counters(bool destination_is_cout,
        bool reset, bool no_output, char const* description,
        std::vector<performance_counters::counter_info> const& infos,
        error_code& ec)
    {
        // Query the performance counters.
        std::vector<std::size_t> indices;
        indices.reserve(infos.size());

        for (std::size_t i = 0; i != infos.size(); ++i)
        {
            if (infos[i].type_ !=
                    performance_counters::counter_type::histogram &&
                infos[i].type_ !=
                    performance_counters::counter_type::raw_values)
            {
                continue;
            }

            indices.push_back(i);
        }

        if (indices.empty())
            return false;

        std::ostringstream output;
        if (description && !no_output)
            output << description << std::endl;

        std::vector<performance_counters::counter_values_array> values =
            counters_.get_counter_values_array(launch::sync, reset, ec);

        HPX_ASSERT(values.size() == indices.size());

        // Output the performance counter value.
        if (!no_output)
            print_headers(output, infos);
        print_values(no_output ? nullptr : &output, HPX_MOVE(values),
            HPX_MOVE(indices), infos);

        if (!no_output)
        {
            if (destination_is_cout)
            {
                std::cout << output.str() << std::flush;
            }
            else
            {
                std::ofstream out(destination_.c_str(), std::ofstream::app);
                out << output.str();
            }
        }
        return true;
    }

    bool query_counters::evaluate_counters(
        bool reset, char const* description, bool force, error_code& ec)
    {
        if (!force && timer_.is_terminated())
        {
            // just do nothing as we're about to terminate the application
            return false;
        }

        bool destination_is_cout;
        bool no_output;

        {
            std::lock_guard<mutex_type> l(mtx_);
            destination_is_cout = destination_ == "cout";
            no_output = destination_ == "none";
        }

        if (!started_.load(std::memory_order_acquire))
        {
            // start has not been called yet. A wildcard pattern matching
            // no counters at all is a legitimate outcome of start(), not
            // an error, so counters_.size() == 0 alone cannot be used to
            // detect this (see #4627). This check must happen before
            // refresh_counters() below: refresh_counters() can start newly
            // discovered counter instances as a side effect, and if that
            // ran while started_ was still false, a subsequent legitimate
            // start() call would restart from index 0 and double-start
            // those same counters.
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "query_counters::evaluate",
                "The counters to be evaluated have not been initialized yet");
            return false;
        }

        // Re-discover the requested counter names so that counters
        // registered after query_counters::start() was called, such as
        // APEX counters that only become known to HPX once sampled for the
        // first time, are still included (see #4627). A forced evaluation
        // (e.g. the one performed when counters are printed at shutdown)
        // always refreshes. A periodic evaluation only pays for the actual,
        // AGAS-touching refresh_counters() call when the performance
        // counter type registry's generation has moved since the last time
        // this object refreshed -- a single atomic load and compare
        // otherwise, so widening this to periodic evaluations does not add
        // per-tick discovery overhead in the common case where nothing new
        // has been registered.
        std::uint64_t const current_generation =
            performance_counters::registry::instance().generation();
        if (force ||
            current_generation !=
                last_known_generation_.load(std::memory_order_relaxed))
        {
            refresh_counters();
        }

        std::vector<performance_counters::counter_info> const infos =
            counters_.get_counter_infos();

        bool result = print_raw_counters(
            destination_is_cout, reset, no_output, description, infos, ec);
        if (ec)
            return false;

        result = print_array_counters(destination_is_cout, reset, no_output,
                     description, infos, ec) ||
            result;
        if (ec)
            return false;

        if (&ec != &throws)
            ec = make_success_code();

        return result;
    }
}    // namespace hpx::util
