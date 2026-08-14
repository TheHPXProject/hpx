//  Copyright (c)      2025 Jiakun Yan
//  Copyright (c) 2013-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/lcw_base.hpp>
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

        bool detect_lcw_environment(
            util::runtime_configuration const& cfg, char const* default_env)
        {
#if !defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCW)
            return false;
#endif
            // We disable the LCW parcelport if any of these hold:
            //
            // - The parcelport is explicitly disabled
            // - The application is not run in an LCW environment
            // - The TCP parcelport is enabled and has higher priority
            // - The MPI parcelport is enabled and has higher priority
            // - The LCI parcelport is enabled and has higher priority
            if (get_entry_as(cfg, "hpx.parcel.lcw.enable", 1) == 0 ||
                (get_entry_as(cfg, "hpx.parcel.tcp.enable", 1) &&
                    (get_entry_as(cfg, "hpx.parcel.tcp.priority", 1) >
                        get_entry_as(cfg, "hpx.parcel.lcw.priority", 0))) ||
                (get_entry_as(cfg, "hpx.parcel.mpi.enable", 1) &&
                    (get_entry_as(cfg, "hpx.parcel.mpi.priority", 1) >
                        get_entry_as(cfg, "hpx.parcel.lcw.priority", 0))) ||
                (get_entry_as(cfg, "hpx.parcel.lci.enable", 1) &&
                    (get_entry_as(cfg, "hpx.parcel.lci.priority", 1) >
                        get_entry_as(cfg, "hpx.parcel.lcw.priority", 0))))
            {
                LBT_(info)
                    << "LCW support disabled via configuration settings\n";
                return false;
            }
            std::string lcw_environment_strings =
                cfg.get_entry("hpx.parcel.lcw.env", default_env);

            hpx::string_util::char_separator sep(";,: ");
            hpx::string_util::tokenizer tokens(lcw_environment_strings, sep);
            for (auto const& tok : tokens)
            {
                char* env = std::getenv(tok.c_str());
                if (env)
                {
                    LBT_(debug)
                        << "Found LCW environment variable: " << tok << "="
                        << std::string(env) << ", enabling LCW support\n";
                    return true;
                }
            }

            LBT_(info) << "No known LCW environment variable found, disabling "
                          "LCW support\n";
            return false;
        }
    }    // namespace detail

    bool lcw_environment::check_lcw_environment(
        util::runtime_configuration& cfg)
    {
        bool ret =
            detail::detect_lcw_environment(cfg, HPX_HAVE_PARCELPORT_LCW_ENV);
        if (!ret)
        {
            cfg.add_entry("hpx.parcel.lcw.enable", "0");
        }
        return ret;
    }
}}    // namespace hpx::util

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCW)) ||      \
    defined(HPX_HAVE_MODULE_LCW_BASE)

namespace hpx { namespace util {
    bool lcw_environment::enabled_ = false;
    lcw_environment::log_level_t lcw_environment::log_level;
#ifdef HPX_HAVE_PARCELPORT_LCW_LOG
    LCT_log_ctx_t lcw_environment::log_ctx;
#endif
    LCT_pcounter_ctx_t lcw_environment::pcounter_ctx;

#define HPX_LCW_PCOUNTER_HANDLE_DEF(name)                                      \
    LCT_pcounter_handle_t lcw_environment::name;

    HPX_LCW_PCOUNTER_NONE_FOR_EACH(HPX_LCW_PCOUNTER_HANDLE_DEF)
    HPX_LCW_PCOUNTER_TREND_FOR_EACH(HPX_LCW_PCOUNTER_HANDLE_DEF)
    HPX_LCW_PCOUNTER_TIMER_FOR_EACH(HPX_LCW_PCOUNTER_HANDLE_DEF)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        void* spinlock_hpx_alloc()
        {
            auto l = new hpx::spinlock;
            return reinterpret_cast<void*>(l);
        }

        void spinlock_hpx_free(void* p)
        {
            auto l = reinterpret_cast<hpx::spinlock*>(p);
            delete l;
        }

        void spinlock_hpx_lock(void* p)
        {
            auto l = reinterpret_cast<hpx::spinlock*>(p);
            l->lock();
        }

        bool spinlock_hpx_trylock(void* p)
        {
            auto l = reinterpret_cast<hpx::spinlock*>(p);
            return l->try_lock();
        }

        void spinlock_hpx_unlock(void* p)
        {
            auto l = reinterpret_cast<hpx::spinlock*>(p);
            l->unlock();
        }

        lcw::custom_spinlock_op_t get_spinlock_hpx_op()
        {
            return {
                .name = "hpx",
                .alloc = spinlock_hpx_alloc,
                .free = spinlock_hpx_free,
                .lock = spinlock_hpx_lock,
                .trylock = spinlock_hpx_trylock,
                .unlock = spinlock_hpx_unlock,
            };
        }
    }    // namespace detail
    void lcw_environment::init(
        int*, char***, util::runtime_configuration& rtcfg)
    {
        if (enabled_)
            return;    // don't call twice

        if (!lcw::is_initialized())
        {
            lcw::custom_spinlock_setup(detail::get_spinlock_hpx_op());
            lcw::initialize();
        }

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

        rtcfg.add_entry("hpx.parcel.bootstrap", "lcw");
        rtcfg.add_entry("hpx.parcel.lcw.rank", std::to_string(this_rank));
        LCT_init();
        // initialize the log context
#ifdef HPX_HAVE_PARCELPORT_LCW_LOG
        char const* const log_levels[] = {"none", "profile", "debug"};
        log_ctx = LCT_log_ctx_alloc(log_levels,
            sizeof(log_levels) / sizeof(log_levels[0]), 0, "hpx_lcw",
            getenv("HPX_LCW_LOG_OUTFILE"), getenv("HPX_LCW_LOG_LEVEL"),
            getenv("HPX_LCW_LOG_WHITELIST"), getenv("HPX_LCW_LOG_BLACKLIST"));
        log_level = static_cast<log_level_t>(LCT_log_get_level(log_ctx));
#else
        log_level = log_level_t::none;
#endif
#ifdef HPX_HAVE_PARCELPORT_LCW_PCOUNTER
        // initialize the performance counters
        pcounter_ctx = LCT_pcounter_ctx_alloc("hpx-lcw");

#define HPX_LCW_PCOUNTER_NONE_REGISTER(name)                                   \
    name = LCT_pcounter_register(pcounter_ctx, #name, LCT_PCOUNTER_NONE);
        HPX_LCW_PCOUNTER_NONE_FOR_EACH(HPX_LCW_PCOUNTER_NONE_REGISTER)

#define HPX_LCW_PCOUNTER_TREND_REGISTER(name)                                  \
    name = LCT_pcounter_register(pcounter_ctx, #name, LCT_PCOUNTER_TREND);
        HPX_LCW_PCOUNTER_TREND_FOR_EACH(HPX_LCW_PCOUNTER_TREND_REGISTER)

#define HPX_LCW_PCOUNTER_TIMER_REGISTER(name)                                  \
    name = LCT_pcounter_register(pcounter_ctx, #name, LCT_PCOUNTER_TIMER);
        HPX_LCW_PCOUNTER_TIMER_FOR_EACH(HPX_LCW_PCOUNTER_TIMER_REGISTER)
#endif
        enabled_ = true;
    }

    std::string lcw_environment::get_processor_name()
    {
        return asio::ip::host_name();
    }

    void lcw_environment::finalize()
    {
        if (enabled())
        {
            enabled_ = false;
#ifdef HPX_HAVE_PARCELPORT_LCW_PCOUNTER
            LCT_pcounter_ctx_free(&pcounter_ctx);
#endif
#ifdef HPX_HAVE_PARCELPORT_LCW_LOG
            LCT_log_ctx_free(&log_ctx);
#endif
            LCT_fina();
            if (lcw::is_initialized())
            {
                lcw::finalize();
            }
        }
    }

    bool lcw_environment::do_progress(lcw::device_t device)
    {
        if (!device)
            return false;
        return lcw::do_progress(device);
    }

    bool lcw_environment::enabled()
    {
        return enabled_;
    }

    int lcw_environment::size()
    {
        int res(-1);
        if (enabled())
            res = static_cast<int>(lcw::get_nranks());
        return res;
    }

    int lcw_environment::rank()
    {
        int res(-1);
        if (enabled())
            res = static_cast<int>(lcw::get_rank());
        return res;
    }

    void lcw_environment::log([[maybe_unused]] log_level_t level,
        [[maybe_unused]] char const* tag, [[maybe_unused]] char const* format,
        ...)
    {
#ifdef HPX_HAVE_PARCELPORT_LCW_LOG
        if (level > log_level)
            return;
        va_list args;
        va_start(args, format);

        LCT_Logv(log_ctx, static_cast<int>(level), tag, format, args);

        va_end(args);
#endif
    }

    int64_t lcw_environment::pcounter_now()
    {
#ifdef HPX_HAVE_PARCELPORT_LCW_PCOUNTER
        return static_cast<int64_t>(LCT_now());
#endif
        return 0;
    }

    int64_t lcw_environment::pcounter_since([[maybe_unused]] int64_t then)
    {
#ifdef HPX_HAVE_PARCELPORT_LCW_PCOUNTER
        return static_cast<int64_t>(LCT_now()) - then;
#endif
        return 0;
    }

    void lcw_environment::pcounter_add(
        [[maybe_unused]] LCT_pcounter_handle_t handle,
        [[maybe_unused]] int64_t val)
    {
#ifdef HPX_HAVE_PARCELPORT_LCW_PCOUNTER
        LCT_pcounter_add(pcounter_ctx, handle, val);
#endif
    }

    void lcw_environment::pcounter_start(
        [[maybe_unused]] LCT_pcounter_handle_t handle)
    {
#ifdef HPX_HAVE_PARCELPORT_LCW_PCOUNTER
        LCT_pcounter_start(pcounter_ctx, handle);
#endif
    }

    void lcw_environment::pcounter_end(
        [[maybe_unused]] LCT_pcounter_handle_t handle)
    {
#ifdef HPX_HAVE_PARCELPORT_LCW_PCOUNTER
        LCT_pcounter_end(pcounter_ctx, handle);
#endif
    }

}}    // namespace hpx::util

#endif
