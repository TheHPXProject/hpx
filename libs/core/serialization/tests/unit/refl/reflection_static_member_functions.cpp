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
    struct compute_server
    {
        static int compute(double x, double y)
        {
            return (int) (x + y);
        }
        static void broadcast(int n) noexcept
        {
            (void) n;
        }
    };
}    // namespace app

namespace app::nested {
    struct rpc_server
    {
        static double transform(float x, int n)
        {
            return x * n;
        }
    };
}    // namespace app::nested

namespace outer::inner {
    struct helper_server
    {
        static int process(int x) noexcept
        {
            return x * 2;
        }
    };
}    // namespace outer::inner

int main()
{
    using hpx::serialization::detail::scope_builder;

    // Static member function in simple namespace
    {
        constexpr auto name =
            scope_builder<^^app::compute_server::compute>::value;
        HPX_TEST_EQ(std::string(name.data, name.size),
            std::string("app::compute_server::compute"));
    }

    // Void return noexcept static member function
    {
        constexpr auto name =
            scope_builder<^^app::compute_server::broadcast>::value;
        HPX_TEST_EQ(std::string(name.data, name.size),
            std::string("app::compute_server::broadcast"));
    }

    // Static member function in nested inline namespace
    {
        constexpr auto name =
            scope_builder<^^app::nested::rpc_server::transform>::value;
        HPX_TEST_EQ(std::string(name.data, name.size),
            std::string("app::nested::rpc_server::transform"));
    }

    // Static member function in traditional nested namespace
    {
        constexpr auto name =
            scope_builder<^^outer::inner::helper_server::process>::value;
        HPX_TEST_EQ(std::string(name.data, name.size),
            std::string("outer::inner::helper_server::process"));
    }

    return hpx::util::report_errors();
}
