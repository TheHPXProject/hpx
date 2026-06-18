//  Copyright (c) 2020-2026 Hartmut Kaiser
//  Copyright (c) 2025 Lukas Zeil
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file create_communicator.hpp

#pragma once

#include <hpx/config.hpp>

#if defined(DOXYGEN)
// clang-format off
/// Top level HPX namespace
namespace hpx { namespace collectives {

    /// A communicator instance represents the list of sites that participate in
    /// a particular collective operation.
    struct communicator
    {
        /// Store the number of used sites and the index of the current site for
        /// this communicator instance.
        ///
        /// \param  num_sites   The number of participating sites (default: all
        ///                     localities).
        /// \param this_site    The sequence number of this site (usually
        ///                     the locality id).
        void set_info(
            num_sites_arg num_sites, this_site_arg this_site) noexcept;

        /// Retrieve the number of used sites and the index of the current site
        /// for this communicator instance.
        [[nodiscard]] std::pair<num_sites_arg, this_site_arg>
        get_info() const noexcept;

        /// Return whether this communicator instance represents the root site
        /// of the communication operation.
        [[nodiscard]] bool is_root() const;
    };

    /// Create a new communicator object usable with any collective operation
    ///
    /// This functions creates a new communicator object that can be called in
    /// order to pre-allocate a communicator object usable with multiple
    /// invocations of the collective operations (such as \a all_gather,
    /// \a all_reduce, \a all_to_all, \a broadcast, etc.).
    ///
    /// \param  basename    The base name identifying the collective operation
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the collective operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the collective operation on the
    ///                     given base name has to be performed more than once.
    /// \param  root_site   The site that is responsible for creating the
    ///                     collective support object. This value is optional
    ///                     and defaults to '0' (zero).
    ///
    /// \returns    This function returns a new communicator object usable
    ///             with the collective operation.
    ///
    communicator create_communicator(char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Create a new communicator object usable with any local collective
    /// operation
    ///
    /// This functions creates a new communicator object that can be called in
    /// order to pre-allocate a communicator object usable with multiple
    /// invocations of the collective operations (such as \a all_gather,
    /// \a all_reduce, \a all_to_all, \a broadcast, etc.).
    ///
    /// \param  basename    The base name identifying the collective operation
    /// \param  num_sites   The number of participating sites
    /// \param  this_site   The sequence number of this invocation (usually
    ///                     the sequence number of the object participating in
    ///                     the collective operation). This value must be in the
    ///                     range [0, num_sites).
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the collective operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the collective operation on the
    ///                     given base name has to be performed more than once.
    /// \param  root_site   The site that is responsible for creating the
    ///                     collective support object. This value is optional
    ///                     and defaults to '0' (zero).
    ///
    /// \returns    This function returns a new communicator object usable
    ///             for all local collective operations.
    ///
    communicator create_local_communicator(char const* basename,
        num_sites_arg num_sites, this_site_arg this_site,
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Create a new hierarchical communicator object usable with collective
    /// operations
    ///
    /// This functions creates a new hierarchical_communicator object that can
    /// be called in order to pre-allocate a communicator object usable with
    /// multiple invocations of a collective operation (such as \a all_gather,
    /// \a all_reduce, \a all_to_all, \a broadcast, etc.).
    ///
    /// \param  basename    The base name identifying the collective operation
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param arity        The arity of the hierarchical tree of communicators
    ///                     to create for the given endpoint. The default arity
    ///                     is 2.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the collective operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the collective operation on the
    ///                     given base name has to be performed more than once.
    /// \param  root_site   The site that is responsible for creating the
    ///                     collective support object. This value is optional
    ///                     and defaults to '0' (zero).
    /// \param  threshold   The site-count threshold below which the
    ///                     communicator collapses to a single flat
    ///                     communicator spanning all sites (strict
    ///                     comparison: num_sites < threshold). The default
    ///                     is 16; pass 0 to disable the fallback and always
    ///                     build a tree.
    ///
    /// \note   The sub-communicators of a hierarchical_communicator share
    ///         one generation sequence per registered name, and the
    ///         hierarchical collectives consume internal generations at
    ///         different rates: \a broadcast and \a barrier consume one
    ///         generation per call on every sub-communicator, \a all_gather
    ///         and \a all_reduce consume two per call, and \a all_to_all
    ///         consumes two per call on the subtree communicators but one on
    ///         the inter-group communicator. A single
    ///         hierarchical_communicator instance must therefore only be
    ///         used with collective operations sharing one consumption
    ///         scheme: \a all_gather and \a all_reduce may be mixed on one
    ///         instance, while \a all_to_all, \a broadcast, and \a barrier
    ///         each require their own instance.
    ///
    /// \returns    This function returns a new communicator object usable
    ///             with the collective operation.
    ///
    hierarchical_communicator create_hierarchical_communicator(
        char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        arity_arg arity = arity_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg(),
        flat_fallback_threshold_arg threshold = flat_fallback_threshold_arg());
}}
// clang-format on

#else

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/detail/communicator.hpp>

#include <cstddef>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::collectives::detail {

    // Data stored in the shared state of the communicator client type below
    struct communicator_data
    {
        num_sites_arg num_sites_;
        this_site_arg this_site_;
        root_site_arg root_site_;
    };
}    // namespace hpx::collectives::detail

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    template <>
    struct extra_data_helper<hpx::collectives::detail::communicator_data>
    {
        HPX_EXPORT static extra_data_id_type id() noexcept;
        static constexpr void reset(
            collectives::detail::communicator_data*) noexcept
        {
        }
    };    // namespace hpx::util
}    // namespace hpx::util

namespace hpx::collectives {

    HPX_CXX_EXPORT struct communicator
      : hpx::components::client_base<communicator, detail::communicator_server,
            detail::communicator_data>
    {
        using base_type = client_base<communicator, detail::communicator_server,
            detail::communicator_data>;
        using future_type = base_type::future_type;

    public:
        // construction
        communicator() = default;

        explicit communicator(base_type const& base) noexcept
          : base_type(base)
        {
        }
        explicit communicator(base_type&& base) noexcept
          : base_type(HPX_MOVE(base))
        {
        }
        explicit communicator(hpx::id_type&& id)
          : base_type(HPX_MOVE(id))
        {
        }
        explicit communicator(future<hpx::id_type>&& id) noexcept
          : base_type(HPX_MOVE(id))
        {
        }
        communicator(future<communicator>&& c)
          : base_type(HPX_MOVE(c))
        {
        }

        HPX_EXPORT void set_info(num_sites_arg num_sites,
            this_site_arg this_site,
            root_site_arg root_site = root_site_arg()) noexcept;

        [[nodiscard]] HPX_EXPORT
            std::tuple<num_sites_arg, this_site_arg, root_site_arg>
            get_info_ex() const noexcept;

        [[nodiscard]] HPX_EXPORT std::pair<num_sites_arg, this_site_arg>
        get_info() const noexcept;

        [[nodiscard]] bool is_root() const
        {
            return !base_type::registered_name().empty();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Predefined global communicator (refers to all localities)
    HPX_CXX_EXPORT HPX_EXPORT communicator get_world_communicator();

    namespace detail {

        HPX_EXPORT void create_global_communicator();
        HPX_EXPORT void reset_global_communicator();
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Predefined local communicator (refers to all threads on the calling
    // locality)
    HPX_CXX_EXPORT HPX_EXPORT communicator get_local_communicator();

    namespace detail {

        HPX_EXPORT void create_local_communicator();
        HPX_EXPORT void reset_local_communicator();
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT communicator create_communicator(
        char const* basename, num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    HPX_CXX_EXPORT HPX_EXPORT communicator create_communicator(
        hpx::launch::sync_policy, char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    HPX_CXX_EXPORT HPX_EXPORT communicator create_local_communicator(
        char const* basename, num_sites_arg num_sites, this_site_arg this_site,
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    HPX_CXX_EXPORT HPX_EXPORT communicator create_local_communicator(
        hpx::launch::sync_policy, char const* basename, num_sites_arg num_sites,
        this_site_arg this_site, generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT struct hierarchical_communicator;

    HPX_CXX_EXPORT HPX_EXPORT hierarchical_communicator
    create_hierarchical_communicator(char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        arity_arg arity = arity_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg(),
        flat_fallback_threshold_arg threshold = flat_fallback_threshold_arg());

    HPX_CXX_EXPORT struct hierarchical_communicator
    {
    private:
        hierarchical_communicator(
            std::vector<hpx::tuple<communicator, this_site_arg>>&& comms,
            arity_arg arity, root_site_arg root_site, num_sites_arg num_sites,
            this_site_arg this_site) noexcept
          : communicators(HPX_MOVE(comms))
          , num_sites(num_sites)
          , this_site(this_site)
          , root_site(root_site)
          , arity(arity)
        {
        }

        friend HPX_EXPORT hierarchical_communicator
        create_hierarchical_communicator(char const* basename,
            num_sites_arg num_sites, this_site_arg this_site, arity_arg arity,
            generation_arg generation, root_site_arg root_site,
            flat_fallback_threshold_arg threshold);

    public:
        hpx::tuple<communicator, this_site_arg>& operator[](std::size_t idx)
        {
            return communicators[idx];
        }
        hpx::tuple<communicator, this_site_arg> const& operator[](
            std::size_t idx) const
        {
            return communicators[idx];
        }

        communicator& get(std::size_t idx)
        {
            return hpx::get<0>(communicators[idx]);
        }
        communicator const& get(std::size_t idx) const
        {
            return hpx::get<0>(communicators[idx]);
        }

        [[nodiscard]] this_site_arg site(std::size_t idx) const
        {
            return hpx::get<1>(communicators[idx]);
        }

        [[nodiscard]] std::size_t size() const
        {
            return communicators.size();
        }

        communicator const& back() const
        {
            return hpx::get<0>(communicators.back());
        }

        [[nodiscard]] this_site_arg last_site() const
        {
            return hpx::get<1>(communicators.back());
        }

        [[nodiscard]] constexpr arity_arg get_arity() const noexcept
        {
            return arity;
        }

        [[nodiscard]] HPX_EXPORT
            hpx::tuple<num_sites_arg, this_site_arg, root_site_arg>
            get_info_ex() const noexcept;

        [[nodiscard]] HPX_EXPORT hpx::tuple<num_sites_arg, this_site_arg>
        get_info() const noexcept;

    private:
        std::vector<hpx::tuple<communicator, this_site_arg>> communicators;
        num_sites_arg num_sites;
        this_site_arg this_site;
        root_site_arg root_site;
        arity_arg arity;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    struct is_communicator
      : hpx::meta::one_of<T, communicator, hierarchical_communicator>
    {
    };

    HPX_CXX_EXPORT template <typename T>
    inline constexpr bool is_communicator_v = is_communicator<T>::value;
}    // namespace hpx::collectives

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
