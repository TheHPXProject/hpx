//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// HPX_CONTRACT_ASSERT preconditions in the collectives module, checked through
// a throwing violation handler: communicator::is_root() needs a valid
// communicator, hierarchical_communicator element access needs idx < size(),
// and hpx::distributed::latch needs count >= 0 and n >= 0. The noexcept
// accessors are covered by contracts_invalid_communicator.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/contracts.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <string>
#include <vector>

using namespace hpx::collectives;

///////////////////////////////////////////////////////////////////////////////
namespace {

    int handler_call_count = 0;
    std::string last_condition;

    struct contract_violation_error
    {
    };

    void throwing_handler(hpx::contracts::contract_violation const& info)
    {
        ++handler_call_count;
        last_condition = info.condition();
        throw contract_violation_error{};
    }

    template <typename F>
    void test_violation(F const& f, char const* condition)
    {
        int const count = handler_call_count;

        bool caught = false;
        try
        {
            f();
        }
        catch (contract_violation_error const&)
        {
            caught = true;
        }

        HPX_TEST(caught);
        HPX_TEST_EQ(handler_call_count, count + 1);
        HPX_TEST_EQ(last_condition, std::string(condition));
    }
}    // namespace

void test_communicator()
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

    HPX_TEST_EQ(handler_call_count, 0);

    communicator const invalid{};
    test_violation([&] { (void) invalid.is_root(); }, "valid()");
}

void test_hierarchical_communicator()
{
    hierarchical_communicator comms = create_hierarchical_communicator(
        "/test/contracts/hierarchical/", num_sites_arg(1), this_site_arg(0));

    int const count = handler_call_count;

    std::size_t const size = comms.size();
    HPX_TEST_NEQ(size, std::size_t(0));
    HPX_TEST(comms.get(size - 1).valid());
    (void) comms.site(size - 1);
    (void) comms[size - 1];

    HPX_TEST_EQ(handler_call_count, count);

    char const* const condition = "idx < communicators.size()";
    test_violation([&] { (void) comms.get(size); }, condition);
    test_violation([&] { (void) comms.site(size); }, condition);
    test_violation([&] { (void) comms[size]; }, condition);
}

void test_distributed_latch()
{
    int const count = handler_call_count;

    {
        hpx::distributed::latch l(1);
        l.count_down(0);
        HPX_TEST(!l.is_ready());
        l.count_down(1);
        HPX_TEST(l.is_ready());
    }

    HPX_TEST_EQ(handler_call_count, count);

    // the latch must not be created
    test_violation([] { (void) hpx::distributed::latch(-1); }, "count >= 0");

    // nothing must be sent to the latch
    hpx::distributed::latch l(1);
    test_violation([&] { l.count_down(-1); }, "n >= 0");

    HPX_TEST(!l.is_ready());
    l.count_down(1);
    HPX_TEST(l.is_ready());
}

int hpx_main()
{
    hpx::contracts::set_violation_handler(throwing_handler);

    test_communicator();
    test_hierarchical_communicator();
    test_distributed_latch();

    hpx::contracts::set_violation_handler(nullptr);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
