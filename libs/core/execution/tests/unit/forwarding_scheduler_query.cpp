//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

namespace mylib {

    // Opt into forwarding_query by inheriting from forwarding_query_t
    inline constexpr struct non_query_t final
      : hpx::execution::experimental::forwarding_query_t
    {
    } non_query{};

}    // namespace mylib

int main()
{
    static_assert(hpx::execution::experimental::forwarding_query(
                      mylib::non_query) == true,
        "non_query inherits forwarding_query_t so returns true");

    return hpx::util::report_errors();
}
