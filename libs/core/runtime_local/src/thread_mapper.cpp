//  Copyright (c) 2012 Maciej Brodowicz
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/runtime_local/os_thread_type.hpp>
#include <hpx/runtime_local/thread_mapper.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#if defined(HPX_WINDOWS)
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(HPX_HAVE_PAPI) && defined(__linux__) && !defined(__ANDROID) &&     \
    !defined(ANDROID)
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace hpx::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        std::uint64_t get_system_thread_id()
        {
#if defined(HPX_WINDOWS)
            return static_cast<std::uint64_t>(::GetCurrentThreadId());
#else
            return std::uint64_t(::pthread_self());
#endif
        }

        // thread-specific data
        os_thread_data::os_thread_data(
            std::string label, runtime_local::os_thread_type const type)
          : label_(HPX_MOVE(label))
          , id_(std::this_thread::get_id())
          , tid_(get_system_thread_id())
#if defined(HPX_HAVE_PAPI) && defined(__linux__) && !defined(__ANDROID) &&     \
    !defined(ANDROID)
          , linux_tid_(syscall(SYS_gettid))
#endif
          , type_(type)
        {
        }

        void os_thread_data::invalidate()
        {
            tid_ = thread_mapper::invalid_tid;
            cleanup_.reset();
        }

        bool os_thread_data::is_valid() const
        {
            return tid_ != thread_mapper::invalid_tid;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    thread_mapper::thread_mapper() = default;

    thread_mapper::~thread_mapper()
    {
        std::scoped_lock m(mtx_);

        std::uint32_t i = 0;
        for (auto&& tinfo : thread_map_)
        {
            if (tinfo.cleanup_)
            {
                [[maybe_unused]] auto _ = tinfo.cleanup_(i++);
            }
        }
    }

    std::uint32_t thread_mapper::register_thread(
        char const* name, runtime_local::os_thread_type type)
    {
        std::unique_lock m(mtx_);

        auto const tid = detail::get_system_thread_id();
        for (auto&& tinfo : thread_map_)
        {
            if (tinfo.tid_ == tid)
            {
                m.unlock();

                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "thread_mapper::register_thread",
                    "thread already registered");
            }
        }

        // create mappings
        thread_map_.emplace_back(name, type);

        std::size_t const idx = thread_map_.size() - 1;
        label_map_[name] = idx;

        return static_cast<std::uint32_t>(idx);
    }

    bool thread_mapper::unregister_thread()
    {
        std::scoped_lock m(mtx_);

        std::uint32_t i = 0;
        auto const tid = detail::get_system_thread_id();
        for (auto&& tinfo : thread_map_)
        {
            if (tinfo.tid_ == tid)
            {
                label_map_.erase(tinfo.label_);
                if (tinfo.cleanup_)
                {
                    [[maybe_unused]] auto _ = tinfo.cleanup_(i);
                }

                std::size_t const size = thread_map_.size();
                if (static_cast<std::size_t>(i) == size)
                {
                    thread_map_.resize(size - 1);
                }
                else
                {
                    tinfo.invalidate();
                }
                return true;
            }
            ++i;
        }
        return false;
    }

    bool thread_mapper::register_callback(
        std::uint32_t const tix, callback_type const& cb)
    {
        std::scoped_lock m(mtx_);

        auto const idx = static_cast<std::size_t>(tix);
        if (idx >= thread_map_.size() ||
            !thread_map_[tix].is_valid())    //-V108
        {
            return false;
        }

        thread_map_[tix].cleanup_ = cb;    //-V108
        return true;
    }

    bool thread_mapper::revoke_callback(std::uint32_t const tix)
    {
        std::scoped_lock m(mtx_);

        auto const idx = static_cast<std::size_t>(tix);
        if (idx >= thread_map_.size() ||
            !thread_map_[tix].is_valid())    //-V108
        {
            return false;
        }

        thread_map_[tix].cleanup_.reset();    //-V108
        return true;
    }

    std::thread::id thread_mapper::get_thread_id(std::uint32_t const tix) const
    {
        std::scoped_lock m(mtx_);

        auto const idx = static_cast<std::size_t>(tix);
        if (idx >= thread_map_.size())
        {
            return std::thread::id{};
        }
        return thread_map_[idx].id_;
    }

    std::uint64_t thread_mapper::get_thread_native_handle(
        std::uint32_t const tix) const
    {
        std::scoped_lock m(mtx_);

        auto const idx = static_cast<std::size_t>(tix);
        if (idx >= thread_map_.size())
        {
            return thread_mapper::invalid_tid;
        }
        return thread_map_[idx].tid_;
    }

#if defined(HPX_HAVE_PAPI) && defined(__linux__) && !defined(__ANDROID) &&     \
    !defined(ANDROID)
    pid_t thread_mapper::get_linux_thread_id(std::uint32_t tix) const
    {
        std::lock_guard<mutex_type> m(mtx_);

        auto idx = static_cast<std::size_t>(tix);
        if (idx >= thread_map_.size())
        {
            return -1;
        }
        return thread_map_[idx].linux_tid_;
    }
#endif

    std::string const& thread_mapper::get_thread_label(
        std::uint32_t const tix) const
    {
        std::scoped_lock m(mtx_);

        auto const idx = static_cast<std::size_t>(tix);
        if (idx >= thread_map_.size())
        {
            static std::string invalid_label;
            return invalid_label;
        }
        return thread_map_[idx].label_;
    }

    runtime_local::os_thread_type thread_mapper::get_thread_type(
        std::uint32_t const tix) const
    {
        std::scoped_lock m(mtx_);

        auto const idx = static_cast<std::size_t>(tix);
        if (idx >= thread_map_.size())
        {
            return runtime_local::os_thread_type::unknown;
        }
        return thread_map_[idx].type_;
    }

    std::uint32_t thread_mapper::get_thread_index(
        std::string const& label) const
    {
        std::scoped_lock m(mtx_);

        auto const it = label_map_.find(label);
        if (it == label_map_.end())
        {
            return invalid_index;
        }
        return static_cast<std::uint32_t>(it->second);
    }

    std::uint32_t thread_mapper::get_thread_count() const
    {
        std::scoped_lock m(mtx_);

        return static_cast<std::uint32_t>(label_map_.size());
    }

    // retrieve all data stored for a given thread
    os_thread_data thread_mapper::get_os_thread_data(
        std::string const& label) const
    {
        std::scoped_lock m(mtx_);

        auto const it = label_map_.find(label);
        if (it == label_map_.end())
        {
            return runtime_local::os_thread_data{.label_ = "",
                .id_ = std::thread::id{},
                .native_handle_ = thread_mapper::invalid_tid,
                .type_ = runtime_local::os_thread_type::unknown};
        }

        auto const idx = static_cast<std::size_t>(it->second);
        if (idx >= thread_map_.size())
        {
            return runtime_local::os_thread_data{.label_ = "",
                .id_ = std::thread::id{},
                .native_handle_ = thread_mapper::invalid_tid,
                .type_ = runtime_local::os_thread_type::unknown};
        }

        auto const& tinfo = thread_map_[idx];
        return runtime_local::os_thread_data{.label_ = tinfo.label_,
            .id_ = tinfo.id_,
            .native_handle_ = tinfo.tid_,
            .type_ = tinfo.type_};
    }

    bool thread_mapper::enumerate_os_threads(
        hpx::function<bool(os_thread_data const&)> const& f) const
    {
        std::scoped_lock m(mtx_);
        for (auto const& tinfo : thread_map_)
        {
            os_thread_data data{.label_ = tinfo.label_,
                .id_ = tinfo.id_,
                .native_handle_ = tinfo.tid_,
                .type_ = tinfo.type_};

            if (!f(data))
            {
                return false;
            }
        }
        return true;
    }
}    // namespace hpx::util
