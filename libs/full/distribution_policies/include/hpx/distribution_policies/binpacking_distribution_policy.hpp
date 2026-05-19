//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file binpacking_distribution_policy.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/performance_counters/performance_counter.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::components {

    inline constexpr char const* const default_binpacking_counter_name =
        "/runtime{locality/total}/count/component@";

    namespace detail {

        /// \cond NOINTERNAL
        HPX_EXPORT std::vector<std::size_t> get_items_count(
            std::size_t count, std::vector<std::uint64_t> const& values);

        HPX_EXPORT hpx::future<std::vector<std::uint64_t>>
        retrieve_counter_values(
            std::vector<performance_counters::performance_counter>&& counters);

        HPX_EXPORT hpx::future<std::vector<std::uint64_t>> get_counter_values(
            std::string const& component_name, std::string const& counter_name,
            std::vector<hpx::id_type> const& localities);

        HPX_EXPORT hpx::id_type const& get_best_locality(
            hpx::future<std::vector<std::uint64_t>>&& f,
            std::vector<hpx::id_type> const& localities);

        template <typename Component>
        struct create_helper
        {
            explicit create_helper(std::vector<hpx::id_type> const& localities)
              : localities_(localities)
            {
            }

            template <typename... Ts>
            hpx::future<hpx::id_type> operator()(
                hpx::future<std::vector<std::uint64_t>>&& values,
                Ts&&... vs) const
            {
                hpx::id_type const& best_locality =
                    get_best_locality(HPX_MOVE(values), localities_);

                return create_async<Component>(
                    best_locality, HPX_FORWARD(Ts, vs)...);
            }

            std::vector<hpx::id_type> const& localities_;
        };

        template <bool WithCount, typename Component>
        struct create_bulk_helper
        {
            using bulk_locality_result =
                std::pair<hpx::id_type, std::vector<hpx::id_type>>;

            explicit create_bulk_helper(
                std::vector<hpx::id_type> const& localities)
              : localities_(localities)
            {
            }

            template <typename... Ts>
            hpx::future<std::vector<bulk_locality_result>> operator()(
                hpx::future<std::vector<std::uint64_t>>&& values,
                std::size_t count, Ts&&... vs) const
            {
                std::vector<std::size_t> to_create =
                    detail::get_items_count(count, values.get());

                std::vector<hpx::future<std::vector<hpx::id_type>>> objs;
                objs.reserve(localities_.size());

                [[maybe_unused]] std::size_t first = 0;
                for (std::size_t i = 0; i != to_create.size(); ++i)
                {
                    if constexpr (WithCount)
                    {
                        std::size_t const local_count = to_create[i];
                        objs.emplace_back(
                            bulk_create_async<WithCount, Component>(
                                localities_[i], local_count, vs...));
                        first += local_count;
                    }
                    else
                    {
                        objs.emplace_back(
                            bulk_create_async<WithCount, Component>(
                                localities_[i], to_create[i], vs...));
                    }
                }

                // consolidate all results
                return hpx::dataflow(
                    hpx::launch::sync,
                    [=, this](
                        std::vector<hpx::future<std::vector<hpx::id_type>>>&&
                            v) mutable -> std::vector<bulk_locality_result> {
                        HPX_ASSERT(localities_.size() == v.size());

                        std::vector<bulk_locality_result> result;
                        result.reserve(v.size());

                        for (std::size_t i = 0; i != v.size(); ++i)
                        {
                            result.emplace_back(
                                HPX_MOVE(localities_[i]), v[i].get());
                        }
                        return result;
                    },
                    HPX_MOVE(objs));
            }

            std::vector<hpx::id_type> const& localities_;
        };
        /// \endcond
    }    // namespace detail

    /// This class specifies the parameters for a binpacking distribution policy
    /// to use for creating a given number of items on a given set of localities.
    /// The binpacking policy will distribute the new objects in a way such that
    /// each of the localities will equalize the number of overall objects of
    /// this type based on a given criteria (by default this criteria is the
    /// overall number of objects of this type).
    struct binpacking_distribution_policy
    {
    public:
        /// Default-construct a new instance of a \a binpacking_distribution_policy.
        /// This policy will represent one locality (the local locality).
        binpacking_distribution_policy()
          : counter_name_(default_binpacking_counter_name)
        {
        }

        /// Create a new \a default_distribution policy representing the given
        /// set of localities.
        ///
        /// \param locs     [in] The list of localities the new instance should
        ///                 represent
        /// \param perf_counter_name  [in] The name of the performance counter which
        ///                      should be used as the distribution criteria
        ///                      (by default the overall number of existing
        ///                      instances of the given component type will be
        ///                      used).
        ///
        binpacking_distribution_policy operator()(
            std::vector<id_type> const& locs,
            char const* perf_counter_name =
                default_binpacking_counter_name) const
        {
#if defined(HPX_DEBUG)
            for (id_type const& loc : locs)
            {
                HPX_ASSERT(naming::is_locality(loc));
            }
#endif
            return binpacking_distribution_policy(locs, perf_counter_name);
        }

        /// Create a new \a default_distribution policy representing the given
        /// set of localities.
        ///
        /// \param locs     [in] The list of localities the new instance should
        ///                 represent
        /// \param perf_counter_name  [in] The name of the performance counter which
        ///                      should be used as the distribution criteria
        ///                      (by default the overall number of existing
        ///                      instances of the given component type will be
        ///                      used).
        ///
        binpacking_distribution_policy operator()(std::vector<id_type>&& locs,
            char const* perf_counter_name =
                default_binpacking_counter_name) const
        {
#if defined(HPX_DEBUG)
            for (id_type const& loc : locs)
            {
                HPX_ASSERT(naming::is_locality(loc));
            }
#endif
            return binpacking_distribution_policy(
                HPX_MOVE(locs), perf_counter_name);
        }

        /// Create a new \a default_distribution policy representing the given
        /// locality
        ///
        /// \param loc     [in] The locality the new instance should
        ///                 represent
        /// \param perf_counter_name  [in] The name of the performance counter
        ///                      that should be used as the distribution criteria
        ///                      (by default the overall number of existing
        ///                      instances of the given component type will be
        ///                      used).
        ///
        binpacking_distribution_policy operator()(id_type const& loc,
            char const* perf_counter_name =
                default_binpacking_counter_name) const
        {
            HPX_ASSERT(naming::is_locality(loc));
            return binpacking_distribution_policy(loc, perf_counter_name);
        }

        /// Create one object on one of the localities associated by
        /// this policy instance
        ///
        /// \param vs  [in] The arguments which will be forwarded to the
        ///            constructor of the new object.
        ///
        /// \returns A future holding the global address which represents
        ///          the newly created object
        ///
        template <typename Component, typename... Ts>
        hpx::future<hpx::id_type> create(Ts&&... vs) const
        {
            // handle special cases
            if (localities_.size() == 0)
            {
                return create_async<Component>(
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    HPX_FORWARD(Ts, vs)...);
            }
            if (localities_.size() == 1)
            {
                return create_async<Component>(
                    localities_.front(), HPX_FORWARD(Ts, vs)...);
            }

            // schedule creation of all objects across given localities
            hpx::future<std::vector<std::uint64_t>> values =
                detail::get_counter_values(get_component_name<Component>(),
                    counter_name_, localities_);

            return values.then(
                hpx::bind_back(detail::create_helper<Component>(localities_),
                    HPX_FORWARD(Ts, vs)...));
        }

        /// \cond NOINTERNAL
        using bulk_locality_result =
            std::pair<hpx::id_type, std::vector<hpx::id_type>>;
        /// \endcond

        /// Create multiple objects on the localities associated by
        /// this policy instance
        ///
        /// \param count [in] The number of objects to create
        /// \param vs   [in] The arguments which will be forwarded to the
        ///             constructors of the new objects.
        ///
        /// \returns A future holding the list of global addresses which
        ///          represent the newly created objects
        ///
        template <bool WithCount, typename Component, typename... Ts>
        hpx::future<std::vector<bulk_locality_result>> bulk_create(
            std::size_t count, Ts&&... vs) const
        {
            if (localities_.size() > 1)
            {
                // schedule creation of all objects across given localities
                hpx::future<std::vector<std::uint64_t>> values =
                    detail::get_counter_values(get_component_name<Component>(),
                        counter_name_, localities_);

                return values.then(hpx::bind_back(
                    detail::create_bulk_helper<WithCount, Component>(
                        localities_),
                    count, HPX_FORWARD(Ts, vs)...));
            }

            // handle special cases
            hpx::id_type id = localities_.empty() ?
                naming::get_id_from_locality_id(agas::get_locality_id()) :
                localities_.front();

            hpx::future<std::vector<hpx::id_type>> f =
                bulk_create_async<WithCount, Component>(
                    id, count, HPX_FORWARD(Ts, vs)...);

            return f.then(hpx::launch::sync,
                [id = HPX_MOVE(id)](hpx::future<std::vector<hpx::id_type>>&& f)
                    -> std::vector<bulk_locality_result> {
                    std::vector<bulk_locality_result> result;
                    result.emplace_back(id, f.get());
                    return result;
                });
        }

        /// Returns the name of the performance counter associated with this
        /// policy instance.
        std::string const& get_counter_name() const
        {
            return counter_name_;
        }

        /// Returns the number of associated localities for this distribution
        /// policy
        ///
        /// \note This function is part of the creation policy implemented by
        ///       this class
        ///
        std::size_t get_num_localities() const
        {
            return localities_.size();
        }

    protected:
        /// \cond NOINTERNAL
        binpacking_distribution_policy(std::vector<id_type> const& localities,
            char const* perf_counter_name)
          : localities_(localities)
          , counter_name_(perf_counter_name)
        {
        }

        binpacking_distribution_policy(
            std::vector<id_type>&& localities, char const* perf_counter_name)
          : localities_(HPX_MOVE(localities))
          , counter_name_(perf_counter_name)
        {
        }

        binpacking_distribution_policy(
            id_type const& locality, char const* perf_counter_name)
          : counter_name_(perf_counter_name)
        {
            localities_.push_back(locality);
        }

        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            // clang-format off
            ar & counter_name_ & localities_;
            // clang-format on
        }

        std::vector<id_type> localities_;    // localities to create things on
        std::string counter_name_;    // name of counter to use as criteria
        /// \endcond
    };

    /// A predefined instance of the binpacking \a distribution_policy. It will
    /// represent the local locality and will place all items to create here.
    static binpacking_distribution_policy const binpacked{};
}    // namespace hpx::components

/// \cond NOINTERNAL
namespace hpx {

    using hpx::components::binpacked;
    using hpx::components::binpacking_distribution_policy;

    template <>
    struct traits::is_distribution_policy<
        components::binpacking_distribution_policy> : std::true_type
    {
    };    // namespace traits
}    // namespace hpx
/// \endcond
