//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/itt_notify/api.hpp>
#include <hpx/itt_notify/detail/use_ittnotify_api.hpp>
#include <hpx/itt_notify/macros.hpp>
#include <hpx/itt_notify/thread_name.hpp>

#if HPX_HAVE_ITTNOTIFY != 0

#define INTEL_ITTNOTIFY_API_PRIVATE

#include <ittnotify.h>
#include <legacy/ittnotify.h>

#include <cstddef>
#include <cstdint>
#include <memory>

///////////////////////////////////////////////////////////////////////////////
// decide whether to use the ITT notify API if it's available
bool use_ittnotify_api = false;

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_PAUSE()                                               \
    if (use_ittnotify_api && __itt_pause_ptr)                                  \
    {                                                                          \
        __itt_pause_ptr();                                                     \
    }                                                                          \
    /**/
#define HPX_INTERNAL_ITT_RESUME()                                              \
    if (use_ittnotify_api && __itt_resume_ptr)                                 \
    {                                                                          \
        __itt_resume_ptr();                                                    \
    }                                                                          \
    /**/
#define HPX_INTERNAL_ITT_DETACH()                                              \
    if (use_ittnotify_api && __itt_detach_ptr)                                 \
    {                                                                          \
        __itt_detach_ptr();                                                    \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_SYNC_CREATE(obj, type, name)                          \
    if (use_ittnotify_api && __itt_sync_create_ptr)                            \
    {                                                                          \
        __itt_sync_create_ptr(                                                 \
            const_cast<void*>(static_cast<volatile void*>(obj)), type, name,   \
            __itt_attr_mutex);                                                 \
    }                                                                          \
    /**/
#define HPX_INTERNAL_ITT_SYNC(fname, obj)                                      \
    if (use_ittnotify_api && __itt_##fname##_ptr)                              \
    {                                                                          \
        __itt_##fname##_ptr(                                                   \
            const_cast<void*>(static_cast<volatile void*>(obj)));              \
    }                                                                          \
    /**/
#define HPX_INTERNAL_ITT_SYNC_RENAME(obj, name)                                \
    if (use_ittnotify_api && __itt_sync_rename_ptr)                            \
    {                                                                          \
        __itt_sync_rename_ptr(                                                 \
            const_cast<void*>(static_cast<volatile void*>(obj)), name);        \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_STACK_CREATE()                                        \
    (use_ittnotify_api && __itt_stack_caller_create_ptr) ?                     \
        __itt_stack_caller_create_ptr() :                                      \
        (__itt_caller) nullptr /**/
#define HPX_INTERNAL_ITT_STACK_ENTER(ctx)                                      \
    if (use_ittnotify_api && __itt_stack_callee_enter_ptr)                     \
        __itt_stack_callee_enter_ptr(ctx);                                     \
    /**/
#define HPX_INTERNAL_ITT_STACK_LEAVE(ctx)                                      \
    if (use_ittnotify_api && __itt_stack_callee_leave_ptr)                     \
        __itt_stack_callee_leave_ptr(ctx);                                     \
    /**/
#define HPX_INTERNAL_ITT_STACK_DESTROY(ctx)                                    \
    if (use_ittnotify_api && __itt_stack_caller_destroy_ptr &&                 \
        (ctx) != (__itt_caller) nullptr)                                       \
        __itt_stack_caller_destroy_ptr(ctx);                                   \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_FRAME_BEGIN(domain, id)                               \
    if (use_ittnotify_api && __itt_frame_begin_v3_ptr)                         \
        __itt_frame_begin_v3_ptr(domain, id);                                  \
    /**/
#define HPX_INTERNAL_ITT_FRAME_END(domain, id)                                 \
    if (use_ittnotify_api && __itt_frame_end_v3_ptr)                           \
        __itt_frame_end_v3_ptr(domain, id);                                    \
    /**/

#define HPX_INTERNAL_ITT_MARKER(domain, name)                                  \
    if (use_ittnotify_api && __itt_marker_ptr)                                 \
        __itt_marker_ptr(domain, __itt_null, name, __itt_marker_scope_global); \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_MARK_CREATE(name)                                     \
    (use_ittnotify_api && __itt_mark_create_ptr) ?                             \
        __itt_mark_create_ptr(name) :                                          \
        0 /**/
#define HPX_INTERNAL_ITT_MARK_OFF(mark)                                        \
    if (use_ittnotify_api && __itt_mark_off_ptr)                               \
        __itt_mark_off_ptr(mark);                                              \
    /**/
#define HPX_INTERNAL_ITT_MARK(mark, parameter)                                 \
    if (use_ittnotify_api && __itt_mark_ptr)                                   \
        __itt_mark_ptr(mark, parameter);                                       \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_THREAD_SET_NAME(name)                                 \
    if (use_ittnotify_api && __itt_thread_set_name_ptr)                        \
        __itt_thread_set_name_ptr(name);                                       \
    /**/
#define HPX_INTERNAL_ITT_THREAD_IGNORE()                                       \
    if (use_ittnotify_api && __itt_thread_ignore_ptr)                          \
        __itt_thread_ignore_ptr();                                             \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_TASK_BEGIN(domain, name)                              \
    if (use_ittnotify_api && __itt_task_begin_ptr)                             \
        __itt_task_begin_ptr(domain, __itt_null, __itt_null, name);            \
    /**/
#define HPX_INTERNAL_ITT_TASK_BEGIN_ID(domain, id, name)                       \
    if (use_ittnotify_api && __itt_task_begin_ptr)                             \
        __itt_task_begin_ptr(domain, id, __itt_null, name);                    \
    /**/
#define HPX_INTERNAL_ITT_TASK_END(domain)                                      \
    if (use_ittnotify_api && __itt_task_end_ptr)                               \
        __itt_task_end_ptr(domain);                                            \
    /**/

#define HPX_INTERNAL_ITT_TASK_BEGIN_OVERLAPPED(domain, id, name)               \
    if (use_ittnotify_api && __itt_task_begin_overlapped_ptr)                  \
        __itt_task_begin_overlapped_ptr(domain, id, __itt_null, name);         \
    /**/
#define HPX_INTERNAL_ITT_TASK_END_OVERLAPPED(domain, id)                       \
    if (use_ittnotify_api && __itt_task_end_overlapped_ptr)                    \
        __itt_task_end_overlapped_ptr(domain, id);                             \
    /**/

#define HPX_INTERNAL_ITT_DOMAIN_CREATE(name)                                   \
    (use_ittnotify_api && __itt_domain_create_ptr) ?                           \
        __itt_domain_create_ptr(name) :                                        \
        nullptr /**/

#define HPX_INTERNAL_ITT_STRING_HANDLE_CREATE(name)                            \
    (use_ittnotify_api && __itt_string_handle_create_ptr) ?                    \
        __itt_string_handle_create_ptr(name) :                                 \
        nullptr /**/

#define HPX_INTERNAL_ITT_MAKE_ID(addr, extra)                                  \
    use_ittnotify_api ? __itt_id_make(addr, extra) : __itt_null /**/

#define HPX_INTERNAL_ITT_ID_CREATE(domain, id)                                 \
    if (use_ittnotify_api && __itt_id_create_ptr)                              \
        __itt_id_create_ptr(domain, id);                                       \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_HEAP_FUNCTION_CREATE(name, domain)                    \
    (use_ittnotify_api && __itt_heap_function_create_ptr) ?                    \
        __itt_heap_function_create_ptr(name, domain) :                         \
        nullptr /**/

#define HPX_INTERNAL_HEAP_ALLOCATE_BEGIN(f, size, init)                        \
    if (use_ittnotify_api && __itt_heap_allocate_begin_ptr)                    \
        __itt_heap_allocate_begin_ptr(f, size, init);                          \
    /**/
#define HPX_INTERNAL_HEAP_ALLOCATE_END(f, addr, size, init)                    \
    if (use_ittnotify_api && __itt_heap_allocate_end_ptr)                      \
        __itt_heap_allocate_end_ptr(f, addr, size, init);                      \
    /**/

#define HPX_INTERNAL_HEAP_FREE_BEGIN(f, addr)                                  \
    if (use_ittnotify_api && __itt_heap_free_begin_ptr)                        \
        __itt_heap_free_begin_ptr(f, addr);                                    \
    /**/
#define HPX_INTERNAL_HEAP_FREE_END(f, addr)                                    \
    if (use_ittnotify_api && __itt_heap_free_end_ptr)                          \
        __itt_heap_free_end_ptr(f, addr);                                      \
    /**/

#define HPX_INTERNAL_HEAP_REALLOCATE_BEGIN(f, addr, size, init)                \
    if (use_ittnotify_api && __itt_heap_reallocate_begin_ptr)                  \
        __itt_heap_reallocate_begin_ptr(f, addr, size, init);                  \
    /**/
#define HPX_INTERNAL_HEAP_REALLOCATE_END(f, addr, new_addr, size, init)        \
    if (use_ittnotify_api && __itt_heap_reallocate_end_ptr)                    \
        __itt_heap_reallocate_end_ptr(f, addr, new_addr, size, init);          \
    /**/

#define HPX_INTERNAL_INTERNAL_ACCESS_BEGIN()                                   \
    if (use_ittnotify_api && __itt_heap_internal_access_begin_ptr)             \
        __itt_heap_internal_access_begin_ptr();                                \
    /**/
#define HPX_INTERNAL_INTERNAL_ACCESS_END()                                     \
    if (use_ittnotify_api && __itt_heap_internal_access_end_ptr)               \
        __itt_heap_internal_access_end_ptr();                                  \
    /**/

///////////////////////////////////////////////////////////////////////////////
#if defined(__itt_counter_create_typed_ptr) &&                                 \
    defined(__itt_counter_set_value_ptr)
#define HPX_INTERNAL_COUNTER_CREATE(name, domain)                              \
    (use_ittnotify_api && __itt_counter_create_ptr) ?                          \
        __itt_counter_create_ptr(name, domain) :                               \
        (__itt_counter) nullptr /**/
#define HPX_INTERNAL_COUNTER_CREATE_TYPED(name, domain, type)                  \
    (use_ittnotify_api && __itt_counter_create_typed_ptr) ?                    \
        __itt_counter_create_typed_ptr(name, domain, type) :                   \
        (__itt_counter) nullptr /**/
#define HPX_INTERNAL_COUNTER_SET_VALUE(id, value_ptr)                          \
    if (use_ittnotify_api && __itt_counter_set_value_ptr)                      \
        __itt_counter_set_value_ptr(id, value_ptr);                            \
    /**/
#define HPX_INTERNAL_COUNTER_DESTROY(id)                                       \
    if (use_ittnotify_api && __itt_counter_destroy_ptr)                        \
        __itt_counter_destroy_ptr(id);                                         \
    /**/
#else
// older itt-notify implementations don't support the typed counter API
#define HPX_INTERNAL_COUNTER_CREATE(name, domain) (__itt_counter) nullptr /**/
#define HPX_INTERNAL_COUNTER_CREATE_TYPED(name, domain, type)                  \
    (__itt_counter) nullptr                           /**/
#define HPX_INTERNAL_COUNTER_SET_VALUE(id, value_ptr) /**/
#define HPX_INTERNAL_COUNTER_DESTROY(id)              /**/
#endif

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_EVENT_CREATE(name, len)                                   \
    (use_ittnotify_api && __itt_event_create_ptr) ?                            \
        __itt_event_create_ptr(name, len) :                                    \
        0;                                                                     \
    /**/
#define HPX_INTERNAL_EVENT_START(e)                                            \
    (use_ittnotify_api && __itt_event_start_ptr) ? __itt_event_start_ptr(e) :  \
                                                   0 /**/
#define HPX_INTERNAL_EVENT_END(e)                                              \
    (use_ittnotify_api && __itt_event_end_ptr) ? __itt_event_end_ptr(e) : 0 /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_METADATA_ADD(domain, id, key, type, count, data)          \
    if (use_ittnotify_api && __itt_metadata_add_ptr)                           \
    __itt_metadata_add_ptr(domain, id, key, type, count, data) /**/
#define HPX_INTERNAL_METADATA_STR_ADD(domain, id, key, data)                   \
    if (use_ittnotify_api && __itt_metadata_str_add_ptr)                       \
    __itt_metadata_str_add_ptr(domain, id, key, data, 0) /**/

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_MSVC) || defined(__BORLANDC__) ||                              \
    (defined(__MWERKS__) && defined(_WIN32) && (__MWERKS__ >= 0x3000)) ||      \
    (defined(__ICL) && defined(_MSC_EXTENSIONS) && (HPX_MSVC >= 1200))

#pragma comment(lib, "libittnotify.lib")
#endif

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_SYNC_PREPARE(obj)                                     \
    HPX_INTERNAL_ITT_SYNC(sync_prepare, obj)
#define HPX_INTERNAL_ITT_SYNC_CANCEL(obj)                                      \
    HPX_INTERNAL_ITT_SYNC(sync_cancel, obj)
#define HPX_INTERNAL_ITT_SYNC_ACQUIRED(obj)                                    \
    HPX_INTERNAL_ITT_SYNC(sync_acquired, obj)
#define HPX_INTERNAL_ITT_SYNC_RELEASING(obj)                                   \
    HPX_INTERNAL_ITT_SYNC(sync_releasing, obj)
#define HPX_INTERNAL_ITT_SYNC_RELEASED(obj)                                    \
    ((void) 0)    //HPX_INTERNAL_ITT_SYNC(sync_released, obj)
#define HPX_INTERNAL_ITT_SYNC_DESTROY(obj)                                     \
    HPX_INTERNAL_ITT_SYNC(sync_destroy, obj)

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::itt {

    stack_context::stack_context()
    {
        HPX_ITT_STACK_CREATE(itt_context_);
    }

    stack_context::~stack_context()
    {
        if (itt_context_)
        {
            HPX_ITT_STACK_DESTROY(itt_context_);
        }
    }

    caller_context::caller_context(stack_context& ctx, bool enter)
      : ctx_(ctx)
      , enter_(enter)
    {
        if (enter && ctx.itt_context_)
        {
            HPX_ITT_STACK_CALLEE_ENTER(ctx_.itt_context_);
        }
    }

    caller_context::~caller_context()
    {
        if (enter_ && ctx_.itt_context_)
        {
            HPX_ITT_STACK_CALLEE_LEAVE(ctx_.itt_context_);
        }
    }

    domain::domain(char const* name) noexcept
      : domain_(HPX_ITT_DOMAIN_CREATE(name))
    {
        if (domain_ != nullptr)
        {
            domain_->flags = 1;
        }
    }

    namespace {

        thread_local std::unique_ptr<___itt_domain> thread_domain_;
    }

    thread_domain::thread_domain() noexcept
      : domain()
    {
        if (!thread_domain_)
        {
            thread_domain_.reset(
                HPX_ITT_DOMAIN_CREATE(detail::thread_name().c_str()));
        }

        domain_ = thread_domain_.get();

        if (domain_ != nullptr)
        {
            domain_->flags = 1;
        }
    }

    id::id(domain const& domain, void* addr, unsigned long const extra) noexcept
      : domain_(domain.domain_)
    {
        if (use_ittnotify_api)
        {
            id_ = HPX_ITT_MAKE_ID(addr, extra);
            HPX_ITT_ID_CREATE(domain_, id_);
        }
    }

    id::~id()
    {
        if (use_ittnotify_api)
        {
            HPX_ITT_ID_DESTROY(domain_, id_);
        }
    }

    frame_context::frame_context(domain const& domain, id* ident) noexcept
      : domain_(domain)
      , ident_(ident)
    {
        HPX_ITT_FRAME_BEGIN(domain_.domain_, ident_ ? ident_->id_ : nullptr);
    }

    frame_context::~frame_context()
    {
        HPX_ITT_FRAME_END(domain_.domain_, ident_ ? ident_->id_ : nullptr);
    }

    undo_frame_context::undo_frame_context(frame_context& frame) noexcept
      : frame_(frame)
    {
        HPX_ITT_FRAME_END(frame_.domain_.domain_, nullptr);
    }

    undo_frame_context::~undo_frame_context()
    {
        HPX_ITT_FRAME_BEGIN(frame_.domain_.domain_, nullptr);
    }

    mark_context::mark_context(char const* name) noexcept
      : itt_mark_(0)
      , name_(name)
    {
        HPX_ITT_MARK_CREATE(itt_mark_, name);
    }

    mark_context::~mark_context()
    {
        HPX_ITT_MARK_OFF(itt_mark_);
    }

    undo_mark_context::undo_mark_context(mark_context& mark) noexcept
      : mark_(mark)
    {
        HPX_ITT_MARK_OFF(mark_.itt_mark_);
    }
    undo_mark_context::~undo_mark_context()
    {
        HPX_ITT_MARK_CREATE(mark_.itt_mark_, mark_.name_);
    }

    string_handle::string_handle(char const* s) noexcept
      : handle_(s == nullptr ? nullptr : HPX_ITT_STRING_HANDLE_CREATE(s))
    {
    }

    task::task(domain const& domain, string_handle name) noexcept
      : domain_(domain)
      , sh_(HPX_MOVE(name))
    {
        if (use_ittnotify_api)
        {
            // id derives from `this`, not the shared name handle, to avoid
            // collisions between concurrent same-name tasks.
            id_ = HPX_ITT_MAKE_ID(
                domain_.domain_, reinterpret_cast<std::size_t>(this));

            HPX_ITT_ID_CREATE(domain_.domain_, id_);
            HPX_ITT_TASK_BEGIN_ID(domain_.domain_, id_, sh_.handle_);
        }
    }

    task::task(domain const& domain, string_handle name,
        std::uint64_t const metadata) noexcept
      : domain_(domain)
      , sh_(HPX_MOVE(name))
    {
        if (use_ittnotify_api)
        {
            id_ = HPX_ITT_MAKE_ID(
                domain_.domain_, reinterpret_cast<std::size_t>(this));

            HPX_ITT_ID_CREATE(domain_.domain_, id_);
            HPX_ITT_TASK_BEGIN_ID(domain_.domain_, id_, sh_.handle_);
            add_metadata(string_handle(sh_.handle_), metadata);
        }
    }

    task::~task()
    {
        if (use_ittnotify_api)
        {
            HPX_ITT_TASK_END(domain_.domain_);
            HPX_ITT_ID_DESTROY(domain_.domain_, id_);
        }
    }

    overlapped_task::overlapped_task(
        domain const& d, string_handle name) noexcept
      : domain_(d)
      , sh_(HPX_MOVE(name))
    {
        if (use_ittnotify_api)
        {
            // The extra value comes from `this` rather than the string
            // handle: callers reuse cached name handles across concurrent
            // instances, so a handle-derived id would collide.
            id_ = HPX_ITT_MAKE_ID(
                domain_.domain_, reinterpret_cast<std::size_t>(this));
            // An overlapped task's id must be created before use and cannot
            // be __itt_null; the destroy in the dtor lets a later region at
            // the same stack address reuse the id value safely.
            HPX_ITT_ID_CREATE(domain_.domain_, id_);
            HPX_ITT_TASK_BEGIN_OVERLAPPED(domain_.domain_, id_, sh_.handle_);
        }
    }

    overlapped_task::~overlapped_task()
    {
        if (use_ittnotify_api)
        {
            HPX_ITT_TASK_END_OVERLAPPED(domain_.domain_, id_);
            HPX_ITT_ID_DESTROY(domain_.domain_, id_);
        }
    }

    void emit_marker(domain const& d, string_handle const& name) noexcept
    {
        HPX_ITT_MARKER(d.domain_, name.handle_);
    }

    void task::add_metadata(
        string_handle const& name, std::uint64_t const val) const noexcept
    {
        HPX_ITT_METADATA_ADD(domain_.domain_, id_, name.handle_, val);
    }

    void task::add_metadata(
        string_handle const& name, double const val) const noexcept
    {
        HPX_ITT_METADATA_ADD(domain_.domain_, id_, name.handle_, val);
    }

    void task::add_metadata(
        string_handle const& name, char const* val) const noexcept
    {
        HPX_ITT_METADATA_ADD(domain_.domain_, id_, name.handle_, val);
    }

    void task::add_metadata(
        string_handle const& name, void const* val) const noexcept
    {
        HPX_ITT_METADATA_ADD(domain_.domain_, id_, name.handle_, val);
    }

    heap_function::heap_function(char const* name, char const* domain) noexcept
      : heap_function_(HPX_ITT_HEAP_FUNCTION_CREATE(name, domain))
    {
    }

    heap_internal_access::heap_internal_access() noexcept
    {
        HPX_ITT_HEAP_INTERNAL_ACCESS_BEGIN();
    }

    heap_internal_access::~heap_internal_access()
    {
        HPX_ITT_HEAP_INTERNAL_ACCESS_END();
    }

    heap_free::heap_free(heap_function& heap_function, void* addr) noexcept
      : heap_function_(heap_function)
      , addr_(addr)
    {
        HPX_ITT_HEAP_FREE_BEGIN(heap_function_.heap_function_, addr_);
    }

    heap_free::~heap_free()
    {
        HPX_ITT_HEAP_FREE_END(heap_function_.heap_function_, addr_);
    }

    counter::counter(char const* name, char const* domain) noexcept
      : id_(HPX_ITT_COUNTER_CREATE(name, domain))
    {
    }

    counter::counter(
        char const* name, char const* domain, int const type) noexcept
      : id_(HPX_ITT_COUNTER_CREATE_TYPED(name, domain, type))
    {
    }

    counter::~counter()
    {
        if (id_)
        {
            HPX_ITT_COUNTER_DESTROY(id_);
        }
    }

    void counter::set_value_void(void* data) const
    {
        if (use_ittnotify_api && id_)
        {
            HPX_ITT_COUNTER_SET_VALUE(id_, data);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void heap_allocate::init() const
    {
        HPX_ITT_HEAP_ALLOCATE_BEGIN(
            heap_function_.heap_function_, size_, init_);
    }

    heap_allocate::~heap_allocate()
    {
        HPX_ITT_HEAP_ALLOCATE_END(
            heap_function_.heap_function_, addr_, size_, init_);
    }
}    // namespace hpx::util::itt

///////////////////////////////////////////////////////////////////////////////
void itt_pause() noexcept
{
    HPX_INTERNAL_ITT_PAUSE()
}

void itt_resume() noexcept
{
    HPX_INTERNAL_ITT_RESUME()
}

void itt_detach() noexcept
{
    HPX_INTERNAL_ITT_DETACH()
}

///////////////////////////////////////////////////////////////////////////////
void itt_sync_create(
    void* addr, char const* objtype, char const* objname) noexcept
{
    HPX_INTERNAL_ITT_SYNC_CREATE(addr, objtype, objname)
}

void itt_sync_rename(void* addr, char const* objname) noexcept
{
    HPX_INTERNAL_ITT_SYNC_RENAME(addr, objname)
}

void itt_sync_prepare(void* addr) noexcept
{
    HPX_INTERNAL_ITT_SYNC_PREPARE(addr)
}

void itt_sync_acquired(void* addr) noexcept
{
    HPX_INTERNAL_ITT_SYNC_ACQUIRED(addr)
}

void itt_sync_cancel(void* addr) noexcept
{
    HPX_INTERNAL_ITT_SYNC_CANCEL(addr)
}

void itt_sync_releasing(void* addr) noexcept
{
    HPX_INTERNAL_ITT_SYNC_RELEASING(addr)
}

void itt_sync_released(void* addr) noexcept
{
    HPX_INTERNAL_ITT_SYNC_RELEASED(addr);
}

void itt_sync_destroy(void* addr) noexcept {HPX_INTERNAL_ITT_SYNC_DESTROY(addr)}

///////////////////////////////////////////////////////////////////////////////
__itt_caller itt_stack_create() noexcept
{
    return HPX_INTERNAL_ITT_STACK_CREATE();
}

void itt_stack_enter(__itt_caller const ctx) noexcept
{
    HPX_INTERNAL_ITT_STACK_ENTER(ctx)
}

void itt_stack_leave(__itt_caller const ctx) noexcept
{
    HPX_INTERNAL_ITT_STACK_LEAVE(ctx)
}

void itt_stack_destroy(__itt_caller const ctx) noexcept
{
    HPX_INTERNAL_ITT_STACK_DESTROY(ctx)
}

///////////////////////////////////////////////////////////////////////////////
void itt_frame_begin(___itt_domain const* domain, ___itt_id* id) noexcept
{
    HPX_INTERNAL_ITT_FRAME_BEGIN(domain, id)
}

void itt_frame_end(___itt_domain const* domain, ___itt_id* id) noexcept
{
    HPX_INTERNAL_ITT_FRAME_END(domain, id)
}

void itt_marker(
    ___itt_domain const* domain, ___itt_string_handle* name) noexcept
{
    HPX_INTERNAL_ITT_MARKER(domain, name)
}

///////////////////////////////////////////////////////////////////////////////
int itt_mark_create(char const* name) noexcept
{
    return HPX_INTERNAL_ITT_MARK_CREATE(name);
}

void itt_mark_off(int const mark) noexcept
{
    HPX_INTERNAL_ITT_MARK_OFF(mark)
}

void itt_mark(int const mark, char const* par) noexcept
{
    HPX_INTERNAL_ITT_MARK(mark, par)
}

///////////////////////////////////////////////////////////////////////////////
void itt_thread_set_name(char const* name) noexcept
{
    HPX_INTERNAL_ITT_THREAD_SET_NAME(name)
}

void itt_thread_ignore() noexcept
{
    HPX_INTERNAL_ITT_THREAD_IGNORE()
}

///////////////////////////////////////////////////////////////////////////////
void itt_task_begin(
    ___itt_domain const* domain, ___itt_string_handle* name) noexcept
{
    HPX_INTERNAL_ITT_TASK_BEGIN(domain, name)
}

void itt_task_begin(___itt_domain const* domain, ___itt_id const* id,
    ___itt_string_handle* name) noexcept
{
    HPX_INTERNAL_ITT_TASK_BEGIN_ID(domain, *id, name)
}

void itt_task_end(___itt_domain const* domain) noexcept
{
    HPX_INTERNAL_ITT_TASK_END(domain)
}

void itt_task_begin_overlapped(___itt_domain const* domain, ___itt_id const* id,
    ___itt_string_handle* name) noexcept
{
    HPX_INTERNAL_ITT_TASK_BEGIN_OVERLAPPED(domain, *id, name)
}

void itt_task_end_overlapped(
    ___itt_domain const* domain, ___itt_id const* id) noexcept {
    HPX_INTERNAL_ITT_TASK_END_OVERLAPPED(domain, *id)}

___itt_domain* itt_domain_create(char const* name) noexcept
{
    return HPX_INTERNAL_ITT_DOMAIN_CREATE(name);
}

___itt_string_handle* itt_string_handle_create(char const* name) noexcept
{
    return HPX_INTERNAL_ITT_STRING_HANDLE_CREATE(name);
}

___itt_id* itt_make_id(void* addr, std::size_t const extra)
{
    return new ___itt_id(HPX_INTERNAL_ITT_MAKE_ID(addr, extra));
}

void itt_id_create(___itt_domain const* domain, ___itt_id const* id) noexcept
{
    HPX_INTERNAL_ITT_ID_CREATE(domain, *id)
}

void itt_id_destroy(___itt_domain const* domain, ___itt_id const* id) noexcept
{
    // End the id instance's lifetime with the collector before releasing the
    // heap slot. Skipping __itt_id_destroy would let a later task reuse the
    // same id value while the collector still considers the old one live.
    if (use_ittnotify_api && __itt_id_destroy_ptr && id != nullptr)
        __itt_id_destroy_ptr(domain, *id);
    delete id;
}

///////////////////////////////////////////////////////////////////////////////
__itt_heap_function itt_heap_function_create(
    char const* name, char const* domain) noexcept
{
    return HPX_INTERNAL_ITT_HEAP_FUNCTION_CREATE(name, domain);
}

void itt_heap_allocate_begin(__itt_heap_function const f,
    std::size_t const size, int const init) noexcept
{
    HPX_INTERNAL_HEAP_ALLOCATE_BEGIN(f, size, init)
}

void itt_heap_allocate_end(__itt_heap_function const f, void** addr,
    std::size_t const size, int const init) noexcept
{
    HPX_INTERNAL_HEAP_ALLOCATE_END(f, addr, size, init)
}

void itt_heap_free_begin(__itt_heap_function const f, void* addr) noexcept
{
    HPX_INTERNAL_HEAP_FREE_BEGIN(f, addr)
}

void itt_heap_free_end(__itt_heap_function const f, void* addr) noexcept
{
    HPX_INTERNAL_HEAP_FREE_END(f, addr)
}

void itt_heap_reallocate_begin(__itt_heap_function const f, void* addr,
    std::size_t const new_size, int const init) noexcept
{
    HPX_INTERNAL_HEAP_REALLOCATE_BEGIN(f, addr, new_size, init)
}

void itt_heap_reallocate_end(__itt_heap_function const f, void* addr,
    void** new_addr, std::size_t const new_size, int const init) noexcept
{
    HPX_INTERNAL_HEAP_REALLOCATE_END(f, addr, new_addr, new_size, init)
}

void itt_heap_internal_access_begin() noexcept
{
    HPX_INTERNAL_INTERNAL_ACCESS_BEGIN()
}

void itt_heap_internal_access_end() noexcept {
    HPX_INTERNAL_INTERNAL_ACCESS_END()}

///////////////////////////////////////////////////////////////////////////////
__itt_counter itt_counter_create(char const* name, char const* domain) noexcept
{
    return HPX_INTERNAL_COUNTER_CREATE(name, domain);
}

__itt_counter itt_counter_create_typed(
    char const* name, char const* domain, int type) noexcept
{
    return HPX_INTERNAL_COUNTER_CREATE_TYPED(
        name, domain, static_cast<__itt_metadata_type>(type));
}

void itt_counter_destroy(__itt_counter const id) noexcept
{
    HPX_INTERNAL_COUNTER_DESTROY(id)
}

void itt_counter_set_value(__itt_counter const id, void* value_ptr) noexcept {
    HPX_INTERNAL_COUNTER_SET_VALUE(id, value_ptr)}

///////////////////////////////////////////////////////////////////////////////
__itt_event itt_event_create(char const* name, int const namelen) noexcept
{
    return HPX_INTERNAL_EVENT_CREATE(name, namelen)
}

int itt_event_start(__itt_event const evnt) noexcept
{
    return HPX_INTERNAL_EVENT_START(evnt);
}

int itt_event_end(__itt_event const evnt) noexcept
{
    return HPX_INTERNAL_EVENT_END(evnt);
}

///////////////////////////////////////////////////////////////////////////////
void itt_metadata_add(___itt_domain const* domain, ___itt_id const* id,
    ___itt_string_handle* key, std::uint64_t const& data) noexcept
{
    HPX_INTERNAL_METADATA_ADD(domain, *id, key, __itt_metadata_u64, 1,
        const_cast<std::uint64_t*>(&data));
}

void itt_metadata_add(___itt_domain const* domain, ___itt_id const* id,
    ___itt_string_handle* key, double const& data) noexcept
{
    HPX_INTERNAL_METADATA_ADD(
        domain, *id, key, __itt_metadata_double, 1, const_cast<double*>(&data));
}

void itt_metadata_add(___itt_domain const* domain, ___itt_id const* id,
    ___itt_string_handle* key, char const* data) noexcept
{
    HPX_INTERNAL_METADATA_STR_ADD(domain, *id, key, data);
}

void itt_metadata_add(___itt_domain const* domain, ___itt_id const* id,
    ___itt_string_handle* key, void const* data) noexcept
{
    HPX_INTERNAL_METADATA_ADD(
        domain, *id, key, __itt_metadata_unknown, 1, const_cast<void*>(data));
}

#endif    // HPX_HAVE_ITTNOTIFY
