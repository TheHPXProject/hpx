//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2007 Richard D. Guidry Jr.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::naming {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT void decrement_refcnt(gid_type const& gid);

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // has side effects, can't be pure
        HPX_CXX_EXPORT HPX_EXPORT std::int64_t add_credit_to_gid(
            gid_type& id, std::int64_t credits);

        HPX_CXX_EXPORT HPX_EXPORT std::int64_t remove_credit_from_gid(
            gid_type& id, std::int64_t debit);

        HPX_CXX_EXPORT HPX_EXPORT std::int64_t fill_credit_for_gid(gid_type& id,
            std::int64_t credits = static_cast<std::int64_t>(
                HPX_GLOBALCREDIT_INITIAL));

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT HPX_EXPORT gid_type move_gid(gid_type& id);
        HPX_CXX_EXPORT HPX_EXPORT gid_type move_gid_locked(
            std::unique_lock<gid_type::mutex_type> l, gid_type& gid);

        HPX_CXX_EXPORT HPX_EXPORT std::int64_t replenish_credits(gid_type& id);
        HPX_CXX_EXPORT HPX_EXPORT std::int64_t replenish_credits_locked(
            std::unique_lock<gid_type::mutex_type>& l, gid_type& id);

        ///////////////////////////////////////////////////////////////////////
        // splits the current credit of the given id and assigns half of it to
        // the returned copy
        HPX_CXX_EXPORT HPX_EXPORT gid_type split_credits_for_gid(gid_type& id);
        HPX_CXX_EXPORT HPX_EXPORT gid_type split_credits_for_gid_locked(
            std::unique_lock<gid_type::mutex_type>& l, gid_type& id);

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT HPX_EXPORT void decrement_refcnt(
            id_type_impl const* gid) noexcept;

        ///////////////////////////////////////////////////////////////////////
        // credit management (called during serialization), this function
        // has to be 'const' as save() above has to be 'const'.
        HPX_CXX_EXPORT HPX_EXPORT void preprocess_gid(
            id_type_impl const&, serialization::output_archive& ar);

        ///////////////////////////////////////////////////////////////////////
        // serialization
        HPX_CXX_EXPORT HPX_EXPORT void save(
            serialization::output_archive& ar, id_type_impl const&, unsigned);
        HPX_CXX_EXPORT HPX_EXPORT void load(
            serialization::input_archive& ar, id_type_impl&, unsigned);

        HPX_SERIALIZATION_SPLIT_FREE(HPX_CXX_EXPORT, id_type_impl)
    }    // namespace detail
}    // namespace hpx::naming

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT void save(
        serialization::output_archive& ar, hpx::id_type const&, unsigned int);
    HPX_CXX_EXPORT HPX_EXPORT void load(
        serialization::input_archive& ar, hpx::id_type&, unsigned int);

    HPX_SERIALIZATION_SPLIT_FREE(HPX_CXX_EXPORT, hpx::id_type)
}    // namespace hpx
