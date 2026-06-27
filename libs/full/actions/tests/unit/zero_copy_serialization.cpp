//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>

#include <hpx/config/endian.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/post.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct data_buffer
{
    data_buffer()
      : flag_(false)
    {
    }
    explicit data_buffer(std::size_t size)
      : data_(size, 0)
      , flag_(false)
    {
    }

    std::vector<T> data_;
    bool flag_;

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        std::uint64_t size = data_.size();
        // clang-format off
        ar & size;
        ar & hpx::serialization::make_array(data_.data(), size);
        ar & flag_;
        // clang-format on
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        std::uint64_t size = 0;
        // clang-format off
        ar & size;
        data_.resize(size);
        ar & hpx::serialization::make_array(data_.data(), size);
        ar & flag_;
        // clang-format on
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

///////////////////////////////////////////////////////////////////////////////
int test_function1(hpx::serialization::serialize_buffer<double> const& /* b */)
{
    return 42;
}
HPX_PLAIN_ACTION(test_function1, test_action1)

int test_function2(hpx::serialization::serialize_buffer<double> const& /* b1 */,
    hpx::serialization::serialize_buffer<int> const& /* b2 */)
{
    return 42;
}
HPX_PLAIN_ACTION(test_function2, test_action2)

int test_function3(double /* d */,
    hpx::serialization::serialize_buffer<double> const& /* b1 */,
    std::string const& /* s */, int /* i */,
    hpx::serialization::serialize_buffer<int> const& /* b2 */)
{
    return 42;
}
HPX_PLAIN_ACTION(test_function3, test_action3)

int test_function4(data_buffer<double> const& /* b */)
{
    return 42;
}
HPX_PLAIN_ACTION(test_function4, test_action4)

std::size_t get_archive_size(hpx::parcelset::parcel const& p,
    std::uint32_t flags,
    std::vector<hpx::serialization::serialization_chunk>* chunks)
{
    // gather the required size for the archive
    hpx::serialization::detail::preprocess_container gather_size;
    hpx::serialization::output_archive archive(gather_size, flags, chunks);
    archive << p;
    return gather_size.size();
}

///////////////////////////////////////////////////////////////////////////////
void test_parcel_serialization(hpx::parcelset::parcel outp,
    std::uint32_t out_archive_flags, bool zero_copy)
{
    // serialize data
    std::vector<hpx::serialization::serialization_chunk> out_chunks;
    std::size_t arg_size = get_archive_size(
        outp, out_archive_flags, zero_copy ? &out_chunks : nullptr);
    std::vector<char> out_buffer;

    out_buffer.resize(arg_size + HPX_PARCEL_SERIALIZATION_OVERHEAD);

    {
        // create an output archive and serialize the parcel
        hpx::serialization::output_archive archive(
            out_buffer, out_archive_flags, zero_copy ? &out_chunks : nullptr);
        archive << outp;

        arg_size = archive.bytes_written();
    }

    out_buffer.resize(arg_size);

    // deserialize data
    hpx::parcelset::parcel inp;

    {
        // create an input archive and deserialize the parcel
        hpx::serialization::input_archive archive(
            out_buffer, arg_size, &out_chunks);

        archive >> inp;
    }

    // make sure the parcel has been de-serialized properly
    HPX_TEST_EQ(outp.source_id(), inp.source_id());
    HPX_TEST_EQ(outp.destination_locality(), inp.destination_locality());
    HPX_TEST_EQ(outp.start_time(), inp.start_time());

    HPX_TEST_EQ(outp.get_component_type(), inp.get_component_type());
    HPX_TEST_EQ(outp.get_action_name(), inp.get_action_name());
    HPX_TEST_EQ(static_cast<int>(outp.get_action_type()),
        static_cast<int>(inp.get_action_type()));

    HPX_TEST_EQ(outp.get_parent_locality_id(), inp.get_parent_locality_id());
    HPX_TEST_EQ(outp.get_parent_thread_id(), inp.get_parent_thread_id());
    HPX_TEST_EQ(outp.get_parent_thread_phase(), inp.get_parent_thread_phase());
    HPX_TEST_EQ(static_cast<int>(outp.get_thread_priority()),
        static_cast<int>(inp.get_thread_priority()));
    HPX_TEST_EQ(static_cast<int>(outp.get_thread_stacksize()),
        static_cast<int>(inp.get_thread_stacksize()));

    //// invoke action encapsulated in inp
    //naming::address const* inaddrs = pin.get_destination_addrs();
    //hpx::threads::thread_init_data data;
    //inact->get_thread_init_data(inaddrs[0].address_, data);
    //data.func(hpx::threads::thread_restart_state::signaled);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Action, typename T>
void test_normal_serialization(T& arg)
{
    hpx::id_type const here = hpx::find_here();
    hpx::naming::address addr(hpx::get_locality(),
        to_int(hpx::components::component_enum_type::invalid),
        (void*) &test_function1);

    // compose archive flags
    auto out_archive_flags = static_cast<std::uint32_t>(
        hpx::serialization::archive_flags::disable_data_chunking);
    if constexpr (hpx::endian::native == hpx::endian::big)
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_big);
    }
    else
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_little);
    }

    // create a parcel with/without continuation
    hpx::naming::gid_type dest = here.get_gid();
    hpx::parcelset::parcel outp(
        hpx::parcelset::detail::create_parcel::call(std::move(dest),
            std::move(addr), hpx::actions::typed_continuation<int>(here),
            Action(), hpx::launch::async, arg));

    outp.set_source_id(here);

    test_parcel_serialization(std::move(outp), out_archive_flags, false);
}

template <typename T1, typename T2>
void test_normal_serialization(T1& arg1, T2& arg2)
{
    hpx::id_type const here = hpx::find_here();
    hpx::naming::address addr(hpx::get_locality(),
        to_int(hpx::components::component_enum_type::invalid),
        (void*) &test_function2);

    // compose archive flags
    auto out_archive_flags = static_cast<std::uint32_t>(
        hpx::serialization::archive_flags::disable_data_chunking);
    if constexpr (hpx::endian::native == hpx::endian::big)
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_big);
    }
    else
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_little);
    }

    // create a parcel with/without continuation
    hpx::naming::gid_type dest = here.get_gid();
    hpx::parcelset::parcel outp(
        hpx::parcelset::detail::create_parcel::call(std::move(dest),
            std::move(addr), hpx::actions::typed_continuation<int>(here),
            test_action2(), hpx::launch::async, arg1, arg2));

    outp.set_source_id(here);

    test_parcel_serialization(std::move(outp), out_archive_flags, false);
}

template <typename T1, typename T2>
void test_normal_serialization(
    double d, T1& arg1, std::string const& s, int i, T2& arg2)
{
    hpx::id_type const here = hpx::find_here();
    hpx::naming::address addr(hpx::get_locality(),
        to_int(hpx::components::component_enum_type::invalid),
        (void*) &test_function2);

    // compose archive flags
    auto out_archive_flags = static_cast<std::uint32_t>(
        hpx::serialization::archive_flags::disable_data_chunking);
    if constexpr (hpx::endian::native == hpx::endian::big)
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_big);
    }
    else
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_little);
    }

    // create a parcel with/without continuation
    hpx::naming::gid_type dest = here.get_gid();
    hpx::parcelset::parcel outp(
        hpx::parcelset::detail::create_parcel::call(std::move(dest),
            std::move(addr), hpx::actions::typed_continuation<int>(here),
            test_action3(), hpx::launch::async, d, arg1, s, i, arg2));

    outp.set_source_id(here);

    test_parcel_serialization(std::move(outp), out_archive_flags, false);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Action, typename T>
void test_zero_copy_serialization(T& arg)
{
    hpx::id_type const here = hpx::find_here();
    hpx::naming::address addr(hpx::get_locality(),
        to_int(hpx::components::component_enum_type::invalid),
        (void*) &test_function1);

    // compose archive flags
    std::uint32_t out_archive_flags = 0U;
    if constexpr (hpx::endian::native == hpx::endian::big)
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_big);
    }
    else
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_little);
    }

    // create a parcel with/without continuation
    hpx::naming::gid_type dest = here.get_gid();
    hpx::parcelset::parcel outp(
        hpx::parcelset::detail::create_parcel::call(std::move(dest),
            std::move(addr), hpx::actions::typed_continuation<int>(here),
            Action(), hpx::launch::async, arg));

    outp.set_source_id(here);

    test_parcel_serialization(std::move(outp), out_archive_flags, true);
}

template <typename T1, typename T2>
void test_zero_copy_serialization(T1& arg1, T2& arg2)
{
    hpx::id_type const here = hpx::find_here();
    hpx::naming::address addr(hpx::get_locality(),
        to_int(hpx::components::component_enum_type::invalid),
        (void*) &test_function2);

    // compose archive flags
    std::uint32_t out_archive_flags = 0U;
    if constexpr (hpx::endian::native == hpx::endian::big)
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_big);
    }
    else
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_little);
    }

    // create a parcel with/without continuation
    hpx::naming::gid_type dest = here.get_gid();
    hpx::parcelset::parcel outp(
        hpx::parcelset::detail::create_parcel::call(std::move(dest),
            std::move(addr), hpx::actions::typed_continuation<int>(here),
            test_action2(), hpx::launch::async, arg1, arg2));

    outp.set_source_id(here);

    test_parcel_serialization(std::move(outp), out_archive_flags, true);
}

template <typename T1, typename T2>
void test_zero_copy_serialization(
    double d, T1& arg1, std::string const& s, int i, T2& arg2)
{
    hpx::id_type const here = hpx::find_here();
    hpx::naming::address addr(hpx::get_locality(),
        to_int(hpx::components::component_enum_type::invalid),
        (void*) &test_function2);

    // compose archive flags
    std::uint32_t out_archive_flags = 0U;
    if constexpr (hpx::endian::native == hpx::endian::big)
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_big);
    }
    else
    {
        out_archive_flags = out_archive_flags |
            static_cast<int>(hpx::serialization::archive_flags::endian_little);
    }

    // create a parcel with/without continuation
    hpx::naming::gid_type dest = here.get_gid();
    hpx::parcelset::parcel outp(
        hpx::parcelset::detail::create_parcel::call(std::move(dest),
            std::move(addr), hpx::actions::typed_continuation<int>(here),
            test_action3(), hpx::launch::async, d, arg1, s, i, arg2));

    outp.set_source_id(here);

    test_parcel_serialization(std::move(outp), out_archive_flags, true);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::size_t const size = 1;
    for (std::size_t i = 0; i != 20; ++i)
    {
        // create argument for action
        std::vector<double> data1;
        data1.resize(size << i);

        hpx::serialization::serialize_buffer<double> buffer1(data1.data(),
            data1.size(),
            hpx::serialization::serialize_buffer<double>::reference);

        test_normal_serialization<test_action1>(buffer1);
        test_zero_copy_serialization<test_action1>(buffer1);

        std::vector<int> data2;
        data2.resize(size << i);

        hpx::serialization::serialize_buffer<int> buffer2(data2.data(),
            data2.size(), hpx::serialization::serialize_buffer<int>::reference);

        test_normal_serialization(buffer1, buffer2);
        test_zero_copy_serialization(buffer1, buffer2);
        test_normal_serialization(42.0, buffer1, "42.0", 42, buffer2);
        test_zero_copy_serialization(42.0, buffer1, "42.0", 42, buffer2);

        data_buffer<double> buffer3(size << i);
        test_normal_serialization<test_action4>(buffer3);
        test_zero_copy_serialization<test_action4>(buffer3);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#endif
