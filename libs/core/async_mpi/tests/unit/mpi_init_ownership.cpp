//  Copyright (c) 2026 John Sorial
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx::mpi::experimental::init(true) must take ownership of MPI only if it
// actually initialized it. A subsequent call must not drop that ownership,
// and hpx::mpi::experimental::finalize must release it and finalize MPI
// (see #5919).

#include <hpx/init.hpp>
#include <hpx/modules/async_mpi.hpp>
#include <hpx/modules/testing.hpp>

#include <mpi.h>

namespace mpix = hpx::mpi::experimental;

int hpx_main(int, char**)
{
    // MPI is initialized by mpix::init below
    int initialized = 0;
    MPI_Initialized(&initialized);
    HPX_TEST(!initialized);

    mpix::init(true);
    HPX_TEST(mpix::detail::get_mpi_info().mpi_initialized_);

    // a second call must not drop the ownership acquired by the first one
    mpix::init(true);
    HPX_TEST(mpix::detail::get_mpi_info().mpi_initialized_);

    // finalize releases the ownership and finalizes MPI
    mpix::finalize();
    HPX_TEST(!mpix::detail::get_mpi_info().mpi_initialized_);

    int finalized = 0;
    MPI_Finalized(&finalized);
    HPX_TEST(finalized);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);

    // make sure MPI is finalized before exiting, even if the test failed
    int finalized = 0;
    MPI_Finalized(&finalized);
    if (!finalized)
    {
        MPI_Finalize();
    }

    return hpx::util::report_errors();
}
