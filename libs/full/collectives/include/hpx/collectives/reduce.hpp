//  Copyright (c) 2019-2026 Hartmut Kaiser
//  Copyright (c) 2025 Lukas Zeil
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file reduce.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// Reduce a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  basename    The base name identifying the reduce operation
    /// \param  result      A value to reduce on the central reduction point
    ///                     from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the reduce operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<std::decay_t<T>> reduce_here(char const* basename, T&& result,
        F&& op, num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Reduce a set of values from different call sites
    ///
    /// This function receives a set of values that are the result of applying
    /// a given operator on values supplied from all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the root_site from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    /// \returns    This function returns a future holding a value calculated
    ///             based on the values send by all participating sites. It will
    ///             become ready once the reduce operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<decay_t<T>> reduce_here(
        communicator comm, T&& result, F&& op,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Reduce a set of values from different call sites
    ///
    /// This function receives a set of values that are the result of applying
    /// a given operator on values supplied from all call sites operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the root_site from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    /// \returns    This function returns a future holding a value calculated
    ///             based on the values send by all participating sites. It will
    ///             become ready once the reduce operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<decay_t<T>> reduce_here(
        communicator comm, T&& result, F&& op,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Reduce a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central reduce
    /// site (where the corresponding \a reduce_here is executed)
    ///
    /// \param  basename    The base name identifying the reduction operation
    /// \param  result      A future referring to the value to transmit to the
    ///                     central reduction point from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The sequence number of the central reduction point
    ///                     (usually the locality id). This value is optional
    ///                     and defaults to 0.
    ///
    /// \returns    This function returns a future<void>. It will become ready
    ///             once the reduction operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<void> reduce_there(char const* basename, T&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Reduce a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central reduce
    /// site (where the corresponding \a reduce_here is executed)
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the central reduction point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    /// \returns    This function returns a future holding a value calculated
    ///             based on the values send by all participating sites. It will
    ///             become ready once the reduce operation has been completed.
    ///
    template <typename T>
    hpx::future<void> reduce_there(
        communicator comm, T&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Reduce a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central reduce
    /// site (where the corresponding \a reduce_here is executed)
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the central reduction point
    ///                     from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    /// \returns    This function returns a future holding a value calculated
    ///             based on the values send by all participating sites. It will
    ///             become ready once the reduce operation has been completed.
    ///
    template <typename T>
    hpx::future<void> reduce_there(
        communicator comm, T&& result,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Reduce a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  basename    The base name identifying the reduce operation
    /// \param  result      A value to reduce on the central reduction point
    ///                     from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns The final reduced value after applying `op` to all contributions.
    ///
    template <typename T, typename F>
    decltype(auto) reduce_here(hpx::launch::sync_policy, char const* basename,
        T&& result, F&& op, num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Reduce a set of values from different call sites
    ///
    /// This function receives a set of values that are the result of applying
    /// a given operator on values supplied from all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the root_site from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    /// \returns The final reduced value after applying `op` to all contributions.
    ///
    template <typename T, typename F>
    decltype(auto) reduce_here(hpx::launch::sync_policy, communicator comm,
        T&& result, F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Reduce a set of values from different call sites
    ///
    /// This function receives a set of values that are the result of applying
    /// a given operator on values supplied from all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the root_site from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    /// \returns The final reduced value after applying `op` to all contributions.
    ///
    template <typename T, typename F>
    decltype(auto) reduce_here(hpx::launch::sync_policy, communicator comm,
        T&& result, F&& op, generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Reduce a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central reduce
    /// site (where the corresponding \a reduce_here is executed)
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  basename    The base name identifying the reduction operation
    /// \param  result      A future referring to the value to transmit to the
    ///                     central reduction point from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The sequence number of the central reduction point
    ///                     (usually the locality id). This value is optional
    ///                     and defaults to 0.
    ///
    template <typename T>
    void reduce_there(hpx::launch::sync_policy, char const* basename,
        T&& result, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Reduce a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central reduce
    /// site (where the corresponding \a reduce_here is executed)
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the central reduction point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    template <typename T>
    void reduce_there(hpx::launch::sync_policy, communicator comm,
        T&& result, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Reduce a given value at the given call site
    ///
    /// This function transmits the value given by \a result to a central reduce
    /// site (where the corresponding \a reduce_here is executed)
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      A value to reduce on the central reduction point
    ///                     from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the reduce operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the reduce operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a reduce_here and
    ///             \a reduce_there have to match.
    ///
    template <typename T>
    void reduce_there(hpx::launch::sync_policy, communicator comm,
        T&& result, generation_arg generation,
        this_site_arg this_site = this_site_arg());
}}    // namespace hpx::collectives

// clang-format on
#else

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
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

    namespace communication {

        HPX_CXX_EXPORT struct reduce_tag;

        template <>
        struct communicator_data<reduce_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    ///////////////////////////////////////////////////////////////////////////
    // support for reduce
    template <typename Communicator>
    struct communication_operation<Communicator, communication::reduce_tag>
    {
        template <typename Result, typename T, typename F>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t,
            F&& op)
        {
            return communicator.template handle_data<std::decay_t<T>>(
                communication::communicator_data<
                    communication::reduce_tag>::name(),
                which, generation,
                // step function (invoked once for get)
                [&t](auto& data, std::size_t site) {
                    data[site] = HPX_FORWARD(T, t);
                },
                // finalizer (invoked once after all data has been received)
                [op = HPX_FORWARD(F, op)](
                    auto& data, bool&, std::size_t) mutable {
                    HPX_ASSERT(!data.empty());

                    if constexpr (!std::is_same_v<std::decay_t<T>, bool>)
                    {
                        if (data.size() > 1)
                        {
                            auto it = data.begin();
                            return hpx::reduce(++it, data.end(),
                                HPX_MOVE(data[0]), HPX_FORWARD(F, op));
                        }
                        return HPX_MOVE(data[0]);
                    }
                    else
                    {
                        if (data.size() > 1)
                        {
                            auto it = data.begin();
                            return static_cast<bool>(hpx::reduce(++it,
                                data.end(), static_cast<bool>(data[0]),
                                [&](auto lhs, auto rhs) {
                                    return HPX_FORWARD(F, op)(
                                        static_cast<bool>(lhs),
                                        static_cast<bool>(rhs));
                                }));
                        }
                        return static_cast<bool>(data[0]);
                    }
                },
                num_generations);
        }

        template <typename Result, typename T>
        static Result set(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t)
        {
            return communicator.template handle_data<std::decay_t<T>>(
                communication::communicator_data<
                    communication::reduce_tag>::name(),
                which, generation,
                // step function (invoked for each set)
                [t = HPX_FORWARD(T, t)](auto& data, std::size_t site) mutable {
                    data[site] = HPX_FORWARD(T, t);
                },
                // no finalizer
                nullptr, num_generations);
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    ///////////////////////////////////////////////////////////////////////////
    // reduce implementation
    namespace detail {

        // reduce_here: detail entry point carrying the internal generation
        // step. The public overload forwards with single_step; the hierarchical
        // reduce_here walks its sub-communicators through this entry with the
        // mapped step.
        template <typename T, typename F>
        hpx::future<std::decay_t<T>> reduce_here(communicator fid,
            T&& local_result, F&& op, this_site_arg this_site,
            generation_arg const generation, generation_mode num_generations)
        {
            using arg_type = std::decay_t<T>;

            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<arg_type>(HPX_GET_EXCEPTION(
                    hpx::error::bad_parameter, "hpx::collectives::reduce_here",
                    "the generation number shouldn't be zero"));
            }

            // Handle operation right away if there is only one value.
            if (auto [num_sites, comm_site] = fid.get_info(); num_sites == 1)
            {
                if (this_site != comm_site)
                {
                    return hpx::make_exceptional_future<arg_type>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::reduce_here",
                            "the local site should be zero if only one site is "
                            "involved"));
                }

                return hpx::make_ready_future(HPX_FORWARD(T, local_result));
            }

            auto reduction_data =
                [local_result = HPX_FORWARD(T, local_result),
                    op = HPX_FORWARD(F, op), this_site, generation,
                    num_generations](
                    communicator&& c) mutable -> hpx::future<arg_type> {
                using func_type = std::decay_t<F>;
                using action_type =
                    communicator_server::communication_get_direct_action<
                        traits::communication::reduce_tag,
                        hpx::future<arg_type>, generation_mode, arg_type,
                        func_type>;

                // explicitly unwrap returned future
                hpx::future<arg_type> result = hpx::async(action_type(), c,
                    this_site, generation, num_generations,
                    HPX_FORWARD(T, local_result), HPX_FORWARD(F, op));

                if (!result.is_ready())
                {
                    // make sure id is kept alive as long as the returned future
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [client = HPX_MOVE(c)] { HPX_UNUSED(client); });
                }

                return result;
            };

            return fid.then(hpx::launch::sync, HPX_MOVE(reduction_data));
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // reduce implementation
    HPX_CXX_EXPORT template <typename T, typename F>
    hpx::future<std::decay_t<T>> reduce_here(communicator fid, T&& local_result,
        F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::reduce_here(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation,
            detail::generation_mode::single_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {

        // reduce_here over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step; all_reduce reuses it as its reduce phase with
        // single_step (it passes 2k-1 already).
        template <typename T, typename F>
        hpx::future<std::decay_t<T>> reduce_here(
            hierarchical_communicator const& communicators, T&& local_result,
            F&& op, this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            std::decay_t<T> result = HPX_FORWARD(T, local_result);
            for (std::size_t i = communicators.size() - 1; i != 0; --i)
            {
                result = reduce_here(communicators.get(i), HPX_MOVE(result), op,
                    this_site_arg(0), run_gen, run_step)
                             .get();
            }

            return reduce_here(communicators.get(0), HPX_MOVE(result),
                HPX_FORWARD(F, op), this_site_arg(0), run_gen, run_step);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T, typename F>
    hpx::future<std::decay_t<T>> reduce_here(
        hierarchical_communicator const& communicators, T&& local_result,
        F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::reduce_here(communicators, HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation,
            detail::generation_mode::double_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T, typename F>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) reduce_here(Communicator&& comm, T&& local_result, F&& op,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return reduce_here(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site,
            generation);
    }

    HPX_CXX_EXPORT template <typename T, typename F>
    decltype(auto) reduce_here(char const* basename, T&& result, F&& op,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return reduce_here(create_communicator(basename, num_sites, this_site,
                               generation, root_site_arg(this_site.argument_)),
            HPX_FORWARD(T, result), HPX_FORWARD(F, op), this_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T, typename F>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) reduce_here(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, F&& op,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return reduce_here(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site,
            generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename Communicator, typename T, typename F>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) reduce_here(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, F&& op, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return reduce_here(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site,
            generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename F>
    decltype(auto) reduce_here(hpx::launch::sync_policy, char const* basename,
        T&& result, F&& op, num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return reduce_here(create_communicator(basename, num_sites, this_site,
                               generation, root_site_arg(this_site.argument_)),
            HPX_FORWARD(T, result), HPX_FORWARD(F, op), this_site)
            .get();
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // reduce_there: detail entry point carrying the internal generation
        // step. The public overload forwards with single_step; the hierarchical
        // reduce_there reuses it with the mapped step.
        template <typename T>
        hpx::future<void> reduce_there(communicator fid, T&& local_result,
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
                    hpx::error::bad_parameter, "hpx::collectives::reduce_there",
                    "the generation number shouldn't be zero"));
            }

            auto reduction_data =
                [local_result = HPX_FORWARD(T, local_result), this_site,
                    generation, num_generations](
                    communicator&& c) mutable -> hpx::future<void> {
                using action_type =
                    communicator_server::communication_set_direct_action<
                        traits::communication::reduce_tag, hpx::future<void>,
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

            return fid.then(hpx::launch::sync, HPX_MOVE(reduction_data));
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // reduce plain values
    HPX_CXX_EXPORT template <typename T>
    hpx::future<void> reduce_there(communicator fid, T&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::reduce_there(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            this_site, generation, detail::generation_mode::single_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {

        // reduce_there over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step; all_reduce reuses it as its reduce phase with
        // single_step.
        template <typename T, typename F>
        hpx::future<void> reduce_there(
            hierarchical_communicator const& communicators, T&& local_result,
            F&& op, this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            // See reduce_here above for the internal generation mapping.
            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            std::decay_t<T> result = HPX_FORWARD(T, local_result);
            for (std::size_t i = communicators.size() - 1; i != 0; --i)
            {
                result = reduce_here(communicators.get(i), HPX_MOVE(result), op,
                    this_site_arg(0), run_gen, run_step)
                             .get();
            }

            return reduce_there(communicators.get(0), HPX_MOVE(result),
                communicators.site(0), run_gen, run_step);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T, typename F>
    hpx::future<void> reduce_there(
        hierarchical_communicator const& communicators, T&& local_result,
        F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::reduce_there(communicators, HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation,
            detail::generation_mode::double_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) reduce_there(Communicator&& comm, T&& local_result,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return reduce_there(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) reduce_there(char const* basename, T&& local_result,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        HPX_ASSERT(this_site != root_site);
        return reduce_there(create_communicator(basename, num_sites_arg(),
                                this_site, generation, root_site),
            HPX_FORWARD(T, local_result), this_site);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    void reduce_there(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        reduce_there(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    void reduce_there(hpx::launch::sync_policy, Communicator&& comm,
        T&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        reduce_there(HPX_FORWARD(Communicator, comm),
            HPX_FORWARD(T, local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    void reduce_there(hpx::launch::sync_policy, char const* basename,
        T&& local_result, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        HPX_ASSERT(this_site != root_site);
        reduce_there(create_communicator(basename, num_sites_arg(), this_site,
                         generation, root_site),
            HPX_FORWARD(T, local_result), this_site)
            .get();
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename F>
    void reduce(communicator comm, T&& local_result, F&& op,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        if (this_site.is_default())
        {
            this_site = static_cast<std::size_t>(agas::get_locality_id());
        }

        comm.wait();    // make sure communicator was created

        if (this_site == std::get<2>(comm.get_info_ex()))
        {
            local_result = reduce_here(hpx::launch::sync, HPX_MOVE(comm),
                HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site,
                generation);
        }
        else
        {
            reduce_there(hpx::launch::sync, HPX_MOVE(comm),
                HPX_FORWARD(T, local_result), this_site, generation);
        }
    }
}    // namespace hpx::collectives

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
