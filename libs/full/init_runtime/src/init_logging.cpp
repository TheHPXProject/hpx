//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_LOGGING)
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/threading_base.hpp>
#endif

#include <hpx/init_runtime/detail/init_logging.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
    ///////////////////////////////////////////////////////////////////////////
    // custom formatter: HPX component id of current thread
    struct thread_component_id : logging::formatter::manipulator
    {
        void operator()(std::ostream& to) const override
        {
            std::uint64_t component_id = threads::get_self_component_id();
            if (0 != component_id)
            {
                // called from inside a HPX thread
                util::format_to(to, "{:016x}", component_id);
            }
            else
            {
                // called from outside a HPX thread
                to << std::string(16, '-');
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // custom log destination: send generated strings to console
    struct console : console_local
    {
        console(logging::level level, logging_destination dest)
          : console_local(level, dest)
        {
        }

        void operator()(logging::message const& msg) override
        {
            components::console_logging(
                dest_, static_cast<std::size_t>(level_), msg.full_string());
        }
    };    // namespace util
#else
    using console = console_local;
#endif

    namespace detail {

        void get_console(logging::writer::named_write& writer, char const* name,
            logging::level lvl, logging_destination dest)
        {
            writer.set_destination(name, console(lvl, dest));
        }

        void define_formatters(logging::writer::named_write& writer)
        {
            define_common_formatters(writer);

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
            writer.set_formatter("hpxcomponent", thread_component_id());
#endif
        }

        void init_logging_full(runtime_configuration& ini)
        {
            init_logging(ini, ini.mode_ == runtime_mode::console, get_console,
                define_formatters);
        }
    }    // namespace detail
}    // namespace hpx::util

#endif
