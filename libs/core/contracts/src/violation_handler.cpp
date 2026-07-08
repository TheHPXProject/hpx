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
#include <string>

namespace hpx::contracts {

    namespace {

        char const* get_contract_violation_kind_name(
            assertion_kind const kind) noexcept
        {
            auto* kind_str = "unknown";
            switch (kind)
            {
            case assertion_kind::pre:
                kind_str = "precondition";
                break;

            case assertion_kind::post:
                kind_str = "postcondition";
                break;

            case assertion_kind::assertion:
                kind_str = "assertion";
                break;

            case assertion_kind::unknown:
                HPX_ASSERT(false);
            }
            return kind_str;
        }

        char const* get_contracts_violation_semantic_name(
            evaluation_semantic const semantic) noexcept
        {
            auto* semantic_str = "unknown";
            switch (semantic)
            {
            case evaluation_semantic::enforce:
                semantic_str = "enforce";
                break;

            case evaluation_semantic::quick_enforce:
                semantic_str = "quick_enforce";
                break;

            case evaluation_semantic::observe:
                semantic_str = "observe";
                break;

            case evaluation_semantic::ignore:
                semantic_str = "ignore";
                break;

            case evaluation_semantic::unknown:
                HPX_ASSERT(false);
            }
            return semantic_str;
        }

        char const* get_contracts_violation_detection_mode_name(
            contracts::detection_mode const mode) noexcept
        {
            auto* detection_mode_str = "unknown";
            switch (mode)
            {
            case contracts::detection_mode::predicate_false:
                detection_mode_str = "predicate_false";
                break;

            case contracts::detection_mode::evaluation_exception:
                detection_mode_str = "evaluation_exception";
                break;

            case contracts::detection_mode::unknown:
                HPX_ASSERT(false);
            }
            return detection_mode_str;
        }

        [[nodiscard]] violation_handler_t& get_handler() noexcept
        {
            static violation_handler_t handler = nullptr;
            return handler;
        }

#if !defined(HPX_HAVE_CONTRACTS_MODE) ||                                       \
    HPX_HAVE_CONTRACTS_MODE != HPX_HAVE_CONTRACTS_MODE_OBSERVE
        [[noreturn]]
#endif
        void default_violation_handler(
            [[maybe_unused]] contract_violation const& info)
        {
            // no diagnostics are generated in quick_enforce mode
#if !defined(HPX_HAVE_CONTRACTS_MODE) ||                                       \
    HPX_HAVE_CONTRACTS_MODE != HPX_HAVE_CONTRACTS_MODE_QUICK_ENFORCE
            std::cerr << info.comment() << std::flush;
#endif

            // abort in ENFORCE, QUICK_ENFORCE (and by default); continue in
            // OBSERVE
#if !defined(HPX_HAVE_CONTRACTS_MODE) ||                                       \
    HPX_HAVE_CONTRACTS_MODE != HPX_HAVE_CONTRACTS_MODE_OBSERVE
            // abort in ENFORCE (and by default); continue in OBSERVE
            std::abort();
#endif
        }
    }    // namespace

    ///////////////////////////////////////////////////////////////////////////
    char const* contract_violation::comment() const noexcept
    {
        thread_local std::string comment;

        try
        {
            thread_local bool handling_violation = false;

            std::ostringstream strm;
            if (handling_violation)
            {
                strm << "Trying to handle contracts violation while handling "
                        "another contracts violation!\n";
            }

            handling_violation = true;

            char const* kind_str = get_contract_violation_kind_name(kind());
            char const* semantic_str =
                get_contracts_violation_semantic_name(semantic());
            char const* detection_mode_str =
                get_contracts_violation_detection_mode_name(detection_mode());

            strm << location() << ": Contract " << kind_str << " '"
                 << condition_ << "' violated, semantic: " << semantic_str
                 << ", detection mode: " << detection_mode_str << ".\n";

            comment = strm.str();
        }
        catch (...)
        {
            return "Exception caught while generating diagnostics for "
                   "contracts violation!\n";
        }

        return comment.c_str();
    }

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

    void invoke_contract_violation_handler(contract_violation const& info)
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

    ////////////////////////////////////////////////////////////////////////////
    contract_violation::contract_violation(contracts::detection_mode const mode,
        char const* condition, hpx::source_location const& location) noexcept
      : kind_(assertion_kind::assertion)
      , mode_(mode)
      , condition_(condition)
      , location_(location)
    {
    }

    namespace detail {

        contract_violation construct_contract_violation(
            contracts::detection_mode const mode, char const* condition,
            hpx::source_location const& location) noexcept
        {
            return {mode, condition, location};
        }
    }    // namespace detail
}    // namespace hpx::contracts

#endif
