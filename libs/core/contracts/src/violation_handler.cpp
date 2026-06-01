//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/contracts/config/defines.hpp>
#include <hpx/contracts/violation_handler.hpp>

#include <cstdlib>
#include <iostream>

namespace hpx::contracts {

    namespace detail {

        [[nodiscard]] violation_handler_t& get_handler() noexcept
        {
            static violation_handler_t handler = nullptr;
            return handler;
        }

    }    // namespace detail

    violation_handler_t set_violation_handler(
        violation_handler_t handler) noexcept
    {
        violation_handler_t old = detail::get_handler();
        detail::get_handler() = handler;
        return old;
    }

    violation_handler_t get_violation_handler() noexcept
    {
        return detail::get_handler();
    }

    void default_violation_handler(violation_info const& info)
    {
        char const* kind_str = nullptr;
        switch (info.kind)
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
        }

        std::cerr << info.location << ": Contract " << kind_str << " '"
                  << info.condition << "' violated\n";

#if !defined(HPX_HAVE_CONTRACTS_MODE) || HPX_HAVE_CONTRACTS_MODE != 2
        // abort in ENFORCE (and by default); continue in OBSERVE
        std::abort();
#endif
    }

    void invoke_violation_handler(violation_info const& info)
    {
        violation_handler_t handler = detail::get_handler();
        if (handler == nullptr)
            default_violation_handler(info);
        else
            handler(info);
    }

}    // namespace hpx::contracts
