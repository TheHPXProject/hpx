//  Copyright (c) 2026 Animesh Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: HPX_PRE rejects update < 0 in hpx::latch::arrive_and_wait.
// Requires the implementation to report the predicate text through comment()
// (as GCC does), so unrelated assertions cannot make this test pass.
// Exit: 0 for the expected violation, 2 for another, 1 if the call returns.

#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>

#include <contracts>
#include <cstddef>
#include <cstdlib>
#include <string_view>

void handle_contract_violation(std::contracts::contract_violation const& v)
{
    std::string_view const predicate = v.comment();

    HPX_TEST(v.kind() == std::contracts::assertion_kind::pre);
    HPX_TEST(predicate.find("update >= 0") != std::string_view::npos);

    std::_Exit(hpx::util::report_errors() == 0 ? 0 : 2);
}

int main()
{
    hpx::latch l(2);

    l.arrive_and_wait(std::ptrdiff_t(-1));

    return 1;
}
