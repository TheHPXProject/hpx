//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/iterator_support/iterator_adaptor.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/type_support.hpp>

#include <iterator>
#include <type_traits>

namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iterator, typename Transformer,
        typename Reference = void, typename Value = void,
        typename Category = void, typename Difference = void>
    class transform_iterator;

    namespace detail {

        template <typename Iterator, typename Transformer, typename Reference,
            typename Value, typename Category, typename Difference>
        struct transform_iterator_base
        {
            // the following type calculations use lazy_conditional to avoid
            // premature instantiations
            using reference_type =
                util::lazy_conditional_t<std::is_void_v<Reference>,
                    util::invoke_result<Transformer, Iterator>,
                    hpx::type_identity<Reference>>;

            using value_type = util::lazy_conditional_t<std::is_void_v<Value>,
                std::remove_reference<reference_type>,
                hpx::type_identity<Value>>;

            using iterator_category =
                util::lazy_conditional_t<std::is_void_v<Category>,
                    category_iterator_traits_helper<Iterator>,
                    hpx::type_identity<Category>>;

            using difference_type =
                util::lazy_conditional_t<std::is_void_v<Difference>,
                    difference_type_iterator_traits_helper<Iterator>,
                    hpx::type_identity<Difference>>;

            using type = hpx::util::iterator_adaptor<
                transform_iterator<Iterator, Transformer, Reference, Value,
                    Category, Difference>,
                Iterator, value_type, iterator_category, reference_type,
                difference_type>;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // The main difference to boost::transform_iterator is that the transformer
    // function will be invoked with the iterator, not with the result of
    // dereferencing the base iterator.
    HPX_CXX_CORE_EXPORT template <typename Iterator, typename Transformer,
        typename Reference, typename Value, typename Category,
        typename Difference>
    class transform_iterator
      : public detail::transform_iterator_base<Iterator, Transformer, Reference,
            Value, Category, Difference>::type
    {
    private:
        using base_type = typename detail::transform_iterator_base<Iterator,
            Transformer, Reference, Value, Category, Difference>::type;

    public:
        transform_iterator() = default;

        explicit transform_iterator(Iterator const& it)
          : base_type(it)
        {
        }

        transform_iterator(Iterator const& it, Transformer const& f)
          : base_type(it)
          , transformer_(f)
        {
        }

        template <typename OtherIterator, typename OtherTransformer,
            typename OtherReference, typename OtherValue,
            typename OtherCategory, typename OtherDifference>
        transform_iterator(
            transform_iterator<OtherIterator, OtherTransformer, OtherReference,
                OtherValue, OtherCategory, OtherDifference> const& t,
            std::enable_if_t<std::is_convertible_v<OtherIterator, Iterator> &&
                std::is_convertible_v<OtherTransformer, Transformer> &&
                std::is_convertible_v<OtherCategory, Category> &&
                std::is_convertible_v<OtherDifference, Difference>>* = nullptr)
          : base_type(t.base())
          , transformer_(t.transformer())
        {
        }

        [[nodiscard]] constexpr Transformer const& transformer() const noexcept
        {
            return transformer_;
        }

    private:
        friend class hpx::util::iterator_core_access;

        typename base_type::reference dereference() const
        {
            return transformer_(this->base());
        }

        Transformer transformer_;
    };

    HPX_CXX_CORE_EXPORT template <typename Iterator, typename Transformer>
    transform_iterator(Iterator const&, Transformer const&)
        -> transform_iterator<Iterator, Transformer>;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Transformer, typename Iterator>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::make_transform_iterator is deprecated, use "
        "hpx::util::transform_iterator instead")
    transform_iterator<Iterator, Transformer> make_transform_iterator(
        Iterator const& it, Transformer const& f)
    {
        return transform_iterator<Iterator, Transformer>(it, f);
    }

    HPX_CXX_CORE_EXPORT template <typename Transformer, typename Iterator>
    transform_iterator<Iterator, Transformer> make_transform_iterator(
        Iterator const& it)
    {
        return transform_iterator<Iterator, Transformer>(it, Transformer());
    }
}    // namespace hpx::util
