//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/pack_traversal/detail/container_category.hpp>
#include <hpx/pack_traversal/traits/pack_traversal_rebind_container.hpp>

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::util::detail {

    /// Exposes useful facilities for dealing with 1:n mappings
    namespace spreading {

        /// A struct to mark a tuple to be unpacked into the parent context
        template <typename... T>
        class spread_box
        {
            hpx::tuple<T...> boxed_;

        public:
            explicit spread_box(hpx::tuple<T...> boxed) noexcept
              : boxed_(HPX_MOVE(boxed))
            {
            }

            hpx::tuple<T...> unbox() noexcept
            {
                return HPX_MOVE(boxed_);
            }
        };

        template <>
        class spread_box<>
        {
        public:
            explicit constexpr spread_box() noexcept = default;
            explicit constexpr spread_box(hpx::tuple<> const&) noexcept {}

            static constexpr hpx::tuple<> unbox() noexcept
            {
                return hpx::tuple<>{};
            }
        };

        /// Returns an empty spread box which represents an empty mapped object.
        constexpr inline spread_box<> empty_spread() noexcept
        {
            return spread_box<>{};
        }

        /// Deduces to a true_type if the given type is a spread marker
        template <typename T>
        struct is_spread : std::false_type
        {
        };

        template <typename... T>
        struct is_spread<spread_box<T...>> : std::true_type
        {
        };

        /// Deduces to a true_type if the given type is an empty spread marker
        template <typename T>
        struct is_empty_spread : std::false_type
        {
        };

        template <>
        struct is_empty_spread<spread_box<>> : std::true_type
        {
        };

        /// Converts types to the type and spread_box objects to its underlying
        /// tuple.
        template <typename T>
        constexpr T unpack(T&& type)
        {
            return HPX_FORWARD(T, type);
        }

        template <typename... T>
        constexpr auto unpack(spread_box<T...> type) noexcept    //-V524
            -> decltype(type.unbox())
        {
            return type.unbox();
        }

        /// Deduces to the type unpack is returning when called with the given
        /// type T.
        template <typename T>
        using unpacked_of_t = decltype(unpack(std::declval<T>()));

        /// Converts types to the type and spread_box objects to its underlying
        /// tuple. If the type is mapped to zero elements, the return type will
        /// be void.
        template <typename T>
        constexpr auto unpack_or_void(T&& type)
            -> decltype(unpack(HPX_FORWARD(T, type)))
        {
            return unpack(HPX_FORWARD(T, type));
        }

        constexpr inline void unpack_or_void(spread_box<>) noexcept {}

        /// Converts types to the tuple carrying the single type and spread_box
        /// objects to its underlying tuple.
        template <typename T>
        constexpr hpx::tuple<T> undecorate(T&& type)
        {
            return hpx::tuple<T>{HPX_FORWARD(T, type)};
        }

        template <typename... T>
        constexpr auto undecorate(spread_box<T...> type) noexcept    //-V524
            -> decltype(type.unbox())
        {
            return type.unbox();
        }

        /// A callable object which maps its content back to a tuple like type.
        template <typename EmptyType, template <typename...> class Type>
        struct tupelizer_base
        {
            // We overload with one argument here so Clang and GCC don't have
            // any issues with overloading against zero arguments.
            template <typename First, typename... T>
            constexpr Type<First, T...> operator()(
                First&& first, T&&... args) const
            {
                return Type<First, T...>{
                    HPX_FORWARD(First, first), HPX_FORWARD(T, args)...};
            }

            // Specifically return the empty object which can be different from
            // a tuple.
            constexpr EmptyType operator()() const
                noexcept(noexcept(EmptyType{}))
            {
                return EmptyType{};
            }
        };

        /// A callable object which maps its content back to a tuple.
        template <template <typename...> class Type = hpx::tuple>
        using tupelizer_of_t = tupelizer_base<hpx::tuple<>, Type>;

        /// A callable object which maps its content back to a tuple like type
        /// if it wasn't empty. For empty types arguments an empty spread box is
        /// returned instead. This is useful to propagate empty mappings back to
        /// the caller.
        template <template <typename...> class Type = hpx::tuple>
        using flat_tupelizer_of_t = tupelizer_base<spread_box<>, Type>;

        /// A callable object which maps its content back to an array like type.
        /// This transform can only be used for (flat) mappings which return an
        /// empty mapping back to the caller.
        template <template <typename, std::size_t> class Type>
        struct flat_arraylizer
        {
            /// Deduces to the array type when the array is instantiated with
            /// the given arguments.
            template <typename First, typename... Rest>
            using array_type_of_t =
                Type<std::decay_t<First>, 1 + sizeof...(Rest)>;

            // We overload with one argument here so Clang and GCC don't have
            // any issues with overloading against zero arguments.
            template <typename First, typename... T>
            constexpr auto operator()(First&& first, T&&... args) const
                -> array_type_of_t<First, T...>
            {
                return array_type_of_t<First, T...>{
                    {HPX_FORWARD(First, first), HPX_FORWARD(T, args)...}};
            }

            constexpr auto operator()() const noexcept
                -> decltype(empty_spread())
            {
                return empty_spread();
            }
        };

        /// Use the recursive instantiation for a variadic pack which may
        /// contain spread types
        template <typename C, typename... T>
        constexpr auto apply_spread_impl(std::true_type, C&& callable,
            T&&... args) -> decltype(hpx::invoke_fused(HPX_FORWARD(C, callable),
            hpx::tuple_cat(undecorate(HPX_FORWARD(T, args))...)))
        {
            return hpx::invoke_fused(HPX_FORWARD(C, callable),
                hpx::tuple_cat(undecorate(HPX_FORWARD(T, args))...));
        }

        /// Use the linear instantiation for variadic packs which don't contain
        /// spread types.
        template <typename C, typename... T>
        constexpr auto apply_spread_impl(std::false_type, C&& callable,
            T&&... args) -> hpx::util::invoke_result_t<C, T...>
        {
            return HPX_INVOKE(
                HPX_FORWARD(C, callable), HPX_FORWARD(T, args)...);
        }

        /// Deduces to a true_type if any of the given types marks the
        /// underlying type to be spread into the current context.
        template <typename... T>
        using is_any_spread_t = util::any_of<is_spread<T>...>;

        template <typename C, typename... T>
        constexpr auto map_spread(C&& callable, T&&... args)
            -> decltype(apply_spread_impl(is_any_spread_t<T...>{},
                HPX_FORWARD(C, callable), HPX_FORWARD(T, args)...))
        {
            // Check whether any of the args is a detail::flatted_tuple_t,
            // if not, use the linear called version for better
            // compilation speed.
            return apply_spread_impl(is_any_spread_t<T...>{},
                HPX_FORWARD(C, callable), HPX_FORWARD(T, args)...);
        }

        /// Converts the given variadic arguments into a tuple in a way that
        /// spread return values are inserted into the current pack.
        template <typename... T>
        constexpr auto tupelize(T&&... args)
            -> decltype(map_spread(tupelizer_of_t<>{}, HPX_FORWARD(T, args)...))
        {
            return map_spread(tupelizer_of_t<>{}, HPX_FORWARD(T, args)...);
        }

        // clang-format off

        /// Converts the given variadic arguments into a tuple in a way that
        /// spread return values are inserted into the current pack. If the
        /// arguments were mapped to zero arguments, the empty mapping is
        /// propagated backwards to the caller.
        template <template <typename...> class Type, typename... T>
        constexpr auto flat_tupelize_to(T&&... args) -> decltype(
            map_spread(flat_tupelizer_of_t<Type>{}, HPX_FORWARD(T, args)...))
        {
            return map_spread(
                flat_tupelizer_of_t<Type>{}, HPX_FORWARD(T, args)...);
        }

        /// Converts the given variadic arguments into an array in a way that
        /// spread return values are inserted into the current pack. Through
        /// this the size of the array like type might change. If the arguments
        /// were mapped to zero arguments, the empty mapping is propagated
        /// backwards to the caller.
        template <template <typename, std::size_t> class Type, typename... T>
        constexpr auto flat_arraylize_to(T&&... args) -> decltype(
            map_spread(flat_arraylizer<Type>{}, HPX_FORWARD(T, args)...))
        {
            return map_spread(flat_arraylizer<Type>{}, HPX_FORWARD(T, args)...);
        }
        // clang-format on

        /// Converts an empty tuple to void
        template <typename First, typename... Rest>
        constexpr hpx::tuple<First, Rest...> voidify_empty_tuple(
            hpx::tuple<First, Rest...> val)
        {
            return val;
        }
        inline void voidify_empty_tuple(hpx::tuple<> const&) noexcept {}

        /// Converts the given variadic arguments into a tuple in a way that
        /// spread return values are inserted into the current pack.
        ///
        /// If the returned tuple is empty, void is returned instead.
        template <typename... T>
        constexpr auto tupelize_or_void(T&&... args)
            -> decltype(voidify_empty_tuple(tupelize(HPX_FORWARD(T, args)...)))
        {
            return voidify_empty_tuple(tupelize(HPX_FORWARD(T, args)...));
        }
    }    // end namespace spreading

    /// Just traverses the pack with the given callable object, no result is
    /// returned or preserved.
    struct strategy_traverse_tag
    {
    };
    /// Remaps the variadic pack with the return values from the mapper.
    struct strategy_remap_tag
    {
    };

    /// Deduces to a true type if the type leads to at least one effective call
    /// to the mapper.
    template <typename Mapper, typename T>
    using is_effective_t = is_invocable<typename Mapper::traversor_type, T>;

    template <typename Mapper, typename T>
    inline constexpr bool is_effective_v = is_effective_t<Mapper, T>::value;

    /// Deduces to a true type if any type leads to at least one effective
    /// call to the mapper.
    template <typename Mapper, typename... T>
    struct is_effective_any_of_t;

    template <typename Mapper, typename First, typename... Rest>
    struct is_effective_any_of_t<Mapper, First, Rest...>
      : std::conditional_t<is_effective_v<Mapper, First>, std::true_type,
            is_effective_any_of_t<Mapper, Rest...>>
    {
    };

    template <typename Mapper>
    struct is_effective_any_of_t<Mapper> : std::false_type
    {
    };

    template <typename Mapper, typename... T>
    inline constexpr bool is_effective_any_of_v =
        is_effective_any_of_t<Mapper, T...>::value;

    /// Provides utilities for remapping the whole content of a container like
    /// type to the same container holding different types.
    namespace container_remapping {

        /// Deduces to a true type if the given parameter T has a push_back
        /// method that accepts a type of E.
        HPX_CXX_CORE_EXPORT template <typename T, typename E, typename = void>
        struct has_push_back : std::false_type
        {
        };

        template <typename T, typename E>
        struct has_push_back<T, E,
            std::void_t<decltype(std::declval<T>().push_back(
                std::declval<E>()))>> : std::true_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T, typename E>
        inline constexpr bool has_push_back_v = has_push_back<T, E>::value;

        // clang-format off

        /// Specialization for a container with a single type T
        template <typename NewType, typename Container>
        auto rebind_container(Container const& container) -> decltype(
            traits::pack_traversal_rebind_container<NewType, Container>::call(
                std::declval<Container>()))
        {
            return traits::pack_traversal_rebind_container<NewType,
                Container>::call(container);
        }
        // clang-format on

        /// Returns the default iterators of the container in case the container
        /// was passed as an l-value reference. Otherwise, move iterators of the
        /// container are returned.
        template <typename C, typename = void>
        class container_accessor
        {
            static_assert(std::is_lvalue_reference_v<C>,
                "This should be a lvalue reference here!");

            C container_;

        public:
            explicit constexpr container_accessor(C container) noexcept
              : container_(container)
            {
            }

            auto begin() -> decltype(container_.begin())
            {
                return container_.begin();
            }

            auto end() -> decltype(container_.end())
            {
                return container_.end();
            }
        };

        template <typename C>
        class container_accessor<C,
            std::enable_if_t<std::is_rvalue_reference_v<C&&>>>
        {
            C&& container_;

        public:
            explicit container_accessor(C&& container)
              : container_(HPX_MOVE(container))
            {
            }

            auto begin()
                -> decltype(std::make_move_iterator(container_.begin()))
            {
                return std::make_move_iterator(container_.begin());
            }

            auto end() -> decltype(std::make_move_iterator(container_.end()))
            {
                return std::make_move_iterator(container_.end());
            }
        };

        template <typename T>
        container_accessor<T> container_accessor_of(T&& container)
        {
            // Don't use any decay here
            return container_accessor<T>(HPX_FORWARD(T, container));
        }

        /// Deduces to the type the homogeneous container is containing
        ///
        /// This alias deduces to the same type on which container_accessor<T>
        /// is iterating.
        ///
        /// The basic idea is that we deduce to the type the homogeneous
        /// container T is carrying as reference while preserving the original
        /// reference type of the container:
        /// - If the container was passed as l-value its containing values are
        ///   referenced through l-values.
        /// - If the container was passed as r-value its containing values are
        ///   referenced through r-values.
        template <typename Container>
        using element_of_t =
            std::conditional_t<std::is_rvalue_reference_v<Container&&>,
                decltype(HPX_MOVE(*(std::declval<Container>().begin()))),
                decltype(*(std::declval<Container>().begin()))>;

        /// Removes all qualifier and references from the given type if the type
        /// is a l-value or r-value reference.
        template <typename T>
        using dereferenced_of_t =
            std::conditional_t<std::is_reference_v<T>, std::decay_t<T>, T>;

        /// Returns the type which is resulting if the mapping is applied to an
        /// element in the container.
        ///
        /// Since standard containers don't allow to be instantiated with
        /// references we try to construct the container from a copied version.
        template <typename Container, typename Mapping>
        using mapped_type_from_t = dereferenced_of_t<spreading::unpacked_of_t<
            hpx::util::invoke_result_t<Mapping, element_of_t<Container>>>>;

        /// Deduces to a true_type if the mapping maps to zero elements.
        template <typename T, typename M>
        using is_empty_mapped = spreading::is_empty_spread<
            std::decay_t<hpx::util::invoke_result_t<M, element_of_t<T>>>>;

        template <typename T, typename M>
        inline constexpr bool is_empty_mapped_v = is_empty_mapped<T, M>::value;

        /// We are allowed to reuse the container if we map to the same type we
        /// are accepting and when we have the full ownership of the container.
        template <typename T, typename M>
        using can_reuse = std::integral_constant<bool,
            std::is_same_v<element_of_t<T>, mapped_type_from_t<T, M>> &&
                std::is_rvalue_reference_v<T&&>>;

        template <typename T, typename M>
        inline constexpr bool can_reuse_v = can_reuse<T, M>::value;

        /// Categorizes a mapping of a homogeneous container
        ///
        /// \tparam IsEmptyMapped Identifies whether the mapping maps to zero
        ///         arguments.
        /// \tparam CanReuse Identifies whether the container can be re-used
        ///         through the mapping.
        template <bool IsEmptyMapped, bool CanReuse>
        struct container_mapping_tag
        {
        };

        /// Categorizes the given container through a container_mapping_tag
        template <typename T, typename M>
        using container_mapping_tag_of_t =
            container_mapping_tag<is_empty_mapped_v<T, M>, can_reuse_v<T, M>>;

        /// We create a new container, which may hold the resulting type
        template <typename M, typename T>
        auto remap_container(
            container_mapping_tag<false, false>, M&& mapper, T&& container)
            -> decltype(rebind_container<mapped_type_from_t<T, M>>(container))
        {
            static_assert(has_push_back_v<std::decay_t<T>, element_of_t<T>>,
                "Can only remap containers that provide a push_back method!");

            // Create the new container, which is capable of holding the
            // re-mapped types.
            auto remapped =
                rebind_container<mapped_type_from_t<T, M>>(container);

            // We try to reserve the original size from the source container to
            // the destination container.
            traits::detail::reserve_if_reservable(remapped, container.size());

            // Perform the actual value remapping from the source to the
            // destination. We could have used std::transform for this, however,
            // I didn't want to pull a whole header for it in.
            for (auto&& val : container_accessor_of(HPX_FORWARD(T, container)))
            {
#if defined(__NVCC__)
                remapped.push_back(spreading::unpack(mapper(val)));
#else
                remapped.push_back(
                    spreading::unpack(mapper(HPX_FORWARD(decltype(val), val))));
#endif
            }

            return remapped;    // RVO
        }

        /// The remapper optimized for the case that we map to the same type we
        /// accepted such as int -> int.
        template <typename M, typename T>
        auto remap_container(container_mapping_tag<false, true>, M&& mapper,
            T&& container) -> std::decay_t<T>
        {
            for (auto&& val : container_accessor_of(HPX_FORWARD(T, container)))
            {
#if defined(__NVCC__)
                val = spreading::unpack(mapper(val));
#else
                val =
                    spreading::unpack(mapper(HPX_FORWARD(decltype(val), val)));
#endif
            }
            return HPX_FORWARD(T, container);
        }

        /// Remap the container to zero arguments
        template <typename M, typename T>
        auto remap_container(container_mapping_tag<true, false>, M&& mapper,
            T&& container) -> decltype(spreading::empty_spread())
        {
            for (auto&& val : container_accessor_of(HPX_FORWARD(T, container)))
            {
                // Don't save the empty mapping for each invocation
                // of the mapper.
#if defined(__NVCC__)
                mapper(val);
#else
                mapper(HPX_FORWARD(decltype(val), val));
#endif
            }
            // Return one instance of an empty mapping for the container
            return spreading::empty_spread();
        }

        /// Remaps the content of the given container with type T, to a
        /// container of the same type which may contain different types.
        template <typename T, typename M>
        auto remap(strategy_remap_tag, T&& container, M&& mapper,
            std::enable_if_t<is_effective_v<M, element_of_t<T>>>* = nullptr)
            -> decltype(remap_container(container_mapping_tag_of_t<T, M>{},
                HPX_FORWARD(M, mapper), HPX_FORWARD(T, container)))
        {
            return remap_container(container_mapping_tag_of_t<T, M>{},
                HPX_FORWARD(M, mapper), HPX_FORWARD(T, container));
        }

        /// Just call the visitor with the content of the container
        template <typename T, typename M>
        void remap(strategy_traverse_tag, T&& container, M&& mapper,
            std::enable_if_t<is_effective_v<M, element_of_t<T>>>* = nullptr)
        {
            // CUDA needs std::forward here
            for (auto&& element : std::forward<T>(container))
            {
#if defined(__NVCC__)
                mapper(element);
#else
                mapper(HPX_FORWARD(decltype(element), element));
#endif
            }
        }
    }    // end namespace container_remapping

    /// Provides utilities for remapping the whole content of a tuple like type
    /// to the same type holding different types.
    namespace tuple_like_remapping {

        template <typename Strategy, typename Mapper, typename T,
            typename Enable = void>
        struct tuple_like_remapper
        {
        };

        /// Specialization for std::tuple like types which contain an arbitrary
        /// amount of heterogeneous arguments.
        template <typename M, template <typename...> typename Base,
            typename... OldArgs>
        struct tuple_like_remapper<strategy_remap_tag, M, Base<OldArgs...>,
            // Support for skipping completely untouched types
            std::enable_if_t<is_effective_any_of_v<M, OldArgs...>>>
        {
            M mapper_;

            template <typename... Args>
            auto operator()(Args&&... args)
                -> decltype(spreading::flat_tupelize_to<Base>(
                    std::declval<M>()(HPX_FORWARD(Args, args))...))
            {
                return spreading::flat_tupelize_to<Base>(
                    mapper_(HPX_FORWARD(Args, args))...);
            }
        };

        template <typename M, template <typename...> typename Base,
            typename... OldArgs>
        struct tuple_like_remapper<strategy_traverse_tag, M, Base<OldArgs...>,
            // Support for skipping completely untouched types
            std::enable_if_t<is_effective_any_of_v<M, OldArgs...>>>
        {
            M mapper_;

            template <typename... Args>
            auto operator()(Args&&... args)
                -> std::void_t<hpx::util::invoke_result_t<M, OldArgs>...>
            {
                (mapper_(HPX_FORWARD(Args, args)), ...);
            }
        };

        /// Specialization for std::array like types, which contains a
        /// compile-time known amount of homogeneous types.
        template <typename M, template <typename, std::size_t> typename Base,
            typename OldArg, std::size_t Size>
        struct tuple_like_remapper<strategy_remap_tag, M, Base<OldArg, Size>,
            // Support for skipping completely untouched types
            std::enable_if_t<is_effective_v<M, OldArg>>>
        {
            M mapper_;

            template <typename... Args>
            auto operator()(Args&&... args)
                -> decltype(spreading::flat_arraylize_to<Base>(
                    mapper_(HPX_FORWARD(Args, args))...))
            {
                return spreading::flat_arraylize_to<Base>(
                    mapper_(HPX_FORWARD(Args, args))...);
            }
        };

        template <typename M, template <typename, std::size_t> typename Base,
            typename OldArg, std::size_t Size>
        struct tuple_like_remapper<strategy_traverse_tag, M, Base<OldArg, Size>,
            // Support for skipping completely untouched types
            std::enable_if_t<is_effective_v<M, OldArg>>>
        {
            M mapper_;

            template <typename... Args>
            auto operator()(Args&&... args) -> hpx::util::invoke_result_t<
                hpx::util::invoke_result_t<M, OldArg>>
            {
                (mapper_(HPX_FORWARD(Args, args)), ...);
            }
        };

        // clang-format off

        /// Remaps the content of the given tuple like type T, to a container of
        /// the same type which may contain different types.
        template <typename Strategy, typename T, typename M>
        auto remap(Strategy, T&& container, M&& mapper) -> decltype(
            hpx::invoke_fused(std::declval<tuple_like_remapper<Strategy,
                                  std::decay_t<M>, std::decay_t<T>>>(),
                 HPX_FORWARD(T, container)))
        {
            return hpx::invoke_fused(
                tuple_like_remapper<Strategy, std::decay_t<M>, std::decay_t<T>>{
                    HPX_FORWARD(M, mapper)},
                HPX_FORWARD(T, container));
        }
        // clang-format on
    }    // end namespace tuple_like_remapping

    /// Base class for making strategy dependent behavior available to the
    /// mapping_helper class.
    template <typename Strategy>
    struct mapping_strategy_base
    {
        template <typename T>
        decltype(auto) may_void(T&& element) const
        {
            return HPX_FORWARD(T, element);
        }
    };

    template <>
    struct mapping_strategy_base<strategy_traverse_tag>
    {
        template <typename T>
        constexpr void may_void(T&& /*element*/) const noexcept
        {
        }
    };

    /// A helper class which applies the mapping or routes the element through
    template <typename Strategy, typename M>
    class mapping_helper : protected mapping_strategy_base<Strategy>
    {
        M mapper_;

        class traversal_callable_base
        {
            mapping_helper* helper_;

        public:
            explicit constexpr traversal_callable_base(
                mapping_helper* helper) noexcept
              : helper_(helper)
            {
            }

        protected:
            constexpr mapping_helper* get_helper() noexcept
            {
                return helper_;
            }
        };

        /// A callable object which forwards its invocations to
        /// mapping_helper::traverse.
        class traversor : public traversal_callable_base
        {
        public:
            using traversal_callable_base::traversal_callable_base;

            /// SFINAE helper
            template <typename T>
            auto operator()(T&& element)
                -> decltype(std::declval<traversor>().get_helper()->traverse(
                    Strategy{}, HPX_FORWARD(T, element)));

            /// An alias to this type
            using traversor_type = traversor;
        };

        /// A callable object which forwards its invocations to
        /// mapping_helper::try_traverse.
        ///
        /// This callable object will accept any input, since elements passed to
        /// it are passed through, if the provided mapper doesn't accept it.
        class try_traversor : public traversal_callable_base
        {
        public:
            using traversal_callable_base::traversal_callable_base;

            // clang-format off
            template <typename T>
            auto operator()(T&& element) -> decltype(
                std::declval<try_traversor>().get_helper()->try_traverse(
                    Strategy{}, HPX_FORWARD(T, element)))
            {
                return this->get_helper()->try_traverse(
                    Strategy{}, HPX_FORWARD(T, element));
            }
            // clang-format on

            /// An alias to the traversor type
            using traversor_type = traversor;
        };

        // clang-format off

        /// Invokes the real mapper with the given element
        template <typename T>
        auto invoke_mapper(T&& element) -> decltype(
            std::declval<mapping_helper>().mapper_(HPX_FORWARD(T, element)))
        {
            return mapper_(HPX_FORWARD(T, element));
        }
        // clang-format on

        /// SFINAE helper for plain elements not satisfying the tuple like or
        /// container requirements.
        ///
        /// We use the proxy function invoke_mapper here, because some compilers
        /// (MSVC) tend to instantiate the invocation before matching the tag,
        /// which leads to build failures.
        template <typename T>
        auto match(container_category_tag<false, false>, T&& element)
            -> decltype(std::declval<mapping_helper>().invoke_mapper(
                HPX_FORWARD(T, element)));

        /// SFINAE helper for elements satisfying the container requirements,
        /// which are not tuple like.
        template <typename T>
        auto match(container_category_tag<true, false>, T&& container)
            -> decltype(container_remapping::remap(Strategy{},
                HPX_FORWARD(T, container), std::declval<traversor>()));

        /// SFINAE helper for elements which are tuple like and that also may
        /// satisfy the container requirements
        template <bool IsContainer, typename T>
        auto match(container_category_tag<IsContainer, true>, T&& tuple_like)
            -> decltype(tuple_like_remapping::remap(Strategy{},
                HPX_FORWARD(T, tuple_like), std::declval<traversor>()));

        // clang-format off

        /// This method implements the functionality for routing elements
        /// through, that aren't accepted by the mapper. Since the real matcher
        /// methods below are failing through SFINAE, the compiler will try to
        /// specialize this function last, since it's the least concrete one.
        /// This works recursively, so we only call the mapper with the minimal
        /// needed set of accepted arguments.
        template <typename MatcherTag, typename T>
        auto try_match(MatcherTag, T&& element) -> decltype(
            std::declval<mapping_helper>().may_void(HPX_FORWARD(T, element)))
        {
            return this->may_void(HPX_FORWARD(T, element));
        }
        // clang-format on

        /// Match plain elements not satisfying the tuple like or container
        /// requirements.
        ///
        /// We use the proxy function invoke_mapper here, because some compilers
        /// (MSVC) tend to instantiate the invocation before matching the tag,
        /// which leads to build failures.
        template <typename T>
        auto try_match(container_category_tag<false, false>, T&& element)
            -> decltype(std::declval<mapping_helper>().invoke_mapper(
                HPX_FORWARD(T, element)))
        {
            // T could be any non container or non tuple like type here, take
            // int or hpx::future<int> as an example.
            return invoke_mapper(HPX_FORWARD(T, element));
        }

        /// Match elements satisfying the container requirements, which are not
        /// tuple like.
        template <typename T>
        auto try_match(container_category_tag<true, false>, T&& container)
            -> decltype(container_remapping::remap(Strategy{},
                HPX_FORWARD(T, container), std::declval<try_traversor>()))
        {
            return container_remapping::remap(
                Strategy{}, HPX_FORWARD(T, container), try_traversor{this});
        }

        /// Match elements which are tuple like and that also may satisfy the
        /// container requirements -> We match tuple like types over container
        /// like ones
        template <bool IsContainer, typename T>
        auto try_match(container_category_tag<IsContainer, true>,
            T&& tuple_like) -> decltype(tuple_like_remapping::remap(Strategy{},
            HPX_FORWARD(T, tuple_like), std::declval<try_traversor>()))
        {
            return tuple_like_remapping::remap(
                Strategy{}, HPX_FORWARD(T, tuple_like), try_traversor{this});
        }

        /// Traverses a single element.
        ///
        /// SFINAE helper: Doesn't allow routing through elements, that aren't
        /// accepted by the mapper
        template <typename T>
        auto traverse(Strategy, T&& element)
            -> decltype(std::declval<mapping_helper>().match(
                std::declval<
                    container_category_of_t<hpx::util::decay_unwrap_t<T>>>(),
                std::declval<T>()));

        /// \copybrief traverse
        template <typename T>
        auto try_traverse(Strategy, T&& element)
            -> decltype(std::declval<mapping_helper>().try_match(
                std::declval<
                    container_category_of_t<hpx::util::decay_unwrap_t<T>>>(),
                std::declval<T>()))
        {
            // We use tag dispatching here, to categorize the type T whether
            // it satisfies the container or tuple like requirements.
            // Then we can choose the underlying implementation accordingly.
            return try_match(
                container_category_of_t<hpx::util::decay_unwrap_t<T>>{},
                HPX_FORWARD(T, element));
        }

    public:
        explicit mapping_helper(M mapper) noexcept
          : mapper_(HPX_MOVE(mapper))
        {
        }

        /// \copybrief try_traverse
        template <typename Tag>
        constexpr void init_traverse(Tag) const noexcept
        {
        }

        template <typename T>
        auto init_traverse(strategy_remap_tag, T&& element)
            -> decltype(spreading::unpack_or_void(
                std::declval<mapping_helper>().try_traverse(
                    strategy_remap_tag{}, std::declval<T>())))
        {
            return spreading::unpack_or_void(
                try_traverse(strategy_remap_tag{}, HPX_FORWARD(T, element)));
        }

        template <typename T>
        void init_traverse(strategy_traverse_tag, T&& element)
        {
            try_traverse(strategy_traverse_tag{}, HPX_FORWARD(T, element));
        }

        /// Calls the traversal method for every element in the pack, and
        /// returns a tuple containing the remapped content.
        template <typename First, typename Second, typename... T>
        auto init_traverse(strategy_remap_tag strategy, First&& first,
            Second&& second, T&&... rest)
            -> decltype(spreading::tupelize_or_void(
                std::declval<mapping_helper>().try_traverse(
                    strategy, HPX_FORWARD(First, first)),
                std::declval<mapping_helper>().try_traverse(
                    strategy, HPX_FORWARD(Second, second)),
                std::declval<mapping_helper>().try_traverse(
                    strategy, HPX_FORWARD(T, rest))...))
        {
            return spreading::tupelize_or_void(
                try_traverse(strategy, HPX_FORWARD(First, first)),
                try_traverse(strategy, HPX_FORWARD(Second, second)),
                try_traverse(strategy, HPX_FORWARD(T, rest))...);
        }

        /// Calls the traversal method for every element in the pack, without
        /// preserving the return values of the mapper.
        template <typename First, typename Second, typename... T>
        void init_traverse(strategy_traverse_tag strategy, First&& first,
            Second&& second, T&&... rest)
        {
            try_traverse(strategy, HPX_FORWARD(First, first));
            try_traverse(strategy, HPX_FORWARD(Second, second));
            (try_traverse(strategy, HPX_FORWARD(T, rest)), ...);
        }
    };

    // clang-format off

    /// Traverses the given pack with the given mapper and strategy
    template <typename Strategy, typename Mapper, typename... T>
    auto apply_pack_transform(Strategy strategy, Mapper&& mapper, T&&... pack)
        -> decltype(
            std::declval<mapping_helper<Strategy, std::decay_t<Mapper>>>()
                .init_traverse(strategy, HPX_FORWARD(T, pack)...))
    {
        mapping_helper<Strategy, std::decay_t<Mapper>> helper(
            HPX_FORWARD(Mapper, mapper));
        return helper.init_traverse(strategy, HPX_FORWARD(T, pack)...);
    }
    // clang-format on
}    // namespace hpx::util::detail
