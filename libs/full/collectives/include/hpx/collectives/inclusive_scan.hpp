//  Copyright (c) 2019-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file inclusive_scan.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// Inclusive inclusive_scan a set of values from different call sites
    ///
    /// This function performs an inclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  basename    The base name identifying the inclusive_scan operation
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the inclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the inclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The site that is responsible for creating the
    ///                     inclusive_scan support object. This value is optional
    ///                     and defaults to '0' (zero).
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the inclusive_scan operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<std::decay_t<T>> inclusive_scan(char const* basename, T&& result,
        F&& op, num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Inclusive inclusive_scan a set of values from different call sites
    ///
    /// This function performs an inclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the inclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the inclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the inclusive_scan operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<std::decay_t<T>> inclusive_scan(
        communicator comm, T&& result, F&& op,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Inclusive inclusive_scan a set of values from different call sites
    ///
    /// This function performs an inclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the inclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the inclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the inclusive_scan operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<std::decay_t<T>> inclusive_scan(
        communicator comm, T&& result, F&& op,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Inclusive inclusive_scan a set of values from different call sites
    ///
    /// This function performs an inclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  basename    The base name identifying the inclusive_scan operation
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the inclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the inclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The site that is responsible for creating the
    ///                     inclusive_scan support object. This value is optional
    ///                     and defaults to '0' (zero).
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T, typename F>
    decltype(auto) inclusive_scan(hpx::launch::sync_policy,
        char const* basename, T&& result, F&& op,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Inclusive inclusive_scan a set of values from different call sites
    ///
    /// This function performs an inclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the inclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the inclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T, typename F>
    decltype(auto) inclusive_scan(hpx::launch::sync_policy, communicator comm,
        T&& result, F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Inclusive inclusive_scan a set of values from different call sites
    ///
    /// This function performs an inclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the inclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the inclusive_scan operation on the
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
    template <typename T, typename F>
    decltype(auto) inclusive_scan(hpx::launch::sync_policy, communicator comm,
        T&& result, F&& op, generation_arg generation,
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
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>
#include <hpx/collectives/detail/hierarchical_scan_helpers.hpp>
#include <hpx/collectives/gather.hpp>
#include <hpx/collectives/scatter.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::traits {

    namespace communication {

        HPX_CXX_EXPORT struct inclusive_scan_tag;

        template <>
        struct communicator_data<inclusive_scan_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    ///////////////////////////////////////////////////////////////////////////
    // support for inclusive_scan
    template <typename Communicator>
    struct communication_operation<Communicator,
        communication::inclusive_scan_tag>
    {
        template <typename Result, typename T, typename F>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t,
            F&& op)
        {
            return communicator.template handle_data<std::decay_t<T>>(
                communication::communicator_data<
                    communication::inclusive_scan_tag>::name(),
                which, generation,
                // step function (invoked for each get)
                [&t](auto& data, std::size_t which) {
                    data[which] = HPX_FORWARD(T, t);
                },
                // finalizer (invoked after all data has been received)
                [op = HPX_FORWARD(F, op)](auto& data, bool& data_available,
                    std::size_t which) mutable {
                    if (!data_available)
                    {
                        using T_ = std::decay_t<T>;

                        data = collectives::detail::make_inclusive_scan_results<
                            T_>(HPX_MOVE(data), op);
                        data_available = true;
                    }
                    return hpx::collectives::detail::handle_bool<
                        std::decay_t<T>>(HPX_MOVE(data[which]));
                },
                num_generations);
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    ////////////////////////////////////////////////////////////////////////////
    // inclusive_scan plain values
    namespace detail {

        template <typename T, typename F>
        hpx::future<std::decay_t<T>> inclusive_scan(communicator fid,
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
                return hpx::make_exceptional_future<arg_type>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::inclusive_scan",
                        "the generation number shouldn't be zero"));
            }

            // Handle operation right away if there is only one value.
            if (auto [num_sites, comm_site] = fid.get_info(); num_sites == 1)
            {
                if (this_site != comm_site)
                {
                    return hpx::make_exceptional_future<arg_type>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::inclusive_scan",
                            "the local site should be zero if only one site is "
                            "involved"));
                }

                return hpx::make_ready_future(HPX_FORWARD(T, local_result));
            }

            auto inclusive_scan_data =
                [local_result = HPX_FORWARD(T, local_result),
                    op = HPX_FORWARD(F, op), this_site, generation,
                    num_generations](
                    communicator&& c) mutable -> hpx::future<arg_type> {
                using func_type = std::decay_t<F>;
                using action_type =
                    communicator_server::communication_get_direct_action<
                        traits::communication::inclusive_scan_tag,
                        hpx::future<arg_type>, generation_mode, arg_type,
                        func_type>;

                // explicitly unwrap returned future
                hpx::future<arg_type> result =
                    hpx::async(action_type(), c, this_site, generation,
                        num_generations, HPX_MOVE(local_result), HPX_MOVE(op));

                if (!result.is_ready())
                {
                    // make sure id is kept alive as long as the returned future
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [client = HPX_MOVE(c)] { HPX_UNUSED(client); });
                }

                return result;
            };

            return fid.then(hpx::launch::sync, HPX_MOVE(inclusive_scan_data));
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T, typename F>
    hpx::future<std::decay_t<T>> inclusive_scan(communicator fid,
        T&& local_result, F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::inclusive_scan(HPX_MOVE(fid),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site,
            generation, detail::generation_mode::single_step);
    }

    HPX_CXX_EXPORT template <typename T, typename F>
    hpx::future<std::decay_t<T>> inclusive_scan(communicator fid,
        T&& local_result, F&& op, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return inclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T, typename F>
    hpx::future<std::decay_t<T>> inclusive_scan(char const* basename,
        T&& local_result, F&& op,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return inclusive_scan(create_communicator(basename, num_sites,
                                  this_site, generation, root_site),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site);
    }

    // Every hierarchical collective advances each communicator by two
    // generations per call, so an instance may be shared freely across
    // collectives; see the note on create_hierarchical_communicator.
    HPX_CXX_EXPORT template <typename T, typename F>
    hpx::future<std::decay_t<T>> inclusive_scan(
        hierarchical_communicator const& communicators, T&& local_result,
        F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        using arg_type = std::decay_t<T>;
        char const* const operation =
            "hpx::collectives::inclusive_scan (hierarchical)";

        return detail::hierarchical_scan<arg_type>(
            operation, communicators, HPX_FORWARD(T, local_result), this_site,
            generation, root_site,
            [&op](std::vector<arg_type>&& gathered) {
                return detail::make_inclusive_scan_results(
                    HPX_MOVE(gathered), HPX_FORWARD(F, op));
            },
            [&op](communicator const& comm, T&& result,
                this_site_arg const flat_site, generation_arg const flat_gen) {
                return detail::inclusive_scan(comm, HPX_FORWARD(T, result),
                    HPX_FORWARD(F, op), flat_site, flat_gen,
                    detail::generation_mode::double_step);
            });
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename F>
    decltype(auto) inclusive_scan(hpx::launch::sync_policy, communicator fid,
        T&& local_result, F&& op,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return inclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename F>
    decltype(auto) inclusive_scan(hpx::launch::sync_policy, communicator fid,
        T&& local_result, F&& op, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return inclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename F>
    decltype(auto) inclusive_scan(hpx::launch::sync_policy,
        char const* basename, T&& local_result, F&& op,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return inclusive_scan(create_communicator(basename, num_sites,
                                  this_site, generation, root_site),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename F>
    decltype(auto) inclusive_scan(hpx::launch::sync_policy,
        hierarchical_communicator const& communicators, T&& local_result,
        F&& op, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        return inclusive_scan(communicators, HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation, root_site)
            .get();
    }
}    // namespace hpx::collectives

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
