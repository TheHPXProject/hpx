//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for #7613: the 2-arg form of HPX_REGISTER_CHANNEL and
// HPX_REGISTER_CHANNEL_DECLARATION is meant to let callers supply an
// identifier-safe name for channel types that are not themselves valid
// identifiers (e.g. std::vector<int>). Verify that the generated
// identifiers are built only from the supplied name, not from the type.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>

#include <vector>

HPX_REGISTER_CHANNEL_DECLARATION(std::vector<int>, vec_int_channel)
HPX_REGISTER_CHANNEL(std::vector<int>, vec_int_channel)

void send_values(hpx::lcos::channel<std::vector<int>> buffer)
{
    buffer.set(hpx::launch::sync, std::vector<int>{1, 2, 3});
}

void receive_values()
{
    hpx::lcos::channel<std::vector<int>> buffer;
    buffer.connect_to("my_vector_channel");

    std::vector<int> const values = buffer.get(hpx::launch::sync);
    HPX_TEST_EQ(std::size_t(3), values.size());
    HPX_TEST_EQ(1, values[0]);
    HPX_TEST_EQ(2, values[1]);
    HPX_TEST_EQ(3, values[2]);
}

int hpx_main()
{
    {
        hpx::lcos::channel<std::vector<int>> buffer(hpx::find_here());
        buffer.register_as("my_vector_channel");

        hpx::future<void> f1 = hpx::async(&send_values, buffer);
        hpx::future<void> f2 = hpx::async(&receive_values);

        hpx::wait_all(f1, f2);

    }    // unregisters 'buffer'

    return hpx::finalize();
}

int main(int argc, char** argv)
{
    HPX_TEST_EQ(0, hpx::init(argc, argv));
    return hpx::util::report_errors();
}
#endif
