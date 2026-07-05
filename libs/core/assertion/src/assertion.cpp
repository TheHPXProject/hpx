//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assertion/api.hpp>
#include <hpx/assertion/evaluate_assert.hpp>
#include <hpx/assertion/source_location.hpp>

#include <iostream>
#include <string>

namespace hpx::assertion {

    namespace {

        [[nodiscard]] assertion_handler& get_handler()
        {
            static assertion_handler handler = nullptr;
            return handler;
        }
    }    // namespace

    assertion_handler set_assertion_handler(assertion_handler const handler)
    {
        assertion_handler const old = get_handler();
        get_handler() = handler;
        return old;
    }

    namespace detail {

        void handle_assert(hpx::source_location const& loc, char const* expr,
            std::string const& msg) noexcept
        {
            if (get_handler() == nullptr)
            {
                std::cerr << loc << ": Assertion '" << expr << "' failed";
                if (!msg.empty())
                {
                    std::cerr << " (" << msg << ")\n";
                }
                else
                {
                    std::cerr << '\n';
                }
                std::abort();
            }
            get_handler()(loc, expr, msg);
        }
    }    // namespace detail
}    // namespace hpx::assertion
