//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/collectives/barrier.hpp
/// \page hpx::distributed::barrier
/// \headerfile hpx/barrier.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace distributed {

    /// The barrier is an implementation performing a barrier over a number of
    /// participating threads. The different threads don't have to be on the
    /// same locality. This barrier can be invoked in a distributed application.
    ///
    /// For a local only barrier \see hpx::barrier.
    class HPX_EXPORT barrier;

    /// Creates a barrier, rank is locality id, size is number of localities
    ///
    /// \param base_name The name of the barrier
    ///
    /// A barrier \a base_name is created. It expects that
    /// hpx::get_num_localities() participate and the local rank is
    /// hpx::get_locality_id().
    explicit barrier(std::string const& base_name);

    /// Creates a barrier with a given size, rank is locality id
    ///
    /// \param base_name The name of the barrier
    /// \param num The number of participating threads
    ///
    /// A barrier \a base_name is created. It expects that
    /// \a num participate and the local rank is hpx::get_locality_id().
    barrier(std::string const& base_name, std::size_t num);

    /// Creates a barrier with a given size and rank
    ///
    /// \param base_name The name of the barrier
    /// \param num The number of participating threads
    /// \param rank The rank of the calling site for this invocation
    ///
    /// A barrier \a base_name is created. It expects that
    /// \a num participate and the local rank is \a rank.
    barrier(
        std::string const& base_name, std::size_t num, std::size_t rank);

    /// Creates a barrier with a vector of ranks
    ///
    /// \param base_name The name of the barrier
    /// \param ranks Gives a list of participating ranks (this could be derived
    ///              from a list of locality ids
    /// \param rank The rank of the calling site for this invocation
    ///
    /// A barrier \a base_name is created. It expects that ranks.size()
    /// and the local rank is \a rank (must be contained in \a ranks).
    barrier(std::string const& base_name,
        std::vector<std::size_t> const& ranks, std::size_t rank);

    /// Wait until each participant entered the barrier. Must be called by
    /// all participants
    ///
    /// \returns This function returns once all participants have entered
    /// the barrier (have called \a wait).
    void wait() const;

    /// Wait until each participant entered the barrier. Must be called by
    /// all participants
    ///
    /// \returns a future that becomes ready once all participants have
    /// entered the barrier (have called \a wait).
    hpx::future<void> wait(hpx::launch::async_policy) const;

    /// Perform a global synchronization using the default global barrier
    /// The barrier is created once at startup and can be reused throughout
    /// the lifetime of an HPX application.
    ///
    /// \note This function currently does not support dynamic connection
    /// and disconnection of localities.
    static void synchronize();

}}    // namespace hpx::distributed

// clang-format on
#else

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>

#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    namespace communication {

        HPX_CXX_EXPORT struct barrier_tag;

        template <>
        struct communicator_data<barrier_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    ///////////////////////////////////////////////////////////////////////////
    // support for barrier (no-payload collective)
    template <typename Communicator>
    struct communication_operation<Communicator, communication::barrier_tag>
    {
        template <typename Result>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation)
        {
            return communicator.template handle_data<void>(
                communication::communicator_data<
                    communication::barrier_tag>::name(),
                which, generation, nullptr, nullptr);
        }
    };
}    // namespace hpx::traits

////////////////////////////////////////////////////////////////////////////////
namespace hpx::collectives {

    // Flat barrier: synchronize all sites associated with a single communicator.
    HPX_CXX_EXPORT inline hpx::future<void> barrier(communicator fid,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }
        if (generation == 0)
        {
            return hpx::make_exceptional_future<void>(HPX_GET_EXCEPTION(
                hpx::error::bad_parameter, "hpx::collectives::barrier",
                "the generation number shouldn't be zero"));
        }

        // Handle operation right away if there is only one site.
        if ([[maybe_unused]] auto const [num_sites, _] = fid.get_info();
            num_sites == 1)
        {
            return hpx::make_ready_future();
        }

        auto barrier_data = [this_site, generation](
                                communicator&& c) -> hpx::future<void> {
            using action_type =
                detail::communicator_server::communication_get_direct_action<
                    traits::communication::barrier_tag, hpx::future<void>>;

            hpx::future<void> result =
                hpx::async(action_type(), c, this_site, generation);

            if (!result.is_ready())
            {
                // make sure id is kept alive as long as the returned future
                traits::detail::get_shared_state(result)->set_on_completed(
                    [client = HPX_MOVE(c)] { HPX_UNUSED(client); });
            }

            return result;
        };

        return fid.then(hpx::launch::sync, HPX_MOVE(barrier_data));
    }

    HPX_CXX_EXPORT inline hpx::future<void> barrier(communicator fid,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return barrier(HPX_MOVE(fid), this_site, generation);
    }

    HPX_CXX_EXPORT inline hpx::future<void> barrier(char const* basename,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return barrier(create_communicator(basename, num_sites, this_site,
                           generation, root_site),
            this_site, generation);
    }

    HPX_CXX_EXPORT inline void barrier(hpx::launch::sync_policy,
        communicator fid, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        barrier(HPX_MOVE(fid), this_site, generation).get();
    }

    HPX_CXX_EXPORT inline void barrier(hpx::launch::sync_policy,
        communicator fid, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        barrier(HPX_MOVE(fid), this_site, generation).get();
    }

    HPX_CXX_EXPORT inline void barrier(hpx::launch::sync_policy,
        char const* basename, num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        barrier(create_communicator(
                    basename, num_sites, this_site, generation, root_site),
            this_site, generation)
            .get();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Hierarchical barrier: reduce-phase + broadcast-phase no-op gates Uses the
    // 2k-1 / 2k generation mapping: user generation k maps to internal
    // generation 2k-1 (reduce phase) and 2k (broadcast phase). This allows the
    // same sub-communicators to be used for both phases without generation
    // collisions.
    HPX_CXX_EXPORT inline hpx::future<void> barrier(
        hierarchical_communicator const& communicators,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg /*root_site*/ = root_site_arg())
    {
        if (generation.is_default() || generation == 0)
        {
            return hpx::make_exceptional_future<void>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::barrier (hierarchical)",
                    "hierarchical barrier requires an explicit, positive "
                    "generation number for the 2k-1/2k internal mapping"));
        }

        if (!detail::is_valid_hierarchical_phase_generation(generation))
        {
            return hpx::make_exceptional_future<void>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::barrier (hierarchical)",
                    "the generation number is too large for the internal "
                    "2k-1/2k generation mapping"));
        }

        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }

        if (communicators.size() == 0)
        {
            return hpx::make_ready_future();
        }

        auto const [reduce_gen, broadcast_gen] =
            detail::hierarchical_phase_generations(generation);

        // Reduce phase: walk sub-communicators from deepest (end of vector) to
        // shallowest (start). Each sub-barrier releases only after all sites
        // within that sub-communicator have checked in, which propagates
        // arrival up the tree.
        for (std::size_t i = communicators.size(); i > 0; --i)
        {
            barrier(
                communicators.get(i - 1), communicators.site(i - 1), reduce_gen)
                .get();
        }

        // Broadcast phase: walk sub-communicators from shallowest to deepest.
        // Returning the final future lets the caller chain on completion.
        for (std::size_t i = 0; i + 1 < communicators.size(); ++i)
        {
            barrier(communicators.get(i), communicators.site(i), broadcast_gen)
                .get();
        }

        return barrier(
            communicators.back(), communicators.last_site(), broadcast_gen);
    }

    HPX_CXX_EXPORT inline void barrier(hpx::launch::sync_policy,
        hierarchical_communicator const& communicators,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        barrier(communicators, this_site, generation, root_site).get();
    }
}    // namespace hpx::collectives

////////////////////////////////////////////////////////////////////////////////
namespace hpx::distributed {

    HPX_CXX_EXPORT class HPX_EXPORT barrier
    {
    public:
        explicit barrier(std::string const& base_name);

        barrier(std::string const& base_name, std::size_t num);

        barrier(
            std::string const& base_name, std::size_t num, std::size_t rank);

        barrier(std::string const& base_name,
            std::vector<std::size_t> const& ranks, std::size_t rank);

        barrier(barrier&& other) noexcept;
        barrier& operator=(barrier&& other) noexcept;

        ~barrier();

        void wait() const;

        hpx::future<void> wait(hpx::launch::async_policy) const;

        // Resets this barrier instance.
        void release();

        void detach();

        // Get the instance of the global barrier
        static barrier& get_global_barrier();
        static barrier create_global_barrier();

        static void synchronize();

    private:
        enum class force_flat_tag
        {
            tag
        };

        barrier();

        barrier(std::string const& base_name, std::size_t num, std::size_t rank,
            force_flat_tag);

        void create_communicator(bool force_flat);

        std::string base_name_;
        std::size_t num_ = 0;
        std::size_t rank_ = 0;
        std::size_t cut_off_ = 0;
        mutable std::atomic<std::size_t> generation_{0};

        std::variant<hpx::collectives::communicator,
            hpx::collectives::hierarchical_communicator>
            comm_;
    };
}    // namespace hpx::distributed

#include <hpx/config/warnings_suffix.hpp>

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
