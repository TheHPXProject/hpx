//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The flat collectives report an invalid (default constructed or moved from)
// communicator through the returned future, the same way the hierarchical
// overloads do.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <functional>
#include <vector>

using namespace hpx::collectives;

///////////////////////////////////////////////////////////////////////////////
template <typename F>
void test_rejected(F const& f)
{
    bool caught = false;
    try
    {
        f();
    }
    catch (hpx::exception const& e)
    {
        caught = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::invalid_status);
    }
    HPX_TEST(caught);
}

void test_invalid_communicator()
{
    this_site_arg const site(0);
    generation_arg const generation(1);

    test_rejected([&] {
        (void) hpx::collectives::all_gather(
            communicator{}, 42, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::all_reduce(
            communicator{}, 42, std::plus<>{}, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::all_to_all(
            communicator{}, std::vector<int>{42}, site, generation)
            .get();
    });
    test_rejected([&] {
        hpx::collectives::barrier(communicator{}, site, generation).get();
    });
    test_rejected([&] {
        (void) hpx::collectives::broadcast_to(
            communicator{}, 42, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::broadcast_from<int>(
            communicator{}, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::exclusive_scan(
            communicator{}, 42, std::plus<>{}, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::exclusive_scan(
            communicator{}, 42, 0, std::plus<>{}, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::inclusive_scan(
            communicator{}, 42, std::plus<>{}, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::gather_here(
            communicator{}, 42, site, generation)
            .get();
    });
    test_rejected([&] {
        hpx::collectives::gather_there(communicator{}, 42, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::reduce_here(
            communicator{}, 42, std::plus<>{}, site, generation)
            .get();
    });
    test_rejected([&] {
        hpx::collectives::reduce_there(communicator{}, 42, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::scatter_to(
            communicator{}, std::vector<int>{42}, site, generation)
            .get();
    });
    test_rejected([&] {
        (void) hpx::collectives::scatter_from<int>(
            communicator{}, site, generation)
            .get();
    });

    // the synchronous convenience functions throw directly
    test_rejected([&] {
        int value = 42;
        hpx::collectives::broadcast(communicator{}, value, site, generation);
    });
    test_rejected([&] {
        hpx::collectives::reduce(
            communicator{}, 42, std::plus<>{}, site, generation);
    });
}

int hpx_main()
{
    test_invalid_communicator();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
