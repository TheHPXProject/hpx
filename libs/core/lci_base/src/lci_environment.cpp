//  Copyright (c) 2025 Jiakun Yan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/lci_base.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/util.hpp>
#include <asio/ip/host_name.hpp>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <stdarg.h>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util {

    namespace detail {

        bool detect_lci_environment(
            util::runtime_configuration const& cfg, char const* default_env)
        {
#if !defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)
            return false;
#endif
            // We disable the LCI parcelport if any of these hold:
            //
            // - The parcelport is explicitly disabled
            // - The application is not run in an LCI environment
            // - The TCP parcelport is enabled and has higher priority
            // - The MPI parcelport is enabled and has higher priority
            if (get_entry_as(cfg, "hpx.parcel.lci.enable", 1) == 0 ||
                (get_entry_as(cfg, "hpx.parcel.tcp.enable", 1) &&
                    (get_entry_as(cfg, "hpx.parcel.tcp.priority", 1) >
                        get_entry_as(cfg, "hpx.parcel.lci.priority", 0))) ||
                (get_entry_as(cfg, "hpx.parcel.mpi.enable", 1) &&
                    (get_entry_as(cfg, "hpx.parcel.mpi.priority", 1) >
                        get_entry_as(cfg, "hpx.parcel.lci.priority", 0))) ||
                (get_entry_as(cfg, "hpx.parcel.lcw.enable", 1) &&
                    (get_entry_as(cfg, "hpx.parcel.lcw.priority", 1) >
                        get_entry_as(cfg, "hpx.parcel.lci.priority", 0))))
            {
                LBT_(info)
                    << "LCI support disabled via configuration settings\n";
                return false;
            }
            std::string lci_environment_strings =
                cfg.get_entry("hpx.parcel.lci.env", default_env);

            hpx::string_util::char_separator sep(";,: ");
            hpx::string_util::tokenizer tokens(lci_environment_strings, sep);
            for (auto const& tok : tokens)
            {
                char* env = std::getenv(tok.c_str());
                if (env)
                {
                    LBT_(debug)
                        << "Found LCI environment variable: " << tok << "="
                        << std::string(env) << ", enabling LCI support\n";
                    return true;
                }
            }

            LBT_(info) << "No known LCI environment variable found, disabling "
                          "LCI support\n";
            return false;
        }
    }    // namespace detail

    bool lci_environment::check_lci_environment(
        util::runtime_configuration& cfg)
    {
        bool ret =
            detail::detect_lci_environment(cfg, HPX_HAVE_PARCELPORT_LCI_ENV);
        if (!ret)
        {
            cfg.add_entry("hpx.parcel.lci.enable", "0");
        }
        return ret;
    }
}}    // namespace hpx::util

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)) ||      \
    defined(HPX_HAVE_MODULE_LCI_BASE)

namespace hpx { namespace util {
    bool lci_environment::enabled_ = false;
    lci_environment::log_level_t lci_environment::log_level;
#ifdef HPX_HAVE_PARCELPORT_LCI_LOG
    LCT_log_ctx_t lci_environment::log_ctx;
#endif
    LCT_pcounter_ctx_t lci_environment::pcounter_ctx;

#define HPX_LCI_PCOUNTER_HANDLE_DEF(name)                                      \
    LCT_pcounter_handle_t lci_environment::name;

    HPX_LCI_PCOUNTER_NONE_FOR_EACH(HPX_LCI_PCOUNTER_HANDLE_DEF)
    HPX_LCI_PCOUNTER_TREND_FOR_EACH(HPX_LCI_PCOUNTER_HANDLE_DEF)
    HPX_LCI_PCOUNTER_TIMER_FOR_EACH(HPX_LCI_PCOUNTER_HANDLE_DEF)

    ///////////////////////////////////////////////////////////////////////////
    void lci_environment::init(
        int*, char***, util::runtime_configuration& rtcfg)
    {
        if (enabled_)
            return;    // don't call twice

        LCT_init();
        ::lci::global_initialize();

        int this_rank = rank();

#if defined(HPX_HAVE_NETWORKING)
        if (this_rank == 0)
        {
            rtcfg.mode_ = hpx::runtime_mode::console;
        }
        else
        {
            rtcfg.mode_ = hpx::runtime_mode::worker;
        }
#elif defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        rtcfg.mode_ = hpx::runtime_mode::console;
#else
        rtcfg.mode_ = hpx::runtime_mode::local;
#endif

        rtcfg.add_entry("hpx.parcel.bootstrap", "lci");
        rtcfg.add_entry("hpx.parcel.lci.rank", std::to_string(this_rank));

#ifdef HPX_HAVE_PARCELPORT_LCI_LOG
        char const* const log_levels[] = {"none", "profile", "debug"};
        log_ctx = LCT_log_ctx_alloc(log_levels,
            sizeof(log_levels) / sizeof(log_levels[0]), 0, "hpx_lci",
            getenv("HPX_LCI_LOG_OUTFILE"), getenv("HPX_LCI_LOG_LEVEL"),
            getenv("HPX_LCI_LOG_WHITELIST"), getenv("HPX_LCI_LOG_BLACKLIST"));
        log_level = static_cast<log_level_t>(LCT_log_get_level(log_ctx));
#else
        log_level = log_level_t::none;
#endif
#ifdef HPX_HAVE_PARCELPORT_LCI_PCOUNTER
        pcounter_ctx = LCT_pcounter_ctx_alloc("hpx-lci");

#define HPX_LCI_PCOUNTER_NONE_REGISTER(name)                                   \
    name = LCT_pcounter_register(pcounter_ctx, #name, LCT_PCOUNTER_NONE);
        HPX_LCI_PCOUNTER_NONE_FOR_EACH(HPX_LCI_PCOUNTER_NONE_REGISTER)

#define HPX_LCI_PCOUNTER_TREND_REGISTER(name)                                  \
    name = LCT_pcounter_register(pcounter_ctx, #name, LCT_PCOUNTER_TREND);
        HPX_LCI_PCOUNTER_TREND_FOR_EACH(HPX_LCI_PCOUNTER_TREND_REGISTER)

#define HPX_LCI_PCOUNTER_TIMER_REGISTER(name)                                  \
    name = LCT_pcounter_register(pcounter_ctx, #name, LCT_PCOUNTER_TIMER);
        HPX_LCI_PCOUNTER_TIMER_FOR_EACH(HPX_LCI_PCOUNTER_TIMER_REGISTER)
#endif
        enabled_ = true;
    }

    std::string lci_environment::get_processor_name()
    {
        return ::asio::ip::host_name();
    }

    void lci_environment::finalize()
    {
        if (enabled())
        {
            enabled_ = false;
#ifdef HPX_HAVE_PARCELPORT_LCI_PCOUNTER
            LCT_pcounter_ctx_free(&pcounter_ctx);
#endif
#ifdef HPX_HAVE_PARCELPORT_LCI_LOG
            LCT_log_ctx_free(&log_ctx);
#endif
            ::lci::global_finalize();
            LCT_fina();
        }
    }

    bool lci_environment::enabled()
    {
        return enabled_;
    }

    int lci_environment::size()
    {
        int res(-1);
        if (enabled())
            res = lci::get_rank_n();
        return res;
    }

    int lci_environment::rank()
    {
        int res(-1);
        if (enabled())
            res = lci::get_rank_me();
        return res;
    }

    int lci_environment::get_max_tag()
    {
        if (enabled())
            return ::lci::get_g_runtime().get_attr_max_tag();
        return -1;
    }

    bool lci_environment::do_progress(::lci::device_t device)
    {
        if (device.is_empty())
            return false;
        auto ret = ::lci::progress_x().device(device)();
        return ret.is_done();
    }

    void lci_environment::log([[maybe_unused]] log_level_t level,
        [[maybe_unused]] char const* tag, [[maybe_unused]] char const* format,
        ...)
    {
#ifdef HPX_HAVE_PARCELPORT_LCI_LOG
        if (level > log_level)
            return;
        va_list args;
        va_start(args, format);
        LCT_Logv(log_ctx, static_cast<int>(level), tag, format, args);
        va_end(args);
#endif
    }

    int64_t lci_environment::pcounter_now()
    {
#ifdef HPX_HAVE_PARCELPORT_LCI_PCOUNTER
        return static_cast<int64_t>(LCT_now());
#else
        return 0;
#endif
    }

    int64_t lci_environment::pcounter_since([[maybe_unused]] int64_t then)
    {
#ifdef HPX_HAVE_PARCELPORT_LCI_PCOUNTER
        return static_cast<int64_t>(LCT_now()) - then;
#else
        return 0;
#endif
    }

    void lci_environment::pcounter_add(
        [[maybe_unused]] LCT_pcounter_handle_t handle,
        [[maybe_unused]] int64_t val)
    {
#ifdef HPX_HAVE_PARCELPORT_LCI_PCOUNTER
        LCT_pcounter_add(pcounter_ctx, handle, val);
#endif
    }

    void lci_environment::pcounter_start(
        [[maybe_unused]] LCT_pcounter_handle_t handle)
    {
#ifdef HPX_HAVE_PARCELPORT_LCI_PCOUNTER
        LCT_pcounter_start(pcounter_ctx, handle);
#endif
    }

    void lci_environment::pcounter_end(
        [[maybe_unused]] LCT_pcounter_handle_t handle)
    {
#ifdef HPX_HAVE_PARCELPORT_LCI_PCOUNTER
        LCT_pcounter_end(pcounter_ctx, handle);
#endif
    }

}}    // namespace hpx::util

#endif
