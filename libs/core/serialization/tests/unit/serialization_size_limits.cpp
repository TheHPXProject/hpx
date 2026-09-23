//  Copyright (c) 2026 Abhishek Kumar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Sizes are stored in an archive as std::uint64_t so that the format does not
// depend on the width of std::size_t at either end. These check what happens
// when such a size reaches a host that cannot represent it.

#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/serialization/detail/to_size.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>

void test_sizes_that_fit()
{
    constexpr std::size_t max_size = (std::numeric_limits<std::size_t>::max)();

    HPX_TEST_EQ(hpx::serialization::detail::to_size(0), std::size_t(0));
    HPX_TEST_EQ(hpx::serialization::detail::to_size(1), std::size_t(1));
    HPX_TEST_EQ(hpx::serialization::detail::to_size(4096), std::size_t(4096));
    HPX_TEST_EQ(hpx::serialization::detail::to_size(
                    static_cast<std::uint64_t>(max_size)),
        max_size);

    HPX_TEST_EQ(hpx::serialization::detail::clamp_to_size(0), std::size_t(0));
    HPX_TEST_EQ(
        hpx::serialization::detail::clamp_to_size(4096), std::size_t(4096));
    HPX_TEST_EQ(hpx::serialization::detail::clamp_to_size(
                    (std::numeric_limits<std::uint64_t>::max)()),
        max_size);
}

void test_sizes_that_do_not_fit()
{
    // on a host where std::size_t is 64 bits wide there is no std::uint64_t
    // value that does not fit, so there is nothing to reject
    if constexpr (sizeof(std::size_t) < sizeof(std::uint64_t))
    {
        constexpr std::uint64_t too_large =
            static_cast<std::uint64_t>(
                (std::numeric_limits<std::size_t>::max)()) +
            1;

        // a length has to be refused rather than truncated
        bool caught_exception = false;
        try
        {
            (void) hpx::serialization::detail::to_size(too_large);
            HPX_TEST(false);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::serialization_error);
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);

        // a limit saturates instead, as one that is larger than this host can
        // represent simply means the limit is never reached
        HPX_TEST_EQ(hpx::serialization::detail::clamp_to_size(too_large),
            (std::numeric_limits<std::size_t>::max)());
    }
}

int main()
{
    test_sizes_that_fit();
    test_sizes_that_do_not_fit();

    return hpx::util::report_errors();
}
