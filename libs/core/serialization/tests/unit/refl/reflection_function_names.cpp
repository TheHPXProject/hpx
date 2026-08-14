//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <string>

namespace app {
    int compute(double x, double y)
    {
        return (int) (x + y);
    }
    void broadcast(int n) noexcept
    {
        (void) n;
    }
    double transform(float x, int n)
    {
        return x * n;
    }
}    // namespace app

namespace app::nested {
    int deep_func(int x)
    {
        return x * 2;
    }
}    // namespace app::nested

namespace outer::inner {
    void my_func(int x) noexcept
    {
        (void) x;
    }
}    // namespace outer::inner

int main()
{
    using hpx::serialization::detail::scope_builder;

    // Simple namespace function
    {
        constexpr auto name = scope_builder<^^app::compute>::value;
        HPX_TEST_EQ(
            std::string(name.data, name.size), std::string("app::compute"));
    }

    // Void return noexcept function
    {
        constexpr auto name = scope_builder<^^app::broadcast>::value;
        HPX_TEST_EQ(
            std::string(name.data, name.size), std::string("app::broadcast"));
    }

    // Multiple parameters
    {
        constexpr auto name = scope_builder<^^app::transform>::value;
        HPX_TEST_EQ(
            std::string(name.data, name.size), std::string("app::transform"));
    }

    // Nested namespace inline
    {
        constexpr auto name = scope_builder<^^app::nested::deep_func>::value;
        HPX_TEST_EQ(std::string(name.data, name.size),
            std::string("app::nested::deep_func"));
    }

    // Nested namespace traditional
    {
        constexpr auto name = scope_builder<^^outer::inner::my_func>::value;
        HPX_TEST_EQ(std::string(name.data, name.size),
            std::string("outer::inner::my_func"));
    }

    return hpx::util::report_errors();
}
