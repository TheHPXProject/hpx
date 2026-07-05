//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX26_CONTRACTS)

int main()
{
    return 0;
}

#else

#include <hpx/contracts.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <string>

namespace {

    std::atomic<int> handler_call_count = 0;

    void capturing_handler(hpx::contracts::contract_violation const& info)
    {
        HPX_TEST(info.condition() != nullptr);
        HPX_TEST_EQ(std::string(info.condition()), std::string("1 == 2"));

        HPX_TEST(info.kind() == hpx::contracts::assertion_kind::assertion);
        HPX_TEST(info.location().line() > 0);

        ++handler_call_count;

        // deliberately does not abort so the test can continue
    }
}    // namespace

int main()
{
    hpx::contracts::set_violation_handler(capturing_handler);

    HPX_CONTRACT_ASSERT(1 == 2);    // NOLINT: intentional false assertion

    HPX_TEST_EQ(handler_call_count.load(), 1);

    return hpx::util::report_errors();
}

#endif
