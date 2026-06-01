//  Copyright (c) 2025-2026 The STE||AR-Group
//  Copyright (c) 2025 Alexandros Papadakis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file
/// \page HPX_PRE, HPX_POST, HPX_CONTRACT_ASSERT
/// \headerfile hpx/contracts.hpp
///
/// This header provides C++ contracts support for HPX with intelligent fallback
/// behavior when contracts are not available.
///
/// ## API Reference:
/// - **HPX_PRE(condition)**: Precondition contracts (declaration specifier in
///   C++26; no-op in fallback mode)
/// - **HPX_POST(condition)**: Postcondition contracts (declaration specifier
///   in C++26; no-op in fallback mode)
/// - **HPX_CONTRACT_ASSERT(condition)**: Contract assertions (always active;
///   dispatches on HPX_WITH_CONTRACTS_MODE in fallback mode)
///
/// ## Configuration:
/// Enable native contracts with: `cmake -DHPX_WITH_CONTRACTS=ON -DCMAKE_CXX_STANDARD=26`
/// Set fallback mode with: `cmake -DHPX_WITH_CONTRACTS_MODE=ENFORCE|OBSERVE|IGNORE`
///
/// See docs/index.rst for comprehensive usage guide.

#pragma once

#include <hpx/config.hpp>
#include <hpx/contracts/config/defines.hpp>
#include <hpx/assert.hpp>

#if defined(HPX_HAVE_CXX26_CONTRACTS)

// Native C++26 contracts mode
#define HPX_PRE(x) pre(x)
#define HPX_CONTRACT_ASSERT(x) contract_assert(x)
#define HPX_POST(x) post(x)

#if defined(HPX_CONTRACTS_HAVE_ASSERTS_AS_CONTRACT_ASSERTS)
// Override HPX_ASSERT to use contract assertions
#undef HPX_ASSERT
#define HPX_ASSERT(x) contract_assert(x)
#endif

#else    // fallback mode

// HPX_PRE and HPX_POST are declaration specifiers in C++26 and cannot be
// replicated as statement macros without changing call sites. Keep them as
// no-ops in fallback regardless of mode.
#define HPX_PRE(x)
#define HPX_POST(x)

#if defined(HPX_HAVE_CONTRACTS_MODE) &&                                        \
    HPX_HAVE_CONTRACTS_MODE == 3    // IGNORE

#define HPX_CONTRACT_ASSERT(x)

#else    // ENFORCE (0) or OBSERVE (1): runtime checking via violation handler

#include <hpx/contracts/violation_handler.hpp>

// clang-format off
#define HPX_CONTRACT_ASSERT(x)                                                 \
    do {                                                                       \
        if (!(x))                                                              \
            ::hpx::contracts::invoke_violation_handler(                       \
                {::hpx::contracts::contract_kind::assertion, #x,              \
                    HPX_CURRENT_SOURCE_LOCATION()});                           \
    } while (false)
// clang-format on

#endif    // HPX_HAVE_CONTRACTS_MODE

#endif    // HPX_HAVE_CXX26_CONTRACTS
