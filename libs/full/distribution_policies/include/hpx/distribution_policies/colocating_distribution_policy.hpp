//  Copyright (c) 2014-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file colocating_distribution_policy.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/serialization.hpp>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::components {

    /// This class specifies the parameters for a distribution policy to use
    /// for creating a given number of items on the locality where a given
    /// object is currently placed.
    HPX_CXX_EXPORT struct colocating_distribution_policy
    {
        /// Default-construct a new instance of a \a colocating_distribution_policy.
        /// This policy will represent the local locality.
        constexpr colocating_distribution_policy() = default;

        /// Create a new \a colocating_distribution_policy representing the
        /// locality where the given object is current located
        ///
        /// \param id     [in] The global address of the object with which
        ///                the new instances should be colocated on
        ///
        colocating_distribution_policy operator()(id_type const& id) const
        {
            return colocating_distribution_policy(id);
        }

        /// Create a new \a colocating_distribution_policy representing the
        /// locality where the given object is current located
        ///
        /// \param client  [in] The client side representation of the object
        ///                with which the new instances should be colocated on
        ///
        template <typename Client, typename Stub, typename Data>
        colocating_distribution_policy operator()(
            client_base<Client, Stub, Data> const& client) const
        {
            return colocating_distribution_policy(client.get_id());
        }

        /// Create one object on the locality of the object this distribution
        /// policy instance is associated with
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
            if (!id_)
            {
                return create_async<Component>(
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    HPX_FORWARD(Ts, vs)...);
            }
            if (!naming::detail::is_migratable(id_.get_gid()))
            {
                return create_async<Component>(
                    naming::get_id_from_locality_id(
                        naming::get_locality_id_from_id(id_)),
                    HPX_FORWARD(Ts, vs)...);
            }
            return create_colocated_async<Component>(
                id_, HPX_FORWARD(Ts, vs)...);
        }

        /// \cond NOINTERNAL
        using bulk_locality_result =
            std::pair<hpx::id_type, std::vector<hpx::id_type>>;
        /// \endcond

        /// Create multiple objects colocated with the object represented
        /// by this policy instance
        ///
        /// \param count [in] The number of objects to create
        /// \param vs   [in] The arguments which will be forwarded to the
        ///             constructors of the new objects.
        ///
        /// \note This function is part of the placement policy implemented by
        ///       this class
        ///
        /// \returns A future holding the list of global addresses which
        ///          represent the newly created objects
        ///
        template <bool WithCount, typename Component, typename... Ts>
        hpx::future<std::vector<bulk_locality_result>> bulk_create(
            std::size_t count, Ts&&... vs) const
        {
            hpx::id_type id;
            hpx::future<std::vector<hpx::id_type>> f;
            if (!id_)
            {
                id = naming::get_id_from_locality_id(agas::get_locality_id());
                if constexpr (WithCount)
                {
                    f = bulk_create_async<WithCount, Component>(
                        id, count, 0, HPX_FORWARD(Ts, vs)...);
                }
                else
                {
                    f = bulk_create_async<WithCount, Component>(
                        id, count, HPX_FORWARD(Ts, vs)...);
                }
            }
            else if (!naming::detail::is_migratable(id_.get_gid()))
            {
                id = naming::get_id_from_locality_id(
                    naming::get_locality_id_from_id(id_));
                if constexpr (WithCount)
                {
                    f = bulk_create_async<WithCount, Component>(
                        id, count, 0, HPX_FORWARD(Ts, vs)...);
                }
                else
                {
                    f = bulk_create_async<WithCount, Component>(
                        id, count, HPX_FORWARD(Ts, vs)...);
                }
            }
            else
            {
                id = id_;
                if constexpr (WithCount)
                {
                    f = bulk_create_colocated_async<WithCount, Component>(
                        id, count, 0, HPX_FORWARD(Ts, vs)...);
                }
                else
                {
                    f = bulk_create_colocated_async<WithCount, Component>(
                        id, count, HPX_FORWARD(Ts, vs)...);
                }
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
            if (!id_)
            {
                return hpx::detail::async_impl<Action>(policy,
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    HPX_FORWARD(Ts, vs)...);
            }
            if (!naming::detail::is_migratable(id_.get_gid()))
            {
                return hpx::detail::async_impl<Action>(policy,
                    naming::get_id_from_locality_id(
                        naming::get_locality_id_from_id(id_)),
                    HPX_FORWARD(Ts, vs)...);
            }
            return hpx::detail::async_colocated<Action>(
                id_, HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Callback, typename... Ts>
        typename async_result<Action>::type async_cb(
            launch policy, Callback&& cb, Ts&&... vs) const
        {
            if (!id_)
            {
                return hpx::detail::async_cb_impl<Action>(policy,
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            }
            if (!naming::detail::is_migratable(id_.get_gid()))
            {
                return hpx::detail::async_cb_impl<Action>(policy,
                    naming::get_id_from_locality_id(
                        naming::get_locality_id_from_id(id_)),
                    HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            }
            return hpx::detail::async_colocated_cb<Action>(
                id_, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Continuation, typename... Ts>
        bool apply(Continuation&& c, launch policy, Ts&&... vs) const
        {
            if (!id_)
            {
                return hpx::detail::post_impl<Action>(
                    HPX_FORWARD(Continuation, c),
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    policy, HPX_FORWARD(Ts, vs)...);
            }
            if (!naming::detail::is_migratable(id_.get_gid()))
            {
                return hpx::detail::post_impl<Action>(
                    HPX_FORWARD(Continuation, c),
                    naming::get_id_from_locality_id(
                        naming::get_locality_id_from_id(id_)),
                    policy, HPX_FORWARD(Ts, vs)...);
            }
            return hpx::detail::post_colocated<Action>(
                HPX_FORWARD(Continuation, c), id_, HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename... Ts>
        bool apply(launch policy, Ts&&... vs) const
        {
            if (!id_)
            {
                return hpx::detail::post_impl<Action>(
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    policy, HPX_FORWARD(Ts, vs)...);
            }
            if (!naming::detail::is_migratable(id_.get_gid()))
            {
                return hpx::detail::post_impl<Action>(
                    naming::get_id_from_locality_id(
                        naming::get_locality_id_from_id(id_)),
                    policy, HPX_FORWARD(Ts, vs)...);
            }
            return hpx::detail::post_colocated<Action>(
                id_, HPX_FORWARD(Ts, vs)...);
        }

        /// \note This function is part of the invocation policy implemented by
        ///       this class
        ///
        template <typename Action, typename Continuation, typename Callback,
            typename... Ts>
        bool apply_cb(
            Continuation&& c, launch policy, Callback&& cb, Ts&&... vs) const
        {
            if (!id_)
            {
                return hpx::detail::post_cb_impl<Action>(
                    HPX_FORWARD(Continuation, c),
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            }
            if (!naming::detail::is_migratable(id_.get_gid()))
            {
                return hpx::detail::post_cb_impl<Action>(
                    HPX_FORWARD(Continuation, c),
                    naming::get_id_from_locality_id(
                        naming::get_locality_id_from_id(id_)),
                    policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            }
            return hpx::detail::post_colocated_cb<Action>(
                HPX_FORWARD(Continuation, c), id_, HPX_FORWARD(Callback, cb),
                HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename Callback, typename... Ts>
        bool apply_cb(launch policy, Callback&& cb, Ts&&... vs) const
        {
            if (!id_)
            {
                return hpx::detail::post_cb_impl<Action>(
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            }
            if (!naming::detail::is_migratable(id_.get_gid()))
            {
                return hpx::detail::post_cb_impl<Action>(
                    naming::get_id_from_locality_id(
                        naming::get_locality_id_from_id(id_)),
                    policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            }
            return hpx::detail::post_colocated_cb<Action>(
                id_, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        /// Returns the number of associated localities for this distribution
        /// policy
        ///
        /// \note This function is part of the creation policy implemented by
        ///       this class
        ///
        [[nodiscard]] static std::size_t get_num_localities()
        {
            return 1;
        }

        /// Returns the locality which is anticipated to be used for the next
        /// async operation
        [[nodiscard]] hpx::id_type get_next_target() const
        {
            return id_ ?
                id_ :
                naming::get_id_from_locality_id(agas::get_locality_id());
        }

    protected:
        /// \cond NOINTERNAL
        explicit colocating_distribution_policy(id_type id)
          : id_(HPX_MOVE(id))
        {
        }

        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            // clang-format off
            ar & id_;
            // clang-format on
        }

        hpx::id_type id_;    // the global address of the object with which the
                             // target objects are colocated
        /// \endcond
    };

    /// A predefined instance of the co-locating \a distribution_policy. It
    /// will represent the local locality and will place all items to create
    /// here.
    HPX_CXX_EXPORT HPX_EXPORT extern colocating_distribution_policy const
        colocated;
}    // namespace hpx::components

/// \cond NOINTERNAL
namespace hpx {

    HPX_CXX_EXPORT using hpx::components::colocated;
    HPX_CXX_EXPORT using hpx::components::colocating_distribution_policy;

    template <>
    struct traits::is_distribution_policy<
        components::colocating_distribution_policy> : std::true_type
    {
    };    // namespace traits
}    // namespace hpx
/// \endcond
