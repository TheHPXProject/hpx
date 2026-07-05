//  Copyright (c) 2020-2026 Hartmut Kaiser
//  Copyright (c) 2025 Lukas Zeil
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file broadcast.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// Broadcast a value to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  basename    The base name identifying the broadcast operation
    /// \param  local_result A value to transmit to all
    ///                     participating sites from this call site.
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \returns    This function returns a future holding the value that was
    ///             sent to all participating sites. It will become
    ///             ready once the broadcast operation has been completed.
    ///
    template <typename T>
    hpx::future<T> broadcast_to(char const* basename, T&& local_result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Broadcast a value to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  local_result A value to transmit to all
    ///                     participating sites from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a future holding the value that was
    ///             sent to all participating sites. It will become
    ///             ready once the broadcast operation has been completed.
    ///
    template <typename T>
    hpx::future<T> broadcast_to(communicator comm,
        T&& local_result, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Broadcast a value to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  local_result A value to transmit to all
    ///                     participating sites from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a future holding the value that was
    ///             sent to all participating sites. It will become
    ///             ready once the broadcast operation has been completed.
    ///
    template <typename T>
    hpx::future<T> broadcast_to(communicator comm,
        generation_arg generation,
        T&& local_result, this_site_arg this_site = this_site_arg());

    /// Receive a value that was broadcast to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  basename    The base name identifying the broadcast operation
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param root_site    The site responsible for broadcasting the value.
    ///                     This is optional and defaults to site `0` (zero).
    ///
    /// \returns    This function returns a future holding the value that was
    ///             sent to all participating sites. It will become
    ///             ready once the broadcast operation has been completed.
    ///
    template <typename T>
    hpx::future<T> broadcast_from(char const* basename,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Receive a value that was broadcast to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a future holding the value that was
    ///             sent to all participating sites. It will become
    ///             ready once the broadcast operation has been completed.
    ///
    template <typename T>
    hpx::future<T> broadcast_from(communicator comm,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Receive a value that was broadcast to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a future holding the value that was
    ///             sent to all participating sites. It will become
    ///             ready once the broadcast operation has been completed.
    ///
    template <typename T>
    hpx::future<T> broadcast_from(communicator comm,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Broadcast a value to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  basename    The base name identifying the broadcast operation
    /// \param  local_result A value to transmit to all
    ///                     participating sites from this call site.
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
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
    decltype(auto) broadcast_to(hpx::launch::sync_policy policy,
        char const* basename, T&& local_result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Broadcast a value to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  local_result A value to transmit to all
    ///                     participating sites from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    decltype(auto) broadcast_to(hpx::launch::sync_policy policy,
        communicator comm, T&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Broadcast a value to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  local_result A value to transmit to all
    ///                     participating sites from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    decltype(auto) broadcast_to(hpx::launch::sync_policy policy,
        communicator comm, T&& local_result, generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Receive a value that was broadcast to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  basename    The base name identifying the broadcast operation
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The site responsible for broadcasting the value.
    ///                     This is optional and defaults to site `0` (zero).
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    T broadcast_from(hpx::launch::sync_policy policy, char const* basename,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Receive a value that was broadcast to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    T broadcast_from(hpx::launch::sync_policy policy, communicator comm,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Receive a value that was broadcast to different call sites
    ///
    /// This function sends a set of values to all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the broadcast operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the broadcast operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a broadcast_to and
    ///             \a broadcast_from have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    T broadcast_from(hpx::launch::sync_policy policy, communicator comm,
        generation_arg generation, this_site_arg this_site = this_site_arg());
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
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // support for broadcast
    namespace communication {

        HPX_CXX_EXPORT struct broadcast_tag;

        template <>
        struct communicator_data<broadcast_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    template <typename Communicator>
    struct communication_operation<Communicator, communication::broadcast_tag>
    {
        template <typename Result>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations)
        {
            using data_type = typename Result::result_type;

            return communicator.template handle_data<data_type>(
                communication::communicator_data<
                    communication::broadcast_tag>::name(),
                which, generation,
                // no step function
                nullptr,
                // finalizer (invoked after all sites have checked in)
                [](auto& data, auto&, std::size_t) {
                    return Communicator::template handle_bool<data_type>(
                        data[0]);
                },
                num_generations, /*num_values=*/1);
        }

        template <typename Result, typename T>
        static Result set(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t)
        {
            return communicator.template handle_data<std::decay_t<T>>(
                communication::communicator_data<
                    communication::broadcast_tag>::name(),
                which, generation,
                // step function (invoked once for set)
                [&t](auto& data, std::size_t) { data[0] = HPX_FORWARD(T, t); },
                // finalizer (invoked after all sites have checked in)
                [](auto& data, auto&, std::size_t) {
                    return Communicator::template handle_bool<std::decay_t<T>>(
                        data[0]);
                },
                num_generations, /*num_values=*/1);
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    namespace detail {

        // broadcast_to: detail entry point carrying the internal generation
        // step. The public overload forwards with single_step; the hierarchical
        // broadcast_to walks its sub-communicators through this entry with the
        // mapped step.
        template <typename T>
        hpx::future<std::decay_t<T>> broadcast_to(communicator fid,
            T&& local_result, this_site_arg this_site,
            generation_arg const generation, generation_mode num_generations)
        {
            using arg_type = std::decay_t<T>;

            if (this_site.is_default())
            {
                this_site = static_cast<std::size_t>(agas::get_locality_id());
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<arg_type>(HPX_GET_EXCEPTION(
                    hpx::error::bad_parameter, "hpx::collectives::broadcast_to",
                    "the generation number shouldn't be zero"));
            }

            // Handle operation right away if there is only one value.
            if (auto [num_sites, _] = fid.get_info(); num_sites == 1)
            {
                if (this_site != 0)
                {
                    return hpx::make_exceptional_future<arg_type>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::broadcast_to",
                            "the local site should be zero if only one site is "
                            "involved"));
                }

                return hpx::make_ready_future(HPX_FORWARD(T, local_result));
            }

            auto broadcast_data =
                [local_result = HPX_FORWARD(T, local_result), this_site,
                    generation, num_generations](
                    communicator&& c) mutable -> hpx::future<arg_type> {
                using action_type =
                    communicator_server::communication_set_direct_action<
                        traits::communication::broadcast_tag,
                        hpx::future<arg_type>, generation_mode, arg_type>;

                // explicitly unwrap returned future
                hpx::future<arg_type> result =
                    hpx::async(action_type(), c, this_site, generation,
                        num_generations, HPX_MOVE(local_result));

                if (!result.is_ready())
                {
                    // make sure id is kept alive as long as the returned future
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [client = HPX_MOVE(c)] { HPX_UNUSED(client); });
                }

                return result;
            };

            return fid.then(hpx::launch::sync, HPX_MOVE(broadcast_data));
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::decay_t<T>> broadcast_to(communicator fid,
        T&& local_result, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::broadcast_to(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            this_site, generation, detail::generation_mode::single_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {

        // broadcast_to over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step (standalone use); all_gather/all_reduce reuse it as their
        // broadcast phase with single_step (they already pass the doubled
        // generation).
        template <typename T>
        hpx::future<std::decay_t<T>> broadcast_to(
            hierarchical_communicator const& communicators, T&& local_result,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            // Each sub-communicator advances num_generations per call. Used
            // standalone (double_step) the user generation k maps to the first
            // of its two internal generations (2k-1); used as the broadcast
            // phase of all_gather/all_reduce (single_step) the caller already
            // passes the doubled generation, so the mapping is the identity.
            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            std::decay_t<T> result = HPX_FORWARD(T, local_result);
            for (std::size_t i = 0; i < communicators.size() - 1; ++i)
            {
                result = broadcast_to(communicators.get(i), HPX_MOVE(result),
                    this_site_arg(0), run_gen, run_step)
                             .get();
            }

            return broadcast_to(communicators.back(), HPX_MOVE(result),
                this_site_arg(0), run_gen, run_step);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::decay_t<T>> broadcast_to(
        hierarchical_communicator const& communicators, T&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::broadcast_to(communicators, HPX_FORWARD(T, local_result),
            this_site, generation, detail::generation_mode::double_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) broadcast_to(Communicator&& comm, T&& local_result,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return broadcast_to(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) broadcast_to(char const* basename, T&& local_result,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return broadcast_to(create_communicator(basename, num_sites, this_site,
                                generation, root_site_arg(this_site.argument_)),
            HPX_FORWARD(T, local_result), this_site);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) broadcast_to(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return broadcast_to(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) broadcast_to(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return broadcast_to(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) broadcast_to(hpx::launch::sync_policy, char const* basename,
        T&& local_result, num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return broadcast_to(hpx::launch::sync,
            create_communicator(basename, num_sites, this_site, generation,
                root_site_arg(this_site.argument_)),
            HPX_FORWARD(T, local_result), this_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // broadcast_from: detail entry point carrying the internal generation
        // step. The public overload forwards with single_step; the hierarchical
        // broadcast_from reuses it with the mapped step.
        template <typename T>
        hpx::future<T> broadcast_from(communicator fid, this_site_arg this_site,
            generation_arg const generation, generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = static_cast<std::size_t>(agas::get_locality_id());
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<T>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::broadcast_from",
                        "the generation number shouldn't be zero"));
            }

            auto broadcast_data = [this_site, generation, num_generations](
                                      communicator&& c) -> hpx::future<T> {
                using action_type =
                    communicator_server::communication_get_direct_action<
                        traits::communication::broadcast_tag, hpx::future<T>,
                        generation_mode>;

                // make sure id is kept alive as long as the returned future,
                // explicitly unwrap returned future
                hpx::future<T> result = hpx::async(
                    action_type(), c, this_site, generation, num_generations);

                if (!result.is_ready())
                {
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [client = HPX_MOVE(c)]() { HPX_UNUSED(client); });
                }

                return result;
            };

            return fid.then(hpx::launch::sync, HPX_MOVE(broadcast_data));
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> broadcast_from(communicator fid,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::broadcast_from<T>(HPX_MOVE(fid), this_site, generation,
            detail::generation_mode::single_step);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // broadcast_from over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step; all_gather/all_reduce reuse it as their broadcast phase
        // with single_step.
        template <typename T>
        hpx::future<T> broadcast_from(
            hierarchical_communicator const& communicators,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            // See broadcast_to above for the internal generation mapping.
            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            if (communicators.size() > 1)
            {
                T result = broadcast_from<T>(communicators.get(0),
                    communicators.site(0), run_gen, run_step)
                               .get();

                for (std::size_t i = 1; i < communicators.size() - 1; ++i)
                {
                    result = broadcast_to(communicators.get(i),
                        HPX_MOVE(result), this_site_arg(0), run_gen, run_step)
                                 .get();
                }

                return broadcast_to(communicators.back(), HPX_MOVE(result),
                    this_site_arg(0), run_gen, run_step);
            }

            return broadcast_from<T>(communicators.back(),
                communicators.last_site(), run_gen, run_step);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> broadcast_from(
        hierarchical_communicator const& communicators,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::broadcast_from<T>(communicators, this_site, generation,
            detail::generation_mode::double_step);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename Communicator>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) broadcast_from(Communicator&& comm,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return broadcast_from<T>(
            HPX_FORWARD(Communicator, comm), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) broadcast_from(char const* basename,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        HPX_ASSERT(this_site != root_site);
        return broadcast_from<T>(create_communicator(basename, num_sites_arg(),
                                     this_site, generation, root_site),
            this_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename Communicator>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) broadcast_from(hpx::launch::sync_policy, Communicator&& comm,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return broadcast_from<T>(
            HPX_FORWARD(Communicator, comm), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename Communicator>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) broadcast_from(hpx::launch::sync_policy, Communicator&& comm,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return broadcast_from<T>(
            HPX_FORWARD(Communicator, comm), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) broadcast_from(hpx::launch::sync_policy,
        char const* basename, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        HPX_ASSERT(this_site != root_site);
        return broadcast_from<T>(create_communicator(basename, num_sites_arg(),
                                     this_site, generation, root_site),
            this_site)
            .get();
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    void broadcast(communicator comm, T& value,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }

        comm.wait();    // make sure communicator was created

        if (this_site == std::get<2>(comm.get_info_ex()))
        {
            broadcast_to(hpx::launch::sync, HPX_MOVE(comm), value, this_site,
                generation);
        }
        else
        {
            value = broadcast_from<T>(
                hpx::launch::sync, comm, this_site, generation);
        }
    }
}    // namespace hpx::collectives

////////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST_DECLARATION(...) /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_BROADCAST(...)             /**/

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
