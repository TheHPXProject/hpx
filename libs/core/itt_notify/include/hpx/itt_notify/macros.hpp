//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config/export_definitions.hpp>

#if !defined(HPX_HAVE_CXX_MODULES) || defined(HPX_CORE_EXPORTS) ||             \
    (defined(HPX_COMPILE_BMI) && defined(HPX_COMPILE_CORE_WITH_MODULES))
#include <hpx/itt_notify/api.hpp>
#endif

///////////////////////////////////////////////////////////////////////////////
#define HPX_ITT_PAUSE() itt_pause()
#define HPX_ITT_RESUME() itt_resume()
#define HPX_ITT_DETACH() itt_detach()

///////////////////////////////////////////////////////////////////////////////
#define HPX_ITT_SYNC_CREATE(obj, type, name) itt_sync_create(obj, type, name)
#define HPX_ITT_SYNC_RENAME(obj, name) itt_sync_rename(obj, name)
#define HPX_ITT_SYNC_PREPARE(obj) itt_sync_prepare(obj)
#define HPX_ITT_SYNC_CANCEL(obj) itt_sync_cancel(obj)
#define HPX_ITT_SYNC_ACQUIRED(obj) itt_sync_acquired(obj)
#define HPX_ITT_SYNC_RELEASING(obj) itt_sync_releasing(obj)
#define HPX_ITT_SYNC_RELEASED(obj) itt_sync_released(obj)
#define HPX_ITT_SYNC_DESTROY(obj) itt_sync_destroy(obj)

#define HPX_ITT_STACK_CREATE(ctx) ctx = itt_stack_create()
#define HPX_ITT_STACK_CALLEE_ENTER(ctx) itt_stack_enter(ctx)
#define HPX_ITT_STACK_CALLEE_LEAVE(ctx) itt_stack_leave(ctx)
#define HPX_ITT_STACK_DESTROY(ctx) itt_stack_destroy(ctx)

#define HPX_ITT_FRAME_BEGIN(frame, id) itt_frame_begin(frame, id)
#define HPX_ITT_FRAME_END(frame, id) itt_frame_end(frame, id)

#define HPX_ITT_MARKER(domain, name) itt_marker(domain, name)

#define HPX_ITT_MARK_CREATE(mark, name) mark = itt_mark_create(name)
#define HPX_ITT_MARK_OFF(mark) itt_mark_off(mark)
#define HPX_ITT_MARK(mark, parameter) itt_mark(mark, parameter)

#define HPX_ITT_THREAD_SET_NAME(name) itt_thread_set_name(name)
#define HPX_ITT_THREAD_IGNORE() itt_thread_ignore()

#define HPX_ITT_TASK_BEGIN(domain, name) itt_task_begin(domain, name)
#define HPX_ITT_TASK_BEGIN_ID(domain, id, name) itt_task_begin(domain, id, name)
#define HPX_ITT_TASK_END(domain) itt_task_end(domain)

#define HPX_ITT_DOMAIN_CREATE(name) itt_domain_create(name)
#define HPX_ITT_STRING_HANDLE_CREATE(name) itt_string_handle_create(name)

#define HPX_ITT_MAKE_ID(addr, extra) itt_make_id(addr, extra)
#define HPX_ITT_ID_CREATE(domain, id) itt_id_create(domain, id)
#define HPX_ITT_ID_DESTROY(id) itt_id_destroy(id)

#define HPX_ITT_HEAP_FUNCTION_CREATE(name, domain)                             \
    itt_heap_function_create(name, domain) /**/
#define HPX_ITT_HEAP_ALLOCATE_BEGIN(f, size, initialized)                      \
    itt_heap_allocate_begin(f, size, initialized) /**/
#define HPX_ITT_HEAP_ALLOCATE_END(f, addr, size, initialized)                  \
    itt_heap_allocate_end(f, addr, size, initialized) /**/
#define HPX_ITT_HEAP_FREE_BEGIN(f, addr) itt_heap_free_begin(f, addr)
#define HPX_ITT_HEAP_FREE_END(f, addr) itt_heap_free_end(f, addr)
#define HPX_ITT_HEAP_REALLOCATE_BEGIN(f, addr, new_size, initialized)          \
    itt_heap_reallocate_begin(f, addr, new_size, initialized) /**/
#define HPX_ITT_HEAP_REALLOCATE_END(f, addr, new_addr, new_size, initialized)  \
    itt_heap_reallocate_end(f, addr, new_addr, new_size, initialized) /**/
#define HPX_ITT_HEAP_INTERNAL_ACCESS_BEGIN() itt_heap_internal_access_begin()
#define HPX_ITT_HEAP_INTERNAL_ACCESS_END() itt_heap_internal_access_end()

#define HPX_ITT_COUNTER_CREATE(name, domain)                                   \
    itt_counter_create(name, domain) /**/
#define HPX_ITT_COUNTER_CREATE_TYPED(name, domain, type)                       \
    itt_counter_create_typed(name, domain, type) /**/
#define HPX_ITT_COUNTER_SET_VALUE(id, value_ptr)                               \
    itt_counter_set_value(id, value_ptr) /**/
#define HPX_ITT_COUNTER_DESTROY(id) itt_counter_destroy(id)

#define HPX_ITT_METADATA_ADD(domain, id, key, data)                            \
    itt_metadata_add(domain, id, key, data) /**/
