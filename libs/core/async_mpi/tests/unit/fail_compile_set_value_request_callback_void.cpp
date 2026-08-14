//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This must fail compiling.
//
// Exercises the static_assert at the top of
// `detail::set_value_request_callback_void` which requires that the
// receiver be invocable as `set_value(receiver)` (no value args) -- this
// is the void MPI-return path. Below we deliberately pass a receiver that
// requires an int value argument, so the receiver-shape check should
// fire at compile time with the assert's diagnostic message.

#include <hpx/async_mpi/transform_mpi.hpp>
#include <hpx/modules/execution_base.hpp>

#include <exception>
#include <utility>

namespace ex = hpx::execution::experimental;

// A receiver shape that requires an int value -- incompatible with the
// void-return callback path.
struct receiver_requiring_int_value
{
    using receiver_concept = ex::receiver_t;
    void set_value(int) && noexcept {}
    void set_error(std::exception_ptr) && noexcept {}
    void set_stopped() && noexcept {}
};

int main()
{
    MPI_Request req{};
    receiver_requiring_int_value r;
    // Instantiating this template should fire the static_assert in
    // set_value_request_callback_void.
    hpx::mpi::experimental::detail::set_value_request_callback_void(
        req, std::move(r));
    return 0;
}
