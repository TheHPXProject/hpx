//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A collective operation on an invalid communicator violates the valid()
// precondition of the noexcept communicator::get_info(). Nothing past the
// violation is safe to run, so the handler ends the process, successfully only
// if it was called for that precondition.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/contracts.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdlib>
#include <cstring>

using namespace hpx::collectives;

///////////////////////////////////////////////////////////////////////////////
namespace {

    void exiting_handler(hpx::contracts::contract_violation const& info)
    {
        bool const expected = std::strcmp(info.condition(), "valid()") == 0;
        std::_Exit(expected ? EXIT_SUCCESS : EXIT_FAILURE);
    }
}    // namespace

int hpx_main()
{
    hpx::contracts::set_violation_handler(exiting_handler);

    (void) all_gather(communicator{}, 42, this_site_arg(0), generation_arg(1));

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
