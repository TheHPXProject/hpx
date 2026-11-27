//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/tuning/host_fingerprint.hpp>
#include <hpx/tuning/ini_file.hpp>
#include <hpx/tuning/machine_description.hpp>
#include <hpx/tuning/tuned_setting.hpp>
#include <hpx/version.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hpx::tuning {

    namespace {

        // a value as HPX's ini reader gives it back: every '#' (the start of
        // a comment there, which nothing escapes) written as "%23", and
        // without the white space around it, which the reader drops
        std::string normalized_value(std::string_view const value)
        {
            std::size_t const first = value.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos)
                return {};
            std::size_t const last = value.find_last_not_of(" \t\r\n");

            std::string result;
            result.reserve(last - first + 1);
            for (char const c : value.substr(first, last - first + 1))
            {
                if (c == '#')
                    result += "%23";
                else
                    result += c;
            }
            return result;
        }
    }    // namespace

    void host_fingerprint::add(
        std::string key, std::string const& raw_value, bool const compared)
    {
        std::string value = normalized_value(raw_value);
        for (fingerprint_entry& e : entries)
        {
            if (e.key == key)
            {
                e.value = HPX_MOVE(value);
                e.compared = compared;
                return;
            }
        }
        entries.push_back(
            fingerprint_entry{HPX_MOVE(key), HPX_MOVE(value), compared});
    }

    fingerprint_entry const* host_fingerprint::find(
        std::string_view const key) const noexcept
    {
        for (fingerprint_entry const& e : entries)
        {
            if (e.key == key)
                return &e;
        }
        return nullptr;
    }

    host_fingerprint describe_host(threads::topology const& topo)
    {
        machine_description const machine = describe_machine(topo);

        host_fingerprint f;
        // a profile is for the hosts of a partition, which differ in name
        f.add("host_name", topo.get_host_name(), false);
        f.add("cpu_model", topo.get_cpu_model());
        f.add("architecture", topo.get_architecture());
        f.add("sockets", std::to_string(machine.sockets));
        f.add("numa_domains", std::to_string(machine.numa_domains));
        f.add("cores", std::to_string(machine.cores));
        f.add("pus", std::to_string(machine.pus));
        f.add("os_name", topo.get_os_name());
        f.add("os_release", topo.get_os_release());
        f.add("os_version", topo.get_os_version());
        f.add("hpx_version", hpx::full_version_as_string());
#if defined(HPX_HAVE_GIT_COMMIT)
        f.add("hpx_commit", HPX_HAVE_GIT_COMMIT);
#else
        f.add("hpx_commit", "");
#endif
        f.add("hpx_build_type", hpx::build_type());
        f.add("compiler", hpx::boost_compiler());
        f.add("stdlib", hpx::boost_stdlib());
        return f;
    }

    std::vector<std::string> to_comments(host_fingerprint const& fingerprint)
    {
        std::vector<std::string> comments;
        comments.reserve(fingerprint.entries.size());
        for (fingerprint_entry const& e : fingerprint.entries)
            comments.push_back("fingerprint: " + e.key + " = " + e.value);
        return comments;
    }

    namespace {

        // the value with every '$' that is followed by '{' or '[' doubled,
        // which HPX's ini reader reads as a literal '$'
        std::string escape_expansions(std::string_view const value)
        {
            std::string result;
            result.reserve(value.size());
            for (std::size_t i = 0; i != value.size(); ++i)
            {
                if (value[i] == '$' && i + 1 < value.size() &&
                    (value[i + 1] == '{' || value[i + 1] == '['))
                {
                    result += '$';
                }
                result += value[i];
            }
            return result;
        }
    }    // namespace

    ini_section to_section(host_fingerprint const& fingerprint)
    {
        ini_section section{fingerprint_section, {}};
        section.settings.reserve(fingerprint.entries.size());
        for (fingerprint_entry const& e : fingerprint.entries)
        {
            section.settings.push_back(
                tuned_setting{std::string(fingerprint_section) + "." + e.key,
                    escape_expansions(e.value),
                    e.compared ? "compared with the host at startup" :
                                 "for information, not compared",
                    false});
        }
        return section;
    }

    std::vector<std::string> mismatches(
        host_fingerprint const& profile, host_fingerprint const& here)
    {
        std::vector<std::string> result;
        for (fingerprint_entry const& e : here.entries)
        {
            if (!e.compared)
                continue;
            fingerprint_entry const* const p = profile.find(e.key);
            if (p == nullptr)
            {
                result.push_back(
                    e.key + ": not in the profile, here " + e.value);
            }
            else if (p->value != e.value)
            {
                result.push_back(e.key + ": " + p->value + ", here " + e.value);
            }
        }
        for (fingerprint_entry const& p : profile.entries)
        {
            if (p.compared && here.find(p.key) == nullptr)
                result.push_back(p.key + ": " + p.value + ", not here");
        }
        return result;
    }
}    // namespace hpx::tuning
