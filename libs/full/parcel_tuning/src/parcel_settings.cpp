//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/tuning.hpp>
#include <hpx/parcel_tuning/parcel_settings.hpp>
#include <hpx/parcel_tuning/transfers.hpp>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hpx::tuning {

    namespace {

        // a point's time for get and set together
        struct cost
        {
            double q1 = 0.0;
            double median = 0.0;
            double q3 = 0.0;
        };

        cost cost_of(transfer_point const& p)
        {
            return cost{p.get_us.q1 + p.set_us.q1,
                p.get_us.median + p.set_us.median, p.get_us.q3 + p.set_us.q3};
        }

        bool is_faster(cost const& a, cost const& b, double const min_gain)
        {
            // lower by more than the margin, and below the bulk of the other
            // (its lower quartile): a skewed curve whose upper quartile
            // reaches into the other's still counts (on the V100 nodes at
            // 1 MiB: 1.21 against 1.46 ms, upper quartile 1.437 ms against a
            // lower quartile of 1.431 ms)
            return a.median < b.median * (1.0 - min_gain) && a.median < b.q1;
        }

        // the points of both curves at the sizes both measured, ascending
        std::vector<std::pair<transfer_point, transfer_point>> common_points(
            std::vector<transfer_point> const& a,
            std::vector<transfer_point> const& b)
        {
            std::map<std::size_t, transfer_point> by_size;
            for (transfer_point const& p : b)
            {
                by_size[p.size] = p;
            }
            std::vector<std::pair<transfer_point, transfer_point>> common;
            for (transfer_point const& p : a)
            {
                auto const it = by_size.find(p.size);
                if (it != by_size.end())
                {
                    common.emplace_back(p, it->second);
                }
            }
            std::sort(
                common.begin(), common.end(), [](auto const& x, auto const& y) {
                    return x.first.size < y.first.size;
                });
            return common;
        }

        std::string size_text(std::size_t const size)
        {
            if (size >= (std::size_t(1) << 20) &&
                size % (std::size_t(1) << 20) == 0)
            {
                return std::to_string(size >> 20) + " MiB";
            }
            if (size >= 1024 && size % 1024 == 0)
            {
                return std::to_string(size >> 10) + " KiB";
            }
            return std::to_string(size) + " bytes";
        }

        // microseconds as milliseconds with about three significant digits
        std::string ms_text(double const us)
        {
            double const ms = us / 1000.0;
            int const decimals = ms >= 100.0 ? 0 :
                ms >= 10.0                   ? 1 :
                ms >= 1.0                    ? 2 :
                                               3;
            std::ostringstream os;
            os << std::fixed << std::setprecision(decimals) << ms << " ms";
            return os.str();
        }

        std::string compare_text(
            transfer_point const& a, transfer_point const& b)
        {
            return size_text(a.size) + ": " + ms_text(cost_of(a).median) +
                " against " + ms_text(cost_of(b).median);
        }

        bool has_errors(std::vector<transfer_point> const& curve)
        {
            return std::any_of(curve.begin(), curve.end(),
                [](transfer_point const& p) { return p.errors != 0; });
        }

        // whether the conduit moved the chunks of the curve at the given size
        // and above
        bool conduit_moved_from(std::vector<transfer_point> const& curve,
            std::size_t const from, double const min_chunks)
        {
            bool any = false;
            for (transfer_point const& p : curve)
            {
                if (p.size >= from)
                {
                    any = true;
                    if (p.conduit_chunks_per_round < min_chunks)
                    {
                        return false;
                    }
                }
            }
            return any;
        }
    }    // namespace

    std::size_t faster_from(std::vector<transfer_point> const& faster,
        std::vector<transfer_point> const& slower, double const min_gain)
    {
        auto const common = common_points(faster, slower);
        std::size_t from = 0;
        for (auto it = common.rbegin(); it != common.rend(); ++it)
        {
            if (!is_faster(cost_of(it->first), cost_of(it->second), min_gain))
            {
                break;
            }
            from = it->first.size;
        }
        return from;
    }

    parcel_rules_result derive_parcel_settings(
        std::vector<parcelport_measurements> const& measured,
        parcel_rule_options const& options)
    {
        parcel_rules_result result;
        std::vector<std::pair<std::string, std::size_t>> host_chunks_on;

        // rule 2: host chunks through the conduit, per parcelport
        for (parcelport_measurements const& m : measured)
        {
            std::string const key =
                "hpx.parcel." + m.parcelport + ".conduit_host_chunks";
            std::string const name = "rule 2 (" + m.parcelport + "): ";

            if (m.plain.empty() || m.conduit.empty())
            {
                result.report.push_back(name +
                    (m.plain.empty() ? "no measurement through the parcelport" :
                                       "no measurement with the conduit") +
                    ", " + key + " not written");
                continue;
            }
            if (has_errors(m.plain) || has_errors(m.conduit))
            {
                result.report.push_back(name +
                    "data arrived wrong in some rounds, " + key +
                    " not written");
                continue;
            }

            auto const common = common_points(m.conduit, m.plain);
            if (common.empty())
            {
                result.report.push_back(name + "no size measured both ways, " +
                    key + " not written");
                continue;
            }

            std::size_t const from =
                faster_from(m.conduit, m.plain, options.min_gain);
            transfer_point const& largest_conduit = common.back().first;
            transfer_point const& largest_plain = common.back().second;

            // the range the rule decides on: from the crossover (or the
            // largest size compared) up; a size left out of it because the
            // passes disagreed leaves the decision open
            std::size_t const decided_from =
                from != 0 ? from : largest_conduit.size;
            std::string unstable_text;
            for (std::size_t const size : m.unstable)
            {
                if (size >= decided_from)
                {
                    unstable_text +=
                        (unstable_text.empty() ? "" : ", ") + size_text(size);
                }
            }
            if (!unstable_text.empty())
            {
                result.report.push_back(name + "the passes disagree at " +
                    unstable_text + ", in the range the rule decides on, " +
                    key + " not written");
                continue;
            }

            if (from != 0)
            {
                if (!conduit_moved_from(
                        m.conduit, from, options.min_conduit_chunks))
                {
                    result.report.push_back(name +
                        "faster with the conduit, but the conduit didn't "
                        "move the chunks, " +
                        key + " not written");
                    continue;
                }
                std::size_t const threshold =
                    (std::max) (from, options.zero_copy_threshold);
                host_chunks_on.emplace_back(m.parcelport, threshold);

                auto const at_from = std::find_if(common.begin(), common.end(),
                    [&](auto const& c) { return c.first.size == from; });
                result.settings.push_back(tuned_setting{key, "1",
                    "the conduit is faster from " + size_text(from) +
                        " on (get and set, " +
                        compare_text(at_from->first, at_from->second) +
                        (at_from->first.size != largest_conduit.size ?
                                ", " +
                                    compare_text(
                                        largest_conduit, largest_plain) :
                                std::string()) +
                        ")",
                    true});
                continue;
            }

            // not faster at the largest size: off, if the conduit did move
            // the chunks there (otherwise the curve says nothing about it)
            if (largest_conduit.conduit_chunks_per_round <
                options.min_conduit_chunks)
            {
                result.report.push_back(name +
                    "the conduit didn't move the host chunks (no host chunk "
                    "support in this HPX, or the destination isn't "
                    "reachable through it), " +
                    key + " not written");
                continue;
            }
            result.settings.push_back(tuned_setting{key, "0",
                "the conduit isn't faster at the largest size (get and set, " +
                    compare_text(largest_conduit, largest_plain) + ")",
                false});
        }

        // rule 3: the threshold, one for the process
        std::string const threshold_key = "hpx.parcel.conduit_host_threshold";
        if (host_chunks_on.empty())
        {
            result.report.push_back(
                "rule 3: no parcelport has host chunks through the conduit, " +
                threshold_key + " not written");
        }
        else
        {
            std::size_t threshold = 0;
            std::string reason = "the size from which the conduit is faster";
            for (auto const& [parcelport, from] : host_chunks_on)
            {
                threshold = (std::max) (threshold, from);
                reason += ", " + parcelport + " " + size_text(from);
            }
            if (threshold == options.zero_copy_threshold)
            {
                reason += " (at least the zero-copy threshold)";
            }
            result.settings.push_back(
                tuned_setting{threshold_key, std::to_string(threshold), reason,
                    threshold == options.default_host_threshold});
        }

        // rule 1, reported only: the fastest configuration by size
        std::map<std::size_t, std::pair<std::string, double>> fastest;
        auto const consider = [&](std::string const& configuration,
                                  std::vector<transfer_point> const& curve) {
            for (transfer_point const& p : curve)
            {
                double const c = cost_of(p).median;
                auto const it = fastest.find(p.size);
                if (it == fastest.end() || c < it->second.second)
                {
                    fastest[p.size] = {configuration, c};
                }
            }
        };
        for (parcelport_measurements const& m : measured)
        {
            if (!has_errors(m.plain))
            {
                consider(m.parcelport, m.plain);
            }
            if (!m.conduit.empty() && !has_errors(m.conduit))
            {
                consider(m.parcelport + " with the conduit", m.conduit);
            }
        }
        if (!fastest.empty())
        {
            std::string line = "rule 1 (reported only): fastest for host data";
            auto it = fastest.begin();
            while (it != fastest.end())
            {
                auto last = it;
                auto next = std::next(it);
                while (next != fastest.end() &&
                    next->second.first == it->second.first)
                {
                    last = next++;
                }
                line += (it == fastest.begin() ? ": " : "; ") +
                    size_text(it->first) +
                    (last != it ? " to " + size_text(last->first) : "") + " " +
                    it->second.first;
                it = next;
            }
            result.report.push_back(line);
        }

        return result;
    }

    std::vector<ini_section> to_sections(
        std::vector<tuned_setting> const& settings)
    {
        std::vector<ini_section> sections;
        for (tuned_setting const& s : settings)
        {
            auto const dot = s.key.rfind('.');
            std::string const name = dot == std::string::npos ?
                std::string("hpx") :
                s.key.substr(0, dot);
            auto it = std::find_if(sections.begin(), sections.end(),
                [&](ini_section const& section) {
                    return section.name == name;
                });
            if (it == sections.end())
            {
                sections.push_back(ini_section{name, {}});
                it = std::prev(sections.end());
            }
            it->settings.push_back(s);
        }
        return sections;
    }
}    // namespace hpx::tuning

#endif
