//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2014 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/iterator_support/detail/minimum_category.hpp>
#include <hpx/iterator_support/iterator_facade.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IteratorTuple>
        struct zip_iterator_value;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct zip_iterator_element_value
        {
            using type = hpx::traits::iter_value_t<T>;
        };

        template <>
        struct zip_iterator_element_value<hpx::default_sentinel_t>
        {
            using type = hpx::util::unused_type;
        };

        template <typename... Ts>
        struct zip_iterator_value<hpx::tuple<Ts...>>
        {
            using type =
                hpx::tuple<typename zip_iterator_element_value<Ts>::type...>;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IteratorTuple>
        struct zip_iterator_reference;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct zip_iterator_reference_value
        {
            using type = hpx::traits::iter_reference_t<T>;
        };

        template <>
        struct zip_iterator_reference_value<hpx::default_sentinel_t>
        {
            using type = hpx::util::unused_type;
        };

        template <typename... Ts>
        struct zip_iterator_reference<hpx::tuple<Ts...>>
        {
            using type =
                hpx::tuple<typename zip_iterator_reference_value<Ts>::type...>;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IteratorTuple,
            typename Enable = void>
        struct zip_iterator_category;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct zip_iterator_element_category
        {
            using type = hpx::traits::iter_category_t<T>;
        };

        template <>
        struct zip_iterator_element_category<hpx::default_sentinel_t>
        {
            using type = std::random_access_iterator_tag;
        };

        template <typename T>
        struct zip_iterator_category<hpx::tuple<T>,
            std::enable_if_t<hpx::tuple_size<hpx::tuple<T>>::value == 1>>
        {
            using type = typename zip_iterator_element_category<T>::type;
        };

        template <typename T, typename U>
        struct zip_iterator_category<hpx::tuple<T, U>,
            std::enable_if_t<hpx::tuple_size<hpx::tuple<T, U>>::value == 2>>
          : minimum_category<typename zip_iterator_element_category<T>::type,
                typename zip_iterator_element_category<U>::type>
        {
        };

        template <typename T, typename U, typename... Tail>
        struct zip_iterator_category<hpx::tuple<T, U, Tail...>,
            std::enable_if_t<(
                hpx::tuple_size<hpx::tuple<T, U, Tail...>>::value > 2)>>
          : minimum_category<
                typename minimum_category<
                    typename zip_iterator_element_category<T>::type,
                    typename zip_iterator_element_category<U>::type>::type,
                typename zip_iterator_category<hpx::tuple<Tail...>>::type>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IteratorTuple>
        struct dereference_iterator;

        template <typename... Ts>
        struct dereference_iterator<hpx::tuple<Ts...>>
        {
            template <std::size_t... Is>
            HPX_HOST_DEVICE static constexpr
                typename zip_iterator_reference<hpx::tuple<Ts...>>::type
                call(
                    util::index_pack<Is...>, hpx::tuple<Ts...> const& iterators)
            {
                return hpx::forward_as_tuple(*hpx::get<Is>(iterators)...);
            }
        };

        HPX_CXX_CORE_EXPORT struct increment_iterator
        {
            template <typename T>
            HPX_HOST_DEVICE constexpr void operator()(T& iter) const
                noexcept(noexcept(++std::declval<T&>()))
            {
                ++iter;
            }
        };

        HPX_CXX_CORE_EXPORT struct decrement_iterator
        {
            template <typename T>
            HPX_HOST_DEVICE constexpr void operator()(T& iter) const
                noexcept(noexcept(--std::declval<T&>()))
            {
                --iter;
            }
        };

        HPX_CXX_CORE_EXPORT struct advance_iterator
        {
            explicit constexpr advance_iterator(std::ptrdiff_t n) noexcept
              : n_(n)
            {
            }

            template <typename T>
            HPX_HOST_DEVICE constexpr void operator()(T& iter) const noexcept(
                noexcept(std::declval<T&>() += std::declval<std::ptrdiff_t>()))
            {
                iter += n_;
            }

            std::ptrdiff_t n_;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        struct has_default_sentinel
          : std::is_same<std::decay_t<T>, hpx::default_sentinel_t>
        {
        };

        template <typename... Ts>
        struct has_default_sentinel<hpx::tuple<Ts...>>
          : util::any_of<has_default_sentinel<Ts>...>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename Tuple>
        inline constexpr bool has_default_sentinel_v =
            has_default_sentinel<Tuple>::value;

        HPX_CXX_CORE_EXPORT template <std::size_t I, std::size_t Size,
            typename Enable = void>
        struct one_tuple_element_equal_to
        {
            template <typename TTuple, typename UTuple>
            static constexpr HPX_HOST_DEVICE HPX_FORCEINLINE bool call(
                TTuple const& t, UTuple const& u)
            {
                return get<I>(t) == get<I>(u) ||
                    one_tuple_element_equal_to<I + 1, Size>::call(t, u);
            }
        };

        template <std::size_t I, std::size_t Size>
        struct one_tuple_element_equal_to<I, Size,
            std::enable_if_t<I + 1 == Size>>
        {
            template <typename TTuple, typename UTuple>
            static constexpr HPX_HOST_DEVICE HPX_FORCEINLINE bool call(
                TTuple const& t, UTuple const& u)
            {
                return get<I>(t) == get<I>(u);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IteratorTuple, typename Derived>
        class zip_iterator_base
          : public hpx::util::iterator_facade<Derived,
                typename zip_iterator_value<IteratorTuple>::type,
                typename zip_iterator_category<IteratorTuple>::type,
                typename zip_iterator_reference<IteratorTuple>::type>
        {
            using base_type = hpx::util::iterator_facade<
                zip_iterator_base<IteratorTuple, Derived>,
                typename zip_iterator_value<IteratorTuple>::type,
                typename zip_iterator_category<IteratorTuple>::type,
                typename zip_iterator_reference<IteratorTuple>::type>;

        public:
            // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
            HPX_HOST_DEVICE zip_iterator_base() = default;

            HPX_HOST_DEVICE explicit constexpr zip_iterator_base(
                IteratorTuple const& iterators)
              : iterators_(iterators)
            {
            }
            HPX_HOST_DEVICE explicit constexpr zip_iterator_base(
                IteratorTuple&& iterators) noexcept
              : iterators_(HPX_MOVE(iterators))
            {
            }

            HPX_HOST_DEVICE zip_iterator_base(
                zip_iterator_base const&) = default;
            HPX_HOST_DEVICE zip_iterator_base(
                zip_iterator_base&&) noexcept = default;
            // NOLINTEND(bugprone-crtp-constructor-accessibility)

            HPX_HOST_DEVICE zip_iterator_base& operator=(
                zip_iterator_base const&) = default;
            HPX_HOST_DEVICE zip_iterator_base& operator=(
                zip_iterator_base&&) noexcept = default;

            HPX_HOST_DEVICE ~zip_iterator_base() = default;

            HPX_HOST_DEVICE
            constexpr zip_iterator_base& operator=(
                IteratorTuple const& iterators)
            {
                iterators_ = iterators;
                return *this;
            }

            HPX_HOST_DEVICE
            constexpr zip_iterator_base& operator=(
                IteratorTuple&& iterators) noexcept
            {
                iterators_ = HPX_MOVE(iterators);
                return *this;
            }

            using iterator_tuple_type = IteratorTuple;

            HPX_HOST_DEVICE constexpr iterator_tuple_type const&
            get_iterator_tuple() const& noexcept
            {
                return iterators_;
            }

            HPX_HOST_DEVICE constexpr iterator_tuple_type&&
            get_iterator_tuple() && noexcept
            {
                return HPX_MOVE(iterators_);
            }

        protected:
            friend class hpx::util::iterator_core_access;

            // Special comparison as two iterators are considered equal if one of
            // the embedded iterators compares equal to hpx::default_sentinel or
            // all iterators are equal. This is done to ensure that the shortest
            // sequence limits the overall iteration (if detectable).
            template <typename IterTuple, typename Derived_,
                typename = std::enable_if_t<hpx::tuple_size_v<IterTuple> ==
                    hpx::tuple_size_v<IteratorTuple>>>
            HPX_HOST_DEVICE constexpr bool equal(
                zip_iterator_base<IterTuple, Derived_> const& other) const
                noexcept(noexcept(
                    std::declval<IteratorTuple>() == std::declval<IterTuple>()))
            {
                return one_tuple_element_equal_to<0,
                    hpx::tuple_size_v<IterTuple>>::call(iterators_,
                    other.get_iterator_tuple());
            }

            HPX_HOST_DEVICE constexpr typename base_type::reference
            dereference() const
            {
                return dereference_iterator<IteratorTuple>::call(
                    util::make_index_pack_t<
                        hpx::tuple_size<IteratorTuple>::value>(),
                    iterators_);
            }

            HPX_HOST_DEVICE void increment()
            {
                this->apply(increment_iterator());
            }

            HPX_HOST_DEVICE void decrement()
            {
                this->apply(decrement_iterator());
            }

            HPX_HOST_DEVICE void advance(std::ptrdiff_t n)
            {
                this->apply(advance_iterator(n));
            }

            HPX_HOST_DEVICE
            std::ptrdiff_t distance_to(zip_iterator_base const& other) const
            {
                return hpx::get<0>(other.iterators_) - hpx::get<0>(iterators_);
            }

        private:
            template <typename F, std::size_t... Is>
            HPX_HOST_DEVICE void apply(F&& f, util::index_pack<Is...>)
            {
                (f(hpx::get<Is>(iterators_)), ...);
            }

            template <typename F>
            HPX_HOST_DEVICE void apply(F&& f)
            {
                return apply(HPX_FORWARD(F, f),
                    util::make_index_pack<
                        hpx::tuple_size<IteratorTuple>::value>());
            }

        private:
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned)
            {
                // clang-format off
                ar & iterators_;
                // clang-format on
            }

        private:
            IteratorTuple iterators_;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    class zip_iterator
      : public detail::zip_iterator_base<hpx::tuple<Ts...>, zip_iterator<Ts...>>
    {
        static_assert(
            sizeof...(Ts) != 0, "zip_iterator must wrap at least one iterator");

        using base_type =
            detail::zip_iterator_base<hpx::tuple<Ts...>, zip_iterator<Ts...>>;

    public:
        HPX_HOST_DEVICE zip_iterator() = default;

        HPX_HOST_DEVICE explicit constexpr zip_iterator(
            Ts const&... vs) noexcept
          : base_type(hpx::tie(vs...))
        {
        }

        HPX_HOST_DEVICE explicit constexpr zip_iterator(
            hpx::tuple<Ts...>&& t) noexcept
          : base_type(HPX_MOVE(t))
        {
        }

        HPX_HOST_DEVICE constexpr zip_iterator(
            zip_iterator const& other) = default;
        HPX_HOST_DEVICE constexpr zip_iterator(
            zip_iterator&& other) noexcept = default;

        HPX_HOST_DEVICE zip_iterator& operator=(
            zip_iterator const& other) = default;
        HPX_HOST_DEVICE zip_iterator& operator=(
            zip_iterator&& other) noexcept = default;

        HPX_HOST_DEVICE ~zip_iterator() = default;

        template <typename... Ts_>
        HPX_HOST_DEVICE std::enable_if_t<
            std::is_assignable_v<typename zip_iterator::iterator_tuple_type&,
                typename zip_iterator<Ts_...>::iterator_tuple_type&&>,
            zip_iterator&>
        operator=(zip_iterator<Ts_...> const& other)
        {
            base_type::operator=(other.get_iterator_tuple());
            return *this;
        }

        template <typename... Ts_>
        HPX_HOST_DEVICE std::enable_if_t<
            std::is_assignable_v<typename zip_iterator::iterator_tuple_type&,
                typename zip_iterator<Ts_...>::iterator_tuple_type&&>,
            zip_iterator&>
        operator=(zip_iterator<Ts_...>&& other) noexcept
        {
            base_type::operator=(HPX_MOVE(other).get_iterator_tuple());
            return *this;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    zip_iterator(Ts const&... vs) -> zip_iterator<Ts...>;

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    zip_iterator(hpx::tuple<Ts...>&& vs) -> zip_iterator<Ts...>;

    template <typename... Ts>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::make_zip_iterator is deprecated, use "
        "hpx::util::zip_iterator instead")
    HPX_HOST_DEVICE
        constexpr zip_iterator<std::decay_t<Ts>...> make_zip_iterator(
            Ts&&... vs)
    {
        using result_type = zip_iterator<std::decay_t<Ts>...>;
        return result_type(HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename... Ts>
    class zip_iterator<hpx::tuple<Ts...>>
      : public detail::zip_iterator_base<hpx::tuple<Ts...>,
            zip_iterator<hpx::tuple<Ts...>>>
    {
        static_assert(
            sizeof...(Ts) != 0, "zip_iterator must wrap at least one iterator");

        using base_type = detail::zip_iterator_base<hpx::tuple<Ts...>,
            zip_iterator<hpx::tuple<Ts...>>>;

    public:
        HPX_HOST_DEVICE zip_iterator() = default;

        HPX_HOST_DEVICE explicit constexpr zip_iterator(
            Ts const&... vs) noexcept
          : base_type(hpx::tie(vs...))
        {
        }

        HPX_HOST_DEVICE explicit constexpr zip_iterator(
            hpx::tuple<Ts...>&& t) noexcept
          : base_type(HPX_MOVE(t))
        {
        }

        HPX_HOST_DEVICE constexpr zip_iterator(
            zip_iterator const& other) = default;
        HPX_HOST_DEVICE constexpr zip_iterator(
            zip_iterator&& other) noexcept = default;

        HPX_HOST_DEVICE zip_iterator& operator=(
            zip_iterator const& other) = default;
        HPX_HOST_DEVICE zip_iterator& operator=(
            zip_iterator&& other) noexcept = default;

        HPX_HOST_DEVICE ~zip_iterator() = default;

        template <typename... Ts_>
        HPX_HOST_DEVICE std::enable_if_t<
            std::is_assignable_v<typename zip_iterator::iterator_tuple_type&,
                typename zip_iterator<Ts_...>::iterator_tuple_type&&>,
            zip_iterator&>
        operator=(zip_iterator<Ts_...> const& other)
        {
            base_type::operator=(base_type(other.get_iterator_tuple()));
            return *this;
        }

        template <typename... Ts_>
        HPX_HOST_DEVICE std::enable_if_t<
            std::is_assignable_v<typename zip_iterator::iterator_tuple_type&,
                typename zip_iterator<Ts_...>::iterator_tuple_type&&>,
            zip_iterator&>
        operator=(zip_iterator<Ts_...>&& other) noexcept
        {
            base_type::operator=(
                base_type(HPX_MOVE(other).get_iterator_tuple()));
            return *this;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ZipIter>
    struct zip_iterator_category
      : detail::zip_iterator_category<typename ZipIter::iterator_tuple_type>
    {
    };
}    // namespace hpx::util

namespace hpx::traits {

    namespace functional {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename F, typename T>
        struct element_result_of : util::invoke_result<F, T>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename F, typename Iter>
        struct lift_zipped_iterators;

        template <typename F, typename... Ts>
        struct lift_zipped_iterators<F, util::zip_iterator<Ts...>>
        {
            using tuple_type =
                typename util::zip_iterator<Ts...>::iterator_tuple_type;
            using result_type = hpx::tuple<typename element_result_of<
                typename F::template apply<Ts>, Ts>::type...>;

            template <std::size_t... Is, typename... Ts_>
            static result_type call(
                util::index_pack<Is...>, hpx::tuple<Ts_...> const& t)
            {
                return hpx::make_tuple(
                    typename F::template apply<Ts>()(hpx::get<Is>(t))...);
            }

            template <typename... Ts_>
            static result_type call(util::zip_iterator<Ts_...> const& iter)
            {
                using hpx::util::make_index_pack_t;
                return call(make_index_pack_t<sizeof...(Ts)>(),
                    iter.get_iterator_tuple());
            }
        };
    }    // namespace functional

    ///////////////////////////////////////////////////////////////////////////
    template <typename... Iter>
    struct is_zip_iterator<hpx::util::zip_iterator<Iter...>> : std::true_type
    {
    };
}    // namespace hpx::traits
