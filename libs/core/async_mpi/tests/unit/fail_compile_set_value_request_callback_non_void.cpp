//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This must fail compiling.
//
// Exercises the receiver-shape static_assert at the top of
// `detail::set_value_request_callback_non_void` which requires that the
// receiver be invocable as `set_value(receiver, result)` -- this is the
// non-void MPI-return path. Below we deliberately pass a receiver that
// takes no value argument, so the receiver-shape check should fire at
// compile time with the assert's diagnostic message.

#include <hpx/async_mpi/transform_mpi.hpp>
#include <hpx/modules/execution_base.hpp>

#include <exception>
#include <utility>

namespace ex = hpx::execution::experimental;

// A receiver shape that takes no value -- incompatible with the
// non-void-return callback path which needs set_value(rcv, res).
struct receiver_taking_no_value
{
    using receiver_concept = ex::receiver_t;
    void set_value() && noexcept {}
    void set_error(std::exception_ptr) && noexcept {}
    void set_stopped() && noexcept {}
};

int main()
{
    MPI_Request req{};
    receiver_taking_no_value r;
    int res = 42;
    // Instantiating this template should fire the receiver-shape
    // static_assert in set_value_request_callback_non_void.
    hpx::mpi::experimental::detail::set_value_request_callback_non_void(
        req, std::move(r), std::move(res));
    return 0;
}
