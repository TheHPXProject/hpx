//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2014-2015 Anton Bikineev
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/serialization/config/defines.hpp>
#include <hpx/serialization/brace_initializable_fwd.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/traits/brace_initializable_traits.hpp>
#include <hpx/serialization/traits/is_bitwise_serializable.hpp>
#include <hpx/serialization/traits/is_not_bitwise_serializable.hpp>
#include <hpx/serialization/traits/is_serializable.hpp>
#include <hpx/serialization/traits/polymorphic_traits.hpp>

#include <string>
#include <type_traits>
#include <utility>

namespace hpx::serialization {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        HPX_FORCEINLINE void serialize_force_adl(
            output_archive& ar, T const& t, unsigned)
        {
            serialize(ar, const_cast<T&>(t), 0);
        }

        HPX_CXX_CORE_EXPORT template <class T>
        HPX_FORCEINLINE void serialize_force_adl(
            input_archive& ar, T& t, unsigned)
        {
            serialize(ar, t, 0);
        }

#if defined(HPX_HAVE_CXX26_REFLECTION)
        // Forward declare
        HPX_CXX_CORE_EXPORT template <typename Archive, typename T>
        void refl_serialize(Archive& ar, T& t, unsigned /*version*/);
#endif
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class access
    {
    public:
        template <typename T>
        class has_serialize
        {
            template <typename T1>
            static std::false_type test(...);

            // the following expression sfinae trick appears to work on
            // clang-3.4, gcc-4.9, icc-16, msvc-2017 (at least) note that this
            // detection would have been much easier to implement if there
            // hadn't been an issue with gcc:
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82478
            // clang-format off
            template <typename T1,
                typename =
                    decltype(std::declval<std::remove_const_t<T1>&>().serialize(
                        std::declval<hpx::serialization::output_archive&>(),
                        0u))>
            static std::true_type test(int);
            // clang-format on

        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };

        template <typename T>
        static constexpr bool has_serialize_v = has_serialize<T>::value;

    public:
        template <typename Archive, typename T>
        static void serialize(Archive& ar, T& t, unsigned)
        {
            using dT = std::decay_t<T>;
            if constexpr (hpx::traits::is_intrusive_polymorphic_v<dT>)
            {
                // intrusive_polymorphic: the following template function is
                // viable to call the right overloaded function according to T
                // constness and to prevent calling templated version of
                // serialize function
                t.serialize(ar, 0);
            }
            else if constexpr (has_serialize_v<dT>)
            {
                // intrusive_usual: cast it to let it be run for templated
                // member functions
                const_cast<dT&>(t).serialize(ar, 0);
            }
            else if constexpr (!std::is_empty_v<dT>)
            {
                // non_intrusive
                if constexpr (hpx::traits::has_serialize_adl_v<dT>)
                {
                    // this additional indirection level is needed to force ADL
                    // on the second phase of template lookup. call of serialize
                    // function directly from base_object finds only
                    // serialize-member function and doesn't perform ADL
                    detail::serialize_force_adl(ar, t, 0);
                }
                else if constexpr (hpx::traits::is_bitwise_serializable_v<dT> ||
                    !hpx::traits::is_not_bitwise_serializable_v<dT>)
                {
                    // bitwise serializable types can be directly dispatched to
                    // the archive functions
                    ar.invoke(t);
                }

#if !defined(HPX_HAVE_CXX26_REFLECTION)
                else if constexpr (hpx::traits::has_struct_serialization_v<dT>)
                {
                    // This is automatic serialization for types that are simple
                    // (brace-initializable) structs, that means every struct's
                    // field has to be serializable and public.
                    serialize_struct(ar, t, 0);
                }
#else
                else if constexpr (    // TODO: I think this is too wide a filter
                    std::is_class_v<dT>)
                {
                    detail::refl_serialize(ar, t, 0);
                }
#endif
                else
                {
                    static_assert(hpx::traits::has_serialize_adl_v<dT> ||
                            hpx::traits::has_struct_serialization_v<dT> ||
                            hpx::traits::is_bitwise_serializable_v<dT> ||
                            !hpx::traits::is_not_bitwise_serializable_v<dT>,
                        "No serialization method found");
                }
            }
        }

        template <typename Archive, typename T>
        HPX_FORCEINLINE static void save_base_object(
            Archive& ar, T const& t, unsigned)
        {
            // explicitly specify virtual function of base class to avoid
            // infinite recursion
            t.T::save(ar, 0);
        }

        template <typename Archive, typename T>
        HPX_FORCEINLINE static void load_base_object(
            Archive& ar, T& t, unsigned)
        {
            // explicitly specify virtual function of base class to avoid
            // infinite recursion
            t.T::load(ar, 0);
        }

        template <typename T>
        [[nodiscard]] HPX_FORCEINLINE static std::string get_name(T const* t)
        {
            return t->hpx_serialization_get_name();
        }
    };
}    // namespace hpx::serialization

#if defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
namespace hpx::traits {

    // the case when hpx::serialization::access::has_serialize_v<T> is true has
    // to be handled separately to avoid circular dependencies
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_not_bitwise_serializable<T,
        std::enable_if_t<!std::is_abstract_v<T> &&
            !hpx::traits::has_serialize_adl_v<T> &&
            hpx::serialization::access::has_serialize_v<T>>> : std::true_type
    {
    };
}    // namespace hpx::traits
#endif

#if defined(HPX_HAVE_CXX26_REFLECTION)
// We need to include refl_serialize_impl.hpp here to avoid circular
// dependencies as refl_serialize_impl.hpp depends on base_object.hpp
#include <hpx/serialization/detail/refl_serialize_impl.hpp>
#endif

// To avoid circular dependencies, we include polymorphic_nonintrusive_factory_impl.hpp
#include <hpx/serialization/detail/polymorphic_nonintrusive_factory_impl.hpp>
