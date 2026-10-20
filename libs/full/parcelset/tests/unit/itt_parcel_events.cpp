//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks the ITT parcel markers with the recording collector of the tracing
// tests. Each locality records its own calls, so the parcel ids received on
// one locality are compared with the ids sent from the other.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/testing.hpp>

#include "itt_test_collector.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

    std::string collector_path;

    std::vector<hpx_itt_test::event> events()
    {
        // The deleter keeps a reference on the collector library while the
        // symbol is in use.
        using fn = std::remove_pointer_t<hpx_itt_test::copy_events_fn>;
        using deleter = hpx::function<void(hpx_itt_test::copy_events_fn)>;
        static auto const copy = []() -> std::shared_ptr<fn> {
            hpx::error_code ec(hpx::throwmode::lightweight);
            hpx::util::plugin::dll lib(collector_path);
            auto [f, d] = lib.get<hpx_itt_test::copy_events_fn, deleter>(
                "hpx_itt_test_copy_events", ec);
            if (ec)
                return nullptr;
            return {f, d};
        }();
        if (!copy)
            return {};

        // Other threads may still be recording, so retry if the list grew,
        // a bounded number of times.
        std::vector<hpx_itt_test::event> ev(copy.get()(nullptr, 0));
        for (int attempt = 0; attempt != 100; ++attempt)
        {
            if (copy.get()(ev.data(), ev.size()) <= ev.size())
                return ev;
            ev.resize(copy.get()(nullptr, 0));
        }
        HPX_TEST_MSG(
            false, "the collector's event list kept outgrowing the copy");
        return {};
    }

    struct parcel_marker
    {
        std::string name;
        bool destroyed = false;
        std::map<std::string, std::uint64_t> metadata;
    };

    // Groups the recorded calls by marker id: everything between a parcel
    // marker and the destroy of its id belongs to that marker. Also reports
    // whether ITT created the hpx domain through the collector, which shows
    // the collector is actually in use.
    std::vector<parcel_marker> parcel_markers(bool* collector_used = nullptr)
    {
        std::vector<parcel_marker> result;
        std::map<std::pair<std::uint64_t, std::uint64_t>, std::size_t> open;

        for (auto const& e : events())
        {
            auto const id = std::make_pair(e.d1, e.d2);
            auto const it = open.find(id);
            switch (e.kind)
            {
            case hpx_itt_test::event_kind::domain_create:
                if (collector_used && std::strcmp(e.name, "hpx") == 0)
                    *collector_used = true;
                break;
            case hpx_itt_test::event_kind::marker:
                if (std::strncmp(e.name, "parcel_", 7) == 0)
                {
                    open[id] = result.size();
                    result.emplace_back().name = e.name;
                }
                break;
            case hpx_itt_test::event_kind::metadata:
                if (it != open.end())
                    result[it->second].metadata[e.name] = e.value;
                break;
            case hpx_itt_test::event_kind::id_destroy:
                if (it != open.end())
                {
                    result[it->second].destroyed = true;
                    open.erase(it);
                }
                break;
            default:
                break;
            }
        }
        return result;
    }

    using id_set = std::set<std::pair<std::uint64_t, std::uint64_t>>;

    id_set to_set(std::vector<std::uint64_t> const& ids)
    {
        id_set result;
        for (std::size_t i = 0; i + 1 < ids.size(); i += 2)
            result.emplace(ids[i], ids[i + 1]);
        return result;
    }

    auto deadline()
    {
        return std::chrono::steady_clock::now() + std::chrono::seconds(10);
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
void ping() {}
HPX_PLAIN_ACTION(ping)

std::size_t bulk(std::vector<char> const& data)
{
    return data.size();
}
HPX_PLAIN_ACTION(bulk)

// Parcel ids, as msb/lsb pairs, of this locality's markers named 'name' whose
// peer locality (target of a send, source otherwise) is 'peer'.
std::vector<std::uint64_t> parcel_ids(
    std::string const& name, std::uint32_t peer)
{
    char const* key =
        name == "parcel_send" ? "target_locality" : "source_locality";
    std::vector<std::uint64_t> ids;
    for (auto const& m : parcel_markers())
    {
        auto const it = m.metadata.find(key);
        if (m.name == name && it != m.metadata.end() && it->second == peer)
        {
            ids.push_back(m.metadata.at("parcel_id_msb"));
            ids.push_back(m.metadata.at("parcel_id_lsb"));
        }
    }
    return ids;
}
HPX_PLAIN_ACTION(parcel_ids)

// Returns what is wrong with this locality's markers, or an empty string.
// Markers whose id is not destroyed yet may still be in flight, so wait for
// those first.
std::string check_markers()
{
    bool collector_used = false;
    std::vector<parcel_marker> markers;
    for (auto const end = deadline();;)
    {
        markers = parcel_markers(&collector_used);
        bool done = true;
        for (auto const& m : markers)
            done = done && m.destroyed;
        if (done || std::chrono::steady_clock::now() > end)
            break;
        hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!collector_used)
        return "ITT did not load the test collector\n";

    std::string errors;
    for (auto const& m : markers)
    {
        if (!m.destroyed)
            errors += m.name + ": id not destroyed\n";
        if (m.metadata.count("parcel_id_msb") == 0 ||
            m.metadata.count("parcel_id_lsb") == 0)
            errors += m.name + ": parcel id missing\n";
        if (m.name == "parcel_send" && m.metadata.count("size") == 0)
            errors += m.name + ": size missing\n";
        if (m.name == "parcel_scheduled" &&
            m.metadata.count("source_thread") == 0)
            errors += m.name + ": source thread missing\n";
    }
    return errors;
}
HPX_PLAIN_ACTION(check_markers)

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::vector<hpx::id_type> const remotes = hpx::find_remote_localities();
    HPX_TEST(!remotes.empty());
    if (remotes.empty())
        return hpx::finalize();

    hpx::id_type const& peer = remotes.front();
    std::uint32_t const here_id = hpx::get_locality_id();
    std::uint32_t const peer_id = hpx::naming::get_locality_id_from_id(peer);

    // Small parcels are scheduled while they are received; parcels well
    // above the zero-copy threshold are scheduled later, after the receive.
    constexpr std::size_t num_pings = 100;
    constexpr std::size_t num_bulk = 4;
    {
        std::vector<hpx::future<void>> pings;
        for (std::size_t i = 0; i != num_pings; ++i)
            pings.push_back(hpx::async<ping_action>(peer));
        hpx::wait_all(pings);

        std::vector<char> const data(std::size_t(1) << 20, 'x');
        for (std::size_t i = 0; i != num_bulk; ++i)
            HPX_TEST_EQ(bulk_action()(peer, data), data.size());
    }

    // The peer records a receive before it schedules the same parcel, so
    // taking the scheduled ids first means all of them have to be among the
    // received ones taken after.
    std::vector<std::uint64_t> const scheduled =
        parcel_ids_action()(peer, "parcel_scheduled", here_id);
    std::vector<std::uint64_t> const received =
        parcel_ids_action()(peer, "parcel_recv", here_id);

    id_set const received_set = to_set(received);
    HPX_TEST_EQ(received_set.size() * 2, received.size());
    HPX_TEST_LTE(num_pings + num_bulk, received_set.size());

    // Parcels this large are received with zero-copy by default, which
    // defers scheduling their actions, so each one gets a scheduled marker.
    HPX_TEST_LTE(num_bulk * 2, scheduled.size());
    for (auto const& id : to_set(scheduled))
        HPX_TEST_EQ(received_set.count(id), std::size_t(1));

    // A send marker is emitted from the write handler, which may run after
    // the peer has already received the parcel, so wait for the stragglers.
    std::size_t missing = 0;
    for (auto const end = deadline();;)
    {
        id_set const sent = to_set(parcel_ids("parcel_send", peer_id));
        missing = 0;
        for (auto const& id : received_set)
            missing += sent.count(id) == 0 ? 1 : 0;
        if (missing == 0 || std::chrono::steady_clock::now() > end)
            break;
        hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    HPX_TEST_EQ(missing, std::size_t(0));

    std::string const local_errors = check_markers();
    HPX_TEST_MSG(local_errors.empty(), local_errors.c_str());
    std::string const peer_errors = check_markers_action()(peer);
    HPX_TEST_MSG(peer_errors.empty(), peer_errors.c_str());

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    namespace po = hpx::program_options;
    po::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()("collector", po::value<std::string>(),
        "path of the recording ITT collector library");

    // ITT reads its environment during runtime start-up, so the collector
    // path is needed before init. Every locality runs main with the same
    // arguments.
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(desc_commandline)
                  .allow_unregistered()
                  .run(),
        vm);
    if (vm.count("collector") == 0)
    {
        HPX_TEST_MSG(false, "--collector=<path to collector library> missing");
        return hpx::util::report_errors();
    }
    collector_path = vm["collector"].as<std::string>();

    // The collector keeps only markers, since a slow start-up would otherwise
    // record millions of background-work calls.
#if defined(_WIN32)
    _putenv_s("INTEL_LIBITTNOTIFY64", collector_path.c_str());
    _putenv_s("INTEL_ITTNOTIFY_GROUPS", "structure");
    _putenv_s("HPX_ITT_TEST_RECORD", "markers");
#else
    ::setenv("INTEL_LIBITTNOTIFY64", collector_path.c_str(), 1);
    ::setenv("INTEL_ITTNOTIFY_GROUPS", "structure", 1);
    ::setenv("HPX_ITT_TEST_RECORD", "markers", 1);
#endif

    hpx::init_params params;
    params.desc_cmdline = desc_commandline;
    params.cfg = {"hpx.use_itt_notify=1"};

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, params), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
#endif
