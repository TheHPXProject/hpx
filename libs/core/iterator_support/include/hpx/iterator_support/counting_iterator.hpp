//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  This code is based on boost::iterators::counting_iterator
//  Copyright David Abrahams 2003.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iterator_support/iterator_adaptor.hpp>
#include <hpx/iterator_support/iterator_range.hpp>
#include <hpx/iterator_support/range.hpp>
#include <hpx/iterator_support/traits/is_range.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT template <typename Incrementable,
        typename CategoryOrTraversal = void, typename Difference = void,
        typename Enable = void>
    class counting_iterator;

    namespace detail {

        template <typename Incrementable, typename CategoryOrTraversal,
            typename Difference>
        struct counting_iterator_base
        {
            // calculate category of the resulting iterator
            template <typename Iterator>
            struct iterator_category
            {
                using type = std::iterator_traits<Iterator>::iterator_category;
            };

            using base_traversal =
                util::lazy_conditional<std::is_integral_v<Incrementable>,
                    hpx::type_identity<std::random_access_iterator_tag>,
                    iterator_category<Incrementable>>;

            using traversal =
                util::lazy_conditional_t<std::is_void_v<CategoryOrTraversal>,
                    base_traversal, hpx::type_identity<CategoryOrTraversal>>;

            // calculate difference_type of the resulting iterator
            template <typename Integer>
            struct integer_difference_type
            {
                using type = std::conditional_t<(sizeof(Integer) >=
                                                    sizeof(std::intmax_t)),

                    std::conditional_t<std::is_signed_v<Integer>, Integer,
                        std::intmax_t>,

                    std::conditional_t<(sizeof(Integer) <
                                           sizeof(std::ptrdiff_t)),
                        std::ptrdiff_t, std::intmax_t>>;
            };

            template <typename Iterator>
            struct iterator_difference_type
            {
                using type = std::iterator_traits<Iterator>::difference_type;
            };

            using base_difference =
                util::lazy_conditional<std::is_integral_v<Incrementable>,
                    integer_difference_type<Incrementable>,
                    iterator_difference_type<Incrementable>>;

            using difference =
                util::lazy_conditional_t<std::is_void_v<Difference>,
                    base_difference, hpx::type_identity<Difference>>;

            using type = iterator_adaptor<counting_iterator<Incrementable,
                                              CategoryOrTraversal, Difference>,
                Incrementable, Incrementable, traversal, Incrementable,
                difference>;
        };
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    // specialization for Iterators (non-integral types)
    HPX_CXX_CORE_EXPORT template <typename Incrementable,
        typename CategoryOrTraversal, typename Difference, typename Enable>
    class counting_iterator
      : public detail::counting_iterator_base<Incrementable,
            CategoryOrTraversal, Difference>::type
    {
    private:
        using base_type = detail::counting_iterator_base<Incrementable,
            CategoryOrTraversal, Difference>::type;

        friend class iterator_core_access;

    public:
        counting_iterator() = default;
        ~counting_iterator() = default;
        counting_iterator(counting_iterator&& rhs) = default;
        counting_iterator& operator=(counting_iterator&& rhs) = default;
        counting_iterator(counting_iterator const& rhs) = default;
        counting_iterator& operator=(counting_iterator const& rhs) = default;

        HPX_HOST_DEVICE explicit constexpr counting_iterator(Incrementable x)
          : base_type(x)
        {
        }

    private:
        HPX_HOST_DEVICE constexpr base_type::reference dereference() const
        {
            return this->base_reference();
        }
    };

    template <typename Incrementable, typename CategoryOrTraversal,
        typename Difference>
    class counting_iterator<Incrementable, CategoryOrTraversal, Difference,
        std::enable_if_t<std::is_integral_v<Incrementable>>>
      : public detail::counting_iterator_base<Incrementable,
            CategoryOrTraversal, Difference>::type
    {
    private:
        using base_type = detail::counting_iterator_base<Incrementable,
            CategoryOrTraversal, Difference>::type;

        friend class iterator_core_access;

    public:
        using use_brackets_proxy = std::false_type;

        counting_iterator() = default;
        ~counting_iterator() = default;
        counting_iterator(counting_iterator&& rhs) = default;
        counting_iterator& operator=(counting_iterator&& rhs) = default;
        counting_iterator(counting_iterator const& rhs) = default;
        counting_iterator& operator=(counting_iterator const& rhs) = default;

        HPX_HOST_DEVICE explicit constexpr counting_iterator(
            Incrementable x) noexcept
          : base_type(x)
        {
        }

    private:
        template <typename Iterator>
        HPX_HOST_DEVICE constexpr bool equal(Iterator const& rhs) const noexcept
        {
            return this->base() == rhs.base();
        }

        HPX_HOST_DEVICE void increment() noexcept
        {
            ++this->base_reference();
        }

        HPX_HOST_DEVICE void decrement() noexcept
        {
            --this->base_reference();
        }

        template <typename Distance>
        HPX_HOST_DEVICE void advance(Distance n) noexcept
        {
            this->base_reference() += static_cast<base_type::base_type>(n);
        }

        HPX_HOST_DEVICE constexpr base_type::reference dereference()
            const noexcept
        {
            return this->base_reference();
        }

        template <typename OtherIncrementable>
        HPX_HOST_DEVICE base_type::difference_type distance_to(
            counting_iterator<OtherIncrementable, CategoryOrTraversal,
                Difference> const& y) const noexcept
        {
            using difference_type = base_type::difference_type;
            return static_cast<difference_type>(y.base()) -
                static_cast<difference_type>(this->base());
        }
    };

    // Manufacture a counting iterator for an arbitrary incrementable type
    HPX_CXX_CORE_EXPORT template <typename Incrementable>
    counting_iterator(Incrementable x) -> counting_iterator<Incrementable>;

    template <typename Incrementable>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::make_counting_iterator is deprecated, use "
        "hpx::util::counting_iterator instead")
    HPX_HOST_DEVICE
        constexpr counting_iterator<Incrementable> make_counting_iterator(
            Incrementable x) noexcept
    {
        return counting_iterator<Incrementable>(HPX_MOVE(x));
    }
}    // namespace hpx::util
