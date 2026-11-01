//  Copyright (c) 2026 John Sorial
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The application initializes and finalizes MPI itself, independently of
// HPX, and runs HPX on top of it using the MPI parcelport. The application
// communicates through MPI directly while the parcelport is active. HPX must
// neither take ownership of MPI nor finalize it, so that the application can
// keep using MPI after the runtime has stopped (see #5919).

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/async_mpi.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <mpi.h>

#include <cstdint>
#include <string>
#include <vector>

namespace mpix = hpx::mpi::experimental;

std::uint32_t get_locality()
{
    return hpx::get_locality_id();
}
HPX_PLAIN_ACTION(get_locality)

bool mpi_is_active()
{
    int initialized = 0;
    MPI_Initialized(&initialized);
    int finalized = 0;
    MPI_Finalized(&finalized);
    return initialized && !finalized;
}

std::vector<hpx::future<std::uint32_t>> call_all_localities()
{
    std::vector<hpx::future<std::uint32_t>> futures;
    for (hpx::id_type const& loc : hpx::find_all_localities())
    {
        futures.push_back(hpx::async(get_locality_action(), loc));
    }
    return futures;
}

void check_all_localities(std::vector<hpx::future<std::uint32_t>>& futures)
{
    std::uint32_t expected = 0;
    for (auto& f : futures)
    {
        HPX_TEST_EQ(f.get(), expected++);
    }
}

// Send a token around the ring of all ranks using HPX futures for the MPI
// requests, on the application's communicator. Requires MPI polling to be
// enabled.
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

int hpx_main()
{
    HPX_TEST(mpi_is_active());

    // HPX runs on the ranks of the application's MPI_COMM_WORLD
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    HPX_TEST_EQ(hpx::get_num_localities(hpx::launch::sync),
        static_cast<std::uint32_t>(size));
    HPX_TEST_EQ(hpx::get_locality_id(), static_cast<std::uint32_t>(rank));

    {
        // application MPI traffic while parcels are in flight
        auto futures = call_all_localities();
        {
            mpix::enable_user_polling enable_polling;
            exchange_ring_token(1);
        }
        check_all_localities(futures);
    }
    HPX_TEST(mpi_is_active());

    // init(true) finds MPI already initialized by the application and must
    // not take ownership of it, finalize must leave it running
    {
        mpix::init(true);
        HPX_TEST(!mpix::detail::get_mpi_info().mpi_initialized_);

        auto futures = call_all_localities();
        exchange_ring_token(2);
        check_all_localities(futures);

        mpix::finalize();
        HPX_TEST(!mpix::detail::get_mpi_info().mpi_initialized_);
    }
    HPX_TEST(mpi_is_active());

    // the parcelport is still functional
    {
        auto futures = call_all_localities();
        check_all_localities(futures);
    }

    // make sure no locality starts shutting down while others still
    // communicate with it
    hpx::distributed::barrier::synchronize();

    return hpx::finalize();
}

int main(int argc, char** argv)
{
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    HPX_TEST_EQ(provided, MPI_THREAD_MULTIPLE);

    hpx::init_params init_args;
    init_args.cfg = {"hpx.run_hpx_main!=1"};

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);

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
#endif
