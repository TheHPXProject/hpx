//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/tuning.hpp>
#include <hpx/parcel_tuning/peer_nodes.hpp>

#include <hpx/modules/filesystem.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <string>
#include <system_error>
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

        // the first line of a sysfs file, empty if it can't be read
        std::string read_line(hpx::filesystem::path const& file)
        {
            std::ifstream in(file);
            std::string line;
            std::getline(in, line);
            return line;
        }

        // the entries of a sysfs directory, by name
        std::vector<hpx::filesystem::path> entries_of(
            hpx::filesystem::path const& dir)
        {
            std::vector<hpx::filesystem::path> paths;
            std::error_code ec;
            for (hpx::filesystem::directory_iterator it(dir, ec), end;
                !ec && it != end; it.increment(ec))
            {
                paths.push_back(it->path());
            }
            std::sort(paths.begin(), paths.end());
            return paths;
        }
    }    // namespace

    std::string describe_network()
    {
        std::vector<std::string> parts;

        // InfiniBand: every active port with its adapter and rate
        for (auto const& device : entries_of("/sys/class/infiniband"))
        {
            std::string const hca = read_line(device / "hca_type");
            for (auto const& port : entries_of(device / "ports"))
            {
                if (read_line(port / "state").find("ACTIVE") ==
                    std::string::npos)
                {
                    continue;
                }
                std::string part = device.filename().string();
                if (!hca.empty())
                    part += " " + hca;
                std::string const rate = read_line(port / "rate");
                if (!rate.empty())
                    part += " " + rate;
                parts.push_back(part);
            }
        }

        // the interfaces that are up with a known speed
        for (auto const& netif : entries_of("/sys/class/net"))
        {
            std::string const name = netif.filename().string();
            if (name == "lo" || read_line(netif / "operstate") != "up")
                continue;
            std::string const speed = read_line(netif / "speed");
            if (speed.empty() || speed[0] == '-')
                continue;
            parts.push_back(name + " " + speed + " Mb/s");
        }

        std::string text;
        for (std::string const& part : parts)
            text += (text.empty() ? "" : "; ") + part;

        // one line of an ini file (HPX's reader before 1be1f5ce01 crashes
        // on a line of a few hundred characters), the same on every run
        constexpr std::size_t longest = 120;
        if (text.size() > longest)
            text = text.substr(0, longest - 4) + " ...";
        return text;
    }

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
                    // the parcel area's own keys without a second "parcel_"
                    std::string const name =
                        key.rfind("parcel_", 0) == 0 ? key.substr(7) : key;
                    fingerprint.add(prefix + name, value, key != "host_name");
                }
            }
        }
        return recorded.size();
    }
}    // namespace hpx::tuning
