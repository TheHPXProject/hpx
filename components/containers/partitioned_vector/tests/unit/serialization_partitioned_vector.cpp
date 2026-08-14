//  Copyright (c) 2017 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>

#include <hpx/include/parallel_fill.hpp>
#include <hpx/include/partitioned_vector.hpp>

#include <hpx/modules/naming.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <numeric>
#include <vector>

// partitioned_vector<int> and partitioned_vector<double> are predefined in the
// partitioned_vector module
#if defined(HPX_HAVE_STATIC_LINKING)
HPX_REGISTER_PARTITIONED_VECTOR(double)
#endif

using ulong = unsigned long;

HPX_REGISTER_PARTITIONED_VECTOR(ulong)
HPX_REGISTER_PARTITIONED_VECTOR(long)
HPX_REGISTER_PARTITIONED_VECTOR(unsigned)

template <typename T>
void test(T minval, T maxval)
{
    {
        std::vector<char> buffer;

        std::size_t sz = static_cast<std::size_t>(maxval - minval);

        hpx::partitioned_vector<T> os(sz);
        os.register_as("test_vector");
        hpx::fill(hpx::execution::par, std::begin(os), std::end(os), 42);

        hpx::serialization::detail::preprocess_container gather_size;
        hpx::serialization::output_archive archive(gather_size,
            hpx::serialization::archive_flags::disable_data_chunking);

        auto& split_gids = archive.get_extra_data<
            hpx::serialization::detail::preprocess_gid_types>();

        archive << os;

        hpx::serialization::output_archive oarchive(
            buffer, hpx::serialization::archive_flags::disable_data_chunking);

        auto& osplit_gids = oarchive.get_extra_data<
            hpx::serialization::detail::preprocess_gid_types>();
        osplit_gids.set_split_gids(split_gids.move_split_gids());

        oarchive << os;

        hpx::serialization::input_archive iarchive(buffer);

        hpx::partitioned_vector<T> is(os.size());
        hpx::fill(hpx::execution::par, std::begin(is), std::end(is), 0);

        iarchive >> is;

        HPX_TEST_EQ(os.size(), is.size());
        for (std::size_t i = 0; i != os.size(); ++i)
        {
            HPX_TEST_EQ(os[i], is[i]);
        }
    }
}

int main()
{
    test<int>((std::numeric_limits<int>::min)(),
        (std::numeric_limits<int>::min)() + 100);
    test<int>((std::numeric_limits<int>::max)() - 100,
        (std::numeric_limits<int>::max)());
    test<int>(-100, 100);
    test<unsigned>((std::numeric_limits<unsigned>::min)(),
        (std::numeric_limits<unsigned>::min)() + 100);
    test<unsigned>((std::numeric_limits<unsigned>::max)() - 100,
        (std::numeric_limits<unsigned>::max)());
    test<long>((std::numeric_limits<long>::min)(),
        (std::numeric_limits<long>::min)() + 100);
    test<long>((std::numeric_limits<long>::max)() - 100,
        (std::numeric_limits<long>::max)());
    test<long>(-100, 100);
    test<unsigned long>((std::numeric_limits<unsigned long>::min)(),
        (std::numeric_limits<unsigned long>::min)() + 100);
    test<unsigned long>((std::numeric_limits<unsigned long>::max)() - 100,
        (std::numeric_limits<unsigned long>::max)());
    test<double>((std::numeric_limits<double>::min)(),
        (std::numeric_limits<double>::min)() + 100);
    test<double>(-100, 100);

    return hpx::util::report_errors();
}

#endif
