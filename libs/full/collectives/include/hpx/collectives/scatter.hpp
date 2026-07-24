//  Copyright (c) 2014-2026 Hartmut Kaiser
//  Copyright (c) 2025 Lukas Zeil
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file scatter.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// Scatter (receive) a set of values to different call sites
    ///
    /// This function receives an element of a set of values operating on
    /// the given base name.
    ///
    /// \param  basename    The base name identifying the scatter operation
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The sequence number of the central scatter point
    ///                     (usually the locality id). This value is optional
    ///                     and defaults to 0.
    ///
    /// \returns    This function returns a future holding the
    ///             scattered value. It will become ready once the scatter
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<T> scatter_from(char const* basename,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Scatter (receive) a set of values to different call sites
    ///
    /// This function receives an element of a set of values operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns a future holding the
    ///             scattered value. It will become ready once the scatter
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<T> scatter_from(
        communicator comm,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Scatter (receive) a set of values to different call sites
    ///
    /// This function receives an element of a set of values operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns a future holding the
    ///             scattered value. It will become ready once the scatter
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<T> scatter_from(
        communicator comm,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Scatter (send) a part of the value set at the given call site
    ///
    /// This function transmits the value given by \a result to a central scatter
    /// site (where the corresponding \a scatter_from is executed)
    ///
    /// \param  basename    The base name identifying the scatter operation
    /// \param  result      The value to transmit to the central scatter point
    ///                     from this call site.
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a future holding the
    ///             scattered value. It will become ready once the scatter
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<T> scatter_to(char const* basename,
        std::vector<T>&& result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Scatter (send) a part of the value set at the given call site
    ///
    /// This function transmits the value given by \a result to a central scatter
    /// site (where the corresponding \a scatter_from is executed)
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central scatter point
    ///                     from this call site.
    /// \param  this_site   The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns a future holding the
    ///             scattered value. It will become ready once the scatter
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<T> scatter_to(
        communicator comm, std::vector<T>&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Scatter (send) a part of the value set at the given call site
    ///
    /// This function transmits the value given by \a result to a central scatter
    /// site (where the corresponding \a scatter_from is executed)
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central scatter point
    ///                     from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns a future holding the
    ///             scattered value. It will become ready once the scatter
    ///             operation has been completed.
    ///
    template <typename T>
    hpx::future<T> scatter_to(
        communicator comm, std::vector<T>&& result,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Scatter (receive) a set of values to different call sites
    ///
    /// This function receives an element of a set of values operating on
    /// the given base name.
    ///
    /// \param basename     The base name identifying the scatter operation
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The sequence number of the central scatter point
    ///                     (usually the locality id). This value is optional
    ///                     and defaults to 0.
    ///
    /// \returns    This function returns the scattered value. It executes
    ///             synchronously and directly returns the result.
    ///
    template <typename T>
    T scatter_from(hpx::launch::sync_policy, char const* basename,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Scatter (receive) a set of values to different call sites
    ///
    /// This function receives an element of a set of values operating on
    /// the given base name.
    ///
    /// \param comm         A communicator object returned from \a create_communicator
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns the scattered value. It executes
    ///             synchronously and directly returns the result.
    ///
    template <typename T>
    T scatter_from(hpx::launch::sync_policy, communicator comm,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Scatter (receive) a set of values to different call sites
    ///
    /// This function receives an element of a set of values operating on
    /// the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns the scattered value. It executes
    ///             synchronously and directly returns the result.
    ///
    template <typename T>
    T scatter_from(hpx::launch::sync_policy, communicator comm,
        generation_arg generation, this_site_arg this_site = this_site_arg());

    /// Scatter (send) a part of the value set at the given call site
    ///
    /// This function transmits the value given by \a result to a central scatter
    /// site (where the corresponding \a scatter_from is executed)
    ///
    /// \param  basename    The base name identifying the scatter operation
    /// \param  result      The value to transmit to the central scatter point
    ///                     from this call site.
    /// \param  num_sites   The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns the scattered value. It executes
    ///             synchronously and directly returns the result.
    ///
    template <typename T>
    T scatter_to(hpx::launch::sync_policy, char const* basename,
        std::vector<T>&& result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Scatter (send) a part of the value set at the given call site
    ///
    /// This function transmits the value given by \a result to a central scatter
    /// site (where the corresponding \a scatter_from is executed)
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central scatter point
    ///                     from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns the scattered value. It executes
    ///             synchronously and directly returns the result.
    ///
    template <typename T>
    T scatter_to(hpx::launch::sync_policy, communicator comm,
        std::vector<T>&& result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Scatter (send) a part of the value set at the given call site
    ///
    /// This function transmits the value given by \a result to a central scatter
    /// site (where the corresponding \a scatter_from is executed)
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to the central scatter point
    ///                     from this call site.
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the scatter operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the scatter operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       The generation values from corresponding \a scatter_to and
    ///             \a scatter_from have to match.
    ///
    /// \returns    This function returns the scattered value. It executes
    ///             synchronously and directly returns the result.
    ///
    template <typename T>
    T scatter_to(hpx::launch::sync_policy, communicator comm,
        std::vector<T>&& result, generation_arg generation,
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

namespace hpx::collectives::detail {

    // Distinguishes the internal row scatter protocol from an ordinary scatter
    // whose user value type happens to be uniform_rows<T>.
    struct uniform_scatter_tag
    {
    };

}    // namespace hpx::collectives::detail

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // support for scatter
    namespace communication {

        HPX_CXX_EXPORT struct scatter_tag;

        template <>
        struct communicator_data<scatter_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    template <typename Communicator>
    struct communication_operation<Communicator, communication::scatter_tag>
    {
        template <typename Result>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations)
        {
            using data_type = typename Result::result_type;
            return communicator.template handle_data<data_type>(
                communication::communicator_data<
                    communication::scatter_tag>::name(),
                which, generation,
                // step function (invoked once for get)
                nullptr,
                // finalizer (invoked after all sites have checked in)
                [](auto& data, bool&, std::size_t which) {
                    return hpx::collectives::detail::handle_bool<data_type>(
                        HPX_MOVE(data[which]));
                },
                num_generations);
        }

        template <typename Result>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations,
            hpx::collectives::detail::uniform_scatter_tag)
        {
            using data_type = typename Result::result_type;
            static_assert(
                hpx::collectives::detail::is_uniform_rows_v<data_type>);

            std::size_t const num_sites = communicator.num_sites_;
            return communicator.template handle_data<data_type>(
                communication::communicator_data<
                    communication::scatter_tag>::name(),
                which, generation,
                // step function (invoked once for get)
                nullptr,
                // finalizer (invoked after all sites have checked in)
                [num_sites](auto& data, bool&, std::size_t which) {
                    return HPX_MOVE(data[0]).extract(which, num_sites);
                },
                num_generations, 1);
        }

        template <typename Result, typename Payload>
        static Result set(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations,
            Payload&& t)
        {
            using payload_type = std::decay_t<Payload>;
            if constexpr (hpx::collectives::detail::is_uniform_rows_v<
                              payload_type>)
            {
                std::size_t const num_sites = communicator.num_sites_;
                return communicator.template handle_data<payload_type>(
                    communication::communicator_data<
                        communication::scatter_tag>::name(),
                    which, generation,
                    // step function (invoked once for set)
                    [&t](auto& data, std::size_t) {
                        data[0] = HPX_FORWARD(Payload, t);
                    },
                    // finalizer (invoked after all sites have checked in)
                    [num_sites](auto& data, bool&, std::size_t which) {
                        return HPX_MOVE(data[0]).extract(which, num_sites);
                    },
                    num_generations, 1);
            }
            else
            {
                using data_type = typename payload_type::value_type;
                return communicator.template handle_data<data_type>(
                    communication::communicator_data<
                        communication::scatter_tag>::name(),
                    which, generation,
                    // step function (invoked once for set)
                    [&t](auto& data, std::size_t) {
                        data = HPX_FORWARD(Payload, t);
                    },
                    // finalizer (invoked after all sites have checked in)
                    [](auto& data, bool&, std::size_t which) {
                        return hpx::collectives::detail::handle_bool<data_type>(
                            HPX_MOVE(data[which]));
                    },
                    num_generations);
            }
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    ///////////////////////////////////////////////////////////////////////////
    // destination site needs to be handled differently
    namespace detail {

        // scatter_from: detail entry point carrying the internal generation
        // step. The public overload forwards with single_step; the hierarchical
        // scatter_from walks its sub-communicators through this entry with the
        // mapped step.
        template <typename T, bool UseUniformRows = false>
        hpx::future<T> scatter_from_impl(communicator fid,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<T>(HPX_GET_EXCEPTION(
                    hpx::error::bad_parameter, "hpx::collectives::scatter_from",
                    "the generation number shouldn't be zero"));
            }

            auto scatter_from_data = [this_site, generation, num_generations](
                                         communicator&& c) -> hpx::future<T> {
                // Explicitly unwrap the returned future. Only the internal
                // uniform-row protocol adds a tag to the action signature.
                hpx::future<T> result = [&]() {
                    if constexpr (UseUniformRows)
                    {
                        using action_type = communicator_server::
                            communication_get_direct_action<
                                traits::communication::scatter_tag,
                                hpx::future<T>, generation_mode,
                                uniform_scatter_tag>;
                        return hpx::async(action_type(), c, this_site,
                            generation, num_generations, uniform_scatter_tag{});
                    }
                    else
                    {
                        using action_type = communicator_server::
                            communication_get_direct_action<
                                traits::communication::scatter_tag,
                                hpx::future<T>, generation_mode>;
                        return hpx::async(action_type(), c, this_site,
                            generation, num_generations);
                    }
                }();

                if (!result.is_ready())
                {
                    // make sure id is kept alive as long as the returned future
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [client = HPX_MOVE(c)] { HPX_UNUSED(client); });
                }

                return result;
            };

            return fid.then(hpx::launch::sync, HPX_MOVE(scatter_from_data));
        }

        template <typename T>
        hpx::future<uniform_rows<T>> scatter_rows_from(communicator fid,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            return scatter_from_impl<uniform_rows<T>, true>(
                HPX_MOVE(fid), this_site, generation, num_generations);
        }

    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> scatter_from(communicator fid,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::scatter_from_impl<T>(HPX_MOVE(fid), this_site,
            generation, detail::generation_mode::single_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Forward declaration: the flat scatter_to detail entry is defined
        // further down, but the hierarchical scatter_from below walks its
        // sub-communicators through it.
        template <typename Result, typename Payload>
        hpx::future<Result> scatter_to_impl(communicator fid,
            Payload&& local_result, this_site_arg this_site,
            generation_arg generation, generation_mode num_generations);

        template <typename T>
        hpx::future<uniform_rows<T>> scatter_rows_to(communicator fid,
            uniform_rows<T>&& local_result, this_site_arg this_site,
            generation_arg generation, generation_mode num_generations);

        // Ordinary communicator storage requires default construction at the
        // leaf; retain the uniform carrier for payloads without it.
        template <typename T>
        hpx::future<T> scatter_leaf_from(communicator fid,
            this_site_arg this_site, generation_arg generation,
            generation_mode num_generations)
        {
            if constexpr (std::is_default_constructible_v<T>)
            {
                return scatter_from_impl<T>(
                    HPX_MOVE(fid), this_site, generation, num_generations);
            }
            else
            {
                return hpx::make_future<T>(
                    scatter_rows_from<T>(
                        HPX_MOVE(fid), this_site, generation, num_generations),
                    [](uniform_rows<T>&& result) {
                        return HPX_MOVE(result).unwrap_value();
                    });
            }
        }

        template <typename T>
        hpx::future<T> scatter_leaf_to(communicator fid,
            uniform_rows<T>&& local_result, this_site_arg this_site,
            generation_arg generation, generation_mode num_generations)
        {
            if constexpr (std::is_default_constructible_v<T>)
            {
                return scatter_to_impl<T>(HPX_MOVE(fid),
                    HPX_MOVE(local_result).unwrap_values(), this_site,
                    generation, num_generations);
            }
            else
            {
                return hpx::make_future<T>(
                    scatter_rows_to(HPX_MOVE(fid), HPX_MOVE(local_result),
                        this_site, generation, num_generations),
                    [](uniform_rows<T>&& result) {
                        return HPX_MOVE(result).unwrap_value();
                    });
            }
        }

        // scatter_from over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step; scan collectives reuse it as their scatter phase with
        // single_step.
        template <typename T>
        hpx::future<T> scatter_from(
            hierarchical_communicator const& communicators,
            this_site_arg this_site, generation_arg const generation,
            generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            if (auto const error =
                    validate_hierarchical_communicator(communicators, this_site,
                        "hpx::collectives::scatter_from (hierarchical)"))
            {
                return hpx::make_exceptional_future<T>(error);
            }

            if (auto const error = validate_hierarchical_non_root_caller(
                    this_site, "hpx::collectives::scatter_from (hierarchical)",
                    "scatter_to"))
            {
                return hpx::make_exceptional_future<T>(error);
            }

            if (!is_valid_hierarchical_run_generation(
                    generation, num_generations))
            {
                return hpx::make_exceptional_future<T>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::scatter_from (hierarchical)",
                        "the generation number is too large for the internal "
                        "generation mapping"));
            }

            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            auto [current_communicator, current_site] = communicators[0];
            if (communicators.size() == 1)
            {
                return scatter_leaf_from<T>(
                    current_communicator, current_site, run_gen, run_step);
            }

            uniform_rows<T> data = scatter_rows_from<T>(
                current_communicator, current_site, run_gen, run_step)
                                       .get();

            for (std::size_t i = 1; i < communicators.size() - 1; ++i)
            {
                data = scatter_rows_to(communicators.get(i), HPX_MOVE(data),
                    this_site_arg(0), run_gen, run_step)
                           .get();
            }

            return scatter_leaf_to(communicators.back(), HPX_MOVE(data),
                this_site_arg(0), run_gen, run_step);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> scatter_from(hierarchical_communicator const& communicators,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::scatter_from<T>(communicators, this_site, generation,
            detail::generation_mode::double_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename Communicator>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) scatter_from(Communicator&& comm,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return scatter_from<T>(
            HPX_FORWARD(Communicator, comm), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) scatter_from(char const* basename,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        this_site_arg const effective_site =
            detail::resolve_this_site(this_site);

        if (auto const error =
                detail::validate_site_differs_from_root(effective_site,
                    root_site, "hpx::collectives::scatter_from", "receiving"))
        {
            return hpx::make_exceptional_future<T>(error);
        }

        return scatter_from<T>(create_communicator(basename, num_sites_arg(),
                                   effective_site, generation, root_site),
            effective_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename Communicator>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) scatter_from(hpx::launch::sync_policy, Communicator&& comm,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return scatter_from<T>(
            HPX_FORWARD(Communicator, comm), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename Communicator>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) scatter_from(hpx::launch::sync_policy, Communicator&& comm,
        generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return scatter_from<T>(
            HPX_FORWARD(Communicator, comm), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) scatter_from(hpx::launch::sync_policy, char const* basename,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return scatter_from<T>(basename, this_site, generation, root_site)
            .get();
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // scatter_to: detail entry point carrying the internal generation step.
        // The public overload forwards with single_step; the hierarchical
        // scatter_to walks its sub-communicators through this entry with the
        // mapped step.
        template <typename Result, typename Payload>
        hpx::future<Result> scatter_to_impl(communicator fid,
            Payload&& local_result, this_site_arg this_site,
            generation_arg const generation, generation_mode num_generations)
        {
            using payload_type = std::decay_t<Payload>;
            constexpr bool uniform_payload = is_uniform_rows_v<payload_type>;

            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<Result>(HPX_GET_EXCEPTION(
                    hpx::error::bad_parameter, "hpx::collectives::scatter_to",
                    "the generation number shouldn't be zero"));
            }

            auto [num_sites, comm_site] = fid.get_info();

            if constexpr (uniform_payload)
            {
                static_assert(std::is_same_v<Result, payload_type>);
                local_result.validate("hpx::collectives::scatter_to");
                if (local_result.num_rows() < num_sites)
                {
                    return hpx::make_exceptional_future<Result>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::scatter_to",
                            "the uniform-row payload must contain at least one "
                            "row per participating site"));
                }
            }
            else if (local_result.size() != num_sites)
            {
                return hpx::make_exceptional_future<Result>(HPX_GET_EXCEPTION(
                    hpx::error::bad_parameter, "hpx::collectives::scatter_to",
                    "the number of values to scatter must be equal to the "
                    "number of participating sites"));
            }

            // Handle operation right away if there is only one value.
            if (num_sites == 1)
            {
                if (this_site != comm_site)
                {
                    return hpx::make_exceptional_future<Result>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::scatter_to",
                            "the local site should be zero if only one site is "
                            "involved"));
                }

                if constexpr (uniform_payload)
                {
                    return hpx::make_ready_future(
                        HPX_FORWARD(Payload, local_result));
                }
                else
                {
                    return hpx::make_ready_future(
                        handle_bool<Result>(HPX_MOVE(local_result[0])));
                }
            }

            auto scatter_to_data =
                [local_result = HPX_FORWARD(Payload, local_result), this_site,
                    generation, num_generations](
                    communicator&& c) mutable -> hpx::future<Result> {
                using action_type =
                    communicator_server::communication_set_direct_action<
                        traits::communication::scatter_tag, hpx::future<Result>,
                        generation_mode, payload_type>;

                // explicitly unwrap returned future
                hpx::future<Result> result =
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

            return fid.then(hpx::launch::sync, HPX_MOVE(scatter_to_data));
        }

        template <typename T>
        hpx::future<uniform_rows<T>> scatter_rows_to(communicator fid,
            uniform_rows<T>&& local_result, this_site_arg this_site,
            generation_arg const generation, generation_mode num_generations)
        {
            return scatter_to_impl<uniform_rows<T>>(HPX_MOVE(fid),
                HPX_MOVE(local_result), this_site, generation, num_generations);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // scatter implementation
    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> scatter_to(communicator fid, std::vector<T>&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::scatter_to_impl<T>(HPX_MOVE(fid), HPX_MOVE(local_result),
            this_site, generation, detail::generation_mode::single_step);
    }

    namespace detail {

        // scatter_to over a hierarchical_communicator: detail entry carrying
        // the internal generation step. The public overload forwards with
        // double_step; scan collectives reuse it as their scatter phase with
        // single_step.
        template <typename T>
        hpx::future<T> scatter_to(
            hierarchical_communicator const& communicators,
            std::vector<T>&& local_result, this_site_arg this_site,
            generation_arg const generation, generation_mode num_generations)
        {
            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }

            if (auto const error =
                    validate_hierarchical_communicator(communicators, this_site,
                        "hpx::collectives::scatter_to (hierarchical)"))
            {
                return hpx::make_exceptional_future<T>(error);
            }

            std::size_t const num_sites_val =
                hpx::get<0>(communicators.get_info());

            if (auto const error = validate_hierarchical_root_caller(this_site,
                    "hpx::collectives::scatter_to (hierarchical)",
                    "scatter_to"))
            {
                return hpx::make_exceptional_future<T>(error);
            }

            if (local_result.size() != num_sites_val)
            {
                return hpx::make_exceptional_future<T>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::scatter_to (hierarchical)",
                        "the number of values to scatter must be equal to the "
                        "number of participating sites"));
            }

            if (!is_valid_hierarchical_run_generation(
                    generation, num_generations))
            {
                return hpx::make_exceptional_future<T>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::scatter_to (hierarchical)",
                        "the generation number is too large for the internal "
                        "generation mapping"));
            }

            auto const [run_gen, run_step] =
                hierarchical_run_params(generation, num_generations);

            auto [current_communicator, current_site] = communicators[0];
            if (communicators.size() == 1)
            {
                return scatter_leaf_to(current_communicator,
                    uniform_rows<T>(HPX_MOVE(local_result)), current_site,
                    run_gen, run_step);
            }

            uniform_rows<T> data(HPX_MOVE(local_result));
            for (std::size_t i = 0; i < communicators.size() - 1; ++i)
            {
                data = scatter_rows_to(communicators.get(i), HPX_MOVE(data),
                    this_site_arg(0), run_gen, run_step)
                           .get();
            }

            return scatter_leaf_to(communicators.back(), HPX_MOVE(data),
                this_site_arg(0), run_gen, run_step);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> scatter_to(hierarchical_communicator const& communicators,
        std::vector<T>&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::scatter_to(communicators, HPX_MOVE(local_result),
            this_site, generation, detail::generation_mode::double_step);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) scatter_to(Communicator&& comm,
        std::vector<T>&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return scatter_to(HPX_FORWARD(Communicator, comm),
            HPX_MOVE(local_result), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) scatter_to(char const* basename,
        std::vector<T>&& local_result,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return scatter_to(create_communicator(basename, num_sites, this_site,
                              generation, root_site_arg(this_site.argument_)),
            HPX_MOVE(local_result), this_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) scatter_to(hpx::launch::sync_policy, Communicator&& comm,
        std::vector<T>&& local_result,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return scatter_to(HPX_FORWARD(Communicator, comm),
            HPX_MOVE(local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    decltype(auto) scatter_to(hpx::launch::sync_policy, Communicator&& comm,
        std::vector<T>&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return scatter_to(HPX_FORWARD(Communicator, comm),
            HPX_MOVE(local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    decltype(auto) scatter_to(hpx::launch::sync_policy, char const* basename,
        std::vector<T>&& local_result,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return scatter_to(create_communicator(basename, num_sites, this_site,
                              generation, root_site_arg(this_site.argument_)),
            HPX_MOVE(local_result), this_site)
            .get();
    }
}    // namespace hpx::collectives

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_SCATTER_DECLARATION(...) /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_SCATTER(...)             /**/

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
