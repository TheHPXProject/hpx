//  Copyright (c)      2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <hpx/actions_base/macros.hpp>

#include <type_traits>

#if defined(HPX_HAVE_NETWORKING)

// Helper macro for action serialization; each of the defined actions needs to
// be registered with the serialization library.

#define HPX_DEFINE_GET_ACTION_NAME(action)                                     \
    HPX_DEFINE_GET_ACTION_NAME_(action, action)                                \
    /**/

#define HPX_DEFINE_GET_ACTION_NAME_(action, actionname)                        \
    HPX_DEFINE_GET_ACTION_NAME_TRACING(action, actionname)                     \
    namespace hpx::actions::detail {                                           \
        template <>                                                            \
        HPX_ALWAYS_EXPORT char const* get_action_name</**/ action>() noexcept  \
        {                                                                      \
            return HPX_PP_STRINGIZE(actionname);                               \
        }                                                                      \
    }                                                                          \
    /**/

#define HPX_DEFINE_GET_ACTION_NAME_TRACING(action, actionname)                 \
    namespace hpx::actions::detail {                                           \
        template <>                                                            \
        HPX_ALWAYS_EXPORT hpx::tracing::annotation_handle const&               \
        get_action_name_tracing</**/ action>() noexcept                        \
        {                                                                      \
            static hpx::tracing::annotation_handle sh =                        \
                hpx::tracing::create_annotation_handle(                        \
                    HPX_PP_STRINGIZE(actionname));                             \
            return sh;                                                         \
        }                                                                      \
    }                                                                          \
    /**/

#define HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID_TRACING(action)        \
    namespace hpx::actions::detail {                                           \
        template <>                                                            \
        HPX_ALWAYS_EXPORT hpx::tracing::annotation_handle const&               \
        get_action_name_tracing</**/ action>() noexcept;                       \
    }                                                                          \
    /**/

#define HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID(action)                \
    HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID_TRACING(action)            \
    namespace hpx::actions::detail {                                           \
        template <>                                                            \
        HPX_ALWAYS_EXPORT char const* get_action_name<action>() noexcept;      \
    }                                                                          \
    HPX_REGISTER_ACTION_EXTERN_DECLARATION(action)                             \
                                                                               \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct is_action</**/ action> : std::true_type                         \
        {                                                                      \
        };                                                                     \
        template <>                                                            \
        struct needs_automatic_registration</**/ action> : std::false_type     \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

#define HPX_REGISTER_ACTION_DECLARATION_2(action, actionname)                  \
    HPX_REGISTER_ACTION_DECLARATION_NO_DEFAULT_GUID(action)                    \
    /**/

#if defined(HPX_MSVC) || defined(HPX_MINGW)
#define HPX_REGISTER_ACTION_2(action, actionname)                              \
    HPX_DEFINE_GET_ACTION_NAME_(action, actionname)                            \
    HPX_REGISTER_ACTION_INVOCATION_COUNT(action)                               \
    HPX_REGISTER_PER_ACTION_DATA_COUNTER_TYPES(action)                         \
    namespace hpx::actions {                                                   \
        template struct HPX_ALWAYS_EXPORT transfer_action</**/ action>;        \
        template struct HPX_ALWAYS_EXPORT                                      \
            transfer_continuation_action</**/ action>;                         \
    }                                                                          \
/**/
#define HPX_REGISTER_ACTION_EXTERN_DECLARATION(action) /**/

#else    // defined(HPX_MSVC) || defined(HPX_MINGW)

#define HPX_REGISTER_ACTION_2(action, actionname)                              \
    HPX_DEFINE_GET_ACTION_NAME_(action, actionname)                            \
    HPX_REGISTER_ACTION_INVOCATION_COUNT(action)                               \
    HPX_REGISTER_PER_ACTION_DATA_COUNTER_TYPES(action)                         \
    namespace hpx::actions {                                                   \
        template struct transfer_action</**/ action>;                          \
        template struct transfer_continuation_action</**/ action>;             \
    }                                                                          \
/**/
#define HPX_REGISTER_ACTION_EXTERN_DECLARATION(action)                         \
    namespace hpx::actions {                                                   \
        extern template struct HPX_ALWAYS_IMPORT transfer_action</**/ action>; \
        extern template struct HPX_ALWAYS_IMPORT                               \
            transfer_continuation_action</**/ action>;                         \
    }                                                                          \
    /**/

#endif    // defined(HPX_MSVC) || defined(HPX_MINGW)

#endif    // HPX_HAVE_NETWORKING
