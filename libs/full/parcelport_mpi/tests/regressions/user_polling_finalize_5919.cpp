//  Copyright (c) 2026 John Sorial
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for #5919: destroying an instance of
// hpx::mpi::experimental::enable_user_polling used to call MPI_Finalize even
// if MPI was initialized by the MPI parcelport. Any subsequent communication
// performed by the parcelport (including runtime shutdown) then aborted the
// application.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/async_mpi.hpp>
#include <hpx/modules/testing.hpp>

#include <mpi.h>

#include <cstddef>
#include <cstdint>
#include <vector>

std::uint32_t get_locality()
{
    return hpx::get_locality_id();
}
HPX_PLAIN_ACTION(get_locality)

void exchange_message_with_self()
{
    hpx::mpi::experimental::enable_user_polling enable_polling;

    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int sent = 42;
    int received = 0;

    MPI_Request requests[2];
    MPI_Irecv(&received, 1, MPI_INT, rank, 0, MPI_COMM_WORLD, &requests[0]);
    MPI_Isend(&sent, 1, MPI_INT, rank, 0, MPI_COMM_WORLD, &requests[1]);

    hpx::mpi::experimental::get_future(requests[0]).get();
    hpx::mpi::experimental::get_future(requests[1]).get();

    HPX_TEST_EQ(received, sent);
}

int hpx_main()
{
    exchange_message_with_self();

    // the parcelport must still be functional after user polling was disabled
    std::vector<hpx::id_type> const localities = hpx::find_all_localities();
    HPX_TEST_EQ(localities.size(), std::size_t(2));

    std::vector<hpx::future<std::uint32_t>> futures;
    for (hpx::id_type const& loc : localities)
    {
        futures.push_back(hpx::async(get_locality_action(), loc));
    }

    for (auto&& f : futures)
    {
        HPX_TEST_LT(f.get(), hpx::get_num_localities(hpx::launch::sync));
    }

    return hpx::finalize();
}

int main(int argc, char** argv)
{
    HPX_TEST_EQ(0, hpx::init(argc, argv));
    return hpx::util::report_errors();
}
#endif
