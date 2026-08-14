//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>

HPX_CXX_CORE_EXPORT struct ___itt_caller;
HPX_CXX_CORE_EXPORT struct ___itt_string_handle;
HPX_CXX_CORE_EXPORT struct ___itt_domain;
HPX_CXX_CORE_EXPORT struct ___itt_id;
HPX_CXX_CORE_EXPORT using __itt_heap_function = void*;
HPX_CXX_CORE_EXPORT struct ___itt_counter;

///////////////////////////////////////////////////////////////////////////////
// decide whether to use the ITT notify API if it's available

#if HPX_HAVE_ITTNOTIFY != 0

#include <ittnotify.h>

///////////////////////////////////////////////////////////////////////////////
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_pause() noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_resume() noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_detach() noexcept;

///////////////////////////////////////////////////////////////////////////////
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_create(
    void* addr, char const* objtype, char const* objname) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_rename(
    void* addr, char const* name) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_prepare(void* addr) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_acquired(void* addr) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_cancel(void* addr) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_releasing(
    void* addr) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_released(void* addr) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_sync_destroy(void* addr) noexcept;

HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT ___itt_caller*
itt_stack_create() noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_stack_enter(
    ___itt_caller* ctx) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_stack_leave(
    ___itt_caller* ctx) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_stack_destroy(
    ___itt_caller* ctx) noexcept;

HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_frame_begin(
    ___itt_domain const* frame, ___itt_id* id) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_frame_end(
    ___itt_domain const* frame, ___itt_id* id) noexcept;

HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT int itt_mark_create(
    char const*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_mark_off(int mark) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_mark(
    int mark, char const*) noexcept;

HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_thread_set_name(
    char const*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_thread_ignore() noexcept;

HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_task_begin(
    ___itt_domain const*, ___itt_string_handle*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_task_begin(
    ___itt_domain const*, ___itt_id const*, ___itt_string_handle*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_task_end(
    ___itt_domain const*) noexcept;

HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT ___itt_domain* itt_domain_create(
    char const*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT ___itt_string_handle*
itt_string_handle_create(char const*) noexcept;

HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT ___itt_id* itt_make_id(
    void*, std::size_t);
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_id_create(
    ___itt_domain const*, ___itt_id const* id) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_id_destroy(
    ___itt_id const* id) noexcept;

HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT __itt_heap_function
itt_heap_function_create(char const*, char const*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_heap_allocate_begin(
    __itt_heap_function, std::size_t, int) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_heap_allocate_end(
    __itt_heap_function, void**, std::size_t, int) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_heap_free_begin(
    __itt_heap_function, void*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_heap_free_end(
    __itt_heap_function, void*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_heap_reallocate_begin(
    __itt_heap_function, void*, std::size_t, int) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_heap_reallocate_end(
    __itt_heap_function, void*, void**, std::size_t, int) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void
itt_heap_internal_access_begin() noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void
itt_heap_internal_access_end() noexcept;

HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT ___itt_counter*
itt_counter_create(char const*, char const*) noexcept;
HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT ___itt_counter*
itt_counter_create_typed(char const*, char const*, int) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_counter_destroy(
    ___itt_counter*) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_counter_set_value(
    ___itt_counter*, void*) noexcept;

HPX_CXX_CORE_EXPORT [[nodiscard]] HPX_CORE_EXPORT int itt_event_create(
    char const* name, int namelen) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int itt_event_start(int evnt) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int itt_event_end(int evnt) noexcept;

HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_metadata_add(
    ___itt_domain const* domain, ___itt_id const* id, ___itt_string_handle* key,
    std::uint64_t const& data) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_metadata_add(
    ___itt_domain const* domain, ___itt_id const* id, ___itt_string_handle* key,
    double const& data) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_metadata_add(
    ___itt_domain const* domain, ___itt_id const* id, ___itt_string_handle* key,
    char const* data) noexcept;
HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void itt_metadata_add(
    ___itt_domain const* domain, ___itt_id const* id, ___itt_string_handle* key,
    void const* data) noexcept;

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads {

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT thread_description;
}    // namespace hpx::threads

namespace hpx::util::itt {

    HPX_CXX_CORE_EXPORT struct stack_context
    {
        HPX_CORE_EXPORT stack_context();
        HPX_CORE_EXPORT ~stack_context();

        stack_context(stack_context const& rhs) = delete;
        stack_context(stack_context&& rhs) noexcept
          : itt_context_(rhs.itt_context_)
        {
            rhs.itt_context_ = nullptr;
        }

        stack_context& operator=(stack_context const& rhs) = delete;
        stack_context& operator=(stack_context&& rhs) noexcept
        {
            if (this != &rhs)
            {
                itt_context_ = rhs.itt_context_;
                rhs.itt_context_ = nullptr;
            }
            return *this;
        }

        ___itt_caller* itt_context_ = nullptr;
    };

    HPX_CXX_CORE_EXPORT struct caller_context
    {
        HPX_CORE_EXPORT explicit caller_context(
            stack_context& ctx, bool enter = true);
        HPX_CORE_EXPORT ~caller_context();

        caller_context(caller_context const&) = delete;
        caller_context(caller_context&&) = delete;
        caller_context& operator=(caller_context const&) = delete;
        caller_context& operator=(caller_context&&) = delete;

        stack_context& ctx_;
        bool enter_;
    };

    //////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct domain
    {
        domain(domain const&) = delete;
        domain(domain&&) = delete;
        domain& operator=(domain const&) = delete;
        domain& operator=(domain&&) = delete;

        domain() = default;
        ~domain() = default;

        HPX_CORE_EXPORT explicit domain(char const*) noexcept;

        ___itt_domain* domain_ = nullptr;
    };

    HPX_CXX_CORE_EXPORT struct thread_domain : domain
    {
        thread_domain(thread_domain const&) = delete;
        thread_domain(thread_domain&&) = delete;
        thread_domain& operator=(thread_domain const&) = delete;
        thread_domain& operator=(thread_domain&&) = delete;

        HPX_CORE_EXPORT thread_domain() noexcept;
        ~thread_domain() = default;
    };

    HPX_CXX_CORE_EXPORT struct id
    {
        HPX_CORE_EXPORT id(
            domain const& domain, void* addr, unsigned long extra = 0) noexcept;
        HPX_CORE_EXPORT ~id();

        id(id const& rhs) = delete;
        id(id&& rhs) noexcept
          : id_(rhs.id_)
        {
            rhs.id_ = nullptr;
        }

        id& operator=(id const& rhs) = delete;
        id& operator=(id&& rhs) noexcept
        {
            if (this != &rhs)
            {
                id_ = rhs.id_;
                rhs.id_ = nullptr;
            }
            return *this;
        }

        ___itt_id* id_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct frame_context
    {
        HPX_CORE_EXPORT explicit frame_context(
            domain const& domain, id* ident = nullptr) noexcept;
        HPX_CORE_EXPORT ~frame_context();

        frame_context(frame_context const&) = delete;
        frame_context(frame_context&&) = delete;
        frame_context& operator=(frame_context const&) = delete;
        frame_context& operator=(frame_context&&) = delete;

        domain const& domain_;
        id* ident_ = nullptr;
    };

    HPX_CXX_CORE_EXPORT struct undo_frame_context
    {
        HPX_CORE_EXPORT explicit undo_frame_context(
            frame_context& frame) noexcept;
        HPX_CORE_EXPORT ~undo_frame_context();

        undo_frame_context(undo_frame_context const&) = delete;
        undo_frame_context(undo_frame_context&&) = delete;
        undo_frame_context& operator=(undo_frame_context const&) = delete;
        undo_frame_context& operator=(undo_frame_context&&) = delete;

        frame_context& frame_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct mark_context
    {
        HPX_CORE_EXPORT explicit mark_context(char const* name) noexcept;
        HPX_CORE_EXPORT ~mark_context();

        mark_context(mark_context const&) = delete;
        mark_context(mark_context&&) = delete;
        mark_context& operator=(mark_context const&) = delete;
        mark_context& operator=(mark_context&&) = delete;

        int itt_mark_;
        char const* name_ = nullptr;
    };

    HPX_CXX_CORE_EXPORT struct undo_mark_context
    {
        HPX_CORE_EXPORT explicit undo_mark_context(mark_context& mark) noexcept;
        HPX_CORE_EXPORT ~undo_mark_context();

        undo_mark_context(undo_mark_context const&) = delete;
        undo_mark_context(undo_mark_context&&) = delete;
        undo_mark_context& operator=(undo_mark_context const&) = delete;
        undo_mark_context& operator=(undo_mark_context&&) = delete;

        mark_context& mark_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct string_handle
    {
        string_handle() noexcept = default;
        ~string_handle() = default;

        HPX_CORE_EXPORT explicit string_handle(char const* s) noexcept;

        explicit string_handle(___itt_string_handle* h) noexcept
          : handle_(h)
        {
        }

        string_handle(string_handle const&) = default;
        string_handle(string_handle&& rhs) noexcept
          : handle_(rhs.handle_)
        {
            rhs.handle_ = nullptr;
        }

        string_handle& operator=(string_handle const&) = default;
        string_handle& operator=(string_handle&& rhs) noexcept
        {
            if (this != &rhs)
            {
                handle_ = rhs.handle_;
                rhs.handle_ = nullptr;
            }
            return *this;
        }

        string_handle& operator=(___itt_string_handle* h) noexcept
        {
            handle_ = h;
            return *this;
        }

        explicit constexpr operator bool() const noexcept
        {
            return handle_ != nullptr;
        }

        ___itt_string_handle* handle_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct task
    {
        HPX_CORE_EXPORT task(
            domain const&, string_handle, std::uint64_t metadata) noexcept;
        HPX_CORE_EXPORT task(domain const&, string_handle) noexcept;
        HPX_CORE_EXPORT ~task();

        task(task const&) = delete;
        task(task&&) = delete;
        task& operator=(task const&) = delete;
        task& operator=(task&&) = delete;

        HPX_CORE_EXPORT void add_metadata(
            string_handle const& name, std::uint64_t val) const noexcept;
        HPX_CORE_EXPORT void add_metadata(
            string_handle const& name, double val) const noexcept;
        HPX_CORE_EXPORT void add_metadata(
            string_handle const& name, char const* val) const noexcept;
        HPX_CORE_EXPORT void add_metadata(
            string_handle const& name, void const* val) const noexcept;

        template <typename T>
        void add_metadata(
            string_handle const& name, T const& val) const noexcept
        {
            add_metadata(name, static_cast<void const*>(&val));
        }

        domain const& domain_;
        ___itt_id* id_ = nullptr;
        string_handle sh_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct heap_function
    {
        HPX_CORE_EXPORT heap_function(
            char const* name, char const* domain) noexcept;

        __itt_heap_function heap_function_ = nullptr;
    };

    HPX_CXX_CORE_EXPORT struct heap_internal_access
    {
        HPX_CORE_EXPORT heap_internal_access() noexcept;
        HPX_CORE_EXPORT ~heap_internal_access();

        heap_internal_access(heap_internal_access const&) = delete;
        heap_internal_access(heap_internal_access&&) = delete;
        heap_internal_access& operator=(heap_internal_access const&) = delete;
        heap_internal_access& operator=(heap_internal_access&&) = delete;
    };

    HPX_CXX_CORE_EXPORT struct heap_allocate
    {
        template <typename T>
        heap_allocate(heap_function& heap_function, T**& addr, std::size_t size,
            int initial) noexcept
          : heap_function_(heap_function)
          , addr_(reinterpret_cast<void**&>(addr))
          , size_(size)
          , init_(initial)
        {
            init();
        }

        HPX_CORE_EXPORT ~heap_allocate();

        heap_allocate(heap_allocate const&) = delete;
        heap_allocate(heap_allocate&&) = delete;
        heap_allocate& operator=(heap_allocate const&) = delete;
        heap_allocate& operator=(heap_allocate&&) = delete;

    private:
        HPX_CORE_EXPORT void init() const;

        heap_function& heap_function_;
        void**& addr_;
        std::size_t size_;
        int init_;
    };

    HPX_CXX_CORE_EXPORT struct heap_free
    {
        HPX_CORE_EXPORT heap_free(
            heap_function& heap_function, void* addr) noexcept;
        HPX_CORE_EXPORT ~heap_free();

        heap_free(heap_free const&) = delete;
        heap_free(heap_free&&) = delete;
        heap_free& operator=(heap_free const&) = delete;
        heap_free& operator=(heap_free&&) = delete;

    private:
        heap_function& heap_function_;
        void* addr_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct counter
    {
        HPX_CORE_EXPORT counter(char const* name, char const* domain) noexcept;
        HPX_CORE_EXPORT counter(
            char const* name, char const* domain, int type) noexcept;
        HPX_CORE_EXPORT ~counter();

        template <typename T>
        void set_value(T const& value) noexcept
        {
            set_value_void(const_cast<void*>(static_cast<void const*>(&value)));
        }

        counter(counter const& rhs) = delete;
        counter(counter&& rhs) noexcept
          : id_(rhs.id_)
        {
            rhs.id_ = nullptr;
        }

        counter& operator=(counter const& rhs) = delete;
        counter& operator=(counter&& rhs) noexcept
        {
            if (this != &rhs)
            {
                id_ = rhs.id_;
                rhs.id_ = nullptr;
            }
            return *this;
        }

    private:
        HPX_CORE_EXPORT void set_value_void(void* data) const;

        ___itt_counter* id_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct event
    {
        explicit event(char const* name) noexcept
          : event_(itt_event_create(name, static_cast<int>(strnlen(name, 256))))
        {
        }

        void start() const noexcept
        {
            itt_event_start(event_);
        }

        void end() const noexcept
        {
            itt_event_end(event_);
        }

    private:
        int event_ = 0;
    };

    HPX_CXX_CORE_EXPORT struct mark_event
    {
        explicit mark_event(event const& e) noexcept
          : e_(e)
        {
            e_.start();
        }
        ~mark_event()
        {
            e_.end();
        }

        mark_event(mark_event const&) = delete;
        mark_event(mark_event&&) = delete;
        mark_event& operator=(mark_event const&) = delete;
        mark_event& operator=(mark_event&&) = delete;

    private:
        event e_;
    };

    HPX_CXX_CORE_EXPORT inline void event_tick(event const& e) noexcept
    {
        e.start();
    }
}    // namespace hpx::util::itt

#else

HPX_CXX_CORE_EXPORT constexpr void itt_pause() noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_resume() noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_detach() noexcept {}

HPX_CXX_CORE_EXPORT constexpr void itt_sync_create(
    void*, char const*, char const*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_sync_rename(void*, char const*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_sync_prepare(void*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_sync_acquired(void*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_sync_cancel(void*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_sync_releasing(void*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_sync_released(void*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_sync_destroy(void*) noexcept {}

HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr ___itt_caller*
itt_stack_create() noexcept
{
    return nullptr;
}
HPX_CXX_CORE_EXPORT constexpr void itt_stack_enter(___itt_caller*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_stack_leave(___itt_caller*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_stack_destroy(___itt_caller*) noexcept {}

HPX_CXX_CORE_EXPORT constexpr void itt_frame_begin(
    ___itt_domain const*, ___itt_id*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_frame_end(
    ___itt_domain const*, ___itt_id*) noexcept
{
}

HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr int itt_mark_create(
    char const*) noexcept
{
    return 0;
}
HPX_CXX_CORE_EXPORT constexpr void itt_mark_off(int) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_mark(int, char const*) noexcept {}

HPX_CXX_CORE_EXPORT constexpr void itt_thread_set_name(char const*) noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_thread_ignore() noexcept {}

HPX_CXX_CORE_EXPORT constexpr void itt_task_begin(
    ___itt_domain const*, ___itt_string_handle*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_task_begin(
    ___itt_domain const*, ___itt_id*, ___itt_string_handle*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_task_end(___itt_domain const*) noexcept
{
}

HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr ___itt_domain* itt_domain_create(
    char const*) noexcept
{
    return nullptr;
}
HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr ___itt_string_handle*
itt_string_handle_create(char const*) noexcept
{
    return nullptr;
}

HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr ___itt_id* itt_make_id(
    void*, unsigned long)
{
    return nullptr;
}
HPX_CXX_CORE_EXPORT constexpr void itt_id_create(
    ___itt_domain const*, ___itt_id*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_id_destroy(___itt_id*) noexcept {}

HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr __itt_heap_function
itt_heap_function_create(char const*, char const*) noexcept
{
    return nullptr;
}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_allocate_begin(
    __itt_heap_function, std::size_t, int) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_allocate_end(
    __itt_heap_function, void**, std::size_t, int) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_free_begin(
    __itt_heap_function, void*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_free_end(
    __itt_heap_function, void*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_reallocate_begin(
    __itt_heap_function, void*, std::size_t, int) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_reallocate_end(
    __itt_heap_function, void*, void**, std::size_t, int) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_internal_access_begin() noexcept {}
HPX_CXX_CORE_EXPORT constexpr void itt_heap_internal_access_end() noexcept {}

HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr ___itt_counter* itt_counter_create(
    char const*, char const*) noexcept
{
    return nullptr;
}
HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr ___itt_counter*
itt_counter_create_typed(char const*, char const*, int) noexcept
{
    return nullptr;
}
HPX_CXX_CORE_EXPORT constexpr void itt_counter_destroy(___itt_counter*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_counter_set_value(
    ___itt_counter*, void*) noexcept
{
}

HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr int itt_event_create(
    char const*, int) noexcept
{
    return 0;
}
HPX_CXX_CORE_EXPORT constexpr int itt_event_start(int) noexcept
{
    return 0;
}
HPX_CXX_CORE_EXPORT constexpr int itt_event_end(int) noexcept
{
    return 0;
}

HPX_CXX_CORE_EXPORT constexpr void itt_metadata_add(
    ___itt_domain*, ___itt_id*, ___itt_string_handle*, std::uint64_t) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_metadata_add(
    ___itt_domain*, ___itt_id*, ___itt_string_handle*, double) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_metadata_add(
    ___itt_domain*, ___itt_id*, ___itt_string_handle*, char const*) noexcept
{
}
HPX_CXX_CORE_EXPORT constexpr void itt_metadata_add(
    ___itt_domain*, ___itt_id*, ___itt_string_handle*, void const*) noexcept
{
}

//////////////////////////////////////////////////////////////////////////////
namespace hpx::threads {

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT thread_description;
}    // namespace hpx::threads

namespace hpx::util::itt {

    HPX_CXX_CORE_EXPORT struct stack_context
    {
        stack_context() = default;
        ~stack_context() = default;
    };

    HPX_CXX_CORE_EXPORT struct caller_context
    {
        constexpr explicit caller_context(stack_context&, bool = true) noexcept
        {
        }
        ~caller_context() = default;
    };

    //////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct domain
    {
        HPX_NON_COPYABLE(domain);

        constexpr explicit domain(char const*) noexcept {}
        domain() = default;
        ~domain() = default;
    };

    HPX_CXX_CORE_EXPORT struct thread_domain : domain
    {
        HPX_NON_COPYABLE(thread_domain);

        thread_domain() = default;
        ~thread_domain() = default;
    };

    HPX_CXX_CORE_EXPORT struct id
    {
        constexpr id(domain const&, void*, unsigned long = 0) noexcept {}
        ~id() = default;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct frame_context
    {
        constexpr explicit frame_context(domain const&, id* = nullptr) noexcept
        {
        }
        ~frame_context() = default;
    };

    HPX_CXX_CORE_EXPORT struct undo_frame_context
    {
        constexpr explicit undo_frame_context(frame_context const&) noexcept {}
        ~undo_frame_context() = default;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct mark_context
    {
        constexpr explicit mark_context(char const*) noexcept {}
        ~mark_context() = default;
    };

    HPX_CXX_CORE_EXPORT struct undo_mark_context
    {
        constexpr explicit undo_mark_context(mark_context const&) noexcept {}
        ~undo_mark_context() = default;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct string_handle
    {
        constexpr explicit string_handle(char const* = nullptr) noexcept {}
    };

    //////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct task
    {
        constexpr task(
            domain const&, string_handle const&, std::uint64_t) noexcept
        {
        }
        constexpr task(domain const&, string_handle const&) noexcept {}

        ~task() = default;

        template <typename T>
        void add_metadata(string_handle const&, T const&) const noexcept
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct heap_function
    {
        constexpr heap_function(char const*, char const*) noexcept {}
        ~heap_function() = default;
    };

    HPX_CXX_CORE_EXPORT struct heap_allocate
    {
        template <typename T>
        constexpr heap_allocate(
            heap_function& /*heap_function*/, T**, std::size_t, int) noexcept
        {
        }
        ~heap_allocate() = default;
    };

    HPX_CXX_CORE_EXPORT struct heap_free
    {
        constexpr heap_free(heap_function& /*heap_function*/, void*) noexcept {}
        ~heap_free() = default;
    };

    HPX_CXX_CORE_EXPORT struct heap_internal_access
    {
        heap_internal_access() = default;
        ~heap_internal_access() = default;
    };

    HPX_CXX_CORE_EXPORT struct counter
    {
        constexpr counter(char const* /*name*/, char const* /*domain*/) noexcept
        {
        }
        ~counter() = default;
    };

    HPX_CXX_CORE_EXPORT struct event
    {
        constexpr explicit event(char const*) noexcept {}
    };

    HPX_CXX_CORE_EXPORT struct mark_event
    {
        constexpr explicit mark_event(event const&) noexcept {}
        ~mark_event() = default;
    };

    HPX_CXX_CORE_EXPORT constexpr void event_tick(event const&) noexcept {}
}    // namespace hpx::util::itt

#endif    // HPX_HAVE_ITTNOTIFY
