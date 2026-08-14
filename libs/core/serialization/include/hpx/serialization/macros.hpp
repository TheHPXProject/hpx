//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/config/defines.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <string>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
// from file: hpx/serialization/serialization_fwd.hpp
#define HPX_SERIALIZATION_SPLIT_MEMBER()                                       \
    void serialize(hpx::serialization::input_archive& ar, unsigned)            \
    {                                                                          \
        load(ar, 0);                                                           \
    }                                                                          \
    void serialize(hpx::serialization::output_archive& ar, unsigned) const     \
    {                                                                          \
        save(ar, 0);                                                           \
    }                                                                          \
    /**/

#define HPX_SERIALIZATION_SPLIT_FREE(...)                                      \
    HPX_SERIALIZATION_SPLIT_FREE_(__VA_ARGS__)                                 \
    /**/
#define HPX_SERIALIZATION_SPLIT_FREE_(...)                                     \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_SPLIT_FREE_,                    \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/
#define HPX_SERIALIZATION_SPLIT_FREE_1(T)                                      \
    HPX_SERIALIZATION_SPLIT_FREE_2(HPX_PP_EMPTY(), T)                          \
    /**/
#define HPX_SERIALIZATION_SPLIT_FREE_2(PREFIX, T)                              \
    PREFIX HPX_FORCEINLINE void serialize(                                     \
        hpx::serialization::input_archive& ar, T& t, unsigned)                 \
    {                                                                          \
        load(ar, t, 0);                                                        \
    }                                                                          \
    PREFIX HPX_FORCEINLINE void serialize(                                     \
        hpx::serialization::output_archive& ar, T& t, unsigned)                \
    {                                                                          \
        save(ar, const_cast<std::add_const_t<T>&>(t), 0);                      \
    }                                                                          \
    /**/

#define HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE(...)                             \
    HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE_(__VA_ARGS__)                        \
    /**/
#define HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE_(...)                            \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE_,           \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/
#define HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE_2(TEMPLATE, ARGS)                \
    HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE_3(HPX_PP_EMPTY(), TEMPLATE, ARGS)    \
    /**/
#define HPX_SERIALIZATION_SPLIT_FREE_TEMPLATE_3(PREFIX, TEMPLATE, ARGS)        \
    PREFIX HPX_PP_STRIP_PARENS(TEMPLATE)                                       \
    HPX_FORCEINLINE void serialize(hpx::serialization::input_archive& ar,      \
        HPX_PP_STRIP_PARENS(ARGS) & t, unsigned)                               \
    {                                                                          \
        load(ar, t, 0);                                                        \
    }                                                                          \
    PREFIX HPX_PP_STRIP_PARENS(TEMPLATE)                                       \
    HPX_FORCEINLINE void serialize(hpx::serialization::output_archive& ar,     \
        HPX_PP_STRIP_PARENS(ARGS) & t, unsigned)                               \
    {                                                                          \
        save(ar, const_cast<std::add_const_t<HPX_PP_STRIP_PARENS(ARGS)>&>(t),  \
            0);                                                                \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from file: hpx/serialization/detail/polymorphic_id_factory.hpp
#define HPX_SERIALIZATION_ADD_CONSTANT_ENTRY(String, Id)                       \
    namespace hpx::serialization::detail {                                     \
        template <>                                                            \
        HPX_ALWAYS_EXPORT std::string get_constant_entry_name</**/ Id>()       \
        {                                                                      \
            [[maybe_unused]] auto& _ = add_constant_entry<                     \
                /**/ Id>::instance(); /* force instantiation */                \
            return HPX_PP_STRINGIZE(String);                                   \
        }                                                                      \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from file: hpx/serialization/detail/polymorphic_intrusive_factory.hpp
#define HPX_SERIALIZATION_ADD_GET_NAME_MEMBERS_WITH_NAME(                      \
    Class, Name, Override)                                                     \
    template <typename, typename>                                              \
    friend struct ::hpx::serialization::detail::register_class_name;           \
                                                                               \
    static std::string hpx_serialization_get_name_impl()                       \
    {                                                                          \
        return Name;                                                           \
    }                                                                          \
    virtual std::string hpx_serialization_get_name() const Override            \
    {                                                                          \
        [[maybe_unused]] auto& _ =                                             \
            hpx::serialization::detail::register_class_name<                   \
                Class>::instance(); /* force instantiation */                  \
        return Class::hpx_serialization_get_name_impl();                       \
    }                                                                          \
    /**/

#define HPX_SERIALIZATION_ADD_INTRUSIVE_MEMBERS(Class, Override)               \
    virtual void load(hpx::serialization::input_archive& ar, unsigned n)       \
        Override                                                               \
    {                                                                          \
        serialize<hpx::serialization::input_archive>(ar, n);                   \
    }                                                                          \
    virtual void save(hpx::serialization::output_archive& ar, unsigned n)      \
        const Override                                                         \
    {                                                                          \
        const_cast<Class*>(this)                                               \
            ->serialize<hpx::serialization::output_archive>(ar, n);            \
    }

#define HPX_SERIALIZATION_ADD_INTRUSIVE_MEMBERS_SPLITTED(Override)             \
    virtual void load(hpx::serialization::input_archive& ar, unsigned n)       \
        Override                                                               \
    {                                                                          \
        load<hpx::serialization::input_archive>(ar, n);                        \
    }                                                                          \
    virtual void save(hpx::serialization::output_archive& ar, unsigned n)      \
        const Override                                                         \
    {                                                                          \
        save<hpx::serialization::output_archive>(ar, n);                       \
    }                                                                          \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED(...)                  \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_(__VA_ARGS__)             \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_(...)                 \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_,          \
            HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                           \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_3(                    \
    Class, Name, Override)                                                     \
    HPX_SERIALIZATION_ADD_GET_NAME_MEMBERS_WITH_NAME(Class, Name, Override);   \
    HPX_SERIALIZATION_ADD_INTRUSIVE_MEMBERS_SPLITTED(Override)                 \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_2(Class, Name)        \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_3(                        \
        Class, Name, HPX_PP_EMPTY())                                           \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME(...)                           \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_(__VA_ARGS__)                      \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_(...)                          \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_,         \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_3(Class, Name, Override)       \
    HPX_SERIALIZATION_ADD_GET_NAME_MEMBERS_WITH_NAME(Class, Name, Override);   \
    HPX_SERIALIZATION_ADD_INTRUSIVE_MEMBERS(Class, Override)                   \
    HPX_SERIALIZATION_SPLIT_MEMBER()                                           \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_2(Class, Name)                 \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_3(Class, Name, HPX_PP_EMPTY())     \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_ABSTRACT(Class)                          \
    virtual std::string hpx_serialization_get_name() const = 0;                \
    HPX_SERIALIZATION_ADD_INTRUSIVE_MEMBERS(Class, HPX_PP_EMPTY())             \
    HPX_SERIALIZATION_SPLIT_MEMBER()                                           \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_ABSTRACT_SPLITTED(Class)                 \
    virtual std::string hpx_serialization_get_name() const = 0;                \
    HPX_SERIALIZATION_ADD_INTRUSIVE_MEMBERS_SPLITTED(HPX_PP_EMPTY())           \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC(...)                                     \
    HPX_SERIALIZATION_POLYMORPHIC_(__VA_ARGS__)                                \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_(...)                                    \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_POLYMORPHIC_,                   \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_2(Class, Override)                       \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_3(                                 \
        Class, HPX_PP_STRINGIZE(Class), Override)                              \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_1(Class)                                 \
    HPX_SERIALIZATION_POLYMORPHIC_2(Class, HPX_PP_EMPTY())                     \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_SPLITTED(...)                            \
    HPX_SERIALIZATION_POLYMORPHIC_SPLITTED_(__VA_ARGS__)                       \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_SPLITTED_(...)                           \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_POLYMORPHIC_SPLITTED_,          \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_SPLITTED_2(Class, Override)              \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_3(                        \
        Class, HPX_PP_STRINGIZE(Class), Override)                              \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_SPLITTED_1(Class)                        \
    HPX_SERIALIZATION_POLYMORPHIC_SPLITTED_2(Class, HPX_PP_EMPTY())            \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE(...)                            \
    HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_(__VA_ARGS__)                       \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_(...)                           \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_,          \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_2(Class, Override)              \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_3(                                 \
        Class, hpx::util::debug::type_id<Class>(), Override)                   \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_1(Class)                        \
    HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_2(Class, HPX_PP_EMPTY())            \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SPLITTED(...)                   \
    HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SPLITTED_(__VA_ARGS__)              \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SPLITTED_(...)                  \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SPLITTED_, \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SPLITTED_2(Class, Override)     \
    HPX_SERIALIZATION_POLYMORPHIC_WITH_NAME_SPLITTED_3(                        \
        Class, hpx::util::debug::type_id<T>(), Override)                       \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SPLITTED_1(Class)               \
    HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SPLITTED_2(Class, HPX_PP_EMPTY())   \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from file: hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp
#define HPX_SERIALIZATION_REGISTER_CLASS_DECLARATION(Class)                    \
    namespace hpx::serialization::detail {                                     \
        template <>                                                            \
        struct HPX_ALWAYS_EXPORT get_serialization_name</**/ Class>;           \
    }                                                                          \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct needs_automatic_registration</**/ action> : std::false_type     \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC(Class)                                 \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_NAME(Class, Name)                     \
    namespace hpx::serialization::detail {                                     \
        inline struct HPX_PP_CAT(register_class_helper_, Class)                \
        {                                                                      \
            HPX_PP_CAT(register_class_helper_, Class)()                        \
            {                                                                  \
                [[maybe_unused]] auto& _ =                                     \
                    hpx::serialization::detail::register_class<                \
                        /**/ Class>::instance(); /* force instantiation */     \
            }                                                                  \
        } HPX_PP_CAT(register_class_helper_instance_, Class);                  \
        template <>                                                            \
        struct HPX_ALWAYS_EXPORT get_serialization_name</**/ Class>            \
        {                                                                      \
            char const* operator()() const                                     \
            {                                                                  \
                return Name;                                                   \
            }                                                                  \
        };                                                                     \
        inline hpx::serialization::detail::register_class</**/ Class>          \
            HPX_PP_CAT(hpx_register_class_instance_, __LINE__){};              \
    }                                                                          \
    /**/

#define HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE(...)                    \
    HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE_(__VA_ARGS__)               \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE_(...)                   \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE_,  \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE_3(                      \
    Parameters, Template, Name)                                                \
    HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE_4(                          \
        HPX_PP_EMPTY(), Parameters, Template, Name)                            \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE_4(                      \
    Prefix, Parameters, Template, Name)                                        \
    namespace hpx::serialization::detail {                                     \
        Prefix HPX_PP_STRIP_PARENS(Parameters) struct HPX_ALWAYS_EXPORT        \
            get_serialization_name<HPX_PP_STRIP_PARENS(Template)>              \
        {                                                                      \
            constexpr char const* operator()() const noexcept                  \
            {                                                                  \
                return Name;                                                   \
            }                                                                  \
        };                                                                     \
    }                                                                          \
    /**/

#define HPX_SERIALIZATION_REGISTER_CLASS(...)                                  \
    HPX_SERIALIZATION_REGISTER_CLASS_(__VA_ARGS__)                             \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_(...)                                 \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_REGISTER_CLASS_,                \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_1(Class)                              \
    HPX_SERIALIZATION_REGISTER_CLASS_2(HPX_PP_EMPTY(), Class)                  \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_2(Prefix, Class)                      \
    HPX_SERIALIZATION_REGISTER_CLASS_NAME(Class, HPX_PP_STRINGIZE(Class))      \
    /**/

#define HPX_SERIALIZATION_REGISTER_CLASS_TEMPLATE(...)                         \
    HPX_SERIALIZATION_REGISTER_CLASS_TEMPLATE_(__VA_ARGS__)                    \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_TEMPLATE_(...)                        \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_SERIALIZATION_REGISTER_CLASS_TEMPLATE_,       \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_TEMPLATE_2(Parameters, Template)      \
    HPX_SERIALIZATION_REGISTER_CLASS_TEMPLATE_3(                               \
        HPX_PP_EMPTY(), Parameters, Template)                                  \
    /**/
#define HPX_SERIALIZATION_REGISTER_CLASS_TEMPLATE_3(                           \
    Prefix, Parameters, Template)                                              \
    HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE(Prefix, Parameters,         \
        Template,                                                              \
        (hpx::util::debug::type_id<HPX_PP_STRIP_PARENS(Template)>()))          \
    HPX_PP_STRIP_PARENS(Parameters)                                            \
    hpx::serialization::detail::register_class<HPX_PP_STRIP_PARENS(Template)>  \
        HPX_PP_STRIP_PARENS(Template)::hpx_register_class_instance{};          \
    /**/

#define HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SEMIINTRUSIVE(Template)         \
    static hpx::serialization::detail::register_class<Template>                \
        hpx_register_class_instance;                                           \
                                                                               \
    virtual hpx::serialization::detail::register_class</**/ Template>&         \
    hpx_get_register_class_instance(                                           \
        hpx::serialization::detail::register_class</**/ Template>*) const      \
    {                                                                          \
        return hpx_register_class_instance;                                    \
    }                                                                          \
    /**/
#define HPX_SERIALIZATION_WITH_CUSTOM_CONSTRUCTOR(Class, Func)                 \
    namespace hpx::serialization::detail {                                     \
        template <>                                                            \
        class constructor_selector_ptr<HPX_PP_STRIP_PARENS(Class)>             \
        {                                                                      \
        public:                                                                \
            static Class* create(input_archive& ar)                            \
            {                                                                  \
                return Func(ar);                                               \
            }                                                                  \
        };                                                                     \
    }                                                                          \
    /**/
#define HPX_SERIALIZATION_WITH_CUSTOM_CONSTRUCTOR_TEMPLATE(                    \
    Parameters, Template, Func)                                                \
    namespace hpx::serialization::detail {                                     \
        HPX_PP_STRIP_PARENS(Parameters)                                        \
        class constructor_selector_ptr<HPX_PP_STRIP_PARENS(Template)>          \
        {                                                                      \
        public:                                                                \
            static HPX_PP_STRIP_PARENS(Template) * create(input_archive& ar)   \
            {                                                                  \
                return Func(                                                   \
                    ar, static_cast<HPX_PP_STRIP_PARENS(Template)*>(0));       \
            }                                                                  \
        };                                                                     \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from file: hpx/serialization/traits/is_bitwise_serializable.hpp
#define HPX_IS_BITWISE_SERIALIZABLE(T)                                         \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct is_bitwise_serializable<T> : std::true_type                     \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from file: hpx/serialization/traits/is_not_bitwise_serializable.hpp
#define HPX_IS_NOT_BITWISE_SERIALIZABLE(T)                                     \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct is_not_bitwise_serializable<T> : std::true_type                 \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

///////////////////////////////////////////////////////////////////////////////
// from file: hpx/serialization/traits/polymorphic_traits.hpp
#define HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC(Class)                             \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct is_nonintrusive_polymorphic<Class> : std::true_type             \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

#define HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC_TEMPLATE(TEMPLATE, ARG_LIST)       \
    namespace hpx::traits {                                                    \
        HPX_PP_STRIP_PARENS(TEMPLATE)                                          \
        struct is_nonintrusive_polymorphic<HPX_PP_STRIP_PARENS(ARG_LIST)>      \
          : std::true_type                                                     \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

#define HPX_TRAITS_SERIALIZED_WITH_ID(Class)                                   \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct is_serialized_with_id<Class> : std::true_type                   \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

#define HPX_TRAITS_SERIALIZED_WITH_ID_TEMPLATE(TEMPLATE, ARG_LIST)             \
    namespace hpx::traits {                                                    \
        HPX_PP_STRIP_PARENS(TEMPLATE)                                          \
        struct is_serialized_with_id<HPX_PP_STRIP_PARENS(ARG_LIST)>            \
          : std::true_type                                                     \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

#if defined(HPX_HAVE_CXX26_REFLECTION)
#define HPX_POLYMORPHIC_AUTO_REGISTER(Class)                                   \
    namespace hpx::serialization::detail {                                     \
        inline struct HPX_PP_CAT(register_class_helper_, __LINE__)             \
        {                                                                      \
            HPX_PP_CAT(register_class_helper_, __LINE__)()                     \
            {                                                                  \
                [[maybe_unused]] static auto& _ =                              \
                    hpx::serialization::detail::register_class<                \
                        /**/ Class>::instance(); /* force instantiation */     \
            }                                                                  \
        } HPX_PP_CAT(register_class_helper_instance_, __LINE__);               \
                                                                               \
        inline hpx::serialization::detail::register_class</**/ Class>          \
            HPX_PP_CAT(hpx_register_class_instance_, __LINE__){};              \
    }                                                                          \
                                                                               \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct is_nonintrusive_polymorphic<Class> : std::true_type             \
        {                                                                      \
        };                                                                     \
    }
/**/
#endif
