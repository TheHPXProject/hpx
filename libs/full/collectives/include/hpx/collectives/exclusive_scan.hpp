//  Copyright (c) 2019-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file exclusive_scan.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// Exclusive scan a set of values from different call sites
    ///
    /// This function performs an exclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  basename    The base name identifying the exclusive_scan operation
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
    ///                     number of the exclusive_scan operation performed on
    ///                     the given base name. This is optional and needs to
    ///                     be supplied only if the exclusive_scan operation on
    ///                     the given base name has to be performed more than
    ///                     once. The generation number (if given) must be a
    ///                     positive number greater than zero.
    /// \param  root_site   The site that is responsible for creating the
    ///                     exclusive_scan support object. This value is
    ///                     optional and defaults to '0' (zero).
    ///
    /// \note       This no-init overload has MPI_Exscan-like semantics, with
    ///             a deterministic HPX first result: participating site 0
    ///             returns a value-initialized result (`std::decay_t<T>{}`, or
    ///             `false` for `bool`). Use the overload taking an explicit
    ///             `init` argument for C++ exclusive-scan semantics where site
    ///             0 returns `init`.
    ///
    /// \returns    For the participating site i this function returns the
    ///             reduction (calculated according to the function op) of
    ///             the values passed in by the participating sites 0, ..., i-1.
    ///             Participating site 0 returns a value-initialized result. The
    ///             returned future will become ready once the exclusive_scan
    ///             operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<std::decay_t<T>> exclusive_scan(char const* basename, T&& result,
        F&& op, num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Exclusive scan a set of values from different call sites
    ///
    /// This function performs an exclusive scan operation on a set of values
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
    ///                     number of the exclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the exclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \note       This no-init overload has MPI_Exscan-like semantics, with
    ///             a deterministic HPX first result: participating site 0
    ///             returns a value-initialized result (`std::decay_t<T>{}`, or
    ///             `false` for `bool`). Use the overload taking an explicit
    ///             `init` argument for C++ exclusive-scan semantics where site
    ///             0 returns `init`.
    ///
    /// \returns    For the participating site i this function returns the
    ///             reduction (calculated according to the function op) of
    ///             the values passed in by the participating sites 0, ..., i-1.
    ///             Participating site 0 returns a value-initialized result. The
    ///             returned future will become ready once the exclusive_scan
    ///             operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<std::decay_t<T>> exclusive_scan(
        communicator comm, T&& result, F&& op,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Exclusive scan a set of values from different call sites
    ///
    /// This function performs an exclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the exclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the exclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       This no-init overload has MPI_Exscan-like semantics, with
    ///             a deterministic HPX first result: participating site 0
    ///             returns a value-initialized result (`std::decay_t<T>{}`, or
    ///             `false` for `bool`). Use the overload taking an explicit
    ///             `init` argument for C++ exclusive-scan semantics where site
    ///             0 returns `init`.
    ///
    /// \returns    For the participating site i this function returns a future
    ///             the reduction (calculated according to the function op) of
    ///             the values passed in by the participating sites 0, ..., i-1.
    ///             Participating site 0 returns a value-initialized result. The
    ///             returned future will become ready once the exclusive_scan
    ///             operation has been completed.
    ///
    template <typename T, typename F>
    hpx::future<std::decay_t<T>> exclusive_scan(
        communicator comm, T&& result, F&& op,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// Exclusive scan a set of values from different call sites
    ///
    /// This function performs an exclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  basename    The base name identifying the exclusive_scan operation
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
    ///                     number of the exclusive_scan operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the exclusive_scan operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param  root_site   The site that is responsible for creating the
    ///                     exclusive_scan support object. This value is optional
    ///                     and defaults to '0' (zero).
    ///
    /// \note       This no-init overload has MPI_Exscan-like semantics, with
    ///             a deterministic HPX first result: participating site 0
    ///             returns a value-initialized result (`std::decay_t<T>{}`, or
    ///             `false` for `bool`). Use the overload taking an explicit
    ///             `init` argument for C++ exclusive-scan semantics where site
    ///             0 returns `init`.
    ///
    /// \returns    For the participating site i this function returns a future
    ///             the reduction (calculated according to the function op) of
    ///             the values passed in by the participating sites 0, ..., i-1.
    ///             Participating site 0 returns a value-initialized result. The
    ///             operation completes before this function returns.
    ///
    template <typename T, typename F>
    decltype(auto) exclusive_scan(hpx::launch::sync_policy,
        char const* basename, T&& result, F&& op,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg());

    /// Exclusive scan a set of values from different call sites
    ///
    /// This function performs an exclusive scan operation on a set of values
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
    ///                     number of the exclusive_scan operation performed on
    ///                     the given base name. This is optional and needs to
    ///                     be supplied only if the exclusive_scan operation on
    ///                     the given base name has to be performed more than
    ///                     once. The generation number (if given) must be a
    ///                     positive number greater than zero.
    ///
    /// \note       This no-init overload has MPI_Exscan-like semantics, with
    ///             a deterministic HPX first result: participating site 0
    ///             returns a value-initialized result (`std::decay_t<T>{}`, or
    ///             `false` for `bool`). Use the overload taking an explicit
    ///             `init` argument for C++ exclusive-scan semantics where site
    ///             0 returns `init`.
    ///
    /// \returns    For the participating site i this function returns a future
    ///             the reduction (calculated according to the function op) of
    ///             the values passed in by the participating sites 0, ..., i-1.
    ///             Participating site 0 returns a value-initialized result. The
    ///             operation completes before this function returns.
    ///
    template <typename T, typename F>
    decltype(auto) exclusive_scan(hpx::launch::sync_policy, communicator comm,
        T&& result, F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// Exclusive scan a set of values from different call sites
    ///
    /// This function performs an exclusive scan operation on a set of values
    /// received from all call sites operating on the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param  comm        A communicator object returned from \a create_communicator
    /// \param  result      The value to transmit to all
    ///                     participating sites from this call site.
    /// \param  op          Reduction operation to apply to all values supplied
    ///                     from all participating sites
    /// \param  generation  The generational counter identifying the sequence
    ///                     number of the exclusive_scan operation performed on
    ///                     the given base name. This is optional and needs to
    ///                     be supplied only if the exclusive_scan operation on
    ///                     the given base name has to be performed more than
    ///                     once. The generation number (if given) must be a
    ///                     positive number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \note       This no-init overload has MPI_Exscan-like semantics, with
    ///             a deterministic HPX first result: participating site 0
    ///             returns a value-initialized result (`std::decay_t<T>{}`, or
    ///             `false` for `bool`). Use the overload taking an explicit
    ///             `init` argument for C++ exclusive-scan semantics where site
    ///             0 returns `init`.
    ///
    /// \returns    For the participating site i this function returns the
    ///             reduction (calculated according to the function op) of
    ///             the values passed in by the participating sites 0, ..., i-1.
    ///             Participating site 0 returns a value-initialized result. The
    ///             operation completes before this function returns.
    ///
    template <typename T, typename F>
    decltype(auto) exclusive_scan(hpx::launch::sync_policy, communicator comm,
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
#include <hpx/collectives/gather.hpp>
#include <hpx/collectives/scatter.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    template <typename T, typename InIter, typename Sent, typename OutIter,
        typename Op>
    constexpr void exclusive_scan(
        InIter first, Sent last, OutIter dest, Op&& op)
    {
        // the first value given goes to the second destination
        T temp = *first++;
        ++dest;    // the first output is ignored
        for (/* */; first != last; (void) ++first, ++dest)
        {
            T next = HPX_INVOKE(op, temp, *first);
            *dest = HPX_MOVE(temp);
            temp = HPX_MOVE(next);
        }
    }

    template <typename InIter, typename Sent, typename OutIter, typename T,
        typename Op>
    constexpr void exclusive_scan_init(
        InIter first, Sent last, OutIter dest, T init, Op&& op)
    {
        T temp = init;
        for (/* */; first != last; (void) ++first, ++dest)
        {
            init = HPX_INVOKE(op, HPX_MOVE(init), *first);
            *dest = HPX_MOVE(temp);
            temp = init;
        }
    }
}    // namespace hpx::collectives::detail

namespace hpx::traits {

    namespace communication {

        HPX_CXX_EXPORT struct exclusive_scan_tag;
        HPX_CXX_EXPORT struct exclusive_scan_init_tag;

        template <>
        struct communicator_data<exclusive_scan_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };

        template <>
        struct communicator_data<exclusive_scan_init_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    ///////////////////////////////////////////////////////////////////////////
    // support for exclusive_scan
    template <typename Communicator>
    struct communication_operation<Communicator,
        communication::exclusive_scan_tag>
    {
        template <typename Result, typename T, typename F>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t,
            F&& op)
        {
            return communicator.template handle_data<std::decay_t<T>>(
                communication::communicator_data<
                    communication::exclusive_scan_tag>::name(),
                which, generation,
                // step function (invoked for each get)
                [&t](auto& data, std::size_t which) {
                    data[which] = HPX_FORWARD(T, t);
                },
                // finalizer (invoked non-concurrently after all data has been
                // received)
                [op = HPX_FORWARD(F, op)](auto& data, bool& data_available,
                    std::size_t which) mutable {
                    auto& scan_op = op;
                    if (!data_available)
                    {
                        using T_ = std::decay_t<T>;

                        std::vector<T_> dest;
                        dest.resize(data.size());

                        if constexpr (!std::same_as<T_, bool>)
                        {
                            collectives::detail::exclusive_scan<T_>(
                                data.begin(), data.end(), dest.begin(),
                                scan_op);
                        }
                        else
                        {
                            collectives::detail::exclusive_scan<bool>(
                                data.begin(), data.end(), dest.begin(),
                                [&scan_op](auto lhs, auto rhs) {
                                    return scan_op(static_cast<bool>(lhs),
                                        static_cast<bool>(rhs));
                                });
                        }
                        std::swap(data, dest);
                        data_available = true;
                    }
                    return Communicator::template handle_bool<std::decay_t<T>>(
                        HPX_MOVE(data[which]));
                },
                num_generations);
        }
    };

    template <typename Communicator>
    struct communication_operation<Communicator,
        communication::exclusive_scan_init_tag>
    {
        template <typename Result, typename T, typename Init, typename F>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations, T&& t,
            Init&& init, F&& op)
        {
            return communicator.template handle_data<std::decay_t<T>>(
                communication::communicator_data<
                    communication::exclusive_scan_init_tag>::name(),
                which, generation,
                // step function (invoked for each get)
                [&t](auto& data, std::size_t which) {
                    data[which] = HPX_FORWARD(T, t);
                },
                // finalizer (invoked non-concurrently after all data has been
                // received)
                [op = HPX_FORWARD(F, op), init = HPX_FORWARD(Init, init)](
                    auto& data, bool& data_available,
                    std::size_t which) mutable {
                    auto& scan_op = op;
                    if (!data_available)
                    {
                        using T_ = std::decay_t<T>;

                        std::vector<T_> dest;
                        dest.resize(data.size());

                        if constexpr (!std::same_as<T_, bool>)
                        {
                            collectives::detail::exclusive_scan_init(
                                data.begin(), data.end(), dest.begin(),
                                static_cast<T_>(HPX_FORWARD(Init, init)),
                                scan_op);
                        }
                        else
                        {
                            collectives::detail::exclusive_scan_init(
                                data.begin(), data.end(), dest.begin(),
                                static_cast<bool>(init),
                                [&scan_op](auto lhs, auto rhs) {
                                    return scan_op(static_cast<bool>(lhs),
                                        static_cast<bool>(rhs));
                                });
                        }
                        std::swap(data, dest);
                        data_available = true;
                    }
                    return Communicator::template handle_bool<std::decay_t<T>>(
                        HPX_MOVE(data[which]));
                },
                num_generations);
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    ////////////////////////////////////////////////////////////////////////////
    // exclusive_scan with semantics similar to MPI_Exscan
    namespace detail {

        template <typename T, typename F>
            requires(std::default_initializable<std::decay_t<T>>)
        hpx::future<std::decay_t<T>> exclusive_scan(communicator fid,
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
                        "hpx::collectives::exclusive_scan",
                        "the generation number shouldn't be zero"));
            }

            // Handle operation right away if there is only one value.
            if (auto [num_sites, comm_site] = fid.get_info(); num_sites == 1)
            {
                if (this_site != comm_site)
                {
                    return hpx::make_exceptional_future<arg_type>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::exclusive_scan",
                            "the local site should be zero if only one site is "
                            "involved"));
                }
                return hpx::make_ready_future(arg_type{});
            }

            auto exclusive_scan_data =
                [local_result = HPX_FORWARD(T, local_result),
                    op = HPX_FORWARD(F, op), this_site, generation,
                    num_generations](
                    communicator&& c) mutable -> hpx::future<arg_type> {
                using func_type = std::decay_t<F>;
                using action_type =
                    communicator_server::communication_get_direct_action<
                        traits::communication::exclusive_scan_tag,
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

            return fid.then(hpx::launch::sync, HPX_MOVE(exclusive_scan_data));
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    hpx::future<std::decay_t<T>> exclusive_scan(communicator fid,
        T&& local_result, F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::exclusive_scan(HPX_MOVE(fid),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site,
            generation, detail::generation_mode::single_step);
    }

    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    hpx::future<std::decay_t<T>> exclusive_scan(communicator fid,
        T&& local_result, F&& op, generation_arg generation,
        this_site_arg this_site = this_site_arg())
    {
        return exclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    hpx::future<std::decay_t<T>> exclusive_scan(char const* basename,
        T&& local_result, F&& op,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return exclusive_scan(create_communicator(basename, num_sites,
                                  this_site, generation, root_site),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site);
    }

    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy, communicator fid,
        T&& local_result, F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg())
    {
        return exclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy, communicator fid,
        T&& local_result, F&& op, generation_arg generation,
        this_site_arg this_site = this_site_arg())
    {
        return exclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy,
        char const* basename, T&& local_result, F&& op,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return exclusive_scan(create_communicator(basename, num_sites,
                                  this_site, generation, root_site),
            HPX_FORWARD(T, local_result), HPX_FORWARD(F, op), this_site)
            .get();
    }

    // Every hierarchical collective advances each communicator by two
    // generations per call, so an instance may be shared freely across
    // collectives; see the note on create_hierarchical_communicator.
    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    hpx::future<std::decay_t<T>> exclusive_scan(
        hierarchical_communicator const& communicators, T&& local_result,
        F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        using arg_type = std::decay_t<T>;

        if (generation.is_default() || generation == 0)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "hierarchical exclusive_scan requires an explicit, "
                    "positive generation number for the 2k-1/2k internal "
                    "mapping"));
        }

        if (!detail::is_valid_hierarchical_phase_generation(generation))
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "the generation number is too large for the internal "
                    "2k-1/2k generation mapping"));
        }

        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }

        std::size_t const num_sites_val = hpx::get<0>(communicators.get_info());
        std::size_t const communicator_site =
            hpx::get<1>(communicators.get_info());
        std::size_t const arity_val = communicators.get_arity();

        if (root_site != 0)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "hierarchical exclusive_scan currently supports only "
                    "root_site == 0 (the tree designates site 0 as the root)"));
        }

        if (this_site >= num_sites_val)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "this_site must be smaller than the number of "
                    "participating sites"));
        }

        if (this_site != communicator_site)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "this_site must match the site used to create the "
                    "hierarchical communicator"));
        }

        auto const [gather_gen, scatter_gen] =
            detail::hierarchical_phase_generations(generation);

        if (arity_val >= num_sites_val)
        {
            HPX_ASSERT(communicators.size() == 1);
            return detail::exclusive_scan(communicators.get(0),
                HPX_FORWARD(T, local_result), HPX_FORWARD(F, op),
                communicators.site(0), gather_gen,
                detail::generation_mode::double_step);
        }

        if (this_site == root_site)
        {
            std::vector<arg_type> gathered =
                detail::gather_here(communicators, HPX_FORWARD(T, local_result),
                    this_site, gather_gen, detail::generation_mode::single_step)
                    .get();

            std::vector<arg_type> results = detail::make_exclusive_scan_results(
                HPX_MOVE(gathered), HPX_FORWARD(F, op));

            return detail::scatter_to(communicators, HPX_MOVE(results),
                this_site, scatter_gen, detail::generation_mode::single_step);
        }
        else
        {
            detail::gather_there(communicators, HPX_FORWARD(T, local_result),
                this_site, gather_gen, detail::generation_mode::single_step)
                .get();

            return detail::scatter_from<arg_type>(communicators, this_site,
                scatter_gen, detail::generation_mode::single_step);
        }
    }

    HPX_CXX_EXPORT template <typename T, typename F>
        requires(std::default_initializable<std::decay_t<T>>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy,
        hierarchical_communicator const& communicators, T&& local_result,
        F&& op, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        return exclusive_scan(communicators, HPX_FORWARD(T, local_result),
            HPX_FORWARD(F, op), this_site, generation, root_site)
            .get();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Version of exclusive scan that takes an initial value for element 0.
    namespace detail {

        template <typename T, typename Init, typename F>
            requires(std::constructible_from<std::decay_t<T>, Init &&>)
        hpx::future<std::decay_t<T>> exclusive_scan(communicator fid,
            T&& local_result, Init&& init, F&& op, this_site_arg this_site,
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
                        "hpx::collectives::exclusive_scan",
                        "the generation number shouldn't be zero"));
            }

            // Handle operation right away if there is only one value.
            if (auto [num_sites, comm_site] = fid.get_info(); num_sites == 1)
            {
                if (this_site != comm_site)
                {
                    return hpx::make_exceptional_future<arg_type>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::exclusive_scan",
                            "the local site should be zero if only one site is "
                            "involved"));
                }
                return hpx::make_ready_future<arg_type>(
                    HPX_FORWARD(Init, init));
            }

            auto exclusive_scan_data =
                [local_result = HPX_FORWARD(T, local_result),
                    init = HPX_FORWARD(Init, init), op = HPX_FORWARD(F, op),
                    this_site, generation, num_generations](
                    communicator&& c) mutable -> hpx::future<arg_type> {
                using init_type = std::decay_t<Init>;
                using func_type = std::decay_t<F>;
                using action_type =
                    communicator_server::communication_get_direct_action<
                        traits::communication::exclusive_scan_init_tag,
                        hpx::future<arg_type>, generation_mode, arg_type,
                        init_type, func_type>;

                // explicitly unwrap returned future
                hpx::future<arg_type> result = hpx::async(action_type(), c,
                    this_site, generation, num_generations,
                    HPX_MOVE(local_result), HPX_MOVE(init), HPX_MOVE(op));

                if (!result.is_ready())
                {
                    // make sure id is kept alive as long as the returned future
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [client = HPX_MOVE(c)] { HPX_UNUSED(client); });
                }

                return result;
            };

            return fid.then(hpx::launch::sync, HPX_MOVE(exclusive_scan_data));
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<this_site_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    hpx::future<std::decay_t<T>> exclusive_scan(communicator fid,
        T&& local_result, Init&& init, F&& op,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::exclusive_scan(HPX_MOVE(fid),
            HPX_FORWARD(T, local_result), HPX_FORWARD(Init, init),
            HPX_FORWARD(F, op), this_site, generation,
            detail::generation_mode::single_step);
    }

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<generation_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    hpx::future<std::decay_t<T>> exclusive_scan(communicator fid,
        T&& local_result, Init&& init, F&& op, generation_arg generation,
        this_site_arg this_site = this_site_arg())
    {
        return exclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(Init, init), HPX_FORWARD(F, op), this_site, generation);
    }

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<num_sites_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    hpx::future<std::decay_t<T>> exclusive_scan(char const* basename,
        T&& local_result, Init&& init, F&& op,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return exclusive_scan(create_communicator(basename, num_sites,
                                  this_site, generation, root_site),
            HPX_FORWARD(T, local_result), HPX_FORWARD(Init, init),
            HPX_FORWARD(F, op), this_site);
    }

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<this_site_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy, communicator fid,
        T&& local_result, Init&& init, F&& op,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return exclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(Init, init), HPX_FORWARD(F, op), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<generation_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy, communicator fid,
        T&& local_result, Init&& init, F&& op, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return exclusive_scan(HPX_MOVE(fid), HPX_FORWARD(T, local_result),
            HPX_FORWARD(Init, init), HPX_FORWARD(F, op), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<num_sites_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy,
        char const* basename, T&& local_result, Init&& init, F&& op,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg())
    {
        return exclusive_scan(create_communicator(basename, num_sites,
                                  this_site, generation, root_site),
            HPX_FORWARD(T, local_result), HPX_FORWARD(Init, init),
            HPX_FORWARD(F, op), this_site)
            .get();
    }

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<this_site_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    hpx::future<std::decay_t<T>> exclusive_scan(
        hierarchical_communicator const& communicators, T&& local_result,
        Init&& init, F&& op, this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        using arg_type = std::decay_t<T>;

        if (generation.is_default() || generation == 0)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "hierarchical exclusive_scan requires an explicit, "
                    "positive generation number for the 2k-1/2k internal "
                    "mapping"));
        }

        if (!detail::is_valid_hierarchical_phase_generation(generation))
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "the generation number is too large for the internal "
                    "2k-1/2k generation mapping"));
        }

        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }

        std::size_t const num_sites_val = hpx::get<0>(communicators.get_info());
        std::size_t const communicator_site =
            hpx::get<1>(communicators.get_info());
        std::size_t const arity_val = communicators.get_arity();

        if (root_site != 0)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "hierarchical exclusive_scan currently supports only "
                    "root_site == 0 (the tree designates site 0 as the root)"));
        }

        if (this_site >= num_sites_val)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "this_site must be smaller than the number of "
                    "participating sites"));
        }

        if (this_site != communicator_site)
        {
            return hpx::make_exceptional_future<arg_type>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::exclusive_scan (hierarchical)",
                    "this_site must match the site used to create the "
                    "hierarchical communicator"));
        }

        auto const [gather_gen, scatter_gen] =
            detail::hierarchical_phase_generations(generation);

        if (arity_val >= num_sites_val)
        {
            HPX_ASSERT(communicators.size() == 1);
            return detail::exclusive_scan(communicators.get(0),
                HPX_FORWARD(T, local_result), HPX_FORWARD(Init, init),
                HPX_FORWARD(F, op), communicators.site(0), gather_gen,
                detail::generation_mode::double_step);
        }

        if (this_site == root_site)
        {
            std::vector<arg_type> gathered =
                detail::gather_here(communicators, HPX_FORWARD(T, local_result),
                    this_site, gather_gen, detail::generation_mode::single_step)
                    .get();

            std::vector<arg_type> results =
                detail::make_exclusive_scan_results(HPX_MOVE(gathered),
                    HPX_FORWARD(Init, init), HPX_FORWARD(F, op));

            return detail::scatter_to(communicators, HPX_MOVE(results),
                this_site, scatter_gen, detail::generation_mode::single_step);
        }
        else
        {
            detail::gather_there(communicators, HPX_FORWARD(T, local_result),
                this_site, gather_gen, detail::generation_mode::single_step)
                .get();

            return detail::scatter_from<arg_type>(communicators, this_site,
                scatter_gen, detail::generation_mode::single_step);
        }
    }

    HPX_CXX_EXPORT template <typename T, typename Init, typename F>
        requires(!std::same_as<this_site_arg, std::decay_t<F>> &&
            std::constructible_from<std::decay_t<T>, Init &&>)
    decltype(auto) exclusive_scan(hpx::launch::sync_policy,
        hierarchical_communicator const& communicators, T&& local_result,
        Init&& init, F&& op, this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg root_site = root_site_arg())
    {
        return exclusive_scan(communicators, HPX_FORWARD(T, local_result),
            HPX_FORWARD(Init, init), HPX_FORWARD(F, op), this_site, generation,
            root_site)
            .get();
    }
}    // namespace hpx::collectives

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
