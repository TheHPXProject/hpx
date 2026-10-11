//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: a collective operation on an invalid (default constructed)
// communicator violates the valid() precondition of communicator::get_info()
// (HPX_CONTRACT_ASSERT, fallback mode only)
//
// get_info() is noexcept and nothing past the violated precondition is safe
// to run, so the violation handler ends the process. The test passes only if
// the handler is called for this precondition, everything else exits with a
// non-zero code (without the check the missing shared state is dereferenced).
//
// Only registered with HPX_WITH_CONTRACTS=ON and not in IGNORE mode, otherwise
// the check falls back to HPX_ASSERT and the handler is never called.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/contracts.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdlib>
#include <cstring>

using namespace hpx::collectives;

namespace {

    void exiting_handler(hpx::contracts::contract_violation const& info)
    {
        bool const expected = std::strcmp(info.condition(), "valid()") == 0;

        // nothing after the violation may run, see above
        std::_Exit(expected ? EXIT_SUCCESS : EXIT_FAILURE);
    }
}    // namespace

int hpx_main()
{
    hpx::contracts::set_violation_handler(exiting_handler);

    communicator c;
    (void) all_gather(c, 42, this_site_arg(0), generation_arg(1));

    HPX_TEST_MSG(false, "the violation handler was not called");

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
