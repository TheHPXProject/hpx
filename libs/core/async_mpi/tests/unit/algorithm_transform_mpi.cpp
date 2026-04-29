//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_mpi.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/tuple.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <mpi.h>
#include <string>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace mpi = hpx::mpi::experimental;
namespace tt = hpx::this_thread::experimental;

// This overload is only used to check dispatching. It is not a useful
// implementation.
template <typename T>
auto tag_invoke(mpi::transform_mpi_t, custom_type<T>& c)
{
    c.tag_invoke_overload_called = true;
    return mpi::transform_mpi(
        ex::just(&c.x, 1, MPI_INT, 0, MPI_COMM_WORLD), MPI_Ibcast);
}

int hpx_main()
{
    int size, rank;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    HPX_TEST_MSG(size > 1, "This test requires N>1 mpi ranks");

    MPI_Datatype datatype = MPI_INT;

    {
        {
            // Use the custom error handler from the async_mpi module which throws
            // exceptions on error returned
            mpi::enable_user_polling enable_polling("", true);
            // Success path
            {
                // MPI function pointer
                int data = 0, count = 1;
                if (rank == 0)
                {
                    data = 42;
                }
                auto s = mpi::transform_mpi(
                    ex::just(&data, count, datatype, 0, comm), MPI_Ibcast);
                auto mpi_result = tt::sync_wait(HPX_MOVE(s));
                auto result = hpx::get<0>(*mpi_result);
                if (rank != 0)
                {
                    HPX_TEST_EQ(data, 42);
                }
                if (rank == 0)
                {
                    HPX_TEST(result == MPI_SUCCESS);
                }
            }

            {
                // Lambda
                int data = 0, count = 1;
                if (rank == 0)
                {
                    data = 42;
                }
                auto s = mpi::transform_mpi(
                    ex::just(&data, count, datatype, 0, comm),
                    [](int* data, int count, MPI_Datatype datatype, int i,
                        MPI_Comm comm, MPI_Request* request) {
                        return MPI_Ibcast(
                            data, count, datatype, i, comm, request);
                    });
                auto mpi_result = tt::sync_wait(HPX_MOVE(s));
                auto result = hpx::get<0>(*mpi_result);
                if (rank != 0)
                {
                    HPX_TEST_EQ(data, 42);
                }
                if (rank == 0)
                {
                    HPX_TEST(result == MPI_SUCCESS);
                }
            }

            {
                // Lambda returning void
                int data = 0, count = 1;
                if (rank == 0)
                {
                    data = 42;
                }
                auto s = mpi::transform_mpi(
                    ex::just(&data, count, datatype, 0, comm),
                    [](int* data, int count, MPI_Datatype datatype, int i,
                        MPI_Comm comm, MPI_Request* request) {
                        MPI_Ibcast(data, count, datatype, i, comm, request);
                    });
                tt::sync_wait(HPX_MOVE(s));
                if (rank != 0)
                {
                    HPX_TEST_EQ(data, 42);
                }
            }

            {
                // tag_invoke overload
                std::atomic<bool> tag_invoke_overload_called{false};
                custom_type<int> c{tag_invoke_overload_called, 0};
                if (rank == 0)
                {
                    c.x = 3;
                }
                auto s = mpi::transform_mpi(c);
                tt::sync_wait(s);
                if (rank == 0)
                {
                    HPX_TEST_EQ(c.x, 3);
                }
                HPX_TEST(tag_invoke_overload_called);
            }

            // Operator| overload
            {
                // MPI function pointer
                int data = 0, count = 1;
                if (rank == 0)
                {
                    data = 42;
                }
                auto result = hpx::get<0>(
                    *tt::sync_wait(ex::just(&data, count, datatype, 0, comm) |
                        mpi::transform_mpi(MPI_Ibcast)));
                if (rank != 0)
                {
                    HPX_TEST_EQ(data, 42);
                }
                if (rank == 0)
                {
                    HPX_TEST(result == MPI_SUCCESS);
                }
            }

            // Failure path
            {
                // Exception with error sender
                bool exception_thrown = false;
                try
                {
                    tt::sync_wait(mpi::transform_mpi(
                        error_sender<int*, int, MPI_Datatype, int, MPI_Comm>{},
                        MPI_Ibcast));
                    HPX_TEST(false);
                }
                catch (std::runtime_error const& e)
                {
                    HPX_TEST_EQ(std::string(e.what()), std::string("error"));
                    exception_thrown = true;
                }
                HPX_TEST(exception_thrown);
            }

            {
                // Exception in the lambda
                bool exception_thrown = false;
                int data = 0, count = 1;
                auto s = mpi::transform_mpi(
                    ex::just(&data, count, datatype, 0, comm),
                    [](int* data, int count, MPI_Datatype datatype, int i,
                        MPI_Comm comm, MPI_Request* request) {
                        MPI_Ibcast(data, count, datatype, i, comm, request);
                        throw std::runtime_error("error in lambda");
                    });
                try
                {
                    tt::sync_wait(HPX_MOVE(s));
                }
                catch (std::runtime_error const& e)
                {
                    HPX_TEST_EQ(
                        std::string(e.what()), std::string("error in lambda"));
                    exception_thrown = true;
                }
                HPX_TEST(exception_thrown);
                // Necessary to avoid a seg fault caused by MPI data going out of scope
                // too early when an exception occurred outside of MPI
                MPI_Barrier(comm);
            }

            {
                // Exception thrown through HPX custom error handler that throws
                int *data = nullptr, count = 0;
                bool exception_thrown = false;
                try
                {
                    tt::sync_wait(mpi::transform_mpi(
                        ex::just(data, count, datatype, -1, comm), MPI_Ibcast));
                    HPX_TEST(false);
                }
                catch (std::runtime_error const& e)
                {
                    auto error_msg = std::string(e.what());
                    hpx::string_util::to_lower(error_msg);
                    HPX_TEST(error_msg.find(std::string("invalid root")) !=
                        std::string::npos);
                    exception_thrown = true;
                }
                HPX_TEST(exception_thrown);
            }
            // let the user polling go out of scope
        }

        {
            // Use the default error handler MPI_ERRORS_ARE_FATAL
            mpi::enable_user_polling enable_polling_no_errhandler;
            {
                // Exception thrown based on the returned error code
                int *data = nullptr, count = 0;
                bool exception_thrown = false;
                try
                {
                    tt::sync_wait(mpi::transform_mpi(
                        ex::just(data, count, datatype, -1, comm), MPI_Ibcast));
                    HPX_TEST(false);
                }
                catch (std::runtime_error const&)
                {
                    exception_thrown = true;
                }
                HPX_TEST(exception_thrown);
            }
            // let the user polling go out of scope
        }
    }

    return hpx::local::finalize();
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    // Disable MPI tests because they
    // hang due to sync_wait consuming the thread
    MPI_Init(&argc, &argv);

    auto result = hpx::local::init(hpx_main, argc, argv);

    MPI_Finalize();

    return result || hpx::util::report_errors();
}
