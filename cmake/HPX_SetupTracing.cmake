# Copyright (c) 2026 The STE||AR Group
# Copyright (c) 2026 Vansh Dobhal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_AddDefinitions)

set(_hpx_tracing_backends "")
if(HPX_WITH_APEX)
  list(APPEND _hpx_tracing_backends "HPX_WITH_APEX")
endif()
if(HPX_WITH_ITTNOTIFY)
  list(APPEND _hpx_tracing_backends "HPX_WITH_ITTNOTIFY")
endif()
if(HPX_WITH_TRACY)
  list(APPEND _hpx_tracing_backends "HPX_WITH_TRACY")
endif()

list(LENGTH _hpx_tracing_backends _hpx_tracing_backends_count)
if(_hpx_tracing_backends_count GREATER 1)
  hpx_error(
    "Only one tracing backend can be active. Disable all but one of: HPX_WITH_APEX, HPX_WITH_ITTNOTIFY, HPX_WITH_TRACY."
  )
endif()
unset(_hpx_tracing_backends)
unset(_hpx_tracing_backends_count)

# Setup Intel amplifier for ITTNotify when APEX is not used.
if(HPX_WITH_ITTNOTIFY AND NOT HPX_WITH_APEX)
  find_package(Amplifier)
  if(NOT Amplifier_FOUND)
    hpx_error(
      "Intel Amplifier could not be found and HPX_WITH_ITTNOTIFY=On, please specify Amplifier_ROOT to point to the root of your Amplifier installation"
    )
  endif()

  hpx_add_config_define(HPX_HAVE_THREAD_DESCRIPTION)
endif()

if(HPX_WITH_ITTNOTIFY)
  hpx_add_config_define(HPX_HAVE_ITTNOTIFY 1)
endif()

if(HPX_WITH_TRACY)
  hpx_add_config_define(HPX_HAVE_TRACY)
endif()

if(HPX_WITH_APEX
   OR HPX_WITH_ITTNOTIFY
   OR HPX_WITH_TRACY
)
  hpx_add_config_define(HPX_HAVE_TRACING)
endif()
