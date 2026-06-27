//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config/defines.hpp>

#include <hpx/config/asio.hpp>

#include <hpx/config/attributes.hpp>
#include <hpx/config/auto_vectorization.hpp>
#include <hpx/config/branch_hints.hpp>
#include <hpx/config/cache_line_size.hpp>
#include <hpx/config/compiler_fence.hpp>
#include <hpx/config/compiler_specific.hpp>
#include <hpx/config/constexpr.hpp>
#include <hpx/config/debug.hpp>
#include <hpx/config/deprecation.hpp>
#include <hpx/config/emulate_deleted.hpp>
#include <hpx/config/endian.hpp>
#include <hpx/config/export_definitions.hpp>
#include <hpx/config/forceinline.hpp>
#include <hpx/config/forward.hpp>
#include <hpx/config/manual_profiling.hpp>
#include <hpx/config/modules_enabled.hpp>
#include <hpx/config/move.hpp>
#include <hpx/config/threads_stack.hpp>
#include <hpx/config/version.hpp>
#include <hpx/config/weak_symbol.hpp>

#include <hpx/modules/preprocessor.hpp>

#include <cstddef>

// clang-format off

///////////////////////////////////////////////////////////////////////////////
/// This is the default ip/port number used by the parcel subsystem
#define HPX_INITIAL_IP_PORT         7910
#define HPX_CONNECTING_IP_PORT      7909
#define HPX_INITIAL_IP_ADDRESS      "127.0.0.1"

///////////////////////////////////////////////////////////////////////////////
/// This defines the maximum number of possible runtime instances in one
/// executable
#if !defined(HPX_RUNTIME_INSTANCE_LIMIT)
#  define HPX_RUNTIME_INSTANCE_LIMIT 1
#endif

///////////////////////////////////////////////////////////////////////////////
/// This defines the type of the parcelport to be used during application
/// bootstrap. This value can be changed at runtime by the configuration
/// parameter:
///
///   hpx.parcel.bootstrap = ...
///
/// (or by setting the corresponding environment variable HPX_PARCEL_BOOTSTRAP).
#if !defined(HPX_PARCEL_BOOTSTRAP)
#  define HPX_PARCEL_BOOTSTRAP "tcp"
#endif

///////////////////////////////////////////////////////////////////////////////
/// This defines the number of outgoing (parcel-) connections kept alive (to
/// all other localities). This value can be changed at runtime by setting
/// the configuration parameter:
///
///   hpx.parcel.max_connections = ...
///
/// (or by setting the corresponding environment variable
/// HPX_PARCEL_MAX_CONNECTIONS).
#if !defined(HPX_PARCEL_MAX_CONNECTIONS)
#  define HPX_PARCEL_MAX_CONNECTIONS 512
#endif

/// This defines the number of outgoing ipc (parcel-) connections kept alive
/// (to each of the other localities on the same node). This value can be changed
/// at runtime by setting the configuration parameter:
///
///   hpx.parcel.ipc.data_buffer_cache_size = ...
///
/// (or by setting the corresponding environment variable
/// HPX_PARCEL_IPC_DATA_BUFFER_CACHE_SIZE).
#if !defined(HPX_PARCEL_IPC_DATA_BUFFER_CACHE_SIZE)
#  define HPX_PARCEL_IPC_DATA_BUFFER_CACHE_SIZE 512
#endif

/// This defines the number of MPI requests in flight
/// This value can be changed at runtime by setting the configuration parameter:
///
///   hpx.parcel.mpi.max_requests = ...
///
/// (or by setting the corresponding environment variable
/// HPX_PARCEL_MPI_MAX_REQUESTS).
#if !defined(HPX_PARCEL_MPI_MAX_REQUESTS)
#  define HPX_PARCEL_MPI_MAX_REQUESTS 2147483647
#endif

/// This defines the number of cores that perform background work for the MPI
/// parcelport
/// This value can be changed at runtime by setting the configuration parameter:
///
///   hpx.parcel.mpi.background_threads = ...
///
/// (or by setting the corresponding environment variable
/// HPX_HAVE_PARCELPORT_MPI_BACKGROUND_THREADS).
#if !defined(HPX_HAVE_PARCELPORT_MPI_BACKGROUND_THREADS)
#  define HPX_HAVE_PARCELPORT_MPI_BACKGROUND_THREADS std::size_t(-1)
#endif

///////////////////////////////////////////////////////////////////////////////
/// This defines the number of outgoing (parcel-) connections kept alive (to
/// each of the other localities). This value can be changed at runtime by
/// setting the configuration parameter:
///
///   hpx.parcel.max_connections_per_locality = ...
///
/// (or by setting the corresponding environment variable
/// HPX_PARCEL_MAX_CONNECTIONS_PER_LOCALITY).
#if !defined(HPX_PARCEL_MAX_CONNECTIONS_PER_LOCALITY)
#  define HPX_PARCEL_MAX_CONNECTIONS_PER_LOCALITY 4
#endif

///////////////////////////////////////////////////////////////////////////////
/// This defines the maximally allowed message size for messages transferred
/// between localities. This value can be changed at runtime by
/// setting the configuration parameter:
///
///   hpx.parcel.max_message_size = ...
///
/// (or by setting the corresponding environment variable
/// HPX_PARCEL_MAX_MESSAGE_SIZE).
#if !defined(HPX_PARCEL_MAX_MESSAGE_SIZE)
#  define HPX_PARCEL_MAX_MESSAGE_SIZE 1000000000
#endif

/// This defines the maximally allowed outbound  message size for coalescing
/// messages transferred between localities. This value can be changed at
/// runtime by setting the configuration parameter:
///
///   hpx.parcel.max_outbound_message_size = ...
///
/// (or by setting the corresponding environment variable
/// HPX_PARCEL_MAX_OUTBOUND_MESSAGE_SIZE).
#if !defined(HPX_PARCEL_MAX_OUTBOUND_MESSAGE_SIZE)
#  define HPX_PARCEL_MAX_OUTBOUND_MESSAGE_SIZE 1000000
#endif

///////////////////////////////////////////////////////////////////////////////
// This defines the number of bytes of overhead it takes to serialize a
// parcel.
#if !defined(HPX_PARCEL_SERIALIZATION_OVERHEAD)
#   define HPX_PARCEL_SERIALIZATION_OVERHEAD 512
#endif

/// This defines the number of AGAS address translations kept in the local
/// cache. This is just the initial size which may be adjusted depending on the
/// load of the system (not implemented yet), etc. It must be a minimum of 3 for AGAS v3
/// bootstrapping.
///
/// This value can be changes at runtime by setting the configuration parameter:
///
///   hpx.agas.local_cache_size = ...
///
/// (or by setting the corresponding environment variable
/// HPX_AGAS_LOCAL_CACHE_SIZE)
#if !defined(HPX_AGAS_LOCAL_CACHE_SIZE)
#  define HPX_AGAS_LOCAL_CACHE_SIZE 4096
#endif

///////////////////////////////////////////////////////////////////////////////
#if !defined(HPX_INITIAL_AGAS_MAX_PENDING_REFCNT_REQUESTS)
#  define HPX_INITIAL_AGAS_MAX_PENDING_REFCNT_REQUESTS 4096
#endif

///////////////////////////////////////////////////////////////////////////////
/// This defines the initial global reference count associated with any created
/// object.
#if !defined(HPX_GLOBALCREDIT_INITIAL)
#  define HPX_GLOBALCREDIT_INITIAL 0x80000000ll     // 2 ^ 31, i.e. 2 ^ 0b11111
#endif

///////////////////////////////////////////////////////////////////////////////
/// This defines the default number of OS-threads created for the different
/// internal thread pools
#if !defined(HPX_NUM_IO_POOL_SIZE)
#  define HPX_NUM_IO_POOL_SIZE 2
#endif
#if !defined(HPX_NUM_PARCEL_POOL_SIZE)
#  define HPX_NUM_PARCEL_POOL_SIZE 2
#endif
#if !defined(HPX_NUM_TIMER_POOL_SIZE)
#  define HPX_NUM_TIMER_POOL_SIZE 2
#endif

///////////////////////////////////////////////////////////////////////////////
/// By default, enable minimal thread deadlock detection in debug builds only.
#if !defined(HPX_SPINLOCK_DEADLOCK_DETECTION_LIMIT)
#  define HPX_SPINLOCK_DEADLOCK_DETECTION_LIMIT 1073741823
#endif

///////////////////////////////////////////////////////////////////////////////
/// By default we do not maintain stack back-traces on suspension. This is a
/// pure debugging aid to be able to see in the debugger where a suspended
/// thread got stuck.
#if defined(HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION) && \
  !defined(HPX_HAVE_STACKTRACES)
#  error HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION requires HPX_HAVE_STACKTRACES to be defined!
#endif

/// By default we capture only 20 levels of stack back trace on suspension
#if !defined(HPX_HAVE_THREAD_BACKTRACE_DEPTH)
#  define HPX_HAVE_THREAD_BACKTRACE_DEPTH 20
#endif

///////////////////////////////////////////////////////////////////////////////
// This defines the maximum number of connect retries to the AGAS service
// allowing for some leeway during startup of the localities
#if !defined(HPX_MAX_NETWORK_RETRIES)
#  define HPX_MAX_NETWORK_RETRIES 1000
#endif

#if !defined(HPX_NETWORK_RETRIES_SLEEP)
#  define HPX_NETWORK_RETRIES_SLEEP 100
#endif

///////////////////////////////////////////////////////////////////////////////
//  Characters used
//    - to delimit several HPX ini paths
//    - used as file extensions for shared libraries
//    - used as path delimiters
#ifdef HPX_WINDOWS  // windows
#  define HPX_INI_PATH_DELIMITER            ";"
#  define HPX_SHARED_LIB_EXTENSION          ".dll"
#  define HPX_EXECUTABLE_EXTENSION          ".exe"
#  define HPX_PATH_DELIMITERS               "\\/"
#else                 // unix like
#  define HPX_INI_PATH_DELIMITER            ":"
#  define HPX_PATH_DELIMITERS               "/"
#  ifdef __APPLE__    // apple
#    define HPX_SHARED_LIB_EXTENSION        ".dylib"
#  elif defined(HPX_HAVE_STATIC_LINKING)
#    define HPX_SHARED_LIB_EXTENSION        ".a"
#  else  // linux & co
#    define HPX_SHARED_LIB_EXTENSION        ".so"
#  endif
#  define HPX_EXECUTABLE_EXTENSION          ""
#endif

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_DEBUG) && defined(HPX_HAVE_DEBUG_POSTFIX)
#  define HPX_DEBUG_POSTFIX_STR HPX_PP_STRINGIZE(HPX_HAVE_DEBUG_POSTFIX)
#else
#  define HPX_DEBUG_POSTFIX_STR ""
#endif

#if !defined(HPX_WINDOWS)
#  if defined(HPX_DEBUG) && defined(HPX_HAVE_DEBUG_POSTFIX)
#    define HPX_MAKE_DLL_STRING(n)                                             \
        "lib" + (n) + HPX_DEBUG_POSTFIX_STR + HPX_SHARED_LIB_EXTENSION
#  else
#    define HPX_MAKE_DLL_STRING(n)  "lib" + (n) + HPX_SHARED_LIB_EXTENSION
#  endif
#elif defined(HPX_DEBUG) && defined(HPX_HAVE_DEBUG_POSTFIX)
#  define HPX_MAKE_DLL_STRING(n)                                               \
      ((n) + HPX_DEBUG_POSTFIX_STR + HPX_SHARED_LIB_EXTENSION)
#else
#  define HPX_MAKE_DLL_STRING(n)   ((n) + HPX_SHARED_LIB_EXTENSION)
#endif

#if defined(HPX_DEBUG) && defined(HPX_HAVE_DEBUG_POSTFIX)
#  define HPX_MANGLE_NAME(n)     HPX_PP_CAT(n, HPX_HAVE_DEBUG_POSTFIX)
#  define HPX_MANGLE_STRING(n)   ((n) + HPX_DEBUG_POSTFIX_STR)
#else
#  define HPX_MANGLE_NAME(n)     n
#  define HPX_MANGLE_STRING(n)   n
#endif

///////////////////////////////////////////////////////////////////////////////
// Set defaults for components
#if !defined(HPX_COMPONENT_NAME_DEFAULT)
#  define HPX_COMPONENT_NAME_DEFAULT hpx
#endif

#if !defined(HPX_COMPONENT_NAME)
#  define HPX_COMPONENT_NAME HPX_COMPONENT_NAME_DEFAULT
#endif

#if !defined(HPX_COMPONENT_STRING)
#  define HPX_COMPONENT_STRING HPX_PP_STRINGIZE(HPX_COMPONENT_NAME)
#endif

#if !defined(HPX_PLUGIN_COMPONENT_PREFIX)
#  if defined(HPX_PLUGIN_NAME)
#    define HPX_PLUGIN_COMPONENT_PREFIX HPX_MANGLE_NAME(HPX_PLUGIN_NAME)
#  elif defined(HPX_COMPONENT_NAME)
#    define HPX_PLUGIN_COMPONENT_PREFIX HPX_MANGLE_NAME(HPX_COMPONENT_NAME)
#  endif
#endif

///////////////////////////////////////////////////////////////////////////////
#if !defined(HPX_PLUGIN_NAME_DEFAULT)
#  define HPX_PLUGIN_NAME_DEFAULT hpx
#endif

#if !defined(HPX_PLUGIN_NAME)
#  define HPX_PLUGIN_NAME HPX_PLUGIN_NAME_DEFAULT
#endif

#if !defined(HPX_PLUGIN_STRING)
#  define HPX_PLUGIN_STRING HPX_PP_STRINGIZE(HPX_PLUGIN_NAME)
#endif

#if !defined(HPX_PLUGIN_PLUGIN_PREFIX)
#  define HPX_PLUGIN_PLUGIN_PREFIX HPX_MANGLE_NAME(HPX_PLUGIN_NAME)
#endif

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_PREFIX_DEFAULT) && !defined(HPX_PREFIX)
#  define HPX_PREFIX HPX_PREFIX_DEFAULT
#endif

///////////////////////////////////////////////////////////////////////////////
// Count number of empty (no HPX thread available) thread manager loop executions
#if !defined(HPX_IDLE_LOOP_COUNT_MAX)
#  define HPX_IDLE_LOOP_COUNT_MAX 200000
#endif

///////////////////////////////////////////////////////////////////////////////
// Count number of busy thread manager loop executions before forcefully
// cleaning up terminated thread objects
#if !defined(HPX_BUSY_LOOP_COUNT_MAX)
#  define HPX_BUSY_LOOP_COUNT_MAX 2000
#endif

///////////////////////////////////////////////////////////////////////////////
// Maximum number of threads to create in the thread queue, except when there is
// no work to do, in which case the count will be increased in steps of
// HPX_THREAD_QUEUE_MIN_ADD_NEW_COUNT.
#if !defined(HPX_THREAD_QUEUE_MAX_THREAD_COUNT)
#  define HPX_THREAD_QUEUE_MAX_THREAD_COUNT 1000
#endif

///////////////////////////////////////////////////////////////////////////////
// Minimum number of pending tasks required to steal tasks.
#if !defined(HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_PENDING)
#  define HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_PENDING 0
#endif

///////////////////////////////////////////////////////////////////////////////
// Minimum number of staged tasks required to steal tasks.
#if !defined(HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_STAGED)
#  define HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_STAGED 0
#endif

///////////////////////////////////////////////////////////////////////////////
// Minimum number of staged tasks to add to work items queue.
#if !defined(HPX_THREAD_QUEUE_MIN_ADD_NEW_COUNT)
#  define HPX_THREAD_QUEUE_MIN_ADD_NEW_COUNT 10
#endif

///////////////////////////////////////////////////////////////////////////////
// Maximum number of staged tasks to add to work items queue.
#if !defined(HPX_THREAD_QUEUE_MAX_ADD_NEW_COUNT)
#  define HPX_THREAD_QUEUE_MAX_ADD_NEW_COUNT 10
#endif

///////////////////////////////////////////////////////////////////////////////
// Minimum number of terminated threads to delete in one go.
#if !defined(HPX_THREAD_QUEUE_MIN_DELETE_COUNT)
#  define HPX_THREAD_QUEUE_MIN_DELETE_COUNT 10
#endif

///////////////////////////////////////////////////////////////////////////////
// Maximum number of terminated threads to delete in one go.
#if !defined(HPX_THREAD_QUEUE_MAX_DELETE_COUNT)
#  define HPX_THREAD_QUEUE_MAX_DELETE_COUNT 1000
#endif

///////////////////////////////////////////////////////////////////////////////
// Maximum number of terminated threads to keep before cleaning them up.
#if !defined(HPX_THREAD_QUEUE_MAX_TERMINATED_THREADS)
#  define HPX_THREAD_QUEUE_MAX_TERMINATED_THREADS 100
#endif

///////////////////////////////////////////////////////////////////////////////
// Number of threads (of the default stack size) to pre-allocate when
// initializing a thread queue.
#if !defined(HPX_THREAD_QUEUE_INIT_THREADS_COUNT)
#  define HPX_THREAD_QUEUE_INIT_THREADS_COUNT 10
#endif

///////////////////////////////////////////////////////////////////////////////
// Number of thread objects to cache.
#if !defined(HPX_THREAD_QUEUE_CACHED_THREADS_COUNT)
#  define HPX_THREAD_QUEUE_CACHED_THREADS_COUNT 1000
#endif

///////////////////////////////////////////////////////////////////////////////
// Maximum sleep time for idle backoff in milliseconds (used only if
// HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF is defined).
#if !defined(HPX_IDLE_BACKOFF_TIME_MAX)
#  define HPX_IDLE_BACKOFF_TIME_MAX 1000
#endif

///////////////////////////////////////////////////////////////////////////////
#if !defined(HPX_WRAPPER_HEAP_STEP)
#  define HPX_WRAPPER_HEAP_STEP 0xFFFFU
#endif

#if !defined(HPX_INITIAL_GID_RANGE)
#  define HPX_INITIAL_GID_RANGE 0xFFFFU
#endif

///////////////////////////////////////////////////////////////////////////////
// This limits how deep the internal recursion of future continuations will go
// before a new operation is re-spawned.
#if !defined(HPX_CONTINUATION_MAX_RECURSION_DEPTH)
#  if defined(__has_feature)
#    if __has_feature(address_sanitizer)
// if we build under AddressSanitizer we set the max recursion depth to 1 to not
// run into stack overflows.
#      define HPX_CONTINUATION_MAX_RECURSION_DEPTH 1
#    endif
#  endif
#endif

#if !defined(HPX_CONTINUATION_MAX_RECURSION_DEPTH)
#if defined(HPX_DEBUG)
#define HPX_CONTINUATION_MAX_RECURSION_DEPTH 14
#else
#define HPX_CONTINUATION_MAX_RECURSION_DEPTH 20
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// Make sure we have support for more than 64 threads for Xeon Phi
#if defined(__MIC__) && !defined(HPX_HAVE_MORE_THAN_64_THREADS)
#  define HPX_HAVE_MORE_THAN_64_THREADS
#endif
#if defined(__MIC__) && !defined(HPX_HAVE_MAX_CPU_COUNT)
#  define HPX_HAVE_MAX_CPU_COUNT 256
#endif

#if !defined(HPX_HAVE_MAX_CPU_COUNT) && !defined(HPX_HAVE_MORE_THAN_64_THREADS)
#define HPX_HAVE_MAX_CPU_COUNT 64
#endif

// clang-format on
