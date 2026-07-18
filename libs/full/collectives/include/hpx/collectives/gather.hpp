//  Copyright (c) 2014-2026 Hartmut Kaiser
//  Copyright (c) 2025 Lukas Zeil
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file gather.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// Gather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  basename    The base name identifying the gather operation
    /// \param  result      The value to transmit to the central gather point
    ///                     from this call site.
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             gathered values. It will become ready once the gather
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<std::vector<decay_t<T>>> gather_here(
        char const* basename, T&& result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Gather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central gather point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             gathered values. It will become ready once the gather
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<std::vector<decay_t<T>>> gather_here(
        communicator comm, T&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Gather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central gather point
    ///                     from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             gathered values. It will become ready once the gather
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<std::vector<decay_t<T>>> gather_here(
        communicator comm, T&& result,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Gather a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central gather
    /// site (where the corresponding \a gather_here is executed)
    ///
    /// \param basename     The base name identifying the gather operation
    /// \param result       The value to transmit to the central gather point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The sequence number of the central gather point
    ///                     (usually the locality id). This value is optional
    ///                     and defaults to 0.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             gathered values. It will become ready once the gather
    ///             operation has been completed.
    ///
    template <typename T> hpx::future<void>
    gather_there(char const* basename, T&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Gather a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central gather
    /// site (where the corresponding \a gather_here is executed)
    ///
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to the central gather point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             gathered values. It will become ready once the gather
    ///             operation has been completed.
    ///
    template <typename T> hpx::future<void>
    gather_there(communicator comm, T&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Gather a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central gather
    /// site (where the corresponding \a gather_here is executed)
    ///
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to the central gather point
    ///                     from this call site.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             gathered values. It will become ready once the gather
    ///             operation has been completed.
    ///
    template <typename T> hpx::future<void>
    gather_there(communicator comm, T&& result,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Gather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  basename    The base name identifying the gather operation
    /// \param  result      The value to transmit to the central gather point
    ///                     from this call site.
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    decltype(auto) gather_here(hpx::launch::sync_policy, char const* basename,
        T&& result, num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Gather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central gather point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    decltype(auto) gather_here(hpx::launch::sync_policy, communicator comm,
        T&& result, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Gather a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central gather point
    ///                     from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    decltype(auto) gather_here(hpx::launch::sync_policy, communicator comm,
        T&& result, generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Gather a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central gather
    /// site (where the corresponding \a gather_here is executed)
    ///
    /// \param basename     The base name identifying the gather operation
    /// \param result       The value to transmit to the central gather point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The sequence number of the central gather point
    ///                     (usually the locality id). This value is optional
    ///                     and defaults to 0.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    void gather_there(hpx::launch::sync_policy, char const* basename,
        T&& result, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Gather a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central gather
    /// site (where the corresponding \a gather_here is executed)
    ///
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to the central gather point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    void gather_there(hpx::launch::sync_policy, communicator comm,
        T&& result, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Gather a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central gather
    /// site (where the corresponding \a gather_here is executed)
    ///
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param result       The value to transmit to the central gather point
    ///                     from this call site.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the gather operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the gather operation on the given
    ///                     base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a gather_here and
    ///             \a gather_there have to match.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    void gather_there(hpx::launch::sync_policy, communicator comm,
        T&& result, generation_arg generation,
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
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/flattened_data.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // support for gather
    namespace communication {

        HPX_CXX_EXPORT struct gather_tag;

        template <>
        struct communicator_data<gather_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    template <typename Communicator>
    struct communication_operation<Communicator, communication::gather_tag>
    {
        template <typename Result, typename T>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t)
        {
            using payload_type = std::decay_t<T>;
            using result_type = typename Result::result_type;
            static_assert(std::is_constructible_v<result_type,
                std::vector<payload_type>&&>);

            return communicator.template handle_data<payload_type>(
                communication::communicator_data<
                    communication::gather_tag>::name(),
                which, generation,
                // step function (invoked once for get)
                [&t](auto& data, std::size_t which) {
                    data[which] = HPX_FORWARD(T, t);
                },
                // finalizer (invoked once after all data has been received)
                [](auto& data, bool&, std::size_t) -> result_type {
                    return result_type(HPX_MOVE(data));
                },
                num_generations);
        }

        template <typename Result, typename T>
        static Result set(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t)
        {
            using payload_type = std::decay_t<T>;
            return communicator.template handle_data<payload_type>(
                communication::communicator_data<
                    communication::gather_tag>::name(),
                which, generation,
                // step function (invoked for each set)
                [&t](auto& data, std::size_t which) {
                    data[which] = HPX_FORWARD(T, t);
                },
                // no finalizer
                nullptr, num_generations);
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    ///////////////////////////////////////////////////////////////////////////
    // gather plain values
    namespace detail {

        // gather_here: detail entry point carrying the internal generation
        // step. The public overload forwards with single_step; the hierarchical
        // gather_here walks its sub-communicators through this entry with the
        // mapped step.
        template <typename Result, typename T>
        hpx::future<Result> gather_here_impl(communicator fid, T&& local_result,
            this_site_arg this_site, generation_arg generation,
            generation_mode num_generations)
        {
            using payload_type = std::decay_t<T>;
            static_assert(
                std::is_constructible_v<Result, std::vector<payload_type>&&>);

            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<Result>(HPX_GET_EXCEPTION(
                    hpx::error::bad_parameter, "hpx::collectives::gather_here",
                    "the generation number shouldn't be zero"));
            }

            // Handle operation right away if there is only one value.
            if (auto [num_sites, comm_site] = fid.get_info(); num_sites == 1)
            {
                if (this_site != comm_site)
                {
                    return hpx::make_exceptional_future<Result>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::gather_here",
                            "the local site should be zero if only one site is "
                            "involved"));
                }

                std::vector<payload_type> values;
                values.emplace_back(HPX_FORWARD(T, local_result));
                return hpx::make_ready_future(Result(HPX_MOVE(values)));
            }

            auto gather_here_data =
                [local_result = HPX_FORWARD(T, local_result), this_site,
                    generation, num_generations](
                    communicator&& c) mutable -> hpx::future<Result> {
                using action_type =
                    communicator_server::communication_get_direct_action<
                        traits::communication::gather_tag, hpx::future<Result>,
                        generation_mode, payload_type>;

                // explicitly unwrap returned future
                hpx::future<Result> result =
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

            return fid.then(hpx::launch::sync, HPX_MOVE(gather_here_data));
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // gather plain values
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<std::decay_t<T>>> gather_here(communicator fid,
        T&& local_result, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg())
    {
        return detail::gather_here_impl<std::vector<std::decay_t<T>>>(
            HPX_MOVE(fid), HPX_FORWARD(T, local_result), this_site, generation,
            detail::generation_mode::single_step);
    }

    namespace detail {

        // gather_here over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step; all_gather reuses it as its gather phase with
        // single_step (it passes 2k-1 already).
        template <typename T>
        hpx::future<std::vector<std::decay_t<T>>> gather_here(
            hierarchical_communicator const& communicators, T&& local_result,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            if (auto const error =
                    validate_hierarchical_communicator(communicators, this_site,
                        "hpx::collectives::gather_here (hierarchical)"))
            {
                return hpx::make_exceptional_future<
                    std::vector<std::decay_t<T>>>(error);
            }

            if (auto const error = validate_hierarchical_root_caller(this_site,
                    "hpx::collectives::gather_here (hierarchical)",
                    "gather_here"))
            {
                return hpx::make_exceptional_future<
                    std::vector<std::decay_t<T>>>(error);
            }

            if (!is_valid_hierarchical_run_generation(
                    generation, num_generations))
            {
                return hpx::make_exceptional_future<
                    std::vector<std::decay_t<T>>>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::gather_here (hierarchical)",
                        "the generation number is too large for the internal "
                        "generation mapping"));
            }

            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            using value_type = std::decay_t<T>;
            uniform_rows<value_type> result(HPX_FORWARD(T, local_result));
            for (std::size_t i = communicators.size() - 1; i != 0; --i)
            {
                result = gather_here_impl<uniform_rows<value_type>>(
                    communicators.get(i), HPX_MOVE(result), this_site_arg(0),
                    run_gen, run_step)
                             .get();
            }

            return hpx::make_future<std::vector<value_type>>(
                gather_here_impl<uniform_rows<value_type>>(communicators.get(0),
                    HPX_MOVE(result), this_site_arg(0), run_gen, run_step),
                [](uniform_rows<value_type>&& data) {
                    return HPX_MOVE(data).unwrap_values();
                });
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<std::decay_t<T>>> gather_here(
        hierarchical_communicator const& communicators, T&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::gather_here(communicators, HPX_FORWARD(T, local_result),
            this_site, generation, detail::generation_mode::double_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) gather_here(Communicator&& comm, T&& local_result,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return gather_here(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) gather_here(char const* basename, T&& result,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return gather_here(create_communicator(basename, num_sites, this_site,
                               generation, root_site_arg(this_site.argument_)),
            HPX_FORWARD(T, result), this_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) gather_here(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return gather_here(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) gather_here(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return gather_here(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) gather_here(hpx::launch::sync_policy, char const* basename,
        T&& result, num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return gather_here(create_communicator(basename, num_sites, this_site,
                               generation, root_site_arg(this_site.argument_)),
            HPX_FORWARD(T, result), this_site)
            .get();
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // gather_there: detail entry point carrying the internal generation
        // step. The public overload forwards with single_step; the hierarchical
        // gather_there reuses it with the mapped step.
        template <typename T>
        hpx::future<void> gather_there(communicator fid, T&& local_result,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<void>(HPX_GET_EXCEPTION(
                    hpx::error::bad_parameter, "hpx::collectives::gather_there",
                    "the generation number shouldn't be zero"));
            }

            auto gather_there_data =
                [local_result = HPX_FORWARD(T, local_result), this_site,
                    generation, num_generations](
                    communicator&& c) mutable -> hpx::future<void> {
                using action_type =
                    communicator_server::communication_set_direct_action<
                        traits::communication::gather_tag, hpx::future<void>,
                        generation_mode, std::decay_t<T>>;

                // explicitly unwrap returned future
                hpx::future<void> result =
                    hpx::async(action_type(), c, this_site, generation,
                        num_generations, HPX_MOVE(local_result));

                if (!result.is_ready())
                {
                    // make sure id is kept alive as long as the returned future
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [client = HPX_MOVE(c)]() { HPX_UNUSED(client); });
                }

                return result;
            };

            return fid.then(hpx::launch::sync, HPX_MOVE(gather_there_data));
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // gather plain values
    HPX_CXX_EXPORT template <typename T>
    hpx::future<void> gather_there(communicator fid, T&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::gather_there(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            this_site, generation, detail::generation_mode::single_step);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // gather_there over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step; all_gather reuses it as its gather phase with
        // single_step.
        template <typename T>
        hpx::future<void> gather_there(
            hierarchical_communicator const& communicators, T&& local_result,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            if (auto const error =
                    validate_hierarchical_communicator(communicators, this_site,
                        "hpx::collectives::gather_there (hierarchical)"))
            {
                return hpx::make_exceptional_future<void>(error);
            }

            if (auto const error = validate_hierarchical_non_root_caller(
                    this_site, "hpx::collectives::gather_there (hierarchical)",
                    "gather_here"))
            {
                return hpx::make_exceptional_future<void>(error);
            }

            // See gather_here above for the internal generation mapping.
            if (!is_valid_hierarchical_run_generation(
                    generation, num_generations))
            {
                return hpx::make_exceptional_future<void>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::gather_there (hierarchical)",
                        "the generation number is too large for the internal "
                        "generation mapping"));
            }

            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            using value_type = std::decay_t<T>;
            uniform_rows<value_type> data(HPX_FORWARD(T, local_result));
            for (std::size_t i = communicators.size() - 1; i != 0; --i)
            {
                data = gather_here_impl<uniform_rows<value_type>>(
                    communicators.get(i), HPX_MOVE(data), this_site_arg(0),
                    run_gen, run_step)
                           .get();
            }

            return gather_there(communicators.get(0), HPX_MOVE(data),
                communicators.site(0), run_gen, run_step);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<void> gather_there(
        hierarchical_communicator const& communicators, T&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::gather_there(communicators, HPX_FORWARD(T, local_result),
            this_site, generation, detail::generation_mode::double_step);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) gather_there(Communicator&& comm, T&& local_result,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return gather_there(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) gather_there(char const* basename, T&& local_result,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        this_site_arg const effective_site =
            detail::resolve_this_site(this_site);

        if (auto const error =
                detail::validate_site_differs_from_root(effective_site,
                    root_site, "hpx::collectives::gather_there", "sending"))
        {
            return hpx::make_exceptional_future<void>(error);
        }

        return gather_there(create_communicator(basename, num_sites_arg(),
                                effective_site, generation, root_site),
            HPX_FORWARD(T, local_result), effective_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    void gather_there(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        gather_there(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    void gather_there(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        gather_there(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    void gather_there(hpx::launch::sync_policy, char const* basename,
        T&& local_result, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        gather_there(basename, HPX_FORWARD(T, local_result), this_site,
            generation, root_site)
            .get();
    }
}    // namespace hpx::collectives

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_GATHER_DECLARATION(...) /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_GATHER(...)             /**/

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
