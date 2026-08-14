//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_distributed.hpp>

#include <hpx/components/iostreams/ostream.hpp>
#include <hpx/components/iostreams/standard_streams.hpp>

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::iostreams::detail {

    std::ostream& get_coutstream() noexcept
    {
        return std::cout;
    }

    std::ostream& get_cerrstream() noexcept
    {
        return std::cerr;
    }

    std::stringstream& get_consolestream()
    {
        static std::stringstream console_stream;
        return console_stream;
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type return_id_type(future<bool> f, hpx::id_type id)
    {
        f.get();    //re-throw any errors
        return id;
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<hpx::id_type> create_ostream(
        char const* cout_name, std::ostream& strm)
    {
        LRT_(info).format(
            "detail::create_ostream: creating '{}' stream object", cout_name);

        if (agas::is_console())
        {
            typedef components::component<server::output_stream> ostream_type;

            hpx::id_type cout_id(
                components::server::construct<ostream_type>(std::ref(strm)),
                hpx::id_type::management_type::managed);

            return agas::register_name(cout_name, cout_id)
                .then(hpx::launch::sync,
                    hpx::bind_back(&return_id_type, cout_id));
        }

        // the console locality will create the ostream during startup
        return agas::on_symbol_namespace_event(cout_name, true);
    }

    ///////////////////////////////////////////////////////////////////////////
    void release_ostream(char const* name, hpx::id_type const& /* id */)
    {
        LRT_(info).format(
            "detail::release_ostream: destroying '{}' stream object", name);

        if (agas::is_console())
        {
            // now unregister the object from AGAS
            agas::unregister_name(launch::sync, name);
        }
    }
}    // namespace hpx::iostreams::detail

namespace hpx::iostreams {

    // force the creation of the singleton stream objects
    void create_cout()
    {
        if (!agas::is_console())
        {
            HPX_THROW_EXCEPTION(hpx::error::service_unavailable,
                "hpx::iostream::create_cout",
                "this function should be called on the console only");
        }
        detail::create_ostream(detail::cout_tag());
    }

    void create_cerr()
    {
        if (!agas::is_console())
        {
            HPX_THROW_EXCEPTION(hpx::error::service_unavailable,
                "hpx::iostream::create_cerr",
                "this function should be called on the console only");
        }
        detail::create_ostream(detail::cerr_tag());
    }

    void create_consolestream()
    {
        if (!agas::is_console())
        {
            HPX_THROW_EXCEPTION(hpx::error::service_unavailable,
                "hpx::iostream::create_consolestream",
                "this function should be called on the console only");
        }
        detail::create_ostream(detail::consolestream_tag());
    }

    std::stringstream const& get_consolestream()
    {
        if (get_runtime_ptr() != nullptr && !agas::is_console())
        {
            HPX_THROW_EXCEPTION(hpx::error::service_unavailable,
                "hpx::iostream::get_consolestream",
                "this function should be called on the console only");
        }
        return detail::get_consolestream();
    }
}    // namespace hpx::iostreams

///////////////////////////////////////////////////////////////////////////////
HPX_PLAIN_ACTION(hpx::iostreams::create_cout, create_cout_action)
HPX_PLAIN_ACTION(hpx::iostreams::create_cerr, create_cerr_action)
HPX_PLAIN_ACTION(
    hpx::iostreams::create_consolestream, create_consolestream_action)

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    // global standard ostream objects
    iostreams::ostream<> cout;
    iostreams::ostream<> cerr;

    // extension: singleton stringstream on console
    iostreams::ostream<> consolestream;
    std::stringstream const& get_consolestream()
    {
        return iostreams::get_consolestream();
    }
}    // namespace hpx
