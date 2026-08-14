//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

namespace ex = hpx::execution::experimental;

namespace mylib {

    // Opt into forwarding_query by inheriting from forwarding_query_t
    // (P2300 / stdexec member-style customization).
    inline constexpr struct query_t final : ex::forwarding_query_t
    {
    } query{};

    inline constexpr struct non_query_t
    {
    } non_query{};

}    // namespace mylib

int main()
{
    static_assert(ex::forwarding_query(mylib::query) == true,
        "query CPO inherits forwarding_query_t so returns true");

    static_assert(ex::forwarding_query(mylib::non_query) == false,
        "non_query has no forwarding_query customization; default is false");

    static_assert(ex::forwarding_query(
                      ex::get_completion_scheduler<ex::set_value_t>) == true,
        "invokes CPO specialization that returns true");

    static_assert(ex::forwarding_query(
                      ex::get_completion_scheduler<ex::set_error_t>) == true,
        "invokes CPO specialization that returns true");

    static_assert(ex::forwarding_query(
                      ex::get_completion_scheduler<ex::set_stopped_t>) == true,
        "invokes CPO specialization that returns true");

    return hpx::util::report_errors();
}
