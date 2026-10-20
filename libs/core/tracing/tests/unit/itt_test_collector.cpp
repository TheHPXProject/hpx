//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Minimal ITT collector that records the calls the HPX tracing backend makes.
// Exporting __itt_api_version makes the ittnotify static part bind each entry
// point below by name. Anything not exported here becomes a no-op returning
// null, so domains and string handles have to be provided as well.

#define INTEL_NO_MACRO_BODY
#define INTEL_ITTNOTIFY_API_PRIVATE
#include <ittnotify.h>

#include "itt_test_collector.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#define HPX_ITT_TEST_EXPORT extern "C" __declspec(dllexport)
#else
#define HPX_ITT_TEST_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace {

    std::mutex mtx;
    std::vector<hpx_itt_test::event> events;

    // Called with mtx held.
    std::size_t push(hpx_itt_test::event_kind kind, __itt_id const& id,
        char const* name, std::uint64_t value)
    {
        hpx_itt_test::event e{};
        e.kind = kind;
        e.d1 = id.d1;
        e.d2 = id.d2;
        e.value = value;
        e.thread = std::hash<std::thread::id>()(std::this_thread::get_id());
        if (name != nullptr)
        {
            std::strncpy(e.name, name, sizeof(e.name) - 1);
        }
        events.push_back(e);

        // Opt-in trace of every recorded call, for diagnosing a failing run.
        static bool const trace = std::getenv("HPX_ITT_TEST_TRACE") != nullptr;
        if (trace)
        {
            static char const* const kinds[] = {"id_create", "id_destroy",
                "begin", "end", "marker", "metadata", "note", "domain"};
            std::fprintf(stderr, "itt[%zu] %-10s %llx,%llx %s\n",
                events.size() - 1, kinds[static_cast<int>(kind)],
                static_cast<unsigned long long>(e.d1),
                static_cast<unsigned long long>(e.d2), e.name);
        }
        return events.size() - 1;
    }

    // HPX_ITT_TEST_RECORD=markers keeps only markers and the metadata and
    // destroy of their ids. Everything is kept otherwise. A slow
    // multi-locality start-up records millions of task calls from the
    // background-work loop, which a test of markers never reads.
    bool markers_only()
    {
        static bool const value = [] {
            char const* mode = std::getenv("HPX_ITT_TEST_RECORD");
            return mode != nullptr && std::strcmp(mode, "markers") == 0;
        }();
        return value;
    }

    // Ids that carry a marker and are not destroyed yet, guarded by mtx.
    std::set<std::pair<std::uint64_t, std::uint64_t>> marked;

    std::size_t record(hpx_itt_test::event_kind kind, __itt_id const& id,
        char const* name, std::uint64_t value = 0)
    {
        std::lock_guard<std::mutex> l(mtx);
        if (!markers_only())
            return push(kind, id, name, value);

        auto const key = std::make_pair(id.d1, id.d2);
        switch (kind)
        {
        case hpx_itt_test::event_kind::marker:
            marked.insert(key);
            return push(kind, id, name, value);

        case hpx_itt_test::event_kind::metadata:
            if (marked.count(key) == 0)
                return events.size();
            return push(kind, id, name, value);

        case hpx_itt_test::event_kind::id_destroy:
            if (marked.erase(key) == 0)
                return events.size();
            return push(kind, id, name, value);

        case hpx_itt_test::event_kind::id_create:
        case hpx_itt_test::event_kind::task_begin:
        case hpx_itt_test::event_kind::task_end:
            return events.size();

        default:
            return push(kind, id, name, value);
        }
    }

    char const* name_of(__itt_string_handle const* h)
    {
        return h != nullptr ? h->strA : nullptr;
    }

    // Handles live for the whole process and are interned by name, as the
    // ITT API expects repeated creation with the same name to return the
    // same handle.
    std::mutex handles_mtx;
    std::map<std::string, std::unique_ptr<__itt_domain>> domains;
    std::map<std::string, std::unique_ptr<__itt_string_handle>> strings;
}    // namespace

HPX_ITT_TEST_EXPORT char const* __itt_api_version()
{
    return "hpx_itt_test_collector";
}

HPX_ITT_TEST_EXPORT __itt_domain* __itt_domain_create(char const* name)
{
    // Recorded so the test can tell that ITT itself loaded this library.
    record(hpx_itt_test::event_kind::domain_create, __itt_null, name);

    std::lock_guard<std::mutex> l(handles_mtx);
    auto& d = domains[name];
    if (!d)
    {
        d = std::make_unique<__itt_domain>();
        d->flags = 1;
        d->nameA = domains.find(name)->first.c_str();
    }
    return d.get();
}

HPX_ITT_TEST_EXPORT __itt_string_handle* __itt_string_handle_create(
    char const* name)
{
    std::lock_guard<std::mutex> l(handles_mtx);
    auto& h = strings[name];
    if (!h)
    {
        h = std::make_unique<__itt_string_handle>();
        h->strA = strings.find(name)->first.c_str();
    }
    return h.get();
}

HPX_ITT_TEST_EXPORT void __itt_id_create(__itt_domain const*, __itt_id id)
{
    record(hpx_itt_test::event_kind::id_create, id, nullptr);
}

HPX_ITT_TEST_EXPORT void __itt_id_destroy(__itt_domain const*, __itt_id id)
{
    record(hpx_itt_test::event_kind::id_destroy, id, nullptr);
}

HPX_ITT_TEST_EXPORT void __itt_task_begin_overlapped(
    __itt_domain const*, __itt_id id, __itt_id, __itt_string_handle* name)
{
    record(hpx_itt_test::event_kind::task_begin, id, name_of(name));
}

HPX_ITT_TEST_EXPORT void __itt_task_end_overlapped(
    __itt_domain const*, __itt_id id)
{
    record(hpx_itt_test::event_kind::task_end, id, nullptr);
}

HPX_ITT_TEST_EXPORT void __itt_marker(
    __itt_domain const*, __itt_id id, __itt_string_handle* name, __itt_scope)
{
    record(hpx_itt_test::event_kind::marker, id, name_of(name));
}

HPX_ITT_TEST_EXPORT void __itt_metadata_add(__itt_domain const*, __itt_id id,
    __itt_string_handle* key, __itt_metadata_type type, std::size_t count,
    void* data)
{
    std::uint64_t value = 0;
    if (type == __itt_metadata_u64 && count == 1 && data != nullptr)
    {
        std::memcpy(&value, data, sizeof(value));
    }
    record(hpx_itt_test::event_kind::metadata, id, name_of(key), value);
}

// Query side, looked up by the test with dlsym. The test reads while HPX
// threads are still recording, and a later push_back may reallocate the list,
// so it only ever gets a copy taken under the lock, never a pointer into it.
HPX_ITT_TEST_EXPORT std::size_t hpx_itt_test_copy_events(
    hpx_itt_test::event* out, std::size_t capacity)
{
    std::lock_guard<std::mutex> l(mtx);
    if (out != nullptr && capacity >= events.size())
    {
        std::copy(events.begin(), events.end(), out);
    }
    return events.size();
}

HPX_ITT_TEST_EXPORT std::size_t hpx_itt_test_note(char const* text)
{
    return record(hpx_itt_test::event_kind::note, __itt_null, text);
}
