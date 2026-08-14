//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/preprocessor/cat.hpp>

#if defined(HPX_HAVE_TRACY) ||                                                 \
    (defined(HPX_HAVE_ITTNOTIFY) && HPX_HAVE_ITTNOTIFY != 0) ||                \
    defined(HPX_HAVE_APEX)
#define HPX_HAVE_TRACING
#endif

#if defined(HPX_HAVE_TRACY)
#define HPX_TRACING_MARK_EVENT(name)                                           \
    hpx::tracing::mark_event HPX_PP_CAT(hpx_trace_mark_, __LINE__)(name)
#define HPX_TRACING_ZONE(name, thread_num)                                     \
    hpx::tracing::loop_context HPX_PP_CAT(hpx_trace_lctx_, __LINE__);          \
    hpx::tracing::region HPX_PP_CAT(hpx_trace_rctx_, __LINE__)(                \
        HPX_PP_CAT(hpx_trace_lctx_, __LINE__),                                 \
        hpx::tracing::region_init_data{name, 0, false}, thread_num)
#define HPX_TRACING_PAUSE()
#define HPX_TRACING_RESUME()
#elif defined(HPX_HAVE_ITTNOTIFY) && HPX_HAVE_ITTNOTIFY != 0
#define HPX_TRACING_MARK_EVENT(name)                                           \
    static hpx::util::itt::event HPX_PP_CAT(hpx_trace_event_, __LINE__)(name); \
    hpx::util::itt::mark_event HPX_PP_CAT(hpx_trace_mark_, __LINE__)(          \
        HPX_PP_CAT(hpx_trace_event_, __LINE__))
#define HPX_TRACING_ZONE(name, thread_num)
#define HPX_TRACING_PAUSE() itt_pause()
#define HPX_TRACING_RESUME() itt_resume()
#elif defined(HPX_HAVE_APEX)
#define HPX_TRACING_MARK_EVENT(name)
#define HPX_TRACING_ZONE(name, thread_num)
#define HPX_TRACING_PAUSE()
#define HPX_TRACING_RESUME()
#else
#define HPX_TRACING_MARK_EVENT(name)
#define HPX_TRACING_ZONE(name, thread_num)
#define HPX_TRACING_PAUSE()
#define HPX_TRACING_RESUME()
#endif
