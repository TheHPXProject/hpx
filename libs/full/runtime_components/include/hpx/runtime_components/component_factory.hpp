//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/runtime_components/component_factory.hpp
/// \page HPX_REGISTER_COMPONENT
/// \headerfile hpx/components.hpp

#pragma once

#ifdef DOXYGEN
/// \def HPX_REGISTER_COMPONENT(type, name, mode)
///
/// \brief Define a component factory for a component type
///
/// This macro is used create and to register a minimal component factory for
/// a component type which allows it to be remotely created using the
/// \a hpx::new_<> function.
///
/// This macro can be invoked with one, two or three arguments
///
/// \param type The \a type parameter is a (fully decorated) type of the
///             component type for which a factory should be defined.
///
/// \param name The \a name parameter specifies the name to use to register
///             the factory. This should uniquely (system-wide) identify the
///             component type. The \a name parameter must conform to the C++
///             identifier rules (without any namespace).
///             If this parameter is not given, the first parameter is used.
///
/// \param mode The \a mode parameter has to be one of the defined enumeration
///             values of the enumeration \a hpx::components::factory_state.
///             The default for this parameter is
///             \a hpx::components::factory_state::enabled.
///
#define HPX_REGISTER_COMPONENT(type, name, mode)

#else

#include <hpx/config.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/runtime_components/component_registry.hpp>
#include <hpx/runtime_components/macros.hpp>

#endif
