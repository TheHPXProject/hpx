//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file transfers.hpp
/// \brief The time it takes to move data of a given size to another
///        locality and back, through the parcel layer as it is configured.

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/tuning.hpp>

#include <cstddef>
#include <functional>
#include <vector>

namespace hpx::tuning {

    HPX_CXX_EXPORT struct transfer_point;

    /// How measure_transfers measures.
    HPX_CXX_EXPORT struct transfer_options
    {
        /// the sizes to measure, in bytes, in the order they are measured
        std::vector<std::size_t> sizes;
        /// the measured rounds per size and direction, after the warm-up
        std::size_t rounds = 50;
        /// the rounds per size and direction that aren't measured
        std::size_t warmup = 3;
        /// once the rounds of a size and direction took this long (in
        /// seconds), no further rounds are started, if at least min_rounds
        /// were measured
        double limit_s = 5.0;
        std::size_t min_rounds = 5;
        /// called with each size's measurements as soon as they are done (a
        /// caller can report progress, the sizes take seconds each)
        std::function<void(transfer_point const&)> progress;
    };

    /// The sizes measured by default: the powers of 2 from 1 KiB to 64 MiB.
    HPX_CXX_EXPORT HPX_EXPORT std::vector<std::size_t> default_transfer_sizes();

    /// The measurements of one size.
    HPX_CXX_EXPORT struct transfer_point
    {
        std::size_t size = 0;
        /// microseconds for the remote locality to return a vector of the
        /// size (get) ...
        summary get_us;
        /// ... and for a vector of the size sent as an argument to arrive and
        /// be checked there (set)
        summary set_us;
        /// the chunks the device conduit moved per round, sent and received,
        /// on the measuring locality (about 1 where every vector went through
        /// the conduit, 0 where none did)
        double conduit_chunks_per_round = 0.0;
        /// rounds whose data arrived wrong (should be 0)
        std::size_t errors = 0;
    };

    /// The measurements of several passes of one configuration, combined.
    HPX_CXX_EXPORT struct combined_curve
    {
        /// for each size measured in every pass, the median over the passes
        /// of each value (the counts and the rounds with wrong data added
        /// up)
        std::vector<transfer_point> points;
        /// the sizes left out because the passes disagreed (see
        /// combine_passes)
        std::vector<std::size_t> unstable;
    };

    /// Combine the measurements of several passes of one configuration. A
    /// size at which the passes' medians (get and set together) differ by
    /// more than max_spread (relative to the smallest of them) is left out and
    /// listed as unstable: a rule must not decide on it. A single pass is
    /// returned as it is.
    HPX_CXX_EXPORT HPX_EXPORT combined_curve combine_passes(
        std::vector<std::vector<transfer_point>> const& passes,
        double max_spread = 0.10);

    /// Measure the round trips of vectors of the given sizes between this
    /// locality and the given one, both directions, each round checked.
    /// Before the first size it sends vectors until the device conduit
    /// moves host chunks to the destination, if it does at all (it checks
    /// that it reaches a destination first, and the parcelport moves them
    /// until then), at most for a few seconds.
    HPX_CXX_EXPORT HPX_EXPORT std::vector<transfer_point> measure_transfers(
        hpx::id_type const& remote, transfer_options const& options);
}    // namespace hpx::tuning

#endif
