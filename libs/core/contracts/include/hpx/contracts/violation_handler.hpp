//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/contracts/config/defines.hpp>

#if defined(HPX_HAVE_CXX26_CONTRACTS)

#include <contracts>

namespace hpx::contracts {

    using assertion_kind = std::contracts::assertion_kind;
    using evaluation_semantic = std::contracts::evaluation_semantic;
    using detection_mode = std::contracts::detection_mode;
    using contract_violation = std::contracts::contract_violation;
}    // namespace hpx::contracts

#else
// Compile contracts fallback support if C++26 contracts are not available.

#include <hpx/assert.hpp>
#include <hpx/contracts/macros.hpp>

#include <cstdint>

namespace hpx::contracts {

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT enum class assertion_kind : std::uint8_t {
        unknown = 0,
        pre = 1,
        post = 2,
        assertion = 3
    };

    HPX_CXX_CORE_EXPORT enum class evaluation_semantic : std::uint8_t {
        unknown = 0,
        ignore = HPX_HAVE_CONTRACTS_MODE_IGNORE,
        observe = HPX_HAVE_CONTRACTS_MODE_OBSERVE,
        enforce = HPX_HAVE_CONTRACTS_MODE_ENFORCE,
        quick_enforce = HPX_HAVE_CONTRACTS_MODE_QUICK_ENFORCE
    };

    HPX_CXX_CORE_EXPORT enum class detection_mode : std::uint8_t {
        unknown = 0,
        predicate_false = 1,
        evaluation_exception = 2
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct contract_violation;

    namespace detail {

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT contract_violation
        construct_contract_violation(detection_mode mode, char const* condition,
            hpx::source_location const& location =
                HPX_CURRENT_SOURCE_LOCATION()) noexcept;
    }    // namespace detail

    struct contract_violation
    {
        contract_violation() = default;
        ~contract_violation() = default;

        contract_violation(contract_violation const&) = delete;
        contract_violation& operator=(contract_violation const&) = delete;

        assertion_kind kind() const noexcept
        {
            return kind_;
        }

        source_location location() const noexcept
        {
            return location_;
        }

        static evaluation_semantic semantic() noexcept
        {
            return static_cast<evaluation_semantic>(HPX_HAVE_CONTRACTS_MODE);
        }

        contracts::detection_mode detection_mode() const noexcept
        {
            return mode_;
        }

        static bool is_terminating() noexcept
        {
            return semantic() == evaluation_semantic::enforce ||
                semantic() == evaluation_semantic::quick_enforce;
        }

        HPX_CORE_EXPORT char const* comment() const noexcept;

        char const* condition() const noexcept
        {
            return condition_;
        }

    private:
        friend HPX_CORE_EXPORT contract_violation
        detail::construct_contract_violation(contracts::detection_mode mode,
            char const* condition,
            hpx::source_location const& location) noexcept;

        HPX_CORE_EXPORT contract_violation(contracts::detection_mode mode,
            char const* condition,
            hpx::source_location const& location) noexcept;

    private:
        assertion_kind kind_ = assertion_kind::unknown;
        contracts::detection_mode mode_ = contracts::detection_mode::unknown;
        char const* condition_ = nullptr;
        hpx::source_location location_ = {};
    };

    HPX_CXX_CORE_EXPORT using violation_handler_t =
        void (*)(contract_violation const&);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT violation_handler_t
    set_violation_handler(violation_handler_t handler) noexcept;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT violation_handler_t
    get_violation_handler() noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void invoke_contract_violation_handler(
        contract_violation const& info);
}    // namespace hpx::contracts

#endif
