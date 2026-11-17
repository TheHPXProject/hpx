//  Copyright (c) 2026 STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Demonstrating #8070: Runtime hangs, busy, when a startup function throws on
// one of two localities

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>

static char const* const error_message = "simulated startup failure";
static std::uint32_t recorded_locality_id = static_cast<std::uint32_t>(-1);
static std::uint32_t target_failing_locality = 1;
static std::string phase = "startup";
static bool hpx_main_executed = false;

void fail_function()
{
    recorded_locality_id = hpx::get_locality_id();
    if (recorded_locality_id == target_failing_locality)
    {
        throw std::runtime_error(error_message);
    }
}

int hpx_main()
{
    if (phase == "optional-registration")
    {
        hpx_main_executed = true;
        return hpx::finalize();
    }

    // hpx_main must never be invoked if startup failed on any locality
    HPX_TEST(false);
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    namespace po = hpx::program_options;
    po::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()("phase",
        po::value<std::string>()->default_value("startup"),
        "phase to throw in (registration, optional-registration, pre-startup, "
        "or startup)")("failing-locality",
        po::value<std::uint32_t>()->default_value(1),
        "locality that throws the exception (0 or 1)");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(desc_commandline)
                  .allow_unregistered()
                  .run(),
        vm);
    po::notify(vm);

    phase = vm["phase"].as<std::string>();
    target_failing_locality = vm["failing-locality"].as<std::uint32_t>();

    std::uint32_t current_node = static_cast<std::uint32_t>(-1);
    for (int i = 1; i < argc; ++i)
    {
        std::string const arg = argv[i];
        if (arg.rfind("--hpx:node=", 0) == 0)
        {
            current_node = std::stoul(arg.substr(11));
            break;
        }
    }

    if (phase == "optional-registration")
    {
        // 1. Register an intentionally unavailable optional message handler
        //    with non-throwing error_code
        hpx::error_code ec(hpx::throwmode::lightweight);
        hpx::register_message_handler(
            "invalid_optional_handler_8070", "optional_action_8070", ec);

        // 2. Verify registration request is stored and non-throwing policy
        //    is preserved
        bool stored = false;
        bool nonthrowing_policy_preserved = false;
        for (auto const& t :
            hpx::parcelset::detail::get_message_handler_registrations())
        {
            if (std::string(hpx::get<0>(t)) ==
                    "invalid_optional_handler_8070" &&
                std::string(hpx::get<1>(t)) == "optional_action_8070")
            {
                stored = true;
                nonthrowing_policy_preserved = hpx::get<2>(t);
                break;
            }
        }
        HPX_TEST(stored);
        HPX_TEST(nonthrowing_policy_preserved);
    }
    else if (phase == "registration")
    {
        if (current_node == target_failing_locality)
        {
            hpx::register_message_handler(
                "invalid_message_handler_8070", "invalid_action_8070");
        }
    }
    else if (phase == "pre-startup")
    {
        hpx::register_pre_startup_function(&fail_function);
    }
    else
    {
        hpx::register_startup_function(&fail_function);
    }

    bool caught_remote_exception = false;
    bool caught_local_exception = false;
    try
    {
        hpx::init_params init_args;
        init_args.desc_cmdline = desc_commandline;
        hpx::init(argc, argv, init_args);
    }
    catch (hpx::exception const& e)
    {
        if (phase == "optional-registration")
        {
            // Optional registration must not fail bootstrap
            HPX_TEST(false);
        }
        else if (e.get_error() == hpx::error::invalid_status)
        {
            caught_remote_exception = true;
        }
        else if (phase == "registration" &&
            e.get_error() == hpx::error::bad_plugin_type)
        {
            caught_local_exception = true;
        }
    }
    catch (std::exception const& e)
    {
        if (phase == "optional-registration")
        {
            HPX_TEST(false);
        }
        else if (std::string(e.what()) == error_message)
        {
            caught_local_exception = true;
        }
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    if (phase == "optional-registration")
    {
        // Normal startup must proceed and hpx_main must execute on locality 0
        if (current_node == 0)
        {
            HPX_TEST(hpx_main_executed);
        }
        HPX_TEST(!caught_local_exception);
        HPX_TEST(!caught_remote_exception);
        return hpx::util::report_errors();
    }

    if (recorded_locality_id == static_cast<std::uint32_t>(-1))
    {
        recorded_locality_id = current_node;
    }

    // Verify per-locality exception invariant
    if (recorded_locality_id == target_failing_locality)
    {
        HPX_TEST(caught_local_exception);
        HPX_TEST(!caught_remote_exception);
    }
    else
    {
        HPX_TEST(caught_remote_exception);
        HPX_TEST(!caught_local_exception);
    }

    return hpx::util::report_errors();
}
#endif
