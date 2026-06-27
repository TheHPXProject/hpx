//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file argument_types.hpp

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::collectives {

    namespace detail {

        // The number of gate generations a single collective call consumes on a
        // communicator. A hierarchical collective that touches a communicator
        // only once per user call but has to stay in lock-step with collectives
        // that touch it twice uses double_step, advancing the gate by two in a
        // single step. This is an internal knob: the public collective API does
        // not expose it, it is threaded only through the detail entry points the
        // hierarchical overloads call. HPX serialization handles scoped enums
        // natively (they are stored as their underlying integral type), so no
        // explicit serialize function is required when a generation_mode crosses
        // the wire as a collective action argument.
        enum class generation_mode : std::uint8_t
        {
            single_step = 1,
            double_step = 2
        };

        template <typename Tag,
            std::size_t Default = static_cast<std::size_t>(-1)>
        struct argument_type
        {
            explicit constexpr argument_type(
                std::size_t const argument = Default) noexcept
              : argument_(argument)
            {
            }

            constexpr argument_type& operator=(
                std::size_t const argument) noexcept
            {
                argument_ = argument;
                return *this;
            }

            constexpr operator std::size_t() const noexcept
            {
                return argument_;
            }

            [[nodiscard]] constexpr bool is_default() const noexcept
            {
                return argument_ == Default;
            }

            std::size_t argument_;
        };

        struct num_sites_tag;
        struct this_site_tag;
        struct that_site_tag;
        struct generation_tag;
        struct root_site_tag;
        struct tag_tag;
        struct arity_tag;
        struct flat_fallback_threshold_tag;
    }    // namespace detail

    /// The number of participating sites (default: all localities)
    HPX_CXX_EXPORT using num_sites_arg =
        detail::argument_type<detail::num_sites_tag>;

    /// The local end of the communication channel
    HPX_CXX_EXPORT using this_site_arg =
        detail::argument_type<detail::this_site_tag>;

    /// The opposite end of the communication channel
    HPX_CXX_EXPORT using that_site_arg =
        detail::argument_type<detail::that_site_tag>;

    /// The generational counter identifying the sequence number of the
    /// operation performed on the given base name. It needs to be supplied only
    /// if the operation on the given base name has to be performed more than
    /// once. It must be a positive number greater than zero.
    HPX_CXX_EXPORT using generation_arg =
        detail::argument_type<detail::generation_tag>;

    /// The site that is responsible for creating the support object
    /// of the operation. It defaults to '0' (zero).
    HPX_CXX_EXPORT using root_site_arg =
        detail::argument_type<detail::root_site_tag, 0>;

    /// The tag identifying the concrete operation
    HPX_CXX_EXPORT using tag_arg = detail::argument_type<detail::tag_tag, 0>;

    /// The number of children each of the communication nodes is connected
    /// to (default: 2).
    HPX_CXX_EXPORT using arity_arg =
        detail::argument_type<detail::arity_tag, 2>;

    /// The site-count threshold below which a hierarchical_communicator
    /// collapses to a single flat communicator spanning all sites. At small
    /// site counts, flat collectives outperform hierarchical composition
    /// because tree-walking overhead exceeds the synchronization-depth benefit.
    /// Pass 0 to disable the fallback and always build a tree.
    HPX_CXX_EXPORT using flat_fallback_threshold_arg =
        detail::argument_type<detail::flat_fallback_threshold_tag, 16>;
}    // namespace hpx::collectives
