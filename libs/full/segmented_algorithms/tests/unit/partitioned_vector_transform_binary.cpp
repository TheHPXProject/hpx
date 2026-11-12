//  Copyright (c) 2017 Ajai V George
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/components/containers/partitioned_vector/partitioned_vector.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_component.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <string>
#include <vector>

#include "test_transform_binary.hpp"

using size_type = std::size_t;
HPX_REGISTER_PARTITIONED_VECTOR(size_type)

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    transform_binary_tests<int, int, double>(localities);

    auto layout = hpx::container_layout(localities);
    hpx::partitioned_vector<std::string> input1(8, std::string("abc"), layout);
    hpx::partitioned_vector<std::string> input2(8, std::string("de"), layout);
    hpx::partitioned_vector<std::size_t> output(8, layout);

    hpx::transform(hpx::execution::par, input1.begin(), input1.end(),
        input2.begin(), output.begin(),
        [](std::string const& first, std::string const& second) {
            return first.size() + second.size();
        });
    for (auto const& length : output)
    {
        HPX_TEST_EQ(length, std::size_t(5));
    }

    return hpx::util::report_errors();
}
#endif
