//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/tuning.hpp>
#include <hpx/parcel_tuning/peer_nodes.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace hpx::tuning {

    namespace {

        std::string value_of(locality_entries const& entries,
            std::string const& key, bool& found)
        {
            auto const it = std::find_if(entries.begin(), entries.end(),
                [&](auto const& e) { return e.first == key; });
            found = it != entries.end();
            return found ? it->second : std::string();
        }
    }    // namespace

    std::size_t add_peer_nodes(host_fingerprint& fingerprint,
        std::vector<locality_entries> const& localities)
    {
        // the host the fingerprint describes, before anything is added
        host_fingerprint const here = fingerprint;
        fingerprint_entry const* const here_name = here.find("host_name");

        std::vector<std::string> recorded;
        for (locality_entries const& locality : localities)
        {
            bool has_name = false;
            std::string const name = value_of(locality, "host_name", has_name);
            if (has_name && here_name != nullptr && name == here_name->value)
            {
                continue;    // the tool's own node
            }
            if (has_name &&
                std::find(recorded.begin(), recorded.end(), name) !=
                    recorded.end())
            {
                continue;    // a node recorded already
            }
            recorded.push_back(name);

            std::string const prefix = recorded.size() == 1 ?
                std::string("parcel_peer_") :
                "parcel_peer" + std::to_string(recorded.size()) + "_";
            for (auto const& [key, value] : locality)
            {
                fingerprint_entry const* const e = here.find(key);
                if (key == "host_name" || e == nullptr || e->value != value)
                {
                    fingerprint.add(prefix + key, value, key != "host_name");
                }
            }
        }
        return recorded.size();
    }
}    // namespace hpx::tuning
