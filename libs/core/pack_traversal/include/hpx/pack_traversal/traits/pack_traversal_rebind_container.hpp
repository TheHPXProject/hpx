//  Copyright (c) 2019 Hartmut Kaiser
//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <array>
#include <cstddef>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>

namespace hpx::traits {

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {

        ////////////////////////////////////////////////////////////////////////
        template <typename NewType, typename OldType, typename Enable = void>
        struct pack_traversal_rebind_container
        {
        };

        // Specialization for a container with a single type T (no allocator
        // support)
        template <typename NewType, template <class> class Base,
            typename OldType>
        struct pack_traversal_rebind_container<NewType, Base<OldType>>
        {
            static Base<NewType> call(Base<OldType> const& /*container*/)
            {
                return Base<NewType>();
            }
        };

        // Specialization for a container with a single type T and a particular
        // allocator, which is preserved across the remap. -> We remap the
        // allocator through std::allocator_traits.
        template <typename NewType, template <class, class> class Base,
            typename OldType, typename OldAllocator>
        struct pack_traversal_rebind_container<NewType,
            Base<OldType, OldAllocator>,
            std::enable_if_t<std::uses_allocator_v<Base<OldType, OldAllocator>,
                OldAllocator>>>
        {
            using NewAllocator = typename std::allocator_traits<
                OldAllocator>::template rebind_alloc<NewType>;

            static Base<NewType, NewAllocator> call(
                Base<OldType, OldAllocator> const& container)
            {
                // Create a new version of the allocator, that is capable of
                // allocating the mapped type.
                return Base<NewType, NewAllocator>(
                    NewAllocator(container.get_allocator()));
            }
        };
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    // Implement a two-level specialization to avoid ambiguities between the
    // specializations for standard containers below and the generic fallback
    // solutions provided above
    HPX_CXX_CORE_EXPORT template <typename NewType, typename OldType,
        typename Enable = void>
    struct pack_traversal_rebind_container
      : detail::pack_traversal_rebind_container<NewType, OldType>
    {
    };

    // gcc reports an ambiguity for any standard container that has a defaulted
    // allocator template argument as it believes both specializations above are
    // viable. This works around by explicitly specializing the trait.
    template <typename NewType, typename OldType, typename OldAllocator>
    struct pack_traversal_rebind_container<NewType,
        std::vector<OldType, OldAllocator>>
    {
        using NewAllocator = typename std::allocator_traits<
            OldAllocator>::template rebind_alloc<NewType>;

        static std::vector<NewType, NewAllocator> call(
            std::vector<OldType, OldAllocator> const& container)
        {
            // Create a new version of the container using the new allocator
            // that is capable of allocating the mapped type.
            return std::vector<NewType, NewAllocator>(
                NewAllocator(container.get_allocator()));
        }
    };

    template <typename NewType, typename OldType, typename OldAllocator>
    struct pack_traversal_rebind_container<NewType,
        std::list<OldType, OldAllocator>>
    {
        using NewAllocator = typename std::allocator_traits<
            OldAllocator>::template rebind_alloc<NewType>;

        static std::list<NewType, NewAllocator> call(
            std::list<OldType, OldAllocator> const& container)
        {
            // Create a new version of the container using the new allocator
            // that is capable of allocating the mapped type.
            return std::list<NewType, NewAllocator>(
                NewAllocator(container.get_allocator()));
        }
    };

    template <typename NewType, typename OldType, std::size_t N>
    struct pack_traversal_rebind_container<NewType, std::array<OldType, N>>
    {
        static std::array<NewType, N> call(
            std::array<OldType, N> const& /*container*/)
        {
            return std::array<NewType, N>();
        }
    };
}    // namespace hpx::traits
