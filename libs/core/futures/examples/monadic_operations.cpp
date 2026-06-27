//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

int hpx_main()
{
    // -----------------------------------------------------------
    // transform: apply a pure function to a future's value.
    //   Wraps the result via hpx::make_ready_future internally.
    //
    //   transform(fut, f)  => hpx::future<decltype(f(val))>
    // -----------------------------------------------------------
    auto f_transformed = hpx::futures::transform(hpx::make_ready_future(42),
        [](int val) { return std::to_string(val); });

    std::cout << "transform:  " << f_transformed.get() << std::endl;

    // -----------------------------------------------------------
    // and_then: chain a continuation. Delegates to .then() directly.
    //   The lambda receives the future object (HPX .then() signature).
    //   .then() auto-flattens if the lambda returns a future<T>.
    // -----------------------------------------------------------
    auto f_chained = hpx::futures::and_then(hpx::make_ready_future(42),
        [](hpx::future<int> val) { return std::to_string(val.get()); });

    std::cout << "and_then:   " << f_chained.get() << std::endl;

    // -----------------------------------------------------------
    // or_else: declarative error recovery without try/catch.
    //   f returns T (the value type) directly -- not a future.
    //   If no exception, value passes through unchanged.
    // -----------------------------------------------------------
    auto f_err = hpx::make_exceptional_future<int>(
        std::runtime_error("something failed"));

    auto f_recovered = hpx::futures::or_else(
        std::move(f_err), [](std::exception_ptr const& /*e*/) { return -1; });

    std::cout << "or_else:    " << f_recovered.get() << std::endl;

    return hpx::local::finalize();
}

int main(int argc, char** argv)
{
    return hpx::local::init(hpx_main, argc, argv);
}
