//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/contracts.hpp>
#include <hpx/contracts/violation_handler.hpp>
#include <hpx/modules/testing.hpp>

#include <cstring>

#if !defined(HPX_HAVE_CXX26_CONTRACTS) && HPX_CONTRACTS_MODE == 2

int main()
{
    return 0;
}

#else

namespace {
    hpx::contracts::violation_info captured{
        hpx::contracts::contract_kind::assertion, nullptr, {}};

    void capturing_handler(hpx::contracts::violation_info const& info)
    {
        captured = info;
    }
}    // namespace

int main()
{
    hpx::contracts::set_violation_handler(capturing_handler);

    HPX_CONTRACT_ASSERT(1 == 2);    // NOLINT: intentional false assertion

    HPX_TEST(captured.condition != nullptr);
    HPX_TEST(std::strstr(captured.condition, "1 == 2") != nullptr);
    HPX_TEST(captured.kind == hpx::contracts::contract_kind::assertion);
    HPX_TEST(captured.location.line() > 0);

    return hpx::util::report_errors();
}

#endif
