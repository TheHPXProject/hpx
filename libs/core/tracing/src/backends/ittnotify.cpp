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

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

            itt_globals() noexcept
              : domain("hpx")
              , fiber("fiber")
              , fiber_suspend("fiber_suspend")
              , background("hpx::background")
              , os_thread_sleep("os_thread_sleep")
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
#if defined(HPX_HAVE_TRACING_LIFECYCLE_EVENTS) ||                              \
    defined(HPX_HAVE_TRACING_WORK_STEALING_EVENTS)
    namespace {

        // A task's overlapped id is recomputed from its thread_data address at
        // every site rather than stored. __itt_id_make is deterministic and
        // the collector correlates begin/end/create/destroy by id value, so
        // the same address always names the same live task.
        ___itt_id task_id_value(void const* task_id) noexcept
        {
            return ::itt_id_value(const_cast<void*>(task_id), 0);
        }
    }    // namespace
#endif

#if defined(HPX_HAVE_TRACING_LIFECYCLE_EVENTS)
    namespace {

        // Per-worker stack of overlapped tasks open on this OS thread. Inline
        // continuations nest a child inside a running parent on the same
        // worker, so a single slot will not do. Depth stays tiny, so storage is
        // inline and the vector is an overflow guard that is never reached.
        struct task_id_stack
        {
            static constexpr std::size_t inline_capacity = 32;

            bool empty() const noexcept
            {
                return count_ == 0;
            }
            void const* back() const noexcept
            {
                return count_ > inline_capacity ?
                    overflow_[count_ - inline_capacity - 1] :
                    inline_[count_ - 1];
            }
            void push(void const* p)
            {
                if (count_ >= inline_capacity)
                    overflow_.push_back(p);
                else
                    inline_[count_] = p;
                ++count_;
            }
            void pop() noexcept
            {
                if (count_ > inline_capacity)
                    overflow_.pop_back();
                if (count_ != 0)
                    --count_;
            }

            void const* inline_[inline_capacity] = {};
            std::vector<void const*> overflow_;
            std::size_t count_ = 0;
        };

        thread_local task_id_stack open_tasks;

        void begin_task(void const* task_id, char const* name) noexcept
        {
            // Begin is idempotent: a resume signals once from the scheduling
            // loop and once from the woken fiber on the same worker, and an
            // already-open task must not be opened twice.
            if (!open_tasks.empty() && open_tasks.back() == task_id)
                return;

            open_tasks.push(task_id);
            ___itt_id id = task_id_value(task_id);
            util::itt::string_handle const sh(name != nullptr ? name : "task");
            ::itt_task_begin_overlapped(
                get_itt_globals().domain.domain_, &id, sh.handle_);
        }

        void end_current_worker() noexcept
        {
            // A park takes the whole fiber off this worker, so every task open
            // here is ended. A nested inline child parks its parent too, which
            // is why the entire worker stack is flushed rather than one frame.
            auto const* domain = get_itt_globals().domain.domain_;
            while (!open_tasks.empty())
            {
                ___itt_id id = task_id_value(open_tasks.back());
                ::itt_task_end_overlapped(domain, &id);
                open_tasks.pop();
            }
        }

        void end_task(void const* task_id) noexcept
        {
            // Completion ends just this frame: an inline child finishing leaves
            // its parent running on the same worker, so only a matching top of
            // stack is closed.
            if (open_tasks.empty() || open_tasks.back() != task_id)
                return;

            ___itt_id id = task_id_value(task_id);
            ::itt_task_end_overlapped(get_itt_globals().domain.domain_, &id);
            open_tasks.pop();
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
        begin_task(task_id, name);
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

        ___itt_id id = task_id_value(task_id);
        ::itt_id_destroy_value(get_itt_globals().domain.domain_, &id);
    }
#endif

#if defined(HPX_HAVE_TRACING_WORK_STEALING_EVENTS)
    void work_stolen(std::size_t, std::size_t, void const* task_id,
        char const* name) noexcept
    {
        if (!use_ittnotify_api)
            return;

        // Tag the steal with the stolen task's id so it lands on that task's
        // timeline instead of appearing as an anonymous instant.
        ___itt_id id = task_id_value(task_id);
        util::itt::string_handle const sh(
            name != nullptr ? name : "work_stolen");
        ::itt_marker(get_itt_globals().domain.domain_, &id, sh.handle_);
    }
#endif

}    // namespace hpx::tracing

#endif
