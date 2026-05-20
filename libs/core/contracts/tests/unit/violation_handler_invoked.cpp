//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/contracts.hpp>
#include <hpx/contracts/violation_handler.hpp>
#include <hpx/modules/testing.hpp>

#if !defined(HPX_HAVE_CXX26_CONTRACTS) && HPX_CONTRACTS_MODE == 2

// IGNORE mode: contracts are no-ops, nothing to test here.
int main()
{
    return 0;
}

#else

namespace {
    int handler_call_count = 0;
    hpx::contracts::contract_kind last_kind =
        hpx::contracts::contract_kind::assertion;

    void recording_handler(hpx::contracts::violation_info const& info)
    {
        ++handler_call_count;
        last_kind = info.kind;
        // deliberately does not abort so the test can continue
    }
}    // namespace

int main()
{
    hpx::contracts::set_violation_handler(recording_handler);

    HPX_CONTRACT_ASSERT(false);    // should invoke recording_handler

    HPX_TEST_EQ(handler_call_count, 1);
    HPX_TEST(last_kind == hpx::contracts::contract_kind::assertion);

    return hpx::util::report_errors();
}

#endif
