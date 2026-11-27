//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file peer_nodes.hpp
/// \brief The nodes of a two-node measurement in the host fingerprint, and
///        their network.

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

    /// The network adapters of this node as the parcel layer's settings
    /// depend on them: each active InfiniBand port (device, adapter, rate,
    /// "mlx4_0 MT4099 56 Gb/sec (4X FDR)") and each network interface that
    /// is up with a known speed ("eno1 10000 Mb/s"), in name order, joined
    /// by "; ", at most 120 characters (one line of an ini file). Empty
    /// where it isn't known (not Linux, no sysfs).
    HPX_CXX_EXPORT HPX_EXPORT std::string describe_network();

    /// Record in 'fingerprint' the nodes of a measurement's localities that
    /// aren't the host it describes (the one the tool runs on, which need
    /// not be one of them): for each locality whose host_name differs from
    /// the fingerprint's, and from those recorded before, its entries whose
    /// values differ from the fingerprint's (host_name always, not compared)
    /// are added as parcel_peer_<key>, those of a second such node as
    /// parcel_peer2_<key>, and so on (a key of the parcel area's own,
    /// parcel_<name>, as parcel_peer_<name>). Returns the number of nodes
    /// recorded.
    HPX_CXX_EXPORT HPX_EXPORT std::size_t add_peer_nodes(
        host_fingerprint& fingerprint,
        std::vector<locality_entries> const& localities);
}    // namespace hpx::tuning
