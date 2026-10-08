//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks the ITT calls the tracing backend makes for the task lifecycle and
// for work stealing, using a recording collector loaded in place of a real
// tool. Run with one worker it covers inline children that park and resume;
// run with more it covers the work-stealing markers.

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>

#include "itt_test_collector.hpp"

#include <dlfcn.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

    std::string collector_path;

    hpx_itt_test::copy_events_fn copy_events = nullptr;
    hpx_itt_test::note_fn note = nullptr;

    bool bind_collector()
    {
        void* lib = dlopen(collector_path.c_str(), RTLD_NOW | RTLD_NOLOAD);
        if (lib == nullptr)
            return false;

        copy_events = reinterpret_cast<hpx_itt_test::copy_events_fn>(
            dlsym(lib, "hpx_itt_test_copy_events"));
        note = reinterpret_cast<hpx_itt_test::note_fn>(
            dlsym(lib, "hpx_itt_test_note"));
        return copy_events != nullptr && note != nullptr;
    }

    // Copy of everything recorded so far. Other workers may still be
    // recording, so if the list grew between sizing and copying, retry.
    std::vector<hpx_itt_test::event> events()
    {
        std::vector<hpx_itt_test::event> ev(copy_events(nullptr, 0));
        for (;;)
        {
            std::size_t const total = copy_events(ev.data(), ev.size());
            if (total <= ev.size())
            {
                ev.resize(total);
                return ev;
            }
            ev.resize(total);
        }
    }

    int markers(std::size_t from, std::size_t to, char const* name)
    {
        auto const ev = events();
        int n = 0;
        for (std::size_t i = from; i != to; ++i)
        {
            if (ev[i].kind == hpx_itt_test::event_kind::marker &&
                std::strcmp(ev[i].name, name) == 0)
            {
                ++n;
            }
        }
        return n;
    }

#if defined(HPX_HAVE_TRACING_LIFECYCLE_EVENTS)
    // get_self_id names the outermost task even inside an inline child, so
    // the task actually running is taken from the coroutine self instead.
    void const* self() noexcept
    {
        return hpx::threads::get_thread_id_data(
            hpx::threads::get_self().get_thread_id());
    }

    // Executor whose tasks a waiting parent runs directly on its own stack.
    auto inline_exec()
    {
        hpx::threads::thread_schedule_hint hint;
        hint.runs_as_child_mode(
            hpx::threads::thread_execution_hint::run_as_child);
        return hpx::execution::experimental::with_hint(
            hpx::execution::parallel_executor{}, hint);
    }

    bool runs_inline() noexcept
    {
        return hpx::threads::get_self().get_outer_thread_id() !=
            hpx::threads::get_self().get_thread_id();
    }

    bool is_task(hpx_itt_test::event const& e, void const* task) noexcept
    {
        return e.d1 == reinterpret_cast<std::uint64_t>(task) && e.d2 == 0;
    }

    // Number of open overlapped segments of a task at event index 'to',
    // counting from 'from' with 'initial' segments already open. Fails if
    // the task is ever begun twice or ended while closed.
    int open_at(std::size_t from, std::size_t to, void const* task, int initial)
    {
        auto const ev = events();
        int open = initial;
        for (std::size_t i = from; i != to; ++i)
        {
            if (!is_task(ev[i], task))
                continue;
            if (ev[i].kind == hpx_itt_test::event_kind::task_begin)
                ++open;
            else if (ev[i].kind == hpx_itt_test::event_kind::task_end)
                --open;
            HPX_TEST_MSG(open == 0 || open == 1, "unbalanced task segment");
        }
        return open;
    }

    int begins(std::size_t from, std::size_t to, void const* task)
    {
        auto const ev = events();
        int n = 0;
        for (std::size_t i = from; i != to; ++i)
        {
            if (is_task(ev[i], task) &&
                ev[i].kind == hpx_itt_test::event_kind::task_begin)
            {
                ++n;
            }
        }
        return n;
    }

    // An inline child that runs to completion must close its own segment
    // before the parent continues.
    void test_inline_completion()
    {
        std::size_t const start = note("completion:start");
        void const* const parent = self();
        void const* child = nullptr;
        bool inlined = false;

        hpx::async(inline_exec(), [&] {
            child = self();
            inlined = runs_inline();
        }).get();

        std::size_t const after = note("completion:after_get");

        HPX_TEST(inlined);
        HPX_TEST_EQ(begins(start, after, child), 1);
        HPX_TEST_EQ(open_at(start, after, child, 0), 0);
        HPX_TEST_EQ(open_at(start, after, parent, 1), 1);
    }

    // An inline child that parks takes its parent's fiber with it. After the
    // fiber resumes, both must have an open segment again.
    template <typename Park>
    void test_inline_park(char const* name, Park&& park)
    {
        std::size_t const start = note(name);
        void const* const parent = self();
        void const* child = nullptr;
        bool inlined = false;
        std::size_t resumed = 0;

        hpx::async(inline_exec(), [&] {
            child = self();
            inlined = runs_inline();
            park();
            resumed = note("park:child_resumed");
        }).get();

        std::size_t const after = note("park:after_get");

        HPX_TEST(inlined);
        HPX_TEST_EQ(begins(start, resumed, child), 2);
        HPX_TEST_EQ(open_at(start, resumed, child, 0), 1);
        HPX_TEST_EQ(open_at(start, resumed, parent, 1), 1);
        HPX_TEST_EQ(open_at(start, after, child, 0), 0);
        HPX_TEST_EQ(open_at(start, after, parent, 1), 1);
    }

    // Two levels of inline children under the parent; the innermost one
    // suspends, so all three frames have to be reopened on resume.
    void test_nested_suspend()
    {
        std::size_t const start = note("nested:start");
        void const* const parent = self();
        void const* b = nullptr;
        void const* c = nullptr;
        bool inlined = false;
        std::size_t c_resumed = 0;
        std::size_t b_after = 0;

        hpx::async(inline_exec(), [&] {
            b = self();
            hpx::async(inline_exec(), [&] {
                c = self();
                inlined = runs_inline();
                hpx::this_thread::sleep_for(std::chrono::milliseconds(2));
                c_resumed = note("nested:c_resumed");
            }).get();
            b_after = note("nested:b_after_get");
        }).get();

        std::size_t const after = note("nested:after_get");

        HPX_TEST(inlined);
        HPX_TEST_EQ(open_at(start, c_resumed, parent, 1), 1);
        HPX_TEST_EQ(open_at(start, c_resumed, b, 0), 1);
        HPX_TEST_EQ(open_at(start, c_resumed, c, 0), 1);
        HPX_TEST_EQ(open_at(start, b_after, b, 0), 1);
        HPX_TEST_EQ(open_at(start, b_after, c, 0), 0);
        HPX_TEST_EQ(open_at(start, after, b, 0), 0);
        HPX_TEST_EQ(open_at(start, after, parent, 1), 1);
    }

    // Inline nesting deeper than the per-worker frame capacity, parking at
    // the bottom. Frames past the capacity are not traced, which must be
    // reported once, and every traced frame must still balance.
    constexpr int deep_levels = 48;

    void nest(int level, std::vector<void const*>& frames, int& inline_depth)
    {
        frames[level] = self();
        if (level != 0 && runs_inline())
            inline_depth = level;
        if (level + 1 == deep_levels)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(2));
            return;
        }
        hpx::async(inline_exec(), [&, level] {
            nest(level + 1, frames, inline_depth);
        }).get();
    }

    void test_deep_nesting()
    {
        std::size_t const start = note("deep:start");
        std::vector<void const*> frames(deep_levels, nullptr);
        int inline_depth = 0;

        hpx::execution::parallel_executor exec(
            hpx::threads::thread_stacksize::huge);
        hpx::async(exec, [&] { nest(0, frames, inline_depth); }).get();

        std::size_t const after = note("deep:after_get");

        HPX_TEST_LT(32, inline_depth);
        HPX_TEST_EQ(markers(start, after, "task_stack_full"), 1);
        for (void const* frame : frames)
        {
            HPX_TEST_EQ(open_at(start, after, frame, 0), 0);
        }
    }

    // More fibers parked with inline frames than the saved-frames table can
    // hold. The overflow must be reported once and nothing may be unbalanced.
    void test_saved_frames_full()
    {
        constexpr std::size_t count = 100;

        std::size_t const start = note("table:start");
        std::vector<void const*> children(count, nullptr);
        hpx::latch parked(count);
        hpx::latch done(count + 1);

        for (std::size_t i = 0; i != count; ++i)
        {
            hpx::post([&, i] {
                hpx::async(inline_exec(), [&, i] {
                    children[i] = self();
                    parked.arrive_and_wait();
                }).get();
                done.count_down(1);
            });
        }
        done.arrive_and_wait();

        std::size_t const after = note("table:after");

        HPX_TEST_EQ(markers(start, after, "saved_frames_full"), 1);
        for (void const* child : children)
        {
            HPX_TEST_EQ(open_at(start, after, child, 0), 0);
        }
    }
#endif

#if defined(HPX_HAVE_TRACING_WORK_STEALING_EVENTS)
    void spin_for(std::chrono::microseconds d)
    {
        auto const t = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - t < d)
        {
        }
    }

    // Each steal gets its own marker instance: created, tagged with the
    // thief, victim and task, then destroyed.
    void test_work_stealing()
    {
        std::size_t const start = note("steal:start");
        std::size_t after = start;

        for (int round = 0; round != 50; ++round)
        {
            constexpr std::size_t tasks = 256;
            hpx::latch done(tasks + 1);
            for (std::size_t i = 0; i != tasks; ++i)
            {
                hpx::post([&] {
                    spin_for(std::chrono::microseconds(50));
                    done.count_down(1);
                });
            }
            done.arrive_and_wait();

            after = note("steal:round");
            if (markers(start, after, "work_stolen") != 0)
                break;
        }

        // Whether a steal happens is up to the scheduler, so a run without
        // one has nothing to check rather than being a failure.
        if (markers(start, after, "work_stolen") == 0)
        {
            std::cout << "no steal observed, steal marker checks skipped\n";
            return;
        }

        auto const ev = events();
        std::set<std::pair<std::uint64_t, std::uint64_t>> ids;
        for (std::size_t i = start; i != after; ++i)
        {
            if (ev[i].kind != hpx_itt_test::event_kind::marker ||
                std::strcmp(ev[i].name, "work_stolen") != 0)
            {
                continue;
            }

            auto const id = std::make_pair(ev[i].d1, ev[i].d2);
            HPX_TEST_MSG(ids.insert(id).second, "steal marker id reused");

            bool created = false;
            for (std::size_t j = start; j != i; ++j)
            {
                created |= ev[j].kind == hpx_itt_test::event_kind::id_create &&
                    ev[j].d1 == id.first && ev[j].d2 == id.second;
            }
            HPX_TEST_MSG(created, "steal marker id not created first");

            std::map<std::string, std::uint64_t> meta;
            bool destroyed = false;
            for (std::size_t j = i + 1; j != after && !destroyed; ++j)
            {
                if (ev[j].d1 != id.first || ev[j].d2 != id.second)
                    continue;
                if (ev[j].kind == hpx_itt_test::event_kind::metadata)
                    meta[ev[j].name] = ev[j].value;
                destroyed = ev[j].kind == hpx_itt_test::event_kind::id_destroy;
            }
            HPX_TEST_MSG(destroyed, "steal marker id not destroyed");
            HPX_TEST(meta.count("thief") == 1 && meta.count("victim") == 1 &&
                meta.count("task") == 1);
            HPX_TEST_NEQ(meta["thief"], meta["victim"]);
            HPX_TEST_NEQ(meta["task"], std::uint64_t(0));
        }
    }
#endif
}    // namespace

int hpx_main()
{
    if (!bind_collector())
    {
        HPX_TEST_MSG(false, "ITT test collector was not loaded");
        return hpx::local::finalize();
    }

    if (hpx::get_os_thread_count() == 1)
    {
#if defined(HPX_HAVE_TRACING_LIFECYCLE_EVENTS)
        test_inline_completion();
        test_inline_park("yield:start", [] { hpx::this_thread::yield(); });
        test_inline_park("suspend:start",
            [] { hpx::this_thread::sleep_for(std::chrono::milliseconds(2)); });
        test_nested_suspend();
        test_deep_nesting();
        test_saved_frames_full();
#endif
    }
    else
    {
#if defined(HPX_HAVE_TRACING_WORK_STEALING_EVENTS)
        test_work_stealing();
#endif
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    namespace po = hpx::program_options;

    po::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()("collector", po::value<std::string>(),
        "path of the recording ITT collector library");

    // The ittnotify static part reads its environment on the first ITT call,
    // which happens during runtime start-up, so the collector path has to be
    // known before init rather than taken from hpx_main's variables_map.
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

    setenv("INTEL_LIBITTNOTIFY64", collector_path.c_str(), 1);
    setenv("INTEL_ITTNOTIFY_GROUPS", "structure", 1);

    hpx::local::init_params params;
    params.desc_cmdline = desc_commandline;
    params.cfg = {"hpx.use_itt_notify=1"};

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, params), 0);
    return hpx::util::report_errors();
}
