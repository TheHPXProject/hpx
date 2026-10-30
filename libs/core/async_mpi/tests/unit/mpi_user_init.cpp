//  Copyright (c) 2026 John Sorial
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The application initializes and finalizes MPI itself, independently of
// HPX. HPX must use MPI while the runtime is active, but must never take
// ownership of MPI or finalize it, neither through
// hpx::mpi::experimental::init(true)/finalize nor through
// enable_user_polling (see #5919).

#include <hpx/init.hpp>
#include <hpx/modules/async_mpi.hpp>
#include <hpx/modules/testing.hpp>

#include <mpi.h>

#include <string>
#include <vector>

namespace mpix = hpx::mpi::experimental;

bool mpi_is_active()
{
    int initialized = 0;
    MPI_Initialized(&initialized);
    int finalized = 0;
    MPI_Finalized(&finalized);
    return initialized && !finalized;
}

// Send a token around the ring of all ranks using HPX futures for the
// MPI requests. Requires MPI polling to be enabled.
void exchange_ring_token(int tag)
{
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int const right = (rank + 1) % size;
    int const left = (rank + size - 1) % size;

    int sent = rank;
    int received = -1;

    MPI_Request requests[2];
    MPI_Irecv(&received, 1, MPI_INT, left, tag, MPI_COMM_WORLD, &requests[0]);
    MPI_Isend(&sent, 1, MPI_INT, right, tag, MPI_COMM_WORLD, &requests[1]);

    mpix::get_future(requests[0]).get();
    mpix::get_future(requests[1]).get();

    HPX_TEST_EQ(received, left);
}

int hpx_main(int, char**)
{
    HPX_TEST(mpi_is_active());

    // init(true) finds MPI already initialized by the application and must
    // not take ownership of it, finalize must leave it running
    mpix::init(true);
    HPX_TEST(!mpix::detail::get_mpi_info().mpi_initialized_);

    exchange_ring_token(1);

    mpix::finalize();
    HPX_TEST(!mpix::detail::get_mpi_info().mpi_initialized_);
    HPX_TEST(mpi_is_active());

    // enable_user_polling must not finalize MPI either, and polling can be
    // enabled again afterwards
    for (int tag = 2; tag != 4; ++tag)
    {
        mpix::enable_user_polling enable_polling;
        exchange_ring_token(tag);
    }
    HPX_TEST(mpi_is_active());

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    HPX_TEST_EQ(provided, MPI_THREAD_MULTIPLE);

    // if this test is run with distributed runtime, we need to make sure
    // that all ranks run their main function
    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.run_hpx_main!=1"};

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);

    // MPI is still owned by the application and fully usable after the HPX
    // runtime has stopped
    HPX_TEST(mpi_is_active());

    int size = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int sum = 0;
    MPI_Allreduce(&rank, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    HPX_TEST_EQ(sum, size * (size - 1) / 2);

    HPX_TEST_EQ(MPI_Finalize(), MPI_SUCCESS);

    return hpx::util::report_errors();
}
