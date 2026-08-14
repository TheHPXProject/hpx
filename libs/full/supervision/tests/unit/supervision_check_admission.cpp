//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include "supervision_test_helpers.hpp"

#include <cstdint>

// ============================================================================
// Test Cases: Dispatch Admission
// ============================================================================

// A target with no recorded state at all has nothing latched against it.
void test_check_admission_admitted_for_unknown_target()
{
    hpx::id_type const target = make_test_target();

    HPX_TEST(hpx::supervision::check_admission(target) ==
        hpx::supervision::dispatch_outcome::admitted);
}

// Non-terminal events must not fence dispatch.
void test_check_admission_admitted_before_terminal()
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(target, hpx::supervision::event::started);
    HPX_TEST(hpx::supervision::check_admission(target) ==
        hpx::supervision::dispatch_outcome::admitted);

    hpx::supervision::publish_event(target, hpx::supervision::event::running);
    HPX_TEST(hpx::supervision::check_admission(target) ==
        hpx::supervision::dispatch_outcome::admitted);
}

// Once `target` has latched a terminal event for an epoch, dispatch under
// that same epoch must be rejected.
void test_check_admission_rejected_after_terminal()
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(target, hpx::supervision::event::started);
    hpx::supervision::publish_event(target, hpx::supervision::event::running);
    hpx::supervision::publish_event(target, hpx::supervision::event::completed);

    HPX_TEST(hpx::supervision::check_admission(target) ==
        hpx::supervision::dispatch_outcome::rejected_fenced);

    hpx::id_type const failed_target = make_test_target();

    hpx::supervision::publish_event(
        failed_target, hpx::supervision::event::started);
    hpx::supervision::publish_event(
        failed_target, hpx::supervision::event::running);
    hpx::supervision::publish_event(
        failed_target, hpx::supervision::event::failed);

    HPX_TEST(hpx::supervision::check_admission(failed_target) ==
        hpx::supervision::dispatch_outcome::rejected_fenced);
}

// A terminal latch is scoped to the epoch it was recorded under: dispatch
// under a different epoch is unaffected.
void test_check_admission_scoped_to_epoch()
{
    hpx::id_type const target = make_test_target();
    std::uint64_t const epoch = 1;

    hpx::supervision::publish_event(
        target, hpx::supervision::event::started, epoch);
    hpx::supervision::publish_event(
        target, hpx::supervision::event::running, epoch);
    hpx::supervision::publish_event(
        target, hpx::supervision::event::completed, epoch);

    HPX_TEST(hpx::supervision::check_admission(target, epoch) ==
        hpx::supervision::dispatch_outcome::rejected_fenced);
    HPX_TEST(hpx::supervision::check_admission(target, epoch + 1) ==
        hpx::supervision::dispatch_outcome::admitted);
    HPX_TEST(hpx::supervision::check_admission(target, epoch) ==
        hpx::supervision::dispatch_outcome::rejected_fenced);
}

// An invalid target has nothing latched against it either.
void test_check_admission_admitted_for_invalid_target()
{
    HPX_TEST(hpx::supervision::check_admission(hpx::invalid_id) ==
        hpx::supervision::dispatch_outcome::admitted);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int hpx_main()
{
    HPX_SUPERVISION_TEST_RUN(test_check_admission_admitted_for_unknown_target);
    HPX_SUPERVISION_TEST_RUN(test_check_admission_admitted_before_terminal);
    HPX_SUPERVISION_TEST_RUN(test_check_admission_rejected_after_terminal);
    HPX_SUPERVISION_TEST_RUN(test_check_admission_scoped_to_epoch);
    HPX_SUPERVISION_TEST_RUN(test_check_admission_admitted_for_invalid_target);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
