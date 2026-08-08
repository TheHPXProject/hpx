//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)

#include <hpx/execution/traits/vector_pack_alignment_size.hpp>
#include <hpx/execution/traits/vector_pack_load_store.hpp>
#include <hpx/execution/traits/vector_pack_type.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <numeric>
#include <vector>

namespace traits = hpx::parallel::traits;

template <typename T>
void test_unaligned_load_distinct_lanes()
{
    using pack_type = typename traits::vector_pack_type<T>::type;
    constexpr std::size_t pack_size = traits::vector_pack_size_v<pack_type>;

    // Buffer is twice the pack size so the load at offset 1 is genuinely
    // unaligned for any reasonable native alignment.
    std::vector<T> buf(pack_size * 2);
    std::iota(buf.begin(), buf.end(), static_cast<T>(1));

    {
        T const* p = buf.data();
        pack_type v(traits::vector_pack_load<pack_type, T>::unaligned(p));
        for (std::size_t i = 0; i < pack_size; ++i)
        {
            HPX_TEST_EQ(static_cast<T>(v[i]), static_cast<T>(i + 1));
        }
    }

    {
        T const* p = buf.data() + 1;
        pack_type v(traits::vector_pack_load<pack_type, T>::unaligned(p));
        for (std::size_t i = 0; i < pack_size; ++i)
        {
            HPX_TEST_EQ(static_cast<T>(v[i]), static_cast<T>(i + 2));
        }
    }
}

template <typename T>
void test_unaligned_store_distinct_lanes()
{
    using pack_type = typename traits::vector_pack_type<T>::type;
    constexpr std::size_t pack_size = traits::vector_pack_size_v<pack_type>;

    std::vector<T> src(pack_size);
    std::iota(src.begin(), src.end(), static_cast<T>(10));

    T const* sp = src.data();
    pack_type v(traits::vector_pack_load<pack_type, T>::unaligned(sp));

    std::vector<T> dst(pack_size * 2, static_cast<T>(0));
    T* dp = dst.data() + 1;
    traits::vector_pack_store<pack_type, T>::unaligned(v, dp);

    HPX_TEST_EQ(dst[0], static_cast<T>(0));
    for (std::size_t i = 0; i < pack_size; ++i)
    {
        HPX_TEST_EQ(dst[i + 1], static_cast<T>(i + 10));
    }
}

int main()
{
    test_unaligned_load_distinct_lanes<int>();
    test_unaligned_load_distinct_lanes<float>();
    test_unaligned_load_distinct_lanes<double>();

    test_unaligned_store_distinct_lanes<int>();
    test_unaligned_store_distinct_lanes<float>();
    test_unaligned_store_distinct_lanes<double>();

    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
