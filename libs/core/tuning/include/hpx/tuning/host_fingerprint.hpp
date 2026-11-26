//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file host_fingerprint.hpp
/// \brief What a set of tuned settings was measured on: the host, its
///        operating system and the build of HPX, so that a profile used on
///        another machine can be recognized.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/tuning/ini_file.hpp>

#include <string>
#include <string_view>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::tuning {

    /// One item of a fingerprint. Keys are single words with underscores
    /// ("cpu_model", "hpx_commit"), as a dot would read as a further section
    /// level in an ini file; the entries of an area start with its name
    /// ("parcel_conduits").
    HPX_CXX_CORE_EXPORT struct fingerprint_entry
    {
        std::string key;
        std::string value;
        /// whether a different value on another host is a mismatch (false
        /// for what differs between the hosts a profile is for, such as the
        /// host's name)
        bool compared = true;
    };

    /// The fingerprint of a host: an ordered list of entries with unique
    /// keys, to which each tuning area appends its own.
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT host_fingerprint
    {
        std::vector<fingerprint_entry> entries;

        /// Append an entry, or replace the value of the entry with the same
        /// key, keeping its place.
        void add(std::string key, std::string value, bool compared = true);

        /// The entry with the given key, nullptr if there is none.
        [[nodiscard]] fingerprint_entry const* find(
            std::string_view key) const noexcept;
    };

    /// The section of the ini file that holds the fingerprint.
    HPX_CXX_CORE_EXPORT inline constexpr char const* fingerprint_section =
        "hpx.tuning.fingerprint";

    /// The core part of the fingerprint of this host, from the topology and
    /// HPX's version: host_name (not compared), cpu_model, architecture,
    /// sockets, numa_domains, cores, pus, os_name, os_release, os_version,
    /// hpx_version, hpx_commit, hpx_build_type, compiler, stdlib. Values
    /// that aren't known are empty.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT host_fingerprint describe_host(
        threads::topology const& topo);

    /// The fingerprint as comment lines of an ini file, one per entry
    /// ("fingerprint: cpu_model = ...").
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<std::string> to_comments(
        host_fingerprint const& fingerprint);

    /// The fingerprint as an ini section, [hpx.tuning.fingerprint], one key
    /// per entry, so that the runtime can read it back. A '$' that is
    /// followed by '{' or '[' is written doubled, so that HPX's ini reader
    /// takes it literally instead of expanding it.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT ini_section to_section(
        host_fingerprint const& fingerprint);

    /// The entries in which a profile's fingerprint and this host's differ,
    /// one line each ("cpu_model: <profile's value>, here <this host's>"):
    /// the compared entries of this host whose value differs or that the
    /// profile lacks, and the compared entries of the profile that this host
    /// lacks. Empty if they agree.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<std::string> mismatches(
        host_fingerprint const& profile, host_fingerprint const& here);
}    // namespace hpx::tuning

#include <hpx/config/warnings_suffix.hpp>
