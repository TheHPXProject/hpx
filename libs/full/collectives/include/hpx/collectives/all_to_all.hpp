//  Copyright (c) 2019-2026 Hartmut Kaiser
//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file all_to_all.hpp
/// \page hpx::collectives::all_to_all
/// \headerfile hpx/collectives.hpp

#pragma once

#if defined(DOXYGEN)
// clang-format off
namespace hpx { namespace collectives {

    /// AllToAll a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param basename     The base name identifying the all_to_all operation
    /// \param local_result A vector of values to transmit to all
    ///                     participating sites from this call site.
    /// \param num_sites    The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_to_all operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_to_all operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The site that is responsible for creating the
    ///                     all_to_all support object. This value is optional
    ///                     and defaults to '0' (zero).
    /// \param threshold    The fixed row-size threshold, in bytes, at or above
    ///                     which rows are exchanged directly between sites
    ///                     instead of being routed through one communicator
    ///                     site. Automatic selection uses sizeof(T) only for a
    ///                     trivially copyable T; other row types stay routed
    ///                     unless every site passes zero. See
    ///                     \a pairwise_threshold_arg for the default and the
    ///                     remaining direct-exchange requirements. Successive
    ///                     generations using one basename may select different
    ///                     paths. Which path runs affects performance only,
    ///                     never the result.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the all_to_all operation has been completed.
    ///
    template <typename T> hpx::future<std::vector<T>>
    all_to_all(char const* basename,
        std::vector<T>&& local_result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg(),
        pairwise_threshold_arg threshold = pairwise_threshold_arg());

    /// AllToAll a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param fid          A communicator object returned from \a create_communicator
    /// \param local_result A vector of values to transmit to all
    ///                     participating sites from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_to_all operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_to_all operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the all_to_all operation has been completed.
    ///
    template <typename T> hpx::future<std::vector<T>>
    all_to_all(communicator fid,
        std::vector<T>&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// AllToAll a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param fid          A communicator object returned from \a create_communicator
    /// \param local_result A vector of values to transmit to all
    ///                     participating sites from this call site.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_to_all operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_to_all operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \returns    This function returns a future holding a vector with all
    ///             values send by all participating sites. It will become
    ///             ready once the all_to_all operation has been completed.
    ///
    template <typename T> hpx::future<std::vector<T>>
    all_to_all(communicator fid,
        std::vector<T>&& local_result,
        generation_arg generation,
        this_site_arg this_site = this_site_arg());

    /// AllToAll a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param policy       The execution policy specifying synchronous execution.
    /// \param basename     The base name identifying the all_to_all operation
    /// \param local_result A vector of values to transmit to all
    ///                     participating sites from this call site.
    /// \param num_sites    The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_to_all operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_to_all operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    /// \param root_site    The site that is responsible for creating the
    ///                     all_to_all support object. This value is optional
    ///                     and defaults to '0' (zero).
    /// \param threshold    The fixed row-size threshold, in bytes, at or above
    ///                     which rows are exchanged directly between sites
    ///                     instead of being routed through one communicator
    ///                     site. Automatic selection uses sizeof(T) only for a
    ///                     trivially copyable T; other row types stay routed
    ///                     unless every site passes zero. See
    ///                     \a pairwise_threshold_arg for the default and the
    ///                     remaining direct-exchange requirements. Successive
    ///                     generations using one basename may select different
    ///                     paths. Which path runs affects performance only,
    ///                     never the result.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    std::vector<T> all_to_all(hpx::launch::sync_policy policy,
        char const* basename, std::vector<T>&& local_result,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg(),
        root_site_arg root_site = root_site_arg(),
        pairwise_threshold_arg threshold = pairwise_threshold_arg());

    /// AllToAll a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param  policy      The execution policy specifying synchronous execution.
    /// \param fid          A communicator object returned from \a create_communicator
    /// \param local_result A vector of values to transmit to all
    ///                     participating sites from this call site.
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_to_all operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_to_all operation on the
    ///                     given base name has to be performed more than once.
    ///                     The generation number (if given) must be a positive
    ///                     number greater than zero.
    ///
    /// \returns    This function returns a vector with all values send by all
    ///             participating sites. This function executes synchronously and
    ///             directly returns the result.
    ///
    template <typename T>
    std::vector<T> all_to_all(hpx::launch::sync_policy policy,
        communicator fid, std::vector<T>&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    /// AllToAll a set of values from different call sites
    ///
    /// This function receives a set of values from all call sites operating on
    /// the given base name.
    ///
    /// \param policy      The execution policy specifying synchronous execution.
    /// \param fid          A communicator object returned from \a create_communicator
    /// \param local_result A vector of values to transmit to all
    ///                     participating sites from this call site.
    /// \param generation   The generational counter identifying the sequence
    ///                     number of the all_to_all operation performed on the
    ///                     given base name. This is optional and needs to be
    ///                     supplied only if the all_to_all operation on the
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
    std::vector<T> all_to_all(hpx::launch::sync_policy policy,
        communicator fid, std::vector<T>&& local_result,
        generation_arg generation, this_site_arg this_site = this_site_arg());
}}    // namespace hpx::collectives

// clang-format on
#else

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/channel_communicator.hpp>
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/flattened_data.hpp>
#include <hpx/collectives/detail/hierarchical_all_to_all_helpers.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>
#include <hpx/collectives/detail/pairwise_all_to_all_helpers.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::traits {

    namespace communication {

        HPX_CXX_EXPORT struct all_to_all_tag;

        template <>
        struct communicator_data<all_to_all_tag>
        {
            HPX_EXPORT static char const* name() noexcept;
        };
    }    // namespace communication

    ///////////////////////////////////////////////////////////////////////////
    // support for all_to_all
    template <typename Communicator>
    struct communication_operation<Communicator, communication::all_to_all_tag>
    {
        template <typename Result, typename Payload>
        static Result get(Communicator& communicator, std::size_t which,
            std::size_t generation,
            hpx::collectives::detail::generation_mode num_generations,
            Payload&& payload)
        {
            using payload_type = std::decay_t<Payload>;
            return communicator.template handle_data<payload_type>(
                communication::communicator_data<
                    communication::all_to_all_tag>::name(),
                which, generation,
                // step function (invoked for each get)
                [&payload](auto& data, std::size_t which) {
                    data[which] = HPX_FORWARD(Payload, payload);
                },
                // finalizer (invoked after all data has been received)
                [](auto& data, auto&, std::size_t which) {
                    return finalize(data, which);
                },
                num_generations);
        }

    private:
        template <typename T>
        static std::vector<T> finalize(
            std::vector<std::vector<T>>& data, std::size_t const which)
        {
            std::vector<T> result;
            result.reserve(data.size());
            for (auto& values : data)
            {
                if (values.size() != data.size())
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::all_to_all",
                        "each participating site must contribute exactly "
                        "num_sites elements");
                }

                result.push_back(hpx::collectives::detail::handle_bool<T>(
                    HPX_MOVE(values[which])));
            }
            return result;
        }

        template <typename T>
        static hpx::collectives::detail::ragged_rows<T> finalize(
            std::vector<hpx::collectives::detail::ragged_rows<T>>& data,
            std::size_t const which)
        {
            return hpx::collectives::detail::ragged_rows<T>(data, which);
        }
    };
}    // namespace hpx::traits

namespace hpx::collectives {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename T>
        class hierarchical_all_to_all_exchange
        {
        public:
            hierarchical_all_to_all_exchange(uniform_rows<T>&& gathered,
                std::size_t const group_index, std::size_t const num_sites,
                std::size_t const arity)
              : group_index_(group_index)
              , num_sites_(num_sites)
              , arity_(arity)
              , my_group_size_(
                    get_top_level_group_size(group_index, num_sites, arity))
              , num_groups_(get_top_level_group_count(num_sites, arity))
            {
                constexpr char const* operation =
                    "hpx::collectives::all_to_all (hierarchical)";

                gathered.validate(operation);
                if (gathered.num_rows() != my_group_size_)
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status, operation,
                        "the subtree gather returned an unexpected number of "
                        "rows");
                }
                if (gathered.row_width() != num_sites)
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status, operation,
                        "the subtree gather returned rows with an unexpected "
                        "number of elements");
                }

                std::size_t const exchange_size = checked_data_size_product(
                    my_group_size_, num_sites_ - my_group_size_);
                std::size_t const exchange_segments =
                    checked_data_size_product(my_group_size_, num_groups_);
                std::size_t const exchange_boundaries =
                    checked_data_size_sum(exchange_segments, 1);
                std::size_t const diagonal_size =
                    checked_data_size_product(my_group_size_, my_group_size_);

                std::vector<T> gathered_data =
                    HPX_MOVE(gathered).release_data();

                exchange_data_.reserve(exchange_size);
                exchange_offsets_.reserve(exchange_boundaries);
                exchange_offsets_.push_back(0);
                diagonal_.reserve(diagonal_size);

                for (std::size_t row = 0; row != my_group_size_; ++row)
                {
                    std::size_t const row_left =
                        checked_data_size_product(row, num_sites_);
                    for (std::size_t group = 0; group != num_groups_; ++group)
                    {
                        std::size_t const destination_group_size =
                            get_top_level_group_size(group, num_sites_, arity_);
                        std::size_t const destination_left =
                            get_top_level_group_left(group, num_sites_, arity_);
                        auto& destination =
                            group == group_index_ ? diagonal_ : exchange_data_;
                        for (std::size_t destination_site = destination_left;
                            destination_site !=
                            destination_left + destination_group_size;
                            ++destination_site)
                        {
                            destination.emplace_back(handle_bool<T>(HPX_MOVE(
                                gathered_data[row_left + destination_site])));
                        }
                        exchange_offsets_.push_back(exchange_data_.size());
                    }
                }

                HPX_ASSERT(exchange_data_.size() == exchange_size);
                HPX_ASSERT(diagonal_.size() == diagonal_size);
            }

            [[nodiscard]] ragged_rows<T> release_exchange() &
            {
                return ragged_rows<T>(
                    HPX_MOVE(exchange_data_), HPX_MOVE(exchange_offsets_));
            }

            [[nodiscard]] uniform_rows<T> transpose(
                ragged_rows<T>&& received) &&
            {
                constexpr char const* operation =
                    "hpx::collectives::all_to_all (hierarchical)";
                received.validate(operation);
                if (received.num_segments() != num_groups_)
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status, operation,
                        "the inter-group exchange returned an unexpected "
                        "number of blocks");
                }
                for (std::size_t group = 0; group != num_groups_; ++group)
                {
                    std::size_t const source_group_size =
                        get_top_level_group_size(group, num_sites_, arity_);
                    std::size_t const expected_size = group == group_index_ ?
                        0 :
                        checked_data_size_product(
                            source_group_size, my_group_size_);
                    if (received.segment_size(group) != expected_size)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                            operation,
                            "the inter-group exchange returned a block with "
                            "an unexpected number of elements");
                    }
                }

                std::vector<std::size_t> const received_offsets =
                    received.offsets();
                std::vector<T> received_data =
                    HPX_MOVE(received).release_data();

                // Create one scatter row for each destination site j in this
                // representative's group. Within each row, source groups are
                // visited from left to right and their sites from low to high,
                // preserving global source-site order. Both an off-diagonal
                // received segment and the retained local diagonal use
                // [source site within group][destination site within this
                // group] storage order. Thus source * my_group_size_ + j
                // selects the value for destination j; off-diagonal data also
                // starts at received_offsets[group].
                std::vector<T> scatter_data;
                scatter_data.reserve(
                    checked_data_size_product(my_group_size_, num_sites_));
                for (std::size_t j = 0; j != my_group_size_; ++j)
                {
                    for (std::size_t group = 0; group != num_groups_; ++group)
                    {
                        std::size_t const source_group_size =
                            get_top_level_group_size(group, num_sites_, arity_);
                        for (std::size_t source = 0;
                            source != source_group_size; ++source)
                        {
                            if (group == group_index_)
                            {
                                std::size_t const index =
                                    source * my_group_size_ + j;
                                scatter_data.emplace_back(
                                    handle_bool<T>(HPX_MOVE(diagonal_[index])));
                            }
                            else
                            {
                                std::size_t const index =
                                    received_offsets[group] +
                                    source * my_group_size_ + j;
                                scatter_data.emplace_back(handle_bool<T>(
                                    HPX_MOVE(received_data[index])));
                            }
                        }
                    }
                }
                HPX_ASSERT(scatter_data.size() ==
                    checked_data_size_product(my_group_size_, num_sites_));

                return uniform_rows<T>(HPX_MOVE(scatter_data), my_group_size_);
            }

        private:
            std::size_t group_index_;
            std::size_t num_sites_;
            std::size_t arity_;
            std::size_t my_group_size_;
            std::size_t num_groups_;
            std::vector<T> exchange_data_;
            std::vector<std::size_t> exchange_offsets_;
            std::vector<T> diagonal_;
        };

        // all_to_all: detail entry point carrying the internal generation step.
        // The public overload forwards with single_step; the hierarchical
        // overload passes double_step for the flat fast path and the inter-group
        // exchange.
        template <typename Payload>
        hpx::future<std::decay_t<Payload>> all_to_all(communicator fid,
            Payload&& local_result, this_site_arg this_site,
            generation_arg const generation, generation_mode num_generations)
        {
            using payload_type = std::decay_t<Payload>;

            if (this_site.is_default())
            {
                this_site = agas::get_locality_id();
            }
            if (generation == 0)
            {
                return hpx::make_exceptional_future<payload_type>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::all_to_all",
                        "the generation number shouldn't be zero"));
            }

            // Handle operation right away if there is only one value.
            if (auto [num_sites, comm_site] = fid.get_info(); num_sites == 1)
            {
                if constexpr (!is_ragged_rows_v<payload_type>)
                {
                    if (local_result.size() != 1)
                    {
                        return hpx::make_exceptional_future<payload_type>(
                            HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                                "hpx::collectives::all_to_all",
                                "each participating site must contribute "
                                "exactly num_sites elements"));
                    }
                }

                if (this_site != comm_site)
                {
                    return hpx::make_exceptional_future<payload_type>(
                        HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                            "hpx::collectives::all_to_all",
                            "the local site should be zero if only one site is "
                            "involved"));
                }

                if constexpr (is_ragged_rows_v<payload_type>)
                {
                    // Match the communicator finalizer: the outgoing carrier
                    // has one segment per source row, while the received
                    // carrier has one segment per contributor. With one
                    // destination column all source-row data is already
                    // contiguous, so only the offsets need to be collapsed.
                    payload_type payload(HPX_FORWARD(Payload, local_result));
                    payload.validate_for_exchange(
                        num_sites, "hpx::collectives::all_to_all");
                    auto data = HPX_MOVE(payload).release_data();
                    std::vector<std::size_t> offsets{0, data.size()};
                    return hpx::make_ready_future(
                        payload_type(HPX_MOVE(data), HPX_MOVE(offsets)));
                }
                else
                {
                    return hpx::make_ready_future(
                        HPX_FORWARD(Payload, local_result));
                }
            }

            auto all_to_all_data =
                [local_result = HPX_FORWARD(Payload, local_result), this_site,
                    generation, num_generations](
                    communicator&& c) mutable -> hpx::future<payload_type> {
                using action_type =
                    communicator_server::communication_get_direct_action<
                        traits::communication::all_to_all_tag,
                        hpx::future<payload_type>, generation_mode,
                        payload_type>;

                // explicitly unwrap returned future
                hpx::future<payload_type> result =
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

            return fid.then(hpx::launch::sync, HPX_MOVE(all_to_all_data));
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // all_to_all
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> all_to_all(communicator fid,
        std::vector<T>&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return detail::all_to_all(HPX_MOVE(fid), HPX_MOVE(local_result),
            this_site, generation, detail::generation_mode::single_step);
    }

    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> all_to_all(communicator fid,
        std::vector<T>&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return all_to_all(
            HPX_MOVE(fid), HPX_MOVE(local_result), this_site, generation);
    }

    namespace detail {

        // all_to_all_by_name: resolves a basename call onto one of the two
        // exchange paths.
        //
        // The routed path is the default and always available. The direct
        // path additionally needs two things the routed path does not: a
        // channel communicator, which only a basename can produce, and an
        // explicit generation, which is what gives every site the same
        // exchange tag. A default generation therefore stays routed. Which
        // path runs is a performance choice rather than a semantic one, so an
        // otherwise valid call is served rather than rejected.
        //
        // An explicit routed generation gets its own collective communicator
        // from create_communicator. Direct generations share a channel
        // communicator and use the generation as the exchange tag. The two
        // paths therefore do not share a gate, and successive generations may
        // choose different paths.
        template <typename T>
        hpx::future<std::vector<T>> all_to_all_by_name(char const* basename,
            std::vector<T>&& local_result, num_sites_arg num_sites,
            this_site_arg this_site, generation_arg const generation,
            root_site_arg const root_site,
            pairwise_threshold_arg const threshold)
        {
            // Validation cannot depend on which path serves the call: the
            // routed path rejects a zero generation inside detail::all_to_all,
            // while the direct path would accept it as an exchange tag, since
            // only a default generation reads as "unspecified" there. Reject it
            // before the path is chosen, so a performance knob cannot move the
            // boundary of what the operation accepts.
            if (generation == 0)
            {
                return hpx::make_exceptional_future<std::vector<T>>(
                    HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                        "hpx::collectives::all_to_all",
                        "the generation number shouldn't be zero"));
            }

            if (num_sites.is_default())
            {
                num_sites =
                    num_sites_arg(agas::get_num_localities(hpx::launch::sync));
            }
            if (this_site.is_default())
            {
                this_site = this_site_arg(agas::get_locality_id());
            }

            // Leave malformed communicator arguments to the routed path's
            // existing validation instead of changing their behavior here.
            if (basename != nullptr && basename[0] != '\0' &&
                this_site < num_sites && root_site < num_sites &&
                !generation.is_default() &&
                exchange_pairwise(static_cast<std::size_t>(num_sites),
                    pairwise_type_bytes<T>(), threshold))
            {
                // The channel communicator carries a name of its own so it
                // cannot collide with the collective communicator registered
                // under this basename. That name spans a group of sites rather
                // than a single call: one communicator serves every
                // generation, and the exchange tag is what keeps the
                // generations apart. Registering a name per generation instead
                // would put an AGAS registration and a full peer lookup back
                // into every call, which is the cost this path exists to
                // remove.
                //
                // The site count belongs in the name because two groups of
                // different size are two different communicators, and the
                // cache creates one only on the first call that names it.
                std::string channel_basename(basename);
                HPX_ASSERT(!channel_basename.empty());
                if (channel_basename.back() != '/')
                {
                    channel_basename += '/';
                }
                channel_basename += "pairwise/" +
                    std::to_string(static_cast<std::size_t>(num_sites)) + "/";

                std::size_t const num_sites_value = num_sites;
                std::size_t const this_site_value = this_site;
                tag_arg const tag(static_cast<std::size_t>(generation));

                // The communicator is chained onto rather than waited for.
                // This function backs an overload that hands the caller a
                // future, so it must not spend the caller's thread on the
                // first call's AGAS registration before returning one. The
                // communicator type is spelled out because the unqualified
                // name resolves to the detail class in this namespace.
                return get_cached_channel_communicator(
                    HPX_MOVE(channel_basename), num_sites, this_site)
                    .then(hpx::launch::sync,
                        [local_result = HPX_MOVE(local_result), num_sites_value,
                            this_site_value, tag](hpx::shared_future<
                            hpx::collectives::channel_communicator>&&
                                f) mutable {
                            return pairwise_all_to_all(f.get(),
                                HPX_MOVE(local_result), num_sites_value,
                                this_site_value, tag);
                        });
            }

            return hpx::collectives::all_to_all(
                hpx::collectives::create_communicator(
                    basename, num_sites, this_site, generation, root_site),
                HPX_MOVE(local_result), this_site);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> all_to_all(char const* basename,
        std::vector<T>&& local_result,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg(),
        pairwise_threshold_arg const threshold = pairwise_threshold_arg())
    {
        return detail::all_to_all_by_name(basename, HPX_MOVE(local_result),
            num_sites, this_site, generation, root_site, threshold);
    }

    // Forward declaration of the hierarchical overload (defined below) so the
    // generic sync overload can resolve to it; otherwise only the flat
    // overloads above are visible at that call site.
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> all_to_all(
        hierarchical_communicator const& communicators,
        std::vector<T>&& local_result,
        this_site_arg this_site = this_site_arg(),
        generation_arg generation = generation_arg());

    ///////////////////////////////////////////////////////////////////////////
    // Generic sync overload: dispatches to the flat or hierarchical async
    // overload based on the communicator type passed.
    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    std::vector<T> all_to_all(hpx::launch::sync_policy, Communicator&& fid,
        std::vector<T>&& local_result,
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg())
    {
        return all_to_all(HPX_FORWARD(Communicator, fid),
            HPX_MOVE(local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename Communicator, typename T>
        requires(is_communicator_v<std::decay_t<Communicator>>)
    std::vector<T> all_to_all(hpx::launch::sync_policy, Communicator&& fid,
        std::vector<T>&& local_result, generation_arg const generation,
        this_site_arg const this_site = this_site_arg())
    {
        return all_to_all(HPX_FORWARD(Communicator, fid),
            HPX_MOVE(local_result), this_site, generation)
            .get();
    }

    HPX_CXX_EXPORT template <typename T>
    std::vector<T> all_to_all(hpx::launch::sync_policy, char const* basename,
        std::vector<T>&& local_result,
        num_sites_arg const num_sites = num_sites_arg(),
        this_site_arg const this_site = this_site_arg(),
        generation_arg const generation = generation_arg(),
        root_site_arg const root_site = root_site_arg(),
        pairwise_threshold_arg const threshold = pairwise_threshold_arg())
    {
        return detail::all_to_all_by_name(basename, HPX_MOVE(local_result),
            num_sites, this_site, generation, root_site, threshold)
            .get();
    }
    ///////////////////////////////////////////////////////////////////////////
    // hierarchical all_to_all
    //
    // Three-phase algorithm:
    //  Phase 1 (gather):   subtree sites gather their local_result vectors
    //                      at their top-level representative.
    //  Phase 2 (exchange): representatives perform flat all_to_all at level 0,
    //                      exchanging blocks of data between groups.
    //  Phase 3 (scatter):  representatives scatter the transposed results
    //                      back to their subtree sites.
    //
    // Generation mapping (user generation k, starting from 1):
    //  - Subtree communicators: 2k-1 (gather) and 2k (scatter)
    //  - Inter-group communicator: 2k-1 (exchange), advancing the gate by two
    //    in a single step so it stays in lock-step with the subtrees
    // Each communicator therefore advances by two generations per call,
    // which lets a single instance be shared across collectives; see the
    // note on create_hierarchical_communicator.

    // Async overload (declared above; default arguments live on that
    // forward declaration).
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> all_to_all(
        hierarchical_communicator const& communicators,
        std::vector<T>&& local_result, this_site_arg this_site,
        generation_arg const generation)
    {
        if (generation.is_default() || generation == 0)
        {
            return hpx::make_exceptional_future<std::vector<T>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::all_to_all (hierarchical)",
                    "hierarchical all_to_all requires an explicit, positive "
                    "generation number for its internal generation "
                    "mapping"));
        }

        if (!detail::is_valid_hierarchical_phase_generation(generation))
        {
            return hpx::make_exceptional_future<std::vector<T>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::all_to_all (hierarchical)",
                    "the generation number is too large for the internal "
                    "generation mapping"));
        }

        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }

        if (auto const error =
                detail::validate_hierarchical_communicator(communicators,
                    this_site, "hpx::collectives::all_to_all (hierarchical)"))
        {
            return hpx::make_exceptional_future<std::vector<T>>(error);
        }

        std::size_t const num_sites_val = hpx::get<0>(communicators.get_info());
        std::size_t const arity_val = communicators.get_arity();

        // The phase-2 packing slices each gathered contribution with
        // unchecked iterator arithmetic, so a wrong-size contribution must
        // be rejected client-side before any data is sent.
        if (local_result.size() != num_sites_val)
        {
            return hpx::make_exceptional_future<std::vector<T>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::all_to_all (hierarchical)",
                    "each participating site must contribute exactly "
                    "num_sites elements"));
        }

        // Generation mapping: every communicator advances two generations per
        // call and must see consecutive generation numbers starting from 1 (the
        // gate blocks until all prior generations complete). There are only two
        // distinct internal generations per call -- the first (2k-1) and the
        // second (2k). The subtree communicators run their gather at the first
        // and their scatter at the second. The inter-group communicator runs
        // its single exchange at that same first generation and advances the
        // gate past the second in one step (double_step below), so it stays in
        // lock-step with the subtrees and the instance can be shared across
        // collectives. Gather, exchange and the collapsed flat fast path
        // therefore share first_gen; only the subtree scatter uses second_gen.
        auto const [first_gen, second_gen] =
            detail::hierarchical_phase_generations(generation);

        // Flat fast path: when arity >= num_sites (either because the user
        // chose a large arity or because the factory overrode arity to
        // num_sites for the flat fallback), the tree builder's leaf condition
        // (right - left < arity) fired at the root call and produced a single
        // flat communicator spanning all sites. The 3-phase algorithm then
        // collapses to a flat all_to_all; dispatch directly to avoid the
        // intermediate allocations, but still advance the gate by two (run at
        // 2k-1, double_step) so the flat instance stays shareable with other
        // collectives.
        if (arity_val >= num_sites_val)
        {
            HPX_ASSERT(communicators.size() == 1);
            return detail::all_to_all(communicators.get(0),
                HPX_MOVE(local_result), communicators.site(0), first_gen,
                detail::generation_mode::double_step);
        }

        std::ptrdiff_t const gidx =
            detail::classify_site(this_site, num_sites_val, arity_val);
        bool const is_representative =
            detail::is_top_level_rep(gidx, this_site, num_sites_val, arity_val);

        if (is_representative)
        {
            // Phase 1: Gather all subtree sites' data at this rep.
            detail::uniform_rows<T> gathered =
                detail::subtree_gather_at_top_rep(
                    communicators, HPX_MOVE(local_result), first_gen);

            std::size_t const group_index = static_cast<std::size_t>(gidx);

            // Phase 2: Pack one flat sequence of destination-group segments
            // for each gathered row. Retain the local diagonal separately and
            // represent it by empty exchange segments, avoiding its round trip
            // through the inter-group communicator.
            detail::hierarchical_all_to_all_exchange<T> exchange(
                HPX_MOVE(gathered), group_index, num_sites_val, arity_val);

            // The exchange touches the inter-group communicator once but must
            // advance it by two generations to stay in lock-step with the
            // subtree communicators, so request double_step.
            hpx::future<detail::ragged_rows<T>> received_future =
                detail::all_to_all(communicators.get(0),
                    exchange.release_exchange(), communicators.site(0),
                    first_gen, detail::generation_mode::double_step);

            detail::ragged_rows<T> received = received_future.get();

            return detail::subtree_scatter_at_top_rep(communicators,
                HPX_MOVE(exchange).transpose(HPX_MOVE(received)), second_gen);
        }
        else
        {
            // Non-representative: send data to rep, then receive
            // result from rep.
            detail::subtree_send_to_top_rep(
                communicators, HPX_MOVE(local_result), first_gen);

            return detail::subtree_receive_from_top_rep<T>(
                communicators, second_gen);
        }
    }
}    // namespace hpx::collectives

////////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_ALLTOALL_DECLARATION(...) /**/

////////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_ALLTOALL(...)             /**/

#endif    // DOXYGEN
