//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/async_base/launch_policy.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstdint>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    detail::async_policy const launch::async =
        detail::async_policy{threads::thread_priority::default_};
    detail::task_policy const launch::task =
        detail::task_policy{threads::thread_priority::default_};
    detail::fork_policy const launch::fork =
        detail::fork_policy{threads::thread_priority::default_};
    detail::sync_policy const launch::sync = detail::sync_policy{};
    detail::deferred_policy const launch::deferred = detail::deferred_policy{};
    detail::apply_policy const launch::apply = detail::apply_policy{};

    detail::select_policy_generator const launch::select =
        detail::select_policy_generator{};

    detail::policy_holder<> const launch::all =
        detail::policy_holder<>{launch_policy::all};
    detail::policy_holder<> const launch::sync_policies =
        detail::policy_holder<>{launch_policy::sync_policies};
    detail::policy_holder<> const launch::async_policies =
        detail::policy_holder<>{launch_policy::async_policies};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        void policy_holder_base::load(
            serialization::input_archive& ar, unsigned)
        {
            ar >> policy_;
            ar >> priority_;
            ar >> hint_.hint >> hint_.mode;
            std::uint8_t mode = 0;
            ar >> mode;
            hint_.placement_mode(
                static_cast<hpx::threads::thread_placement_hint>(mode));
            ar >> mode;
            hint_.sharing_mode(
                static_cast<hpx::threads::thread_sharing_hint>(mode));
            ar >> mode;
            hint_.runs_as_child_mode(
                static_cast<hpx::threads::thread_execution_hint>(mode));
        }

        void policy_holder_base::save(
            serialization::output_archive& ar, unsigned) const
        {
            ar << policy_;
            ar << priority_;
            ar << hint_.hint << hint_.mode
               << static_cast<std::uint8_t>(hint_.placement_mode())
               << static_cast<std::uint8_t>(hint_.sharing_mode())
               << static_cast<std::uint8_t>(hint_.runs_as_child_mode());
        }
    }    // namespace detail
}    // namespace hpx
