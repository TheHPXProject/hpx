//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: HPX_CONTRACT_ASSERT preconditions in the collectives module (fallback
// mode only)
//
// - collective operations on an invalid communicator report a violation and
//   then fail with no_state instead of dereferencing a missing shared state
// - hierarchical_communicator element access requires idx < size()
// - hpx::distributed::latch requires count >= 0 and n >= 0
//
// A custom violation handler must be called exactly once per violating call
// and never for a valid one. Not registered in IGNORE mode, where the checks
// compile out.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/contracts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

namespace {

    std::atomic<int> handler_call_count = 0;
    std::string last_condition;

    // returns, so the caller continues after the violation
    void recording_handler(hpx::contracts::contract_violation const& info)
    {
        ++handler_call_count;
        last_condition = info.condition();
    }

    struct contract_violation_error
    {
    };

    // throws, for checks that have no safe way to continue
    void throwing_handler(hpx::contracts::contract_violation const& info)
    {
        ++handler_call_count;
        last_condition = info.condition();
        throw contract_violation_error{};
    }

    // Run a collective operation on an invalid communicator. It must report
    // exactly one violation and then fail with no_state.
    template <typename F>
    void test_invalid_communicator(F&& f)
    {
        int const count = handler_call_count.load();

        bool caught = false;
        try
        {
            communicator c;
            HPX_FORWARD(F, f)(HPX_MOVE(c)).get();
        }
        catch (hpx::exception const& e)
        {
            caught = true;
            HPX_TEST_EQ(e.get_error(), hpx::error::no_state);
        }

        HPX_TEST(caught);
        HPX_TEST_EQ(handler_call_count.load(), count + 1);
        HPX_TEST_EQ(last_condition, std::string("valid()"));
    }

    template <typename F>
    void test_throwing_violation(F&& f, char const* condition)
    {
        int const count = handler_call_count.load();

        bool caught = false;
        try
        {
            HPX_FORWARD(F, f)();
        }
        catch (contract_violation_error const&)
        {
            caught = true;
        }

        HPX_TEST(caught);
        HPX_TEST_EQ(handler_call_count.load(), count + 1);
        HPX_TEST_EQ(last_condition, std::string(condition));
    }
}    // namespace

void test_communicator()
{
    hpx::contracts::set_violation_handler(recording_handler);

    // valid communicator: no violation
    {
        communicator c = create_communicator(
            "/test/contracts/valid/", num_sites_arg(1), this_site_arg(0));

        auto const [num_sites, this_site] = c.get_info();
        HPX_TEST_EQ(static_cast<std::size_t>(num_sites), std::size_t(1));
        HPX_TEST_EQ(static_cast<std::size_t>(this_site), std::size_t(0));
        HPX_TEST(c.is_root());

        std::vector<int> const result =
            all_gather(c, 42, this_site_arg(0), generation_arg(1)).get();
        HPX_TEST_EQ(result.size(), std::size_t(1));
        HPX_TEST_EQ(result[0], 42);

        HPX_TEST_EQ(handler_call_count.load(), 0);
    }

    // direct use of an invalid communicator
    {
        communicator c;

        auto const [num_sites, this_site] = c.get_info();
        HPX_TEST(num_sites.is_default());
        HPX_TEST(this_site.is_default());
        HPX_TEST_EQ(handler_call_count.load(), 1);

        HPX_TEST(!c.is_root());
        HPX_TEST_EQ(handler_call_count.load(), 2);

        c.set_info(num_sites_arg(1), this_site_arg(0));
        HPX_TEST_EQ(handler_call_count.load(), 3);
        HPX_TEST_EQ(last_condition, std::string("valid()"));
    }

    // collective operations on an invalid communicator
    test_invalid_communicator([](communicator&& c) {
        return all_gather(HPX_MOVE(c), 42, this_site_arg(0), generation_arg(1));
    });
    test_invalid_communicator([](communicator&& c) {
        return all_reduce(HPX_MOVE(c), 42, std::plus<int>{}, this_site_arg(0),
            generation_arg(1));
    });
    test_invalid_communicator([](communicator&& c) {
        return barrier(HPX_MOVE(c), this_site_arg(0), generation_arg(1));
    });
    test_invalid_communicator([](communicator&& c) {
        return broadcast_to(
            HPX_MOVE(c), 42, this_site_arg(0), generation_arg(1));
    });
    test_invalid_communicator([](communicator&& c) {
        return gather_here(
            HPX_MOVE(c), 42, this_site_arg(0), generation_arg(1));
    });

    hpx::contracts::set_violation_handler(nullptr);
}

void test_hierarchical_communicator()
{
    hpx::contracts::set_violation_handler(throwing_handler);

    hierarchical_communicator comms = create_hierarchical_communicator(
        "/test/contracts/hierarchical/", num_sites_arg(1), this_site_arg(0));

    int const count = handler_call_count.load();

    // valid access: no violation
    std::size_t const size = comms.size();
    HPX_TEST_NEQ(size, std::size_t(0));
    HPX_TEST(comms.get(size - 1).valid());
    (void) comms.site(size - 1);
    (void) comms[size - 1];
    HPX_TEST_EQ(handler_call_count.load(), count);

    // out of range access
    char const* const condition = "idx < communicators.size()";
    test_throwing_violation([&] { (void) comms.get(size); }, condition);
    test_throwing_violation([&] { (void) comms.site(size); }, condition);
    test_throwing_violation([&] { (void) comms[size]; }, condition);

    hpx::contracts::set_violation_handler(nullptr);
}

void test_distributed_latch()
{
    hpx::contracts::set_violation_handler(throwing_handler);

    int const count = handler_call_count.load();

    // valid use: no violation
    {
        hpx::distributed::latch l(1);
        l.count_down(0);
        HPX_TEST(!l.is_ready());
        l.count_down(1);
        HPX_TEST(l.is_ready());
        HPX_TEST_EQ(handler_call_count.load(), count);
    }

    // count is negative, the latch must not be created
    test_throwing_violation(
        [] { (void) hpx::distributed::latch(-1); }, "count >= 0");

    // n is negative, nothing must be sent to the latch
    {
        hpx::distributed::latch l(1);
        test_throwing_violation([&] { l.count_down(-1); }, "n >= 0");

        HPX_TEST(!l.is_ready());
        l.count_down(1);
        HPX_TEST(l.is_ready());
    }

    hpx::contracts::set_violation_handler(nullptr);
}

int hpx_main()
{
    test_communicator();
    test_hierarchical_communicator();
    test_distributed_latch();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
