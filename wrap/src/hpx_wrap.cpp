//  Copyright (c) 2018-2020 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/init.hpp>

// The following implementation has been divided for Linux and Mac OSX
#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                                      \
    (defined(__linux) || defined(__linux__) || defined(linux) ||               \
        defined(__APPLE__))

#include <string>

namespace hpx_start {
    // include_libhpx_wrap is a weak symbol which helps to determine the course
    // of function calls at runtime. It has a default value of `false` which
    // corresponds to the program's entry point being main().
    // It is overridden in hpx/hpx_main.hpp or set as TRUE if
    // HPX_AUTO_WRAP_MAIN_ACTIVATE is defined.
    // Thus, inclusion of the header file or defining HPX_AUTO_WRAP_MAIN_ACTIVATE
    // will change the program's entry point to HPX's own custom entry point
    // initialize_main. Subsequent calls before entering main() are handled
    // by this code.
    HPX_SYMBOL_EXPORT extern bool include_libhpx_wrap;
#if defined(HPX_AUTO_WRAP_MAIN_ACTIVATE)
    HPX_SYMBOL_EXPORT bool include_libhpx_wrap __attribute__((weak)) = true;
#else
    HPX_SYMBOL_EXPORT bool include_libhpx_wrap __attribute__((weak)) = false;
#endif
    // The default application name is populated by including hpx/hpx_main.hpp
    // or if HPX_AUTO_WRAP_MAIN_ACTIVATE is defined.
    HPX_SYMBOL_EXPORT extern char const* app_name_libhpx_wrap;
    HPX_SYMBOL_EXPORT const char* app_name_libhpx_wrap __attribute__((weak)) =
        HPX_APPLICATION_STRING;

    // Provide a definition of is_linked variable defined weak in hpx_main.hpp
    // header. This variable is solely to trigger a different exception when
    // trying to register thread when not linked to libhpx_wrap and using
    // hpx_main.hpp functionality.
    HPX_SYMBOL_EXPORT bool is_linked = true;
}    // namespace hpx_start

#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <vector>

////////////////////////////////////////////////////////////////////////////////
// Function declarations
//
namespace hpx_start {
    // Main entry point of HPX runtime system
    extern int hpx_entry(int argc, char* argv[]);
}    // namespace hpx_start

// Program's entry point depending upon Operating System.
// For Mac OSX it is the program's entry point. In case of Linux
// it is called by __wrap_main.
extern "C" int initialize_main(int argc, char** argv);

#if defined(__linux) || defined(__linux__) || defined(linux)
// Actual main() function
extern "C" int __real_main(int argc, char** argv);

// Our wrapper for main() function
extern "C" int __wrap_main(int argc, char** argv);
#endif

#if defined(__APPLE__)
// Declaration for main() for Mac OS implementation
extern "C" int main(int argc, char** argv);
#endif

namespace hpx_start {
    // main entry point of the HPX runtime system
    int hpx_entry(int argc, char* argv[])
    {
#if defined(__linux) || defined(__linux__) || defined(linux)
        // Call to the main() function
        int return_value = __real_main(argc, argv);
#else /* APPLE */
        // call to the main() function
        int return_value = main(argc, argv);
#endif

        // Finalizing the HPX runtime
        hpx::finalize();
        return return_value;
    }
}    // namespace hpx_start

// This is the main entry point of C runtime system.
// The HPX runtime system is initialized here, which
// is similar to initializing HPX from main() and utilising
// the hpx_main() as the entry point.
extern "C" int initialize_main(int argc, char** argv)
{
#if defined(__APPLE__)
    if (hpx_start::include_libhpx_wrap)
    {
#endif
        // Configuring HPX system before runtime
        std::vector<std::string> const cfg = {
            "hpx.commandline.allow_unknown!=1",
            "hpx.commandline.aliasing=0",
        };
        hpx::function<int(int, char**)> start_function = &hpx_start::hpx_entry;
        using hpx::program_options::options_description;
        options_description desc = options_description(std::string("Usage: ") +
            ::hpx_start::app_name_libhpx_wrap + " [options]");
        // Create the init_params struct
        hpx::init_params iparams;
        iparams.desc_cmdline = desc;
        iparams.cfg = cfg;

        // Initialize the HPX runtime system
        return hpx::init(start_function, argc, argv, iparams);
#if defined(__APPLE__)
    }
    return main(argc, argv);
#endif
}

#if defined(__linux) || defined(__linux__) || defined(linux)
////////////////////////////////////////////////////////////////////////////////
// Wrapper for main function
//

// We are wrapping the main function to initialize our
// runtime system prior to real main call.
extern "C" int __wrap_main(int argc, char** argv)
{
    // We determine the function call stack at runtime
    // from the value of include_libhpx_wrap. If hpx_main
    // is included include_libhpx_wrap is set to 1
    // due to override variable.
    if (hpx_start::include_libhpx_wrap)
        return initialize_main(argc, argv);

    // call main() since hpx_main.hpp is not included
    return __real_main(argc, argv);
}
////////////////////////////////////////////////////////////////////////////////
#endif

#endif
