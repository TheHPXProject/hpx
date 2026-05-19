//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <string>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
// from hpx/actions_base/detail/invocation_count_registry.hpp

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS) &&                            \
    defined(HPX_HAVE_NETWORKING)

#define HPX_REGISTER_ACTION_INVOCATION_COUNT(Action)                           \
    namespace hpx::actions::detail {                                           \
        template register_action_invocation_count<Action>                      \
            register_action_invocation_count<Action>::instance;                \
    }                                                                          \
    /**/

#define HPX_REGISTER_PER_ACTION_DATA_COUNTER_TYPES(Action)                     \
    namespace hpx::actions::detail {                                           \
        template register_per_action_data_counters<Action>                     \
            register_per_action_data_counters<Action>::instance;               \
    }                                                                          \
    /**/
#else

#define HPX_REGISTER_ACTION_INVOCATION_COUNT(Action)
#define HPX_REGISTER_PER_ACTION_DATA_COUNTER_TYPES(Action)

#endif

///////////////////////////////////////////////////////////////////////////////
// from hpx/actions_base/basic_action.hpp

// Macros usable to refer to an action given the function to expose
#define HPX_MAKE_ACTION(func)                                                  \
    hpx::actions::make_action<decltype(&func), &func> /**/ /**/
#define HPX_MAKE_DIRECT_ACTION(func)                                           \
    hpx::actions::make_direct_action<decltype(&func), &func> /**/ /**/

///////////////////////////////////////////////////////////////////////////////
/// \def HPX_DECLARE_ACTION(func, name)
/// \brief Declares an action type
///
#define HPX_DECLARE_ACTION(...)                                                \
    HPX_DECLARE_ACTION_(__VA_ARGS__)                                           \
    /**/

/// \cond NOINTERNAL

#define HPX_DECLARE_DIRECT_ACTION(...)                                         \
    HPX_DECLARE_ACTION(__VA_ARGS__)                                            \
    /**/

#define HPX_DECLARE_ACTION_(...)                                               \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_DECLARE_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(  \
        __VA_ARGS__))                                                          \
    /**/

#define HPX_DECLARE_ACTION_1(func)                                             \
    HPX_DECLARE_ACTION_2(func, HPX_PP_CAT(func, _action))                      \
    /**/

#define HPX_DECLARE_ACTION_2(func, name)                                       \
    struct name;                                                               \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_ACTION_(...)                                              \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_ACTION_, HPX_PP_NARGS(__VA_ARGS__))( \
        __VA_ARGS__))                                                          \
/**/
#define HPX_REGISTER_ACTION_1(action)                                          \
    HPX_REGISTER_ACTION_2(action, action)                                      \
    /**/

#if !defined(HPX_HAVE_NETWORKING)

#define HPX_DEFINE_GET_ACTION_NAME(action) /**/
#if defined(HPX_HAVE_ITTNOTIFY) && HPX_HAVE_ITTNOTIFY != 0 &&                  \
    !defined(HPX_HAVE_APEX)
#define HPX_DEFINE_GET_ACTION_NAME_ITT(action, actionname) /**/
#endif
#define HPX_REGISTER_ACTION_EXTERN_DECLARATION(action) /**/

#define HPX_REGISTER_ACTION_2(action, actionname)                              \
    HPX_REGISTER_ACTION_INVOCATION_COUNT(action)                               \
    HPX_REGISTER_PER_ACTION_DATA_COUNTER_TYPES(action)                         \
    /**/

#define HPX_REGISTER_ACTION_DECLARATION_2(action, actionname) /**/

#endif

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_COMPUTE_DEVICE_CODE)
#define HPX_ACTION_USES_STACK(action, size)  /**/
#define HPX_ACTION_USES_SMALL_STACK(action)  /**/
#define HPX_ACTION_USES_MEDIUM_STACK(action) /**/
#define HPX_ACTION_USES_LARGE_STACK(action)  /**/
#define HPX_ACTION_USES_HUGE_STACK(action)   /**/
#else
#define HPX_ACTION_USES_STACK(action, size)                                    \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct action_stacksize<action>                                        \
        {                                                                      \
            static constexpr threads::thread_stacksize value = size;           \
        };                                                                     \
    }                                                                          \
    /**/

#define HPX_ACTION_USES_SMALL_STACK(action)                                    \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize::small_)           \
/**/
#define HPX_ACTION_USES_MEDIUM_STACK(action)                                   \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize::medium)           \
/**/
#define HPX_ACTION_USES_LARGE_STACK(action)                                    \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize::large)            \
/**/
#define HPX_ACTION_USES_HUGE_STACK(action)                                     \
    HPX_ACTION_USES_STACK(action, threads::thread_stacksize::huge)             \
/**/
#endif

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_COMPUTE_DEVICE_CODE)
#define HPX_ACTION_HAS_PRIORITY(action, priority)      /**/
#define HPX_ACTION_HAS_LOW_PRIORITY(action)            /**/
#define HPX_ACTION_HAS_NORMAL_PRIORITY(action)         /**/
#define HPX_ACTION_HAS_HIGH_PRIORITY(action)           /**/
#define HPX_ACTION_HAS_HIGH_RECURSIVE_PRIORITY(action) /**/
// obsolete, kept for compatibility
#define HPX_ACTION_HAS_CRITICAL_PRIORITY(action) /**/
#else
///////////////////////////////////////////////////////////////////////////////
#define HPX_ACTION_HAS_PRIORITY(action, priority)                              \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct action_priority<action>                                         \
        {                                                                      \
            static constexpr threads::thread_priority value = priority;        \
        };                                                                     \
        /* make sure the action is not executed directly */                    \
        template <>                                                            \
        struct has_decorates_action<action> : std::true_type                   \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

#define HPX_ACTION_HAS_LOW_PRIORITY(action)                                    \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority::low)             \
/**/
#define HPX_ACTION_HAS_NORMAL_PRIORITY(action)                                 \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority::normal)          \
/**/
#define HPX_ACTION_HAS_HIGH_PRIORITY(action)                                   \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority::high)            \
/**/
#define HPX_ACTION_HAS_HIGH_RECURSIVE_PRIORITY(action)                         \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority::high_recursive)  \
/**/

// obsolete, kept for compatibility
#define HPX_ACTION_HAS_CRITICAL_PRIORITY(action)                               \
    HPX_ACTION_HAS_PRIORITY(action, threads::thread_priority::high_recursive)  \
/**/
#endif

/// \endcond

/// \def HPX_REGISTER_ACTION_DECLARATION(action)
///
/// \brief Declare the necessary component action boilerplate code.
///
/// The macro \a HPX_REGISTER_ACTION_DECLARATION can be used to declare all the
/// boilerplate code which is required for proper functioning of component
/// actions in the context of HPX.
///
/// The parameter \a action is the type of the action to declare the
/// boilerplate for.
///
/// This macro can be invoked with an optional second parameter. This parameter
/// specifies a unique name of the action to be used for serialization purposes.
/// The second parameter has to be specified if the first parameter is not
/// usable as a plain (nonqualified) C++ identifier, i.e. the first parameter
/// contains special characters which cannot be part of a C++ identifier, such
/// as '<', '>', or ':'.
///
/// \par Example:
///
/// \code
///      namespace app
///      {
///          // Define a simple component exposing one action 'print_greeting'
///          class HPX_COMPONENT_EXPORT server
///            : public hpx::components::component_base<server>
///          {
///              void print_greeting ()
///              {
///                  hpx::cout << "Hey, how are you?\n" << std::flush;
///              }
///
///              // Component actions need to be declared, this also defines the
///              // type 'print_greeting_action' representing the action.
///              HPX_DEFINE_COMPONENT_ACTION(server,
///                  print_greeting, print_greeting_action);
///          };
///      }
///
///      // Declare boilerplate code required for each of the component actions.
///      HPX_REGISTER_ACTION_DECLARATION(app::server::print_greeting_action)
/// \endcode
///
/// \note This macro has to be used once for each of the component actions
/// defined using one of the \a HPX_DEFINE_COMPONENT_ACTION macros. It has to
/// be visible in all translation units using the action, thus it is
/// recommended to place it into the header file defining the component.
#if defined(HPX_COMPUTE_DEVICE_CODE)
#define HPX_REGISTER_ACTION_DECLARATION(...) /**/
#else
#define HPX_REGISTER_ACTION_DECLARATION(...)                                   \
    HPX_REGISTER_ACTION_DECLARATION_(__VA_ARGS__)                              \
    /**/

#define HPX_REGISTER_ACTION_DECLARATION_(...)                                  \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_ACTION_DECLARATION_,                 \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
/**/
#define HPX_REGISTER_ACTION_DECLARATION_1(action)                              \
    HPX_REGISTER_ACTION_DECLARATION_2(action, action)                          \
/**/
#endif

/// \def HPX_REGISTER_ACTION(action)
///
/// \brief Define the necessary component action boilerplate code.
///
/// The macro \a HPX_REGISTER_ACTION can be used to define all the
/// boilerplate code which is required for proper functioning of component
/// actions in the context of HPX.
///
/// The parameter \a action is the type of the action to define the
/// boilerplate for.
///
/// This macro can be invoked with an optional second parameter. This parameter
/// specifies a unique name of the action to be used for serialization purposes.
/// The second parameter has to be specified if the first parameter is not
/// usable as a plain (nonqualified) C++ identifier, i.e. the first parameter
/// contains special characters which cannot be part of a C++ identifier, such
/// as '<', '>', or ':'.
///
/// \note This macro has to be used once for each of the component actions
/// defined using one of the \a HPX_DEFINE_COMPONENT_ACTION or
/// \a HPX_DEFINE_PLAIN_ACTION macros. It has to occur exactly once for each of
/// the actions, thus it is recommended to place it into the source file defining
/// the component.
///
/// \note Only one of the forms of this macro \a HPX_REGISTER_ACTION or
///       \a HPX_REGISTER_ACTION_ID should be used for a particular action,
///       never both.
///
#if defined(HPX_COMPUTE_DEVICE_CODE)
#define HPX_REGISTER_ACTION(...) /**/
#else
#define HPX_REGISTER_ACTION(...)                                               \
    HPX_REGISTER_ACTION_(__VA_ARGS__)                                          \
/**/
#endif

/// \def HPX_REGISTER_ACTION_ID(action, actionname, actionid)
///
/// \brief Define the necessary component action boilerplate code and assign a
///        predefined unique id to the action.
///
/// The macro \a HPX_REGISTER_ACTION can be used to define all the
/// boilerplate code which is required for proper functioning of component
/// actions in the context of HPX.
///
/// The parameter \a action is the type of the action to define the
/// boilerplate for.
///
/// The parameter \a actionname specifies an unique name of the action to be
/// used for serialization purposes.
/// The second parameter has to be usable as a plain (nonqualified) C++
/// identifier, it should not contain special characters which cannot be part
/// of a C++ identifier, such as '<', '>', or ':'.
///
/// The parameter \a actionid specifies a unique integer value which will be
/// used to represent the action during serialization.
///
/// \note This macro has to be used once for each of the component actions
/// defined using one of the \a HPX_DEFINE_COMPONENT_ACTION or global actions
/// \a HPX_DEFINE_PLAIN_ACTION macros. It has to occur exactly once for each of
/// the actions, thus it is recommended to place it into the source file defining
/// the component.
///
/// \note Only one of the forms of this macro \a HPX_REGISTER_ACTION or
///       \a HPX_REGISTER_ACTION_ID should be used for a particular action,
///       never both.
///
#if defined(HPX_COMPUTE_DEVICE_CODE)
#define HPX_REGISTER_ACTION_ID(action, actionname, actionid) /**/
#else
#define HPX_REGISTER_ACTION_ID(action, actionname, actionid)                   \
    HPX_REGISTER_ACTION_2(action, actionname)                                  \
    HPX_REGISTER_ACTION_FACTORY_ID(actionname, actionid)                       \
    /**/

#endif

////////////////////////////////////////////////////////////////////////////////
// from hpx/actions_base/component_action.hpp

/// \def HPX_DEFINE_COMPONENT_ACTION(component, func, action_type)
///
/// \brief Registers a  member function of a component as an action type
/// with HPX
///
/// The macro \a HPX_DEFINE_COMPONENT_ACTION can be used to register a
/// member function of a component as an action type named \a action_type.
///
/// The parameter \a component is the type of the component exposing the
/// member function \a func which should be associated with the newly defined
/// action type. The parameter \p action_type is the name of the action type to
/// register with HPX.
///
/// \par Example:
///
/// \code
///       namespace app
///       {
///           // Define a simple component exposing one action 'print_greeting'
///           class HPX_COMPONENT_EXPORT server
///             : public hpx::components::component_base<server>
///           {
///               void print_greeting() const
///               {
///                   hpx::cout << "Hey, how are you?\n" << std::flush;
///               }
///
///               // Component actions need to be declared, this also defines the
///               // type 'print_greeting_action' representing the action.
///               HPX_DEFINE_COMPONENT_ACTION(server, print_greeting,
///                   print_greeting_action);
///           };
///       }
/// \endcode
///
/// The first argument must provide the type name of the component the
/// action is defined for.
///
/// The second argument must provide the member function name the action
/// should wrap.
///
/// \note The macro \a HPX_DEFINE_COMPONENT_ACTION can be used with 2 or
/// 3 arguments. The third argument is optional.
///
/// The default value for the third argument (the typename of the defined
/// action) is derived from the name of the function (as passed as the second
/// argument) by appending '_action'. The third argument can be omitted only
/// if the second argument with an appended suffix '_action' resolves to a valid,
/// unqualified C++ type name.
///
#if defined(HPX_COMPUTE_DEVICE_CODE)
#define HPX_DEFINE_COMPONENT_ACTION(...)                            /**/
#define HPX_DEFINE_COMPONENT_ACTION_3(component, func, name)        /**/
#define HPX_DEFINE_COMPONENT_ACTION_2(component, func)              /**/
#define HPX_DEFINE_COMPONENT_DIRECT_ACTION(...)                     /**/
#define HPX_DEFINE_COMPONENT_DIRECT_ACTION_3(component, func, name) /**/
#define HPX_DEFINE_COMPONENT_DIRECT_ACTION_2(component, func)       /**/
#else
#define HPX_DEFINE_COMPONENT_ACTION(...)                                       \
    HPX_DEFINE_COMPONENT_ACTION_(__VA_ARGS__)                                  \
    /**/

/// \cond NOINTERNAL
#define HPX_DEFINE_COMPONENT_ACTION_(...)                                      \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_DEFINE_COMPONENT_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__)) \
    /**/

#define HPX_DEFINE_COMPONENT_ACTION_3(component, func, name)                   \
    struct name                                                                \
      : hpx::actions::make_action_t<decltype(&component::func),                \
            &component::func, name>                                            \
    {                                                                          \
    }; /**/
#define HPX_DEFINE_COMPONENT_ACTION_2(component, func)                         \
    HPX_DEFINE_COMPONENT_ACTION_3(component, func, HPX_PP_CAT(func, _action))  \
    /**/
/// \endcond

/// \cond NOINTERNAL
#define HPX_DEFINE_COMPONENT_DIRECT_ACTION(...)                                \
    HPX_DEFINE_COMPONENT_DIRECT_ACTION_(__VA_ARGS__)                           \
    /**/

#define HPX_DEFINE_COMPONENT_DIRECT_ACTION_(...)                               \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_DEFINE_COMPONENT_DIRECT_ACTION_,              \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_DEFINE_COMPONENT_DIRECT_ACTION_3(component, func, name)            \
    struct name                                                                \
      : hpx::actions::make_direct_action_t<decltype(&component::func),         \
            &component::func, name>                                            \
    {                                                                          \
    };                                                                         \
    /**/

#define HPX_DEFINE_COMPONENT_DIRECT_ACTION_2(component, func)                  \
    HPX_DEFINE_COMPONENT_DIRECT_ACTION_3(                                      \
        component, func, HPX_PP_CAT(func, _action))                            \
    /**/
/// \endcond

#endif

////////////////////////////////////////////////////////////////////////////////
// from hpx/actions_base/plain_action.hpp

/// \def HPX_DEFINE_PLAIN_ACTION(func, name)
/// \brief Defines a plain action type
///
/// \par Example:
///
/// \code
///       namespace app
///       {
///           void some_global_function(double d)
///           {
///               cout << d;
///           }
///
///           // This will define the action type 'app::some_global_action' which
///           // represents the function 'app::some_global_function'.
///           HPX_DEFINE_PLAIN_ACTION(some_global_function, some_global_action);
///       }
/// \endcode
///
/// \note Usually this macro will not be used in user code unless the intent is
/// to avoid defining the action_type in global namespace. Normally, the use of
/// the macro \a HPX_PLAIN_ACTION is recommended.
///
/// \note The macro \a HPX_DEFINE_PLAIN_ACTION can be used with 1 or 2
/// arguments. The second argument is optional. The default value for the
/// second argument (the typename of the defined action) is derived from the
/// name of the function (as passed as the first argument) by appending '_action'.
/// The second argument can be omitted only if the first argument with an
/// appended suffix '_action' resolves to a valid, unqualified C++ type name.
///
#define HPX_DEFINE_PLAIN_ACTION(...)                                           \
    HPX_DEFINE_PLAIN_ACTION_(__VA_ARGS__)                                      \
    /**/

/// \cond NOINTERNAL

#define HPX_DEFINE_PLAIN_DIRECT_ACTION(...)                                    \
    HPX_DEFINE_PLAIN_DIRECT_ACTION_(__VA_ARGS__)                               \
    /**/

#define HPX_DEFINE_PLAIN_ACTION_(...)                                          \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_DEFINE_PLAIN_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))     \
    /**/

#define HPX_DEFINE_PLAIN_DIRECT_ACTION_(...)                                   \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_DEFINE_PLAIN_DIRECT_ACTION_,                  \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_DEFINE_PLAIN_ACTION_1(func)                                        \
    HPX_DEFINE_PLAIN_ACTION_2(func, HPX_PP_CAT(func, _action))                 \
    /**/

#if defined(HPX_HAVE_CXX26_REFLECTION)
// clang-format off
/// When C++26 reflection is available, HPX_DEFINE_PLAIN_ACTION_2 uses
/// reflect_action<^^func> instead of make_action_t. This eliminates the
/// need for HPX_REGISTER_ACTION while keeping the same user-facing syntax.
#define HPX_DEFINE_PLAIN_ACTION_2(func, name)                                  \
    using name = hpx::actions::reflect_action<^^func>                          \
    /**/
// clang-format on
#elif defined(__NVCC__) || defined(__CUDACC__)
#define HPX_DEFINE_PLAIN_ACTION_2(func, name)                                  \
    struct name                                                                \
      : hpx::actions::make_action<                                             \
            typename std::add_pointer<                                         \
                typename std::remove_pointer<decltype(&func)>::type>::type,    \
            &func, name>::type                                                 \
    {                                                                          \
    } /**/
#else
#define HPX_DEFINE_PLAIN_ACTION_2(func, name)                                  \
    struct name : hpx::actions::make_action_t<decltype(&func), &func, name>    \
    {                                                                          \
    } /**/
#endif

#define HPX_DEFINE_PLAIN_DIRECT_ACTION_1(func)                                 \
    HPX_DEFINE_PLAIN_DIRECT_ACTION_2(func, HPX_PP_CAT(func, _action))          \
    /**/

#define HPX_DEFINE_PLAIN_DIRECT_ACTION_2(func, name)                           \
    struct name                                                                \
      : hpx::actions::make_direct_action_t<decltype(&func), &func, name>       \
    {                                                                          \
    } /**/

/// \endcond

///////////////////////////////////////////////////////////////////////////////
/// \def HPX_DECLARE_PLAIN_ACTION(func, name)
/// \brief Declares a plain action type
///
#define HPX_DECLARE_PLAIN_ACTION(...)                                          \
    HPX_DECLARE_ACTION(__VA_ARGS__)                                            \
/**/

/// \def HPX_PLAIN_ACTION(func, name)
///
/// \brief Defines a plain action type based on the given function
/// \a func and registers it with HPX.
///
/// The macro \a HPX_PLAIN_ACTION can be used to define a plain action (e.g. an
/// action encapsulating a global or free function) based on the given function
/// \a func. It defines the action type \a name representing the given function.
/// This macro additionally registers the newly define action type with HPX.
///
/// The parameter \p func is a global or free (non-member) function which should
/// be encapsulated into a plain action. The parameter \p name is the name of
/// the action type defined by this macro.
///
/// \par Example:
///
/// \code
///     namespace app {
///         void some_global_function(double d) {
///             cout << d;
///         }
///     }
///
///     // This will define the action type 'some_global_action' which
///     // represents the function 'app::some_global_function'.
///     HPX_PLAIN_ACTION(app::some_global_function, some_global_action)
/// \endcode
///
/// \note The macro \a HPX_PLAIN_ACTION has to be used at global namespace even
/// if the wrapped function is located in some other namespace. The newly
/// defined action type is placed into the global namespace as well.
///
/// \note The macro \a HPX_PLAIN_ACTION_ID can be used with 1, 2, or 3
///       arguments. The second and third arguments are optional. The default
///       value for the second argument (the typename of the defined action) is
///       derived from the name of the function (as passed as the first
///       argument) by appending '_action'. The second argument can be omitted
///       only if the first argument with an appended suffix '_action' resolves
///       to a valid, unqualified C++ type name. The default value for the third
///       argument is \a hpx::components::factory_state::check.
///
/// \note Only one of the forms of this macro \a HPX_PLAIN_ACTION or
///       \a HPX_PLAIN_ACTION_ID should be used for a particular action,
///       never both.
///
#define HPX_PLAIN_ACTION(...)                                                  \
    HPX_PLAIN_ACTION_(__VA_ARGS__)                                             \
/**/

/// \def HPX_PLAIN_ACTION_ID(func, actionname, actionid)
///
/// \brief Defines a plain action type based on the given function \a func and
///   registers it with HPX.
///
/// The macro \a HPX_PLAIN_ACTION_ID can be used to define a plain action (e.g.
/// an action encapsulating a global or free function) based on the given
/// function \a func. It defines the action type \a actionname representing the
/// given function.
///
/// The parameter \a actionid specifies a unique integer value which will be
/// used to represent the action during serialization.
///
/// The parameter \p func is a global or free (non-member) function which should
/// be encapsulated into a plain action. The parameter \p name is the name of
/// the action type defined by this macro.
///
/// The second parameter has to be usable as a plain (non-qualified) C++
/// identifier, it should not contain special characters which cannot be part of
/// a C++ identifier, such as '<', '>', or ':'.
///
/// \par Example:
///
/// \code
///     namespace app {
///         void some_global_function(double d) {
///             cout << d;
///         }
///     }
///
///     // This will define the action type 'some_global_action' which
///     // represents the function 'app::some_global_function'.
///     HPX_PLAIN_ACTION_ID(app::some_global_function, some_global_action,
///         some_unique_id);
/// \endcode
///
/// \note The macro \a HPX_PLAIN_ACTION_ID has to be used at global namespace
///       even if the wrapped function is located in some other namespace. The
///       newly defined action type is placed into the global namespace as well.
///
/// \note Only one of the forms of this macro \a HPX_PLAIN_ACTION or
///       \a HPX_PLAIN_ACTION_ID should be used for a particular action,
///       never both.
///
#define HPX_PLAIN_ACTION_ID(func, name, id)                                    \
    HPX_DEFINE_PLAIN_ACTION(func, name);                                       \
    HPX_REGISTER_ACTION_DECLARATION(name, name)                                \
    HPX_REGISTER_ACTION_ID(name, name, id)                                     \
    /**/

/// \cond NOINTERNAL

#define HPX_PLAIN_DIRECT_ACTION(...)                                           \
    HPX_PLAIN_DIRECT_ACTION_(__VA_ARGS__)                                      \
/**/

/// \endcond

/// \cond NOINTERNAL

//
// macros for plain actions
#define HPX_PLAIN_ACTION_(...)                                                 \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_PLAIN_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__)) \
/**/
#define HPX_PLAIN_ACTION_2(func, name)                                         \
    HPX_DEFINE_PLAIN_ACTION(func, name);                                       \
    HPX_REGISTER_ACTION_DECLARATION(name, name)                                \
    HPX_REGISTER_ACTION(name, name)                                            \
/**/
#define HPX_PLAIN_ACTION_1(func)                                               \
    HPX_PLAIN_ACTION_2(func, HPX_PP_CAT(func, _action))                        \
/**/

// same for direct actions
#define HPX_PLAIN_DIRECT_ACTION_(...)                                          \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_PLAIN_DIRECT_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))     \
/**/
#define HPX_PLAIN_DIRECT_ACTION_2(func, name)                                  \
    HPX_DEFINE_PLAIN_DIRECT_ACTION(func, name);                                \
    HPX_REGISTER_ACTION_DECLARATION(name, name)                                \
    HPX_REGISTER_ACTION(name, name)                                            \
/**/
#define HPX_PLAIN_DIRECT_ACTION_1(func)                                        \
    HPX_PLAIN_DIRECT_ACTION_2(func, HPX_PP_CAT(func, _action))                 \
/**/
#define HPX_PLAIN_DIRECT_ACTION_ID(func, name, id)                             \
    HPX_DEFINE_PLAIN_DIRECT_ACTION(func, name);                                \
    HPX_REGISTER_ACTION_DECLARATION(name, name)                                \
    HPX_REGISTER_ACTION_ID(name, name, id)                                     \
    /**/

/// \endcond

////////////////////////////////////////////////////////////////////////////////
// from hpx/actions_base/detail/action_factory.hpp

#if defined(HPX_HAVE_NETWORKING)

#define HPX_REGISTER_ACTION_FACTORY_ID(Name, Id)                               \
    namespace hpx::actions::detail {                                           \
        template <>                                                            \
        HPX_ALWAYS_EXPORT std::string get_action_name_id<Id>()                 \
        {                                                                      \
            return HPX_PP_STRINGIZE(Name);                                     \
        }                                                                      \
        template add_constant_entry<Id> add_constant_entry<Id>::instance;      \
    }                                                                          \
    /**/

#else

#define HPX_REGISTER_ACTION_FACTORY_ID(Name, Id)

#endif

#if defined(HPX_HAVE_CXX26_REFLECTION)
/// \brief Define a reflection-based HPX action for a free function.
///
/// Usage:
///   HPX_ACTION(app::compute, compute_action);
///
/// This replaces the three-step macro sequence:
///   HPX_PLAIN_ACTION(app::compute, compute_action)
///   HPX_REGISTER_ACTION_DECLARATION(compute_action)
///   HPX_REGISTER_ACTION(compute_action)
///
/// The action name is extracted automatically from the function reflection.
/// No manual registration is required.
///
/// \note Requires HPX_WITH_CXX26_REFLECTION=ON
#define HPX_ACTION(func, name)                                                 \
    using name = hpx::actions::reflect_action<^^func> /**/
#endif    // HPX_HAVE_CXX26_REFLECTION
