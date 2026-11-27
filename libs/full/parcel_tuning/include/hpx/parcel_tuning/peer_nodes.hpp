//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file peer_nodes.hpp
/// \brief The nodes of a two-node measurement in the host fingerprint.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/tuning.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace hpx::tuning {

    /// The core fingerprint entries (key and value, see describe_host) of
    /// one locality of a measurement.
    HPX_CXX_EXPORT using locality_entries =
        std::vector<std::pair<std::string, std::string>>;

    /// Record in 'fingerprint' the nodes of a measurement's localities that
    /// aren't the host it describes (the one the tool runs on, which need
    /// not be one of them): for each locality whose host_name differs from
    /// the fingerprint's, and from those recorded before, its entries whose
    /// values differ from the fingerprint's (host_name always, not compared)
    /// are added as parcel_peer_<key>, those of a second such node as
    /// parcel_peer2_<key>, and so on. Returns the number of nodes recorded.
    HPX_CXX_EXPORT HPX_EXPORT std::size_t add_peer_nodes(
        host_fingerprint& fingerprint,
        std::vector<locality_entries> const& localities);
}    // namespace hpx::tuning
