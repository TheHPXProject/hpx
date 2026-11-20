//  Copyright (c) 2015-2026 Hartmut Kaiser
//  Copyright (c) 2015-2016 Thomas Heller
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/naming/credit_handling.hpp>

#include <cstddef>
#include <map>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::serialization::detail {

    ///////////////////////////////////////////////////////////////////////////
    // This class allows to handle credit splitting for gid_types during
    // serialization.
    //
    // Preprocessing splits the credit of each managed id once and keeps the
    // split gid, the real pass sends it. The split gids are matched to the
    // ids by the gid they refer to, not by the address of the id_type: an id
    // that a serialize() function creates on the fly has a new address in
    // every pass, and another id can appear at an address that an earlier id
    // had. Credits of the same gid are interchangeable: any id of a gid can
    // send the credit split from another id of that gid.
    //
    // Each preprocessing round matches the managed ids it serializes to the
    // split gids of the earlier rounds and splits the credit of the ids that
    // have none yet (e.g. ids that a future which was pending in an earlier
    // round has revealed). The real pass takes a split gid for each managed
    // id it sends. The split gids it doesn't take are returned: e.g. those of
    // ids an earlier parcel of the same message has written already, which
    // the parcels after it write as back-references.
    HPX_CXX_EXPORT class preprocess_gid_types
    {
        using mutex_type = hpx::spinlock;

    public:
        // The gid each split gid was split from (without its credit), and the
        // split gid (invalid_gid while the credit is being split, or once the
        // real pass has taken it).
        using split_gids_type =
            std::vector<std::pair<naming::gid_type, naming::gid_type>>;

        preprocess_gid_types() = default;

        ~preprocess_gid_types()
        {
            std::unique_lock<mutex_type> l(mtx_);
            reset_locked(l);
        }

        preprocess_gid_types(preprocess_gid_types&& rhs) noexcept = delete;
        preprocess_gid_types& operator=(
            preprocess_gid_types&& rhs) noexcept = delete;

        template <typename Lock>
        void reset_locked(Lock& l)
        {
            HPX_ASSERT_OWNS_LOCK(l);

            index_.clear();

            // If there are gids left then several parcels got preprocessed
            // separately, but serialized into the same archive (an id was
            // written as a back-reference), or a parcel was serialized
            // differently than it was preprocessed. In this case we must
            // explicitly return the credits for those gids before clearing
            // them.
            if (!split_gids_.empty())
            {
                std::vector<naming::gid_type> gids;
                gids.reserve(split_gids_.size());

                for (auto const& entry : split_gids_)
                {
                    if (entry.second != naming::invalid_gid)
                    {
                        gids.push_back(entry.second);
                    }
                }

                split_gids_.clear();

                // now return credits to AGAS
                unlock_guard<Lock> ul(l);

                for (auto const& gid : gids)
                {
                    naming::decrement_refcnt(gid);
                }
            }
        }

        // Preprocessing: the entry for the managed id 'id', and whether an
        // earlier preprocessing round has split a credit for it already (or
        // is still splitting it). Each entry is matched to one id per round.
        [[nodiscard]] std::pair<std::size_t, bool> find_or_add(
            naming::gid_type const& id)
        {
            naming::gid_type const stripped =
                naming::detail::get_stripped_gid(id);

            std::lock_guard<mutex_type> l(mtx_);

            entries& e = index_[stripped];
            if (e.round != round_)
            {
                e.round = round_;
                e.matched = 0;
            }
            if (e.matched != e.positions.size())
            {
                return {e.positions[e.matched++], true};
            }

            std::size_t const pos = split_gids_.size();
            split_gids_.emplace_back(stripped, naming::invalid_gid);
            e.positions.push_back(pos);
            ++e.matched;
            return {pos, false};
        }

        // Preprocessing: the split gid of the entry 'pos', possibly set
        // asynchronously once the credit has been split.
        void set_gid(std::size_t const pos, naming::gid_type const& split_gid)
        {
            HPX_ASSERT(split_gid != naming::invalid_gid);

            std::unique_lock<mutex_type> l(mtx_);
            if (pos < split_gids_.size() &&
                split_gids_[pos].second == naming::invalid_gid)
            {
                split_gids_[pos].second = split_gid;
                return;
            }

            // the gids were handed on or dropped while the credit was being
            // split: nobody will send this one, return its credit
            l.unlock();
            naming::decrement_refcnt(split_gid);
        }

        // The real pass: a split gid for the managed id 'id'.
        [[nodiscard]] naming::gid_type take(naming::gid_type const& id)
        {
            naming::gid_type const stripped =
                naming::detail::get_stripped_gid(id);

            // the exceptions are thrown without holding the lock (creating
            // them may suspend the thread)
            bool other_id = false;
            {
                std::lock_guard<mutex_type> l(mtx_);
                auto const it = index_.find(stripped);
                for (entries* e = it != index_.end() ? &it->second : nullptr;
                    e != nullptr && e->taken != e->positions.size(); ++e->taken)
                {
                    auto& entry = split_gids_[e->positions[e->taken]];
                    if (entry.second == naming::invalid_gid)
                    {
                        continue;
                    }
                    ++e->taken;
                    if (naming::detail::get_stripped_gid(entry.second) ==
                        stripped)
                    {
                        return std::exchange(entry.second, naming::invalid_gid);
                    }
                    other_id = true;
                    break;
                }
            }

            if (other_id)
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "preprocess_gid_types::take",
                    "the split credit for a managed id of this parcel "
                    "belongs to another id");
            }

            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "preprocess_gid_types::take",
                "no split credit for a managed id of this parcel: it was "
                "serialized differently than during preprocessing");
        }

        // A new pass over the same ids (preprocessing round) matches all
        // entries again.
        void start_round() noexcept
        {
            std::lock_guard<mutex_type> l(mtx_);
            ++round_;
        }

        split_gids_type move_split_gids() noexcept
        {
            std::lock_guard<mutex_type> l(mtx_);
            index_.clear();
            return std::exchange(split_gids_, split_gids_type());
        }

        // The real pass: the split gids of the parcel that is serialized next.
        void set_split_gids(split_gids_type&& gids)
        {
            std::unique_lock<mutex_type> l(mtx_);

            reset_locked(l);
            split_gids_ = HPX_MOVE(gids);
            for (std::size_t i = 0; i != split_gids_.size(); ++i)
            {
                index_[split_gids_[i].first].positions.push_back(i);
            }
        }

    private:
        // The entries of one gid: preprocessing matches them in this order
        // (those up to 'matched' are matched in round 'round'), the real pass
        // takes them in this order (up to 'taken').
        struct entries
        {
            std::vector<std::size_t> positions;
            std::size_t round = 0;
            std::size_t matched = 0;
            std::size_t taken = 0;
        };

        mutable mutex_type mtx_ = mutex_type("preprocess_gid_types");
        split_gids_type split_gids_;
        std::map<naming::gid_type, entries> index_;    // by gid
        std::size_t round_ = 0;    // the current preprocessing round
    };
}    // namespace hpx::serialization::detail

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries. Resetting the archive keeps the split gids: a
    // preprocessing round matches its ids to the gids of the earlier rounds
    // again.
    template <>
    struct extra_data_helper<hpx::serialization::detail::preprocess_gid_types>
    {
        HPX_EXPORT static extra_data_id_type id() noexcept;
        static void reset(
            serialization::detail::preprocess_gid_types* data) noexcept
        {
            data->start_round();
        }
    };
}    // namespace hpx::util
