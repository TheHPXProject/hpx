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

#include <iterator>
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

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        is_iterator();
        is_forward_iterator();
    }

    return hpx::util::report_errors();
}
