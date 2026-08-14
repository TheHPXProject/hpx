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
/// Enable native contracts with:
///     `cmake -DHPX_WITH_CONTRACTS=ON -DCMAKE_CXX_STANDARD=26`
/// Set fallback mode with:
///     `cmake -DHPX_WITH_CONTRACTS_MODE=ENFORCE|QUICK_ENFORCE|OBSERVE|IGNORE`

// Don't report missing #include for HPX_ASSERT macro
// hpxinspect:noinclude:HPX_ASSERT

#pragma once

#include <hpx/config.hpp>
#include <hpx/contracts/config/defines.hpp>
#include <hpx/assertion/macros.hpp>

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
    HPX_HAVE_CONTRACTS_MODE == HPX_HAVE_CONTRACTS_MODE_IGNORE

#define HPX_CONTRACT_ASSERT(x)

#else    // ENFORCE, QUICK_ENFORCE or OBSERVE: runtime checking

#include <hpx/modules/preprocessor.hpp>

// https://eel.is/c%2B%2Bdraft/basic.contract.eval#7 states:
//
// A contract violation occurs when:
// - (7.1) expr is false,
// - (7.2) the evaluation of the predicate exits via an exception,
// - ...

#define HPX_CONTRACT_ASSERT(expr)                                              \
    do                                                                         \
    {                                                                          \
        ::hpx::contracts::detection_mode mode =                                \
            ::hpx::contracts::detection_mode::unknown;                         \
        try                                                                    \
        {                                                                      \
            if (!!(expr))                                                      \
                break;                                                         \
            mode = ::hpx::contracts::detection_mode::predicate_false;          \
        }                                                                      \
        catch (...)                                                            \
        {                                                                      \
            mode = ::hpx::contracts::detection_mode::evaluation_exception;     \
        }                                                                      \
                                                                               \
        ::hpx::contracts::invoke_contract_violation_handler(                   \
            ::hpx::contracts::detail::construct_contract_violation(            \
                mode, HPX_PP_STRINGIZE(expr)));                                \
                                                                               \
    } while (false) /**/

#endif    // HPX_HAVE_CONTRACTS_MODE

#endif    // HPX_HAVE_CXX26_CONTRACTS
