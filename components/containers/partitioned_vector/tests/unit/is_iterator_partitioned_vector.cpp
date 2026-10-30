//  Copyright (c) 2019 Austin McCartney
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/components/containers/partitioned_vector/partitioned_vector.hpp>
#endif

#include <concepts>
#include <iterator>
#include <memory>
#include <vector>

void is_iterator()
{
    using hpx::traits::is_iterator;

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    using iterator = hpx::segmented::vector_iterator<int, std::vector<int>>;
    HPX_TEST_MSG((is_iterator<iterator>::value), "hpx-specific iterator");
#endif
}

void is_forward_iterator()
{
#if !defined(HPX_COMPUTE_DEVICE_CODE)
    using iterator = hpx::segmented::vector_iterator<int, std::vector<int>>;
    HPX_TEST_MSG((std::forward_iterator<iterator>), "hpx-specific iterator");
#endif
}

void is_random_access_local_iterator()
{
#if !defined(HPX_COMPUTE_DEVICE_CODE)
    using data_type = std::vector<int>;
    using iterator = hpx::segmented::local_raw_vector_iterator<int, data_type,
        data_type::iterator>;
    using const_iterator = hpx::segmented::const_local_raw_vector_iterator<int,
        data_type, data_type::const_iterator>;
    static_assert(std::random_access_iterator<iterator>);
    static_assert(std::random_access_iterator<const_iterator>);
    static_assert(std::same_as<std::iter_reference_t<iterator>, int&>);
    static_assert(
        std::same_as<std::iter_reference_t<const_iterator>, int const&>);

    data_type data{1, 2, 3};
    iterator first(data.begin(), {});
    const_iterator cfirst(data.cbegin(), {});
    HPX_TEST_EQ(std::addressof(first[1]), std::addressof(data[1]));
    HPX_TEST_EQ(std::addressof(cfirst[1]), std::addressof(data[1]));
    first[1] = 42;
    HPX_TEST_EQ(data[1], 42);
    HPX_TEST_EQ(cfirst[1], 42);
#endif
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        is_iterator();
        is_forward_iterator();
        is_random_access_local_iterator();
    }

    return hpx::util::report_errors();
}
