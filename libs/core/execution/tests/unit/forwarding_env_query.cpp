//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

namespace mylib {

    struct derived_forwarding_query_t
      : hpx::execution::experimental::forwarding_query_t
    {
    } derived_forwarding_query{};

    inline constexpr struct non_query_t final
      : hpx::functional::tag<non_query_t>
    {
        friend constexpr auto tag_invoke(
            hpx::execution::experimental::forwarding_query_t,
            non_query_t) noexcept
        {
            return true;
        }
    } non_query{};

}    // namespace mylib

int main()
{
    static_assert(hpx::execution::experimental::forwarding_query(
                      mylib::non_query) == true,
        "non_query CPO is user implemented that returns true");
    // P2300R8: "forwarding_query(execution::get_scheduler) is a core constant
    // expression and has value true."
    static_assert(hpx::execution::experimental::forwarding_query(
                      hpx::execution::experimental::get_scheduler) == true,
        "get scheduler is a forwarding query");

    static_assert(hpx::execution::experimental::forwarding_query(
                      mylib::derived_forwarding_query) == true,
        "derived_forwarding_query is a forwarding query");

    return hpx::util::report_errors();
}
