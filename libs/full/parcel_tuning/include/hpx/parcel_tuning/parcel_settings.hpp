//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parcel_settings.hpp
/// \brief The rules that turn the transfer measurements into the parcel
///        layer's settings.

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/tuning.hpp>
#include <hpx/parcel_tuning/transfers.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace hpx::tuning {

    /// The measurements of one parcelport: host data through the parcelport
    /// alone, and with the device conduit moving the large host chunks.
    HPX_CXX_EXPORT struct parcelport_measurements
    {
        /// the parcelport as its settings name it ("tcp", "mpi")
        std::string parcelport;
        /// host chunks through the parcelport (no conduit)
        std::vector<transfer_point> plain;
        /// host chunks of at least the zero-copy threshold through the
        /// device conduit; empty if not measured (no conduit that moves host
        /// memory in this build, or the measurement failed)
        std::vector<transfer_point> conduit;
        /// sizes left out of either curve because the passes disagreed
        /// (combine_passes): a rule doesn't decide on a range that has one
        std::vector<std::size_t> unstable;
    };

    /// HPX's defaults and the margins the rules use.
    HPX_CXX_EXPORT struct parcel_rule_options
    {
        /// a configuration is faster at a size if its median (get and set
        /// together) is lower by more than this fraction and the
        /// interquartile ranges don't overlap
        double min_gain = 0.05;
        /// hpx.parcel.conduit_host_threshold
        std::size_t default_host_threshold = std::size_t(1) << 20;
        /// hpx.parcel.<parcelport>.zero_copy_serialization_threshold: smaller
        /// chunks are copied into the message and never reach the conduit
        std::size_t zero_copy_threshold = 8192;
        /// a curve counts as moved through the conduit at a size if the
        /// conduit moved at least this many chunks per round there
        double min_conduit_chunks = 0.9;
    };

    /// What the rules chose: the settings, and lines for the report and the
    /// ini file's comments (the rules that only report, and why a rule
    /// wrote nothing).
    HPX_CXX_EXPORT struct parcel_rules_result
    {
        std::vector<tuned_setting> settings;
        std::vector<std::string> report;
    };

    /// The smallest size from which the first curve stays faster than the
    /// second (see parcel_rule_options::min_gain) up to the largest size
    /// both measured, 0 if there is none. Sizes measured by only one of the
    /// curves are ignored.
    HPX_CXX_EXPORT HPX_EXPORT std::size_t faster_from(
        std::vector<transfer_point> const& faster,
        std::vector<transfer_point> const& slower, double min_gain);

    /// The parcel layer's settings for the given measurements, the first
    /// version of the parcel area's rules:
    ///
    /// - rule 2, hpx.parcel.<parcelport>.conduit_host_chunks: 1 if the
    ///   curve with the conduit is faster than the parcelport's own from
    ///   some size on up to the largest size measured, 0 otherwise;
    /// - rule 3, hpx.parcel.conduit_host_threshold: the largest of those
    ///   sizes over the parcelports with host chunks on, at least the
    ///   zero-copy threshold (one value for the process);
    /// - rule 1, reported only: which configuration is fastest by size.
    ///
    /// A rule whose measurements are missing, didn't go through the conduit,
    /// or are unstable in the range it decides on (a size at or above the
    /// crossover, or above the largest size it compares, left out because
    /// the passes disagreed) writes nothing and says so in the report.
    HPX_CXX_EXPORT HPX_EXPORT parcel_rules_result derive_parcel_settings(
        std::vector<parcelport_measurements> const& measured,
        parcel_rule_options const& options = {});

    /// The settings grouped into ini sections by their keys
    /// ("hpx.parcel.tcp.conduit_host_chunks" into "hpx.parcel.tcp"), in the
    /// order of their first setting.
    HPX_CXX_EXPORT HPX_EXPORT std::vector<ini_section> to_sections(
        std::vector<tuned_setting> const& settings);
}    // namespace hpx::tuning

#endif
