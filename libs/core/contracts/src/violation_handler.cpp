//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/contracts/config/defines.hpp>

// Compile contracts fallback support if C++26 contracts are not available.
#if !defined(HPX_HAVE_CXX26_CONTRACTS)

#include <hpx/assert.hpp>
#include <hpx/contracts/violation_handler.hpp>

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace hpx::contracts {

    namespace {

        char const* get_contract_violation_name(contract_kind kind) noexcept
        {
            char const* kind_str = "unknown";
            switch (kind)
            {
            case contract_kind::pre:
                kind_str = "precondition";
                break;

            case contract_kind::post:
                kind_str = "postcondition";
                break;

            case contract_kind::assertion:
                kind_str = "assertion";
                break;

            default:
                HPX_ASSERT(false);
            }

            return kind_str;
        }

        [[nodiscard]] violation_handler_t& get_handler() noexcept
        {
            static violation_handler_t handler = nullptr;
            return handler;
        }

#if !defined(HPX_HAVE_CONTRACTS_MODE) || HPX_HAVE_CONTRACTS_MODE != 2
        [[noreturn]]
#endif
        void default_violation_handler(contract_violation const& info)
        {
            thread_local bool handling_violation = false;

            std::ostringstream strm;
            if (handling_violation)
            {
                strm << "Trying to handle contracts violation while handling "
                        "another contracts violation!\n";
            }

            handling_violation = true;

            char const* kind_str = get_contract_violation_name(info.kind);
            strm << info.location << ": Contract " << kind_str << " '"
                 << info.condition << "' violated";
            if (!info.message.empty())
            {
                strm << " (" << info.message << ")";
            }
            strm << ".\n";

            std::cerr << strm.str() << std::flush;

#if !defined(HPX_HAVE_CONTRACTS_MODE) || HPX_HAVE_CONTRACTS_MODE != 2
            // abort in ENFORCE (and by default); continue in OBSERVE
            std::abort();
#endif
        }
    }    // namespace

    violation_handler_t set_violation_handler(
        violation_handler_t const handler) noexcept
    {
        violation_handler_t const old = get_handler();
        get_handler() = handler;
        return old;
    }

    violation_handler_t get_violation_handler() noexcept
    {
        return get_handler();
    }

    void handle_contract_violation(contract_violation const& info)
    {
        if (violation_handler_t const handler = get_handler();
            handler == nullptr)
        {
            default_violation_handler(info);
        }
        else
        {
            handler(info);
        }
    }

}    // namespace hpx::contracts

#endif
