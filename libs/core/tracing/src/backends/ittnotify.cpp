//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_ITTNOTIFY) && HPX_HAVE_ITTNOTIFY != 0

#include <hpx/config/thread_name.hpp>
#include <hpx/itt_notify/detail/use_ittnotify_api.hpp>
#include <hpx/modules/itt_notify.hpp>
#include <hpx/tracing/tracing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace hpx::tracing {

    ////////////////////////////////////////////////////////////////////////////
    namespace {

        struct itt_globals
        {
            util::itt::domain domain;
            util::itt::string_handle fiber;
            util::itt::string_handle fiber_suspend;
            util::itt::string_handle background;
            util::itt::string_handle os_thread_sleep;
            util::itt::string_handle task_stack_full;
            util::itt::string_handle saved_frames_full;
            util::itt::string_handle work_stolen;
            util::itt::string_handle thief;
            util::itt::string_handle victim;
            util::itt::string_handle task;

            itt_globals() noexcept
              : domain("hpx")
              , fiber("fiber")
              , fiber_suspend("fiber_suspend")
              , background("hpx::background")
              , os_thread_sleep("os_thread_sleep")
              , task_stack_full("task_stack_full")
              , saved_frames_full("saved_frames_full")
              , work_stolen("work_stolen")
              , thief("thief")
              , victim("victim")
              , task("task")
            {
            }
        };

        itt_globals const& get_itt_globals() noexcept
        {
            // While the tool is inactive, hand back a throwaway empty set so
            // the active one is never built with null handles. The active set
            // is constructed once, lazily, on the first call after
            // use_ittnotify_api is flipped on, so its handles are real.
            if (!use_ittnotify_api)
            {
                static itt_globals const empty;
                return empty;
            }
            static itt_globals const active;
            return active;
        }
    }    // namespace

    void tracing_init(char const*, int, char**, std::uint32_t, std::uint32_t,
        std::string_view) noexcept
    {
        [[maybe_unused]] auto const& _ = get_itt_globals();
    }

    ////////////////////////////////////////////////////////////////////////////
    // itt_counters map for caching counter metadata
    // Note: This map is not protected against concurrent modifications.
    // It is assumed that counters are created during startup and only
    // read/sampled concurrently during execution.
    static std::map<std::string, util::itt::counter> itt_counters_;

    ////////////////////////////////////////////////////////////////////////////
    // loop_context

    loop_context::loop_context() noexcept
      : domain(get_itt_globals().domain)
      , task_id("task_id")
      , task_phase("task_phase")
    {
    }

    loop_context::~loop_context() = default;

    ////////////////////////////////////////////////////////////////////////////
    // region

    util::itt::task region::make_task(
        loop_context& ctx, region_init_data const& data)
    {
        if (data.is_address_type)
        {
            return util::itt::task(
                ctx.domain, util::itt::string_handle("address"), data.address);
        }
        if (data.handle)
        {
            return util::itt::task(ctx.domain, data.handle);
        }
        return util::itt::task(ctx.domain, util::itt::string_handle(data.name));
    }

    region::region(loop_context& ctx, region_init_data const& data, std::size_t)
      : cctx(ctx.stack_ctx, !data.is_stackless)
      , task(make_task(ctx, data))
    {
        task.add_metadata(ctx.task_id, data.thread_ptr);
        task.add_metadata(ctx.task_phase, data.thread_phase);
    }

    region::~region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // counters

    void create_counter(
        std::string const& full_name, std::string const& short_name) noexcept
    {
        if (use_ittnotify_api)
        {
            // check if the counter name already exists
            if (itt_counters_.find(full_name) == itt_counters_.end())
            {
                itt_counters_.insert(std::make_pair(full_name,
                    util::itt::counter(short_name.c_str(),
                        hpx::detail::thread_name().c_str(),
                        __itt_metadata_double)));
            }
        }
    }

    void sample_counter(
        std::string const& full_name, std::string const&, double value) noexcept
    {
        if (use_ittnotify_api)
        {
            auto it = itt_counters_.find(full_name);
            if (it != itt_counters_.end())
            {
                (*it).second.set_value(value);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // regions

    fiber_region::fiber_region(
        fiber_region_init_data const& data, std::size_t) noexcept
      : task_(get_itt_globals().domain,
            data.name != nullptr ? util::itt::string_handle(data.name) :
                                   get_itt_globals().fiber)
    {
    }

    fiber_region::~fiber_region() = default;

    fiber_suspend_region::fiber_suspend_region(char const* desc) noexcept
      : task_(get_itt_globals().domain,
            desc != nullptr ? util::itt::string_handle(desc) :
                              get_itt_globals().fiber_suspend)
    {
    }

    fiber_suspend_region::~fiber_suspend_region() = default;

    background_work_region::background_work_region(std::size_t) noexcept
      : task_(get_itt_globals().domain, get_itt_globals().background)
    {
    }

    background_work_region::~background_work_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // markers

    mark_event::mark_event(char const* name) noexcept
    {
        util::itt::emit_marker(get_itt_globals().domain,
            util::itt::string_handle(name != nullptr ? name : "mark"));
    }

    void frame_mark(char const* name) noexcept
    {
        util::itt::emit_marker(get_itt_globals().domain,
            util::itt::string_handle(name != nullptr ? name : "frame"));
    }

    void os_thread_sleep(std::size_t) noexcept
    {
        util::itt::emit_marker(
            get_itt_globals().domain, get_itt_globals().os_thread_sleep);
    }

    ////////////////////////////////////////////////////////////////////////////
    // task lifecycle
#if defined(HPX_HAVE_TRACING_LIFECYCLE_EVENTS)
    namespace {

        // A task's overlapped id is recomputed from its thread_data address at
        // every site rather than stored. __itt_id_make is deterministic and
        // the collector correlates begin/end/create/destroy by id value, so
        // the same address always names the same live task.
        ___itt_id task_id_value(void const* task_id) noexcept
        {
            return ::itt_id_value(const_cast<void*>(task_id), 0);
        }

        // Emits a marker the first time one of the fixed limits below is hit,
        // so a trace that lost frames says so instead of looking complete.
        void report_once(std::atomic<bool>& reported,
            util::itt::string_handle const& name) noexcept
        {
            if (!reported.exchange(true, std::memory_order_relaxed))
                util::itt::emit_marker(get_itt_globals().domain, name);
        }

        struct frame
        {
            void const* task;
            char const* name;
        };

        // Inline continuations nest a child inside a running parent on the
        // same worker, so each worker keeps the stack of tasks open on it.
        // Storage is fixed so a hook never allocates; frames past the capacity
        // still count towards the depth but get no segment of their own.
        constexpr std::size_t max_frames = 32;

        struct open_frames
        {
            frame frames[max_frames] = {};
            std::size_t stored = 0;
            std::size_t depth = 0;
        };

        thread_local open_frames worker;

        // A park takes the whole fiber, inline frames included, and it may
        // resume on another worker. The frames above the outer task wait here,
        // keyed by that task, until it begins again. Fixed size for the same
        // reason as above; when full, those frames are not reopened.
        constexpr std::size_t max_saved = 64;

        struct saved_frames
        {
            std::atomic<void const*> owner{nullptr};
            frame frames[max_frames - 1] = {};
            std::size_t stored = 0;
            std::size_t extra_depth = 0;
        };

        saved_frames saved[max_saved];
        std::atomic<std::size_t> saved_count{0};
        char const claimed = 0;

        std::atomic<bool> stack_full_reported{false};
        std::atomic<bool> table_full_reported{false};

        void push_frame(void const* task, char const* name) noexcept
        {
            if (worker.stored == max_frames)
            {
                ++worker.depth;
                report_once(
                    stack_full_reported, get_itt_globals().task_stack_full);
                return;
            }

            worker.frames[worker.stored++] = {task, name};
            ++worker.depth;

            ___itt_id id = task_id_value(task);
            util::itt::string_handle const sh(name != nullptr ? name : "task");
            ::itt_task_begin_overlapped(
                get_itt_globals().domain.domain_, &id, sh.handle_);
        }

        void save_frames() noexcept
        {
            for (auto& slot : saved)
            {
                void const* expected = nullptr;
                if (!slot.owner.compare_exchange_strong(expected, &claimed,
                        std::memory_order_acquire, std::memory_order_relaxed))
                {
                    continue;
                }

                slot.stored = worker.stored - 1;
                for (std::size_t i = 0; i != slot.stored; ++i)
                    slot.frames[i] = worker.frames[i + 1];
                slot.extra_depth = worker.depth - worker.stored;

                slot.owner.store(
                    worker.frames[0].task, std::memory_order_release);
                saved_count.fetch_add(1, std::memory_order_relaxed);
                return;
            }
            report_once(
                table_full_reported, get_itt_globals().saved_frames_full);
        }

        saved_frames* find_saved(void const* task) noexcept
        {
            if (saved_count.load(std::memory_order_relaxed) == 0)
                return nullptr;

            for (auto& slot : saved)
            {
                if (slot.owner.load(std::memory_order_acquire) == task)
                    return &slot;
            }
            return nullptr;
        }

        void release(saved_frames& slot) noexcept
        {
            slot.owner.store(nullptr, std::memory_order_release);
            saved_count.fetch_sub(1, std::memory_order_relaxed);
        }

        void begin_task(void const* task, char const* name) noexcept
        {
            if (worker.depth != 0)
            {
                if (worker.stored == worker.depth &&
                    worker.frames[worker.stored - 1].task == task)
                {
                    return;
                }
                push_frame(task, name);
                return;
            }

            push_frame(task, name);
            if (saved_frames* slot = find_saved(task))
            {
                for (std::size_t i = 0; i != slot->stored; ++i)
                    push_frame(slot->frames[i].task, slot->frames[i].name);
                worker.depth += slot->extra_depth;
                release(*slot);
            }
        }

        // The resume hook fires from the woken fiber after the scheduler has
        // already begun its outer task. It never starts a new nesting level,
        // so it only opens a segment if nothing on this worker has yet.
        void resume_task(void const* task, char const* name) noexcept
        {
            if (worker.depth == 0)
                begin_task(task, name);
        }

        // A park takes the whole fiber off this worker, so every open frame
        // is ended; the inline ones are saved to reopen with the outer task.
        void end_current_worker() noexcept
        {
            if (worker.depth == 0)
                return;
            if (worker.depth > 1)
                save_frames();

            auto const* domain = get_itt_globals().domain.domain_;
            while (worker.stored != 0)
            {
                ___itt_id id =
                    task_id_value(worker.frames[--worker.stored].task);
                ::itt_task_end_overlapped(domain, &id);
            }
            worker.depth = 0;
        }

        // Completion ends just the innermost frame, since an inline child
        // finishing leaves its parent running. Past the capacity that frame
        // has no segment, and inline frames unwind in order, so it is only
        // counted down.
        void end_task(void const* task) noexcept
        {
            if (worker.depth > worker.stored)
            {
                --worker.depth;
                return;
            }
            if (worker.stored == 0 ||
                worker.frames[worker.stored - 1].task != task)
            {
                return;
            }

            ___itt_id id = task_id_value(task);
            ::itt_task_end_overlapped(get_itt_globals().domain.domain_, &id);
            --worker.stored;
            --worker.depth;
        }
    }    // namespace

    void task_staged(char const* name, void const*) noexcept
    {
        if (!use_ittnotify_api)
            return;

        // A staged task has no thread_data yet, hence no id; a plain marker
        // records the enqueue without opening an overlapped task.
        util::itt::emit_marker(get_itt_globals().domain,
            util::itt::string_handle(name != nullptr ? name : "staged"));
    }

    void task_created(char const*, void const* task_id, void const*) noexcept
    {
        if (!use_ittnotify_api)
            return;

        ___itt_id id = task_id_value(task_id);
        ::itt_id_create(get_itt_globals().domain.domain_, &id);
    }

    void task_executing(
        void const* task_id, char const* name, std::size_t) noexcept
    {
        if (!use_ittnotify_api)
            return;
        begin_task(task_id, name);
    }

    void task_yielded(void const*, char const*) noexcept
    {
        if (!use_ittnotify_api)
            return;
        end_current_worker();
    }

    void task_suspended(void const*, char const*, char const*) noexcept
    {
        if (!use_ittnotify_api)
            return;
        end_current_worker();
    }

    void task_resumed(
        void const* task_id, char const* name, char const*) noexcept
    {
        if (!use_ittnotify_api)
            return;
        resume_task(task_id, name);
    }

    void task_completed(void const* task_id, char const*) noexcept
    {
        if (!use_ittnotify_api)
            return;
        end_task(task_id);
    }

    void task_deleted(void const* task_id) noexcept
    {
        if (!use_ittnotify_api)
            return;

        // A parked task destroyed without resuming must not leave its frames
        // behind for the next task allocated at the same address.
        if (saved_frames* slot = find_saved(task_id))
            release(*slot);

        ___itt_id id = task_id_value(task_id);
        ::itt_id_destroy_value(get_itt_globals().domain.domain_, &id);
    }
#endif

#if defined(HPX_HAVE_TRACING_WORK_STEALING_EVENTS)
    void work_stolen(std::size_t thief, std::size_t victim, void const* task_id,
        char const*) noexcept
    {
        if (!use_ittnotify_api)
            return;

        // A marker id only names that marker, so each steal gets a fresh one
        // from a per-thread counter. The stolen task is carried as metadata,
        // which needs no live task id and so also works with lifecycle off.
        thread_local std::size_t steal_count = 0;
        std::size_t const n = ++steal_count;
        ___itt_id id = ::itt_id_value(&steal_count, n);

        auto const& g = get_itt_globals();
        auto const* domain = g.domain.domain_;
        std::uint64_t const thief_id = thief;
        std::uint64_t const victim_id = victim;
        std::uint64_t const task = static_cast<std::uint64_t>(
            reinterpret_cast<std::uintptr_t>(task_id));

        ::itt_id_create(domain, &id);
        ::itt_marker(domain, &id, g.work_stolen.handle_);
        ::itt_metadata_add(domain, &id, g.thief.handle_, thief_id);
        ::itt_metadata_add(domain, &id, g.victim.handle_, victim_id);
        ::itt_metadata_add(domain, &id, g.task.handle_, task);
        ::itt_id_destroy_value(domain, &id);
    }
#endif

}    // namespace hpx::tracing

#endif
