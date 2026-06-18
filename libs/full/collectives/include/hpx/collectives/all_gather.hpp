//  Copyright (c) 2019-2026 Hartmut Kaiser
//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file all_gather.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// AllGather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param basename     The base name identifying the all_gather operation
    /// \param result       The value to transmit to all
    ///                     participating sites from this call site.
    /// \param num_sites    The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_gather operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The site that is responsible for creating the
    ///                     all_gather support object. This value is optional
    ///                     and defaults to '0' (zero).
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the all_gather operation has been completed.
    ///
    template <typename T>
    hpx::future<std::vector<std::decay_t<T>>>
    all_gather(char const* basename, T&& result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// AllGather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to all
    ///                     participating sites from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_gather operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the all_gather operation has been completed.
    ///
    template <typename T>
    hpx::future<std::vector<std::decay_t<T>>>
    all_gather(communicator comm, T&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// AllGather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to all
    ///                     participating sites from this call site.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_gather operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the all_gather operation has been completed.
    ///
    template <typename T>
    hpx::future<std::vector<std::decay_t<T>>>
    all_gather(communicator comm, T&& result,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// AllGather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param policy       The execution policy specifying synchronous execution.
    /// \param basename     The base name identifying the all_gather operation
    /// \param result       The value to transmit to all
    ///                     participating sites from this call site.
    /// \param num_sites    The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_gather operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The site that is responsible for creating the
    ///                     all_gather support object. This value is optional
    ///                     and defaults to '0' (zero).
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    std::vector<std::decay_t<T>> all_gather(hpx::launch::sync_policy,
        char const* basename, T&& result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// AllGather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param policy       The execution policy specifying synchronous execution.
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to all
    ///                     participating sites from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_gather operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    std::vector<std::decay_t<T>> all_gather(hpx::launch::sync_policy,
        communicator comm, T&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// AllGather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param policy       The execution policy specifying synchronous execution.
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to all
    ///                     participating sites from this call site.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_gather operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    std::vector<std::decay_t<T>> all_gather(hpx::launch::sync_policy,
        communicator comm, T&& result, generation_arg generation,
        this_site_arg this_site = this_site_arg());
}}    // namespace hpx::collectives

// clang-format on
#else

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/broadcast.hpp>
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/gather.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::traits {

    namespace communication {

        HPX_CXX_EXPORT struct all_gather_tag;

        template <>
        struct communicator_data<all_gather_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    ///////////////////////////////////////////////////////////////////////////
    // support for all_gather
    template <typename Communicator>
    struct communication_operation<Communicator, communication::all_gather_tag>
    {
        template <typename Result, typename T>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation, T&& t)
        {
            return communicator.template handle_data<std::decay_t<T>>(
                communication::communicator_data<
                    communication::all_gather_tag>::name(),
                which, generation,
                // step function (invoked for each get)
                [&t](auto& data, std::size_t which) {
                    data[which] = HPX_FORWARD(T, t);
                },
                // finalizer (invoked after all data has been received)
                [](auto& data, auto&, std::size_t) { return data; });
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    ///////////////////////////////////////////////////////////////////////////
    // all_gather plain values
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<std::decay_t<T>>> all_gather(communicator fid,
        T&& local_result, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        using arg_type = std::decay_t<T>;

        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }
        if (generation == 0)
        {
            return hpx::make_exceptional_future<std::vector<arg_type>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::all_gather",
                    "the generation number shouldn't be zero"));
        }

        // Handle operation right away if there is only one value.
        if (auto [num_sites, comm_site] = fid.get_info(); num_sites == 1)
        {
            if (this_site != comm_site)
            {
                return hpx::make_exceptional_future<std::vector<arg_type>>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::all_gather",
                        "the local site should be zero if only one site is "
                        "involved"));
            }

            std::vector<arg_type> result(1, HPX_FORWARD(T, local_result));
            return hpx::make_ready_future(HPX_MOVE(result));
        }

        auto all_gather_data = [local_result = HPX_FORWARD(T, local_result),
                                   this_site,
                                   generation](communicator&& c) mutable
            -> hpx::future<std::vector<arg_type>> {
            using action_type =
                detail::communicator_server::communication_get_direct_action<
                    traits::communication::all_gather_tag,
                    hpx::future<std::vector<arg_type>>, arg_type>;

            // explicitly unwrap returned future
            hpx::future<std::vector<arg_type>> result =
                hpx::async(action_type(), c, this_site, generation,
                    HPX_MOVE(local_result));

            if (!result.is_ready())
            {
                // make sure id is kept alive as long as the returned future
                traits::detail::get_shared_state(result)->set_on_completed(
                    [client = HPX_MOVE(c)] { HPX_UNUSED(client); });
            }

            return result;
        };

        return fid.then(hpx::launch::sync, HPX_MOVE(all_gather_data));
    }

    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<std::decay_t<T>>> all_gather(communicator fid,
        T&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return all_gather(
            HPX_MOVE(fid), HPX_FORWARD(T, local_result), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<std::decay_t<T>>> all_gather(char const* basename,
        T&& local_result, num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return all_gather(create_communicator(basename, num_sites, this_site,
                              generation, root_site),
            HPX_FORWARD(T, local_result), this_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    std::vector<std::decay_t<T>> all_gather(hpx::launch::sync_policy,
        communicator fid, T&& local_result,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return all_gather(
            HPX_MOVE(fid), HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    std::vector<std::decay_t<T>> all_gather(hpx::launch::sync_policy,
        communicator fid, T&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return all_gather(
            HPX_MOVE(fid), HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    std::vector<std::decay_t<T>> all_gather(hpx::launch::sync_policy,
        char const* basename, T&& local_result,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return all_gather(create_communicator(basename, num_sites, this_site,
                              generation, root_site),
            HPX_FORWARD(T, local_result), this_site)
            .get();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Hierarchical all_gather: gather (bottom-up) + broadcast (top-down)
    // Uses 2k-1/2k generation mapping: user generation k maps to
    // internal generation 2k-1 (gather phase) and 2k (broadcast phase)
    //
    // Key difference from all_reduce: the gather phase produces vector<T>
    // (O(N) data), so the broadcast phase transfers O(N) instead of a scalar.
    //
    // An instance may be shared between all_gather and all_reduce (identical
    // generation scheme), but not with other collectives; see the note on
    // create_hierarchical_communicator.

    // Async overload
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<std::decay_t<T>>> all_gather(
        hierarchical_communicator const& communicators, T&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        using arg_type = std::decay_t<T>;

        if (generation.is_default())
        {
            return hpx::make_exceptional_future<std::vector<arg_type>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::all_gather (hierarchical)",
                    "hierarchical all_gather requires an explicit generation "
                    "number for the 2k-1/2k internal mapping"));
        }

        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }

        std::size_t const num_sites_val = hpx::get<0>(communicators.get_info());
        std::size_t const arity_val = communicators.get_arity();

        // The hierarchical helpers hardcode site 0 as the local root at
        // every tree level, so a non-zero root is not supported.
        if (root_site != 0)
        {
            return hpx::make_exceptional_future<std::vector<arg_type>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::all_gather (hierarchical)",
                    "hierarchical all_gather currently supports only "
                    "root_site == 0 (the tree designates site 0 as the root)"));
        }

        if (this_site >= num_sites_val)
        {
            return hpx::make_exceptional_future<std::vector<arg_type>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::all_gather (hierarchical)",
                    "this_site must be smaller than the number of "
                    "participating sites"));
        }

        // Flat fast path: when arity >= num_sites, the tree builder's leaf
        // condition (right - left < arity) fired at the root call and
        // produced a single flat communicator spanning all sites. The
        // gather+broadcast decomposition then collapses to a single flat
        // all_gather; dispatch directly to avoid the two separate gate
        // synchronizations.
        if (arity_val >= num_sites_val)
        {
            HPX_ASSERT(communicators.size() == 1);
            return all_gather(communicators.get(0),
                HPX_FORWARD(T, local_result), communicators.site(0),
                generation);
        }

        generation_arg const gather_gen(2 * generation - 1);
        generation_arg const broadcast_gen(2 * generation);

        if (this_site == root_site)
        {
            std::vector<arg_type> gathered = gather_here(communicators,
                HPX_FORWARD(T, local_result), this_site, gather_gen)
                                                 .get();

            return broadcast_to(
                communicators, HPX_MOVE(gathered), this_site, broadcast_gen);
        }
        else
        {
            gather_there(communicators, HPX_FORWARD(T, local_result), this_site,
                gather_gen)
                .get();

            return broadcast_from<std::vector<arg_type>>(
                communicators, this_site, broadcast_gen);
        }
    }

    // Sync version
    HPX_CXX_EXPORT template <typename T>
    std::vector<std::decay_t<T>> all_gather(hpx::launch::sync_policy,
        hierarchical_communicator const& communicators, T&& local_result,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        return all_gather(communicators, HPX_FORWARD(T, local_result),
            this_site, generation, root_site)
            .get();
    }
}    // namespace hpx::collectives

////////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_ALLGATHER_DECLARATION(...) /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_ALLGATHER(...)             /**/

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
