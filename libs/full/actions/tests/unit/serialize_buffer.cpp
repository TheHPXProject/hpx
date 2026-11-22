//  Copyright (c) 2014-2022 Hartmut Kaiser
//  Copyright (c) 2015 Andreas Schaefer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <memory>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
typedef hpx::serialization::serialize_buffer<char> buffer_plain_type;

buffer_plain_type bounce_plain(buffer_plain_type const& receive_buffer)
{
    return receive_buffer;
}
HPX_PLAIN_ACTION(bounce_plain)

HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    buffer_plain_type, serialization_buffer_char)
HPX_REGISTER_BASE_LCO_WITH_VALUE(buffer_plain_type, serialization_buffer_char)

///////////////////////////////////////////////////////////////////////////////
template <typename Buffer, typename Action>
void test(hpx::id_type dest, char* send_buffer, std::size_t size)
{
    using buffer_type = Buffer;
    buffer_type recv_buffer;

    std::vector<hpx::future<buffer_type>> recv_buffers;
    recv_buffers.reserve(10);

    Action act;
    for (std::size_t j = 0; j != 10; ++j)
    {
        recv_buffers.push_back(hpx::async(
            act, dest, buffer_type(send_buffer, size, buffer_type::reference)));
    }
    hpx::wait_all(recv_buffers);

    for (hpx::future<buffer_type>& f : recv_buffers)
    {
        buffer_type b = f.get();
        HPX_TEST_EQ(b.size(), size);
        HPX_TEST_EQ(0, memcmp(b.data(), send_buffer, size));
    }
}

template <typename Allocator>
void test_stateful_allocator(hpx::id_type dest, char* send_buffer,
    std::size_t size, Allocator const& alloc)
{
    using buffer_type = buffer_plain_type;
    buffer_type recv_buffer;

    std::vector<hpx::future<buffer_type>> recv_buffers;
    recv_buffers.reserve(10);

    bounce_plain_action act;
    for (std::size_t j = 0; j != 10; ++j)
    {
        recv_buffers.push_back(hpx::async(act, dest,
            buffer_type(send_buffer, size, buffer_type::reference, alloc)));
    }
    hpx::wait_all(recv_buffers);

    for (hpx::future<buffer_type>& f : recv_buffers)
    {
        buffer_type b = f.get();
        HPX_TEST_EQ(b.size(), size);
        HPX_TEST_EQ(0, memcmp(b.data(), send_buffer, size));
    }
}

template <typename T>
void test_fixed_size_initialization_for_persistent_buffers(std::size_t max_size)
{
    for (std::size_t size = 1; size <= max_size; size *= 2)
    {
        std::vector<T> send_vec;
        std::vector<T> recv_vec;
        send_vec.reserve(size);
        for (std::size_t i = 0; i < size; ++i)
        {
            send_vec.push_back(static_cast<T>(size - i));
        }

        hpx::serialization::serialize_buffer<T> send_buffer(size);
        hpx::serialization::serialize_buffer<T> recv_buffer;
        std::copy(send_vec.begin(), send_vec.end(), send_buffer.begin());
        recv_buffer = send_buffer;

        std::copy(recv_buffer.begin(), recv_buffer.end(),
            std::back_inserter(recv_vec));
        HPX_TEST(send_vec == recv_vec);
    }
}

template <typename T>
void test_initialization_from_vector(std::size_t max_size)
{
    for (std::size_t size = 1; size <= max_size; size *= 2)
    {
        std::vector<T> send_vec;
        std::vector<T> recv_vec;
        send_vec.reserve(size);
        for (std::size_t i = 0; i < size; ++i)
        {
            send_vec.push_back(size - i);
        }

        // default init mode is "copy"
        hpx::serialization::serialize_buffer<T> send_buffer(
            send_vec[0], send_vec.size());
        hpx::serialization::serialize_buffer<T> recv_buffer;
        std::copy(send_vec.begin(), send_vec.end(), send_buffer.begin());
        recv_buffer = send_buffer;

        std::copy(recv_buffer.begin(), recv_buffer.end(),
            std::back_inserter(recv_vec));
        HPX_TEST(send_vec == recv_vec);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
#if defined(HPX_DEBUG)
    std::size_t const max_size = 1 << 10;
#else
    std::size_t const max_size = 1 << 20;
#endif

    std::unique_ptr<char[]> send_buffer(new char[max_size]);

    for (hpx::id_type const& loc : hpx::find_all_localities())
    {
        for (std::size_t size = 1; size <= max_size; size *= 2)
        {
            test<buffer_plain_type, bounce_plain_action>(
                loc, send_buffer.get(), size);
            test_stateful_allocator(
                loc, send_buffer.get(), size, std::allocator<char>());
        }
    }

    for (std::size_t size = 1; size <= max_size; size *= 2)
    {
        test_fixed_size_initialization_for_persistent_buffers<int>(size);
        test_fixed_size_initialization_for_persistent_buffers<char>(size);
        test_fixed_size_initialization_for_persistent_buffers<float>(size);
        test_fixed_size_initialization_for_persistent_buffers<double>(size);
    }

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return 0;
}
#endif
