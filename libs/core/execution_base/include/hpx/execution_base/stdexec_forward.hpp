//  Copyright (c) 2024 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <hpx/config.hpp>

#if defined(HPX_HAVE_STDEXEC)

/* TODO: Find out what diagnostics should be disabled for stdexec to compile.
 * currently it seems to need at least "-Wgnu-zero-variadic-macro-arguments"
 * and "-Wmissing-braces" even though they are explicitly disabled inside
 * stdexec.
 *
 * Clang needs "-Wgnu-zero-variadic-macro-arguments", but this option does not
 * exist in GCC. Will leave this commented out for now.
 *
 * #if defined(HPX_CLANG_VERSION)
 * #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
 * #endif
 */

#if defined(HPX_GCC_VERSION)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wmissing-braces"
#elif defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <exec/ensure_started.hpp>
#include <exec/execute.hpp>
#include <exec/sender_for.hpp>
#include <exec/split.hpp>
#include <exec/start_detached.hpp>
#include <stdexec/execution.hpp>

#if defined(HPX_GCC_VERSION)
#pragma GCC diagnostic pop
#elif defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif

namespace hpx::execution::experimental {
    // Domain
    HPX_CXX_CORE_EXPORT using stdexec::default_domain;
    HPX_CXX_CORE_EXPORT using stdexec::get_domain;
    HPX_CXX_CORE_EXPORT using stdexec::get_domain_t;

    // Completion domain (P3826R5)
    HPX_CXX_CORE_EXPORT using stdexec::get_completion_domain;
    HPX_CXX_CORE_EXPORT using stdexec::get_completion_domain_t;
    HPX_CXX_CORE_EXPORT using stdexec::indeterminate_domain;

    // Receiver
    HPX_CXX_CORE_EXPORT using stdexec::set_error_t;
    HPX_CXX_CORE_EXPORT using stdexec::set_stopped_t;
    HPX_CXX_CORE_EXPORT using stdexec::set_value_t;

    HPX_CXX_CORE_EXPORT using stdexec::set_error;
    HPX_CXX_CORE_EXPORT using stdexec::set_stopped;
    HPX_CXX_CORE_EXPORT using stdexec::set_value;

    HPX_CXX_CORE_EXPORT using stdexec::enable_receiver;
    HPX_CXX_CORE_EXPORT using stdexec::receiver_t;

    // Environment
    HPX_CXX_CORE_EXPORT using stdexec::get_env;
    HPX_CXX_CORE_EXPORT using stdexec::get_env_t;

    HPX_CXX_CORE_EXPORT using stdexec::env;
    HPX_CXX_CORE_EXPORT using stdexec::prop;

    HPX_CXX_CORE_EXPORT using empty_env = env<>;
    HPX_CXX_CORE_EXPORT using stdexec::env_of_t;

    // Queries
    HPX_CXX_CORE_EXPORT using stdexec::forward_progress_guarantee;

    HPX_CXX_CORE_EXPORT using stdexec::execute_may_block_caller_t;
    HPX_CXX_CORE_EXPORT using stdexec::forwarding_query_t;
    HPX_CXX_CORE_EXPORT using stdexec::get_allocator_t;
    HPX_CXX_CORE_EXPORT using stdexec::get_completion_scheduler_t;
    HPX_CXX_CORE_EXPORT using stdexec::get_delegatee_scheduler_t;
    HPX_CXX_CORE_EXPORT using stdexec::get_forward_progress_guarantee_t;
    HPX_CXX_CORE_EXPORT using stdexec::get_scheduler_t;
    HPX_CXX_CORE_EXPORT using stdexec::get_stop_token_t;

    HPX_CXX_CORE_EXPORT using stdexec::execute_may_block_caller;
    HPX_CXX_CORE_EXPORT using stdexec::forwarding_query;
    HPX_CXX_CORE_EXPORT using stdexec::get_allocator;
    HPX_CXX_CORE_EXPORT using stdexec::get_completion_scheduler;
    HPX_CXX_CORE_EXPORT using stdexec::get_delegatee_scheduler;
    HPX_CXX_CORE_EXPORT using stdexec::get_forward_progress_guarantee;
    HPX_CXX_CORE_EXPORT using stdexec::get_scheduler;
    HPX_CXX_CORE_EXPORT using stdexec::get_stop_token;

    HPX_CXX_CORE_EXPORT using stdexec::in_place_stop_callback;
    HPX_CXX_CORE_EXPORT using stdexec::inplace_stop_source;
    HPX_CXX_CORE_EXPORT using stdexec::inplace_stop_token;
    HPX_CXX_CORE_EXPORT using stdexec::never_stop_token;

    HPX_CXX_CORE_EXPORT using stdexec::stop_token_of_t;

    HPX_CXX_CORE_EXPORT using stdexec::completion_signatures;
    HPX_CXX_CORE_EXPORT using stdexec::get_completion_signatures;
    HPX_CXX_CORE_EXPORT using stdexec::get_completion_signatures_t;

    // Operation State
    HPX_CXX_CORE_EXPORT using stdexec::operation_state_t;

    // Sender
    HPX_CXX_CORE_EXPORT using stdexec::connect;
    HPX_CXX_CORE_EXPORT using stdexec::connect_result_t;
    HPX_CXX_CORE_EXPORT using stdexec::connect_t;

    HPX_CXX_CORE_EXPORT using stdexec::enable_sender;
    HPX_CXX_CORE_EXPORT using stdexec::sender_t;

    // Start
    HPX_CXX_CORE_EXPORT using stdexec::start;
    HPX_CXX_CORE_EXPORT using stdexec::start_t;

    // Schedule
    HPX_CXX_CORE_EXPORT using stdexec::schedule;
    HPX_CXX_CORE_EXPORT using stdexec::schedule_t;

    HPX_CXX_CORE_EXPORT using stdexec::schedule_result_t;

    // As awaitable
    HPX_CXX_CORE_EXPORT using stdexec::as_awaitable;
    HPX_CXX_CORE_EXPORT using stdexec::as_awaitable_t;

    // Start on
    HPX_CXX_CORE_EXPORT using stdexec::start_on;
    HPX_CXX_CORE_EXPORT using stdexec::start_on_t;

    HPX_CXX_CORE_EXPORT using stdexec::on;
    HPX_CXX_CORE_EXPORT using stdexec::on_t;

    // Continues on
    HPX_CXX_CORE_EXPORT using stdexec::continues_on;
    HPX_CXX_CORE_EXPORT using stdexec::continues_on_t;

    // Transfer just
    HPX_CXX_CORE_EXPORT using stdexec::transfer_just;
    HPX_CXX_CORE_EXPORT using stdexec::transfer_just_t;

    // Sender for
    HPX_CXX_CORE_EXPORT using exec::sender_for;

    // Bulk operations
    HPX_CXX_CORE_EXPORT using stdexec::bulk;
    HPX_CXX_CORE_EXPORT using stdexec::bulk_chunked;
    HPX_CXX_CORE_EXPORT using stdexec::bulk_chunked_t;
    HPX_CXX_CORE_EXPORT using stdexec::bulk_t;
    HPX_CXX_CORE_EXPORT using stdexec::bulk_unchunked;
    HPX_CXX_CORE_EXPORT using stdexec::bulk_unchunked_t;

    // Execution policies
    HPX_CXX_CORE_EXPORT using stdexec::is_execution_policy;
    HPX_CXX_CORE_EXPORT using stdexec::is_execution_policy_v;
    HPX_CXX_CORE_EXPORT using stdexec::par;
    HPX_CXX_CORE_EXPORT using stdexec::par_unseq;
    HPX_CXX_CORE_EXPORT using stdexec::seq;
    HPX_CXX_CORE_EXPORT using stdexec::unseq;

    // Split (moved to exec:: namespace in newer stdexec)
    HPX_CXX_CORE_EXPORT using exec::split;
    HPX_CXX_CORE_EXPORT using exec::split_t;

    // Ensure started (moved to exec:: namespace in newer stdexec)
    HPX_CXX_CORE_EXPORT using exec::ensure_started;
    HPX_CXX_CORE_EXPORT using exec::ensure_started_t;

    // Transfer
    HPX_CXX_CORE_EXPORT using stdexec::transfer;
    HPX_CXX_CORE_EXPORT using stdexec::transfer_t;

    // Tags namespace removed - not present in P3826R5 stdexec

    // Execute (moved to exec:: namespace in newer stdexec)
    HPX_CXX_CORE_EXPORT using exec::execute;
    HPX_CXX_CORE_EXPORT using exec::execute_t;

    // Into Variant
    HPX_CXX_CORE_EXPORT using stdexec::into_variant;
    HPX_CXX_CORE_EXPORT using stdexec::into_variant_t;

    // Just
    HPX_CXX_CORE_EXPORT using stdexec::just_error_t;
    HPX_CXX_CORE_EXPORT using stdexec::just_stopped_t;
    HPX_CXX_CORE_EXPORT using stdexec::just_t;

    HPX_CXX_CORE_EXPORT using stdexec::just;
    HPX_CXX_CORE_EXPORT using stdexec::just_error;
    HPX_CXX_CORE_EXPORT using stdexec::just_stopped;

    // Let
    HPX_CXX_CORE_EXPORT using stdexec::let_error_t;
    HPX_CXX_CORE_EXPORT using stdexec::let_stopped_t;
    HPX_CXX_CORE_EXPORT using stdexec::let_value_t;

    HPX_CXX_CORE_EXPORT using stdexec::let_error;
    HPX_CXX_CORE_EXPORT using stdexec::let_stopped;
    HPX_CXX_CORE_EXPORT using stdexec::let_value;

    // Run loop
    HPX_CXX_CORE_EXPORT using stdexec::run_loop;

    // Schedule from
    HPX_CXX_CORE_EXPORT using stdexec::schedule_from;
    HPX_CXX_CORE_EXPORT using stdexec::schedule_from_t;

    // Start detached (moved to exec:: namespace in newer stdexec)
    HPX_CXX_CORE_EXPORT using exec::start_detached;
    HPX_CXX_CORE_EXPORT using exec::start_detached_t;

    // Stop token
    HPX_CXX_CORE_EXPORT using stdexec::stop_callback_for_t;
    HPX_CXX_CORE_EXPORT using stdexec::stoppable_token;
    HPX_CXX_CORE_EXPORT using stdexec::unstoppable_token;

    // Stopped as error
    HPX_CXX_CORE_EXPORT using stdexec::stopped_as_error;
    HPX_CXX_CORE_EXPORT using stdexec::stopped_as_error_t;

    // Stopped as optional
    HPX_CXX_CORE_EXPORT using stdexec::stopped_as_optional;
    HPX_CXX_CORE_EXPORT using stdexec::stopped_as_optional_t;

    // Sync wait
    HPX_CXX_CORE_EXPORT using stdexec::sync_wait;
    HPX_CXX_CORE_EXPORT using stdexec::sync_wait_t;

    // Sync wait with variant
    HPX_CXX_CORE_EXPORT using stdexec::sync_wait_with_variant;
    HPX_CXX_CORE_EXPORT using stdexec::sync_wait_with_variant_t;

    // Then
    HPX_CXX_CORE_EXPORT using stdexec::then;
    HPX_CXX_CORE_EXPORT using stdexec::then_t;

    // Completion signature manipulators
    HPX_CXX_CORE_EXPORT using stdexec::completion_signatures_of_t;
    HPX_CXX_CORE_EXPORT using stdexec::error_types_of_t;
    HPX_CXX_CORE_EXPORT using stdexec::sends_stopped;
    HPX_CXX_CORE_EXPORT using stdexec::value_types_of_t;

    HPX_CXX_CORE_EXPORT using stdexec::transform_completion_signatures;
    HPX_CXX_CORE_EXPORT using stdexec::transform_completion_signatures_of;

    // Transform sender
    HPX_CXX_CORE_EXPORT using stdexec::transform_sender;
    HPX_CXX_CORE_EXPORT using stdexec::transform_sender_result_t;
    HPX_CXX_CORE_EXPORT using stdexec::transform_sender_t;

    HPX_CXX_CORE_EXPORT using stdexec::apply_sender;
    HPX_CXX_CORE_EXPORT using stdexec::apply_sender_result_t;
    HPX_CXX_CORE_EXPORT using stdexec::apply_sender_t;

    // Upon error
    HPX_CXX_CORE_EXPORT using stdexec::upon_error;
    HPX_CXX_CORE_EXPORT using stdexec::upon_error_t;

    // Upon stopped
    HPX_CXX_CORE_EXPORT using stdexec::upon_stopped;
    HPX_CXX_CORE_EXPORT using stdexec::upon_stopped_t;

    // When all
    HPX_CXX_CORE_EXPORT using stdexec::when_all;
    HPX_CXX_CORE_EXPORT using stdexec::when_all_t;

    HPX_CXX_CORE_EXPORT using stdexec::when_all_with_variant;
    HPX_CXX_CORE_EXPORT using stdexec::when_all_with_variant_t;

    HPX_CXX_CORE_EXPORT using stdexec::transfer_when_all;
    HPX_CXX_CORE_EXPORT using stdexec::transfer_when_all_t;

    HPX_CXX_CORE_EXPORT using stdexec::transfer_when_all_with_variant;
    HPX_CXX_CORE_EXPORT using stdexec::transfer_when_all_with_variant_t;

    // With awaitable senders
    HPX_CXX_CORE_EXPORT using stdexec::with_awaitable_senders;

    // Concepts
    HPX_CXX_CORE_EXPORT using stdexec::sender;
    HPX_CXX_CORE_EXPORT using stdexec::sender_in;
    HPX_CXX_CORE_EXPORT using stdexec::sender_of;
    HPX_CXX_CORE_EXPORT using stdexec::sender_to;

    HPX_CXX_CORE_EXPORT using stdexec::receiver;
    HPX_CXX_CORE_EXPORT using stdexec::receiver_of;

    HPX_CXX_CORE_EXPORT using stdexec::scheduler;

    HPX_CXX_CORE_EXPORT using stdexec::operation_state;

    namespace stdexec_non_standard_tag_invoke {

        // Presently, the stdexec repository implements tag invoke,
        // however it includes a non-standard (in the sense of unexpected) extension.
        // tag invoke first checks for the existence of a .query member function or
        // a ::query static function.
        HPX_CXX_CORE_EXPORT using stdexec::tag_invoke;
        HPX_CXX_CORE_EXPORT using stdexec::tag_invoke_result;

        HPX_CXX_CORE_EXPORT using stdexec::nothrow_tag_invocable;
        HPX_CXX_CORE_EXPORT using stdexec::tag_invocable;
    }    // namespace stdexec_non_standard_tag_invoke

    namespace stdexec_internal {

        HPX_CXX_CORE_EXPORT using stdexec::__single_sender_value_t;

        namespace __connect_await {
            HPX_CXX_CORE_EXPORT using namespace stdexec::__connect_await;
        }

        HPX_CXX_CORE_EXPORT using stdexec::__connect_awaitable_t;

        // Additional stdexec concepts and utilities needed for domain customization
        HPX_CXX_CORE_EXPORT using stdexec::__completes_on;
        HPX_CXX_CORE_EXPORT using stdexec::__sender_for;
    }    // namespace stdexec_internal
}    // namespace hpx::execution::experimental

// Leaving this as a placeholder
namespace hpx::this_thread {
}

#endif
