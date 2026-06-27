//  Copyright (c) 2014-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file default_distribution_policy.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/serialization.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::components {

    /// This class specifies the parameters for a simple distribution policy
    /// to use for creating (and evenly distributing) a given number of items
    /// on a given set of localities.
    HPX_CXX_EXPORT struct default_distribution_policy
    {
    public:
        /// Default-construct a new instance of a \a default_distribution_policy.
        /// This policy will represent one locality (the local locality).
        constexpr default_distribution_policy() = default;

        /// Create a new \a default_distribution policy representing the given
        /// set of localities.
        ///
        /// \param locs     [in] The list of localities the new instance should
        ///                 represent
        default_distribution_policy operator()(std::vector<id_type> locs) const
        {
            return default_distribution_policy(HPX_MOVE(locs));
        }

        /// Create a new \a default_distribution policy representing the given
        /// locality
        ///
        /// \param loc     [in] The locality the new instance should
        ///                 represent
        default_distribution_policy operator()(id_type loc) const
        {
            return default_distribution_policy(HPX_MOVE(loc));
        }

        /// Create one object on one of the localities associated by
        /// this policy instance
        ///
        /// \param vs  [in] The arguments which will be forwarded to the
        ///            constructor of the new object.
        ///
        /// \note This function is part of the placement policy implemented by
        ///       this class
        ///
        /// \returns A future holding the global address which represents
        ///          the newly created object
        ///
        template <typename Component, typename... Ts>
        hpx::future<hpx::id_type> create(Ts&&... vs) const
        {
            if (localities_)
            {
                for (hpx::id_type const& loc : *localities_)
                {
                    if (get_num_items(1, loc) != 0)
                    {
                        return create_async<Component>(
                            loc, HPX_FORWARD(Ts, vs)...);
                    }
                }
            }

            // by default the object will be created on the current locality
            return create_async<Component>(
                naming::get_id_from_locality_id(agas::get_locality_id()),
                HPX_FORWARD(Ts, vs)...);
        }

        /// \cond NOINTERNAL
        using bulk_locality_result =
            std::pair<hpx::id_type, std::vector<hpx::id_type>>;
        /// \endcond

        /// Create multiple objects on the localities associated by this policy
        /// instance
        ///
        /// \param count [in] The number of objects to create
        /// \param vs   [in] The arguments which will be forwarded to the
        ///             constructors of the new objects.
        ///
        /// \note This function is part of the placement policy implemented by
        ///       this class
        ///
        /// \returns A future holding the list of global addresses that
        ///          represent the newly created objects
        ///
        template <bool WithCount, typename Component, typename... Ts>
        hpx::future<std::vector<bulk_locality_result>> bulk_create(
            std::size_t count, Ts&&... vs) const
        {
            if (localities_ && localities_->size() > 1)
            {
                // schedule creation of all objects across given localities
                std::vector<hpx::future<std::vector<hpx::id_type>>> objs;
                objs.reserve(localities_->size());

                [[maybe_unused]] std::size_t first = 0;
                for (hpx::id_type const& loc : *localities_)
                {
                    if constexpr (WithCount)
                    {
                        std::size_t const local_count =
                            get_num_items(count, loc);
                        objs.emplace_back(bulk_create_async<true, Component>(
                            loc, local_count, first, vs...));
                        first += local_count;
                    }
                    else
                    {
                        objs.emplace_back(bulk_create_async<false, Component>(
                            loc, get_num_items(count, loc), vs...));
                    }
                }

                // consolidate all results
                auto localities = localities_;
                return hpx::dataflow(
                    hpx::launch::sync,
                    [localities = HPX_MOVE(localities)](
                        std::vector<hpx::future<std::vector<hpx::id_type>>>&&
                            v) mutable -> std::vector<bulk_locality_result> {
                        HPX_ASSERT(localities->size() == v.size());

                        std::vector<bulk_locality_result> result;
                        result.reserve(v.size());

                        for (std::size_t i = 0; i != v.size(); ++i)
                        {
                            result.emplace_back((*localities)[i], v[i].get());
                        }
                        return result;
                    },
                    HPX_MOVE(objs));
            }

            // handle special cases
            hpx::id_type id = get_next_target();

            hpx::future<std::vector<hpx::id_type>> f;
            if constexpr (WithCount)
            {
                f = bulk_create_async<true, Component>(
                    id, count, 0, HPX_FORWARD(Ts, vs)...);
            }
            else
            {
                f = bulk_create_async<false, Component>(
                    id, count, HPX_FORWARD(Ts, vs)...);
            }

            return f.then(hpx::launch::sync,
                [id = HPX_MOVE(id)](hpx::future<std::vector<hpx::id_type>>&& f)
                    -> std::vector<bulk_locality_result> {
                    std::vector<bulk_locality_result> result;
                    result.emplace_back(id, f.get());
                    return result;
                });
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action>
        struct async_result
        {
            using type = hpx::future<
                typename traits::promise_local_result<typename hpx::traits::
                        extract_action<Action>::remote_result_type>::type>;
        };

        template <typename Action, typename... Ts>
        typename async_result<Action>::type async(
            launch policy, Ts&&... vs) const
        {
            return hpx::detail::async_impl<Action>(
                policy, get_next_target(), HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Callback, typename... Ts>
        typename async_result<Action>::type async_cb(
            launch policy, Callback&& cb, Ts&&... vs) const
        {
            return hpx::detail::async_cb_impl<Action>(policy, get_next_target(),
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Continuation, typename... Ts>
        bool apply(Continuation&& c, launch policy, Ts&&... vs) const
        {
            return hpx::detail::post_impl<Action>(HPX_FORWARD(Continuation, c),
                get_next_target(), policy, HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename... Ts>
        bool apply(threads::thread_priority priority, Ts&&... vs) const
        {
            return hpx::detail::post_impl<Action>(
                get_next_target(), priority, HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Continuation, typename Callback,
            typename... Ts>
        bool apply_cb(
            Continuation&& c, launch policy, Callback&& cb, Ts&&... vs) const
        {
            return hpx::detail::post_cb_impl<Action>(
                HPX_FORWARD(Continuation, c), get_next_target(), policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename Callback, typename... Ts>
        bool apply_cb(launch policy, Callback&& cb, Ts&&... vs) const
        {
            return hpx::detail::post_cb_impl<Action>(get_next_target(), policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        /// Returns the number of associated localities for this distribution
        /// policy
        ///
        /// \note This function is part of the creation policy implemented by
        ///       this class
        ///
        [[nodiscard]] std::size_t get_num_localities() const
        {
            return !localities_ ? static_cast<std::size_t>(1) :
                                  localities_->size();
        }

        /// Returns the locality which is anticipated to be used for the next
        /// async operation
        [[nodiscard]] hpx::id_type get_next_target() const
        {
            return !localities_ ?
                naming::get_id_from_locality_id(agas::get_locality_id()) :
                localities_->front();
        }

    protected:
        /// \cond NOINTERNAL
        std::size_t get_num_items(
            std::size_t items, hpx::id_type const& loc) const
        {
            // make sure the given id is known to this distribution policy
            HPX_ASSERT(localities_ &&
                std::find(localities_->begin(), localities_->end(), loc) !=
                    localities_->end());

            // this distribution policy places an equal number of items onto
            // each locality
            std::size_t const locs = localities_->size();

            // the overall number of items to create is smaller than the number
            // of localities
            if (items < locs)
            {
                auto const it =
                    std::find(localities_->begin(), localities_->end(), loc);
                std::size_t const num_loc =
                    std::distance(localities_->begin(), it);
                return (num_loc < items) ? 1 : 0;
            }

            std::size_t const items_per_loc = (items + locs - 1) / locs;

            // the last locality might get fewer items
            if (locs > 1 && loc == localities_->back())
            {
                return items - (locs - 1) * items_per_loc;
            }

            // otherwise just distribute evenly
            return items_per_loc;
        }
        /// \endcond

    protected:
        /// \cond NOINTERNAL
        explicit default_distribution_policy(std::vector<id_type> localities)
          : localities_(
                std::make_shared<std::vector<id_type>>(HPX_MOVE(localities)))
        {
            if (localities_->empty())
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "default_distribution_policy::default_distribution_policy",
                    "unexpectedly empty list of localities");
            }
        }

        explicit default_distribution_policy(id_type locality)
          : localities_(
                std::make_shared<std::vector<id_type>>(1, HPX_MOVE(locality)))
        {
        }

        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            // clang-format off
            ar & localities_;
            // clang-format on
        }

        // localities to create things on
        std::shared_ptr<std::vector<id_type>> localities_;
        /// \endcond
    };

    /// A predefined instance of the default \a distribution_policy. It will
    /// represent the local locality and will place all items to create here.
    HPX_CXX_EXPORT HPX_EXPORT extern default_distribution_policy const
        default_layout;
}    // namespace hpx::components

/// \cond NOINTERNAL
namespace hpx {

    HPX_CXX_EXPORT using hpx::components::default_distribution_policy;
    HPX_CXX_EXPORT using hpx::components::default_layout;

    template <>
    struct traits::is_distribution_policy<
        components::default_distribution_policy> : std::true_type
    {
    };    // namespace traits
}    // namespace hpx
/// \endcond
