//  Copyright Vladimir Prus 2004.
//  Copyright (c) 2005-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/plugin/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/format.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>

#if !defined(__ANDROID__) && !defined(ANDROID) && !defined(__APPLE__)
#include <link.h>
#endif
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include <dlfcn.h>
#include <limits.h>

#if !defined(HPX_HAS_DLOPEN)
#error                                                                         \
    "This file shouldn't be included directly, use the file hpx/plugin/dll.hpp only."
#endif

///////////////////////////////////////////////////////////////////////////////
#if !defined(RTLD_LOCAL)
#define RTLD_LOCAL 0    // some systems do not have RTLD_LOCAL
#endif
#if !defined(RTLD_DEEPBIND)
#define RTLD_DEEPBIND 0    // some systems do not have RTLD_DEEPBIND
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::plugin {

    ///////////////////////////////////////////////////////////////////////////////
#if !defined(_WIN32)
    HPX_CXX_CORE_EXPORT using HMODULE = void*;
#else
    HPX_CXX_CORE_EXPORT using HMODULE = struct HINSTANCE__*;
#endif

    ///////////////////////////////////////////////////////////////////////////////
    inline int MyFreeLibrary(HMODULE x)
    {
        return dlclose(x);
    }

    inline HMODULE MyLoadLibrary(char const* path)
    {
        return reinterpret_cast<HMODULE>(dlopen(path, RTLD_GLOBAL | RTLD_LAZY));
    }

    inline void* MyGetProcAddress(HMODULE x, char const* y)
    {
        return dlsym(x, y);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class dll
    {
    protected:
        ///////////////////////////////////////////////////////////////////////
        static void init_library(HMODULE)
        {
#if defined(__AIX__) && defined(__GNUC__)
            dlerror();    // Clear the error state.
            using init_proc_type = void (*)();
            init_proc_type init_proc =
                (init_proc_type) MyGetProcAddress(dll_hand, "_GLOBAL__DI");
            if (init_proc)
                init_proc();
#endif
        }

        static void deinit_library(HMODULE)
        {
#if defined(__AIX__) && defined(__GNUC__)
            dlerror();    // Clear the error state.
            using free_proc_type = void (*)();
            free_proc_type free_proc =
                (free_proc_type) MyGetProcAddress(dll_hand, "_GLOBAL__DD");
            if (free_proc)
                free_proc();
#endif
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct free_dll
        {
            free_dll(
                HMODULE h, std::shared_ptr<std::recursive_mutex> mtx) noexcept
              : h_(h)
              , mtx_(HPX_MOVE(mtx))
            {
            }

            void operator()(T) const
            {
                if (nullptr != h_)
                {
                    std::lock_guard<std::recursive_mutex> lock(*mtx_);

                    dll::deinit_library(h_);
                    dlerror();
                    MyFreeLibrary(h_);
                }
            }

            HMODULE h_;
            std::shared_ptr<std::recursive_mutex> mtx_;
        };

        template <typename T>
        friend struct free_dll;

    public:
        dll()
          : mtx_(mutex_instance())
        {
        }

        dll(dll const& rhs)
          : dll_name(rhs.dll_name)
          , map_name(rhs.map_name)
          , mtx_(rhs.mtx_)
        {
        }

        explicit dll(std::string name)
          : dll_name(HPX_MOVE(name))
          , mtx_(mutex_instance())
        {
            // map_name defaults to dll base name
            namespace fs = filesystem;

            fs::path const dll_path(dll_name);
            map_name = fs::basename(dll_path);
        }

        void load_library(error_code& ec = throws)
        {
            LoadLibrary(ec);
        }

        dll(std::string libname, std::string mapname)
          : dll_name(HPX_MOVE(libname))
          , map_name(HPX_MOVE(mapname))
          , mtx_(mutex_instance())
        {
        }

        dll(dll&& rhs) noexcept
          : dll_name(HPX_MOVE(rhs.dll_name))
          , map_name(HPX_MOVE(rhs.map_name))
          , dll_handle(rhs.dll_handle)
          , mtx_(HPX_MOVE(rhs.mtx_))
        {
            rhs.dll_handle = nullptr;
        }

        dll& operator=(dll const& rhs)
        {
            if (this != &rhs)
            {
                //  free any existing dll_handle
                FreeLibrary();

                //  load the library for this instance of the dll class
                dll_name = rhs.dll_name;
                map_name = rhs.map_name;
                mtx_ = rhs.mtx_;
                LoadLibrary();
            }
            return *this;
        }

        dll& operator=(dll&& rhs) noexcept
        {
            if (&rhs != this)
            {
                dll_name = HPX_MOVE(rhs.dll_name);
                map_name = HPX_MOVE(rhs.map_name);
                dll_handle = rhs.dll_handle;
                rhs.dll_handle = nullptr;
                mtx_ = HPX_MOVE(rhs.mtx_);
            }
            return *this;
        }

        ~dll()
        {
            FreeLibrary();
        }

        [[nodiscard]] std::string const& get_name() const noexcept
        {
            return dll_name;
        }
        [[nodiscard]] std::string const& get_mapname() const noexcept
        {
            return map_name;
        }

        template <typename SymbolType, typename Deleter>
        [[nodiscard]] std::pair<SymbolType, Deleter> get(
            std::string const& symbol_name, error_code& ec = throws) const
        {
            // make sure everything is initialized
            const_cast<dll&>(*this).LoadLibrary(ec);
            if (ec)
                return std::pair<SymbolType, Deleter>();

            std::unique_lock<std::recursive_mutex> lock(*mtx_);

            static_assert(
                std::is_pointer_v<SymbolType>, "std::is_pointer_v<SymbolType>");

            auto address =
                (SymbolType) MyGetProcAddress(dll_handle, symbol_name.c_str());
            if (nullptr == address)
            {
                std::string const str = hpx::util::format(
                    "Hpx.Plugin: Unable to locate the exported symbol name "
                    "'{}' in the shared library '{}' (dlerror: {})",
                    symbol_name, dll_name, dlerror());

                dlerror();

                lock.unlock();

                // report error
                HPX_THROWS_IF(
                    ec, hpx::error::dynamic_link_failure, "plugin::get", str);
                return std::pair<SymbolType, Deleter>();
            }

            // Open the library. Yes, we do it on every access to a symbol, the
            // LoadLibrary function increases the refcnt of the dll so in the
            // end the dll class holds one refcnt and so does every symbol.
            dlerror();    // Clear the error state.
            HMODULE handle =
                MyLoadLibrary((dll_name.empty() ? nullptr : dll_name.c_str()));
            if (!handle)
            {
                std::string const str =
                    hpx::util::format("Hpx.Plugin: Could not open shared "
                                      "library '{}' (dlerror: {})",
                        dll_name, dlerror());

                lock.unlock();

                // report error
                HPX_THROWS_IF(
                    ec, hpx::error::filesystem_error, "plugin::get", str);
                return std::pair<SymbolType, Deleter>();
            }

#if !defined(__AIX__)
            // AIX seems to return different handle values for the second and
            // any following call
            HPX_ASSERT(handle == dll_handle);
#endif

            init_library(handle);    // initialize library

            // Cast to the right type.
            dlerror();    // Clear the error state.

            return std::make_pair(address, free_dll<SymbolType>(handle, mtx_));
        }

        void keep_alive(error_code& ec = throws)
        {
            LoadLibrary(ec, true);
        }

    protected:
        void LoadLibrary(error_code& ec = throws, bool force = false)
        {
            if (!dll_handle || force)
            {
                std::unique_lock<std::recursive_mutex> lock(*mtx_);

                ::dlerror();    // Clear the error state.
                dll_handle = MyLoadLibrary(
                    (dll_name.empty() ? nullptr : dll_name.c_str()));
                if (!dll_handle)
                {
                    std::string const str =
                        hpx::util::format("Hpx.Plugin: Could not open shared "
                                          "library '{}' (dlerror: {})",
                            dll_name, dlerror());

                    lock.unlock();

                    HPX_THROWS_IF(ec, hpx::error::filesystem_error,
                        "plugin::LoadLibrary", str);
                    return;
                }

                init_library(dll_handle);    // initialize library
            }

            if (&ec != &throws)
                ec = make_success_code();
        }

    public:
        [[nodiscard]] std::string get_directory(error_code& ec = throws) const
        {
            // now find the full path of the loaded library
            using filesystem::path;
            std::string result;

#if !defined(__ANDROID__) && !defined(ANDROID) && !defined(__APPLE__)
#if defined(RTLD_DI_ORIGIN)
            char directory[PATH_MAX] = {'\0'};
            const_cast<dll&>(*this).LoadLibrary(ec);
            if (!ec && ::dlinfo(dll_handle, RTLD_DI_ORIGIN, directory) < 0)
            {
                std::string const str = hpx::util::format(
                    "Hpx.Plugin: Could not extract path the shared library "
                    "'{}' has been loaded from (dlerror: {})",
                    dll_name, dlerror());

                HPX_THROWS_IF(ec, hpx::error::filesystem_error,
                    "plugin::get_directory", str);
            }
            result = directory;
            ::dlerror();    // Clear the error state.
#else
            result = hpx::filesystem::to_string(path(dll_name).parent_path());
#endif
#elif defined(__APPLE__)
            // SO staticfloat's solution
            const_cast<dll&>(*this).LoadLibrary(ec);
            if (ec)
            {
                // iterate through all images currently in memory
                for (size_t i = 0; i < ::_dyld_image_count(); ++i)
                {
                    if (char const* image_name = ::_dyld_get_image_name(i))
                    {
                        HMODULE probe_handle = ::dlopen(image_name, RTLD_NOW);
                        ::dlclose(probe_handle);

                        // If the handle is the same as what was passed in
                        // (modulo mode bits), return this image name
                        if (((intptr_t) dll_handle & (-4)) ==
                            ((intptr_t) probe_handle & (-4)))
                        {
                            result = hpx::filesystem::to_string(
                                path(image_name).parent_path());
                            break;
                        }
                    }
                }
            }
            ::dlerror();    // Clear the error state.
#endif
            if (&ec != &throws)
                ec = make_success_code();

            return result;
        }

    protected:
        void FreeLibrary()
        {
            if (nullptr != dll_handle)
            {
                std::lock_guard<std::recursive_mutex> lock(*mtx_);

                deinit_library(dll_handle);
                dlerror();
                MyFreeLibrary(dll_handle);
            }
        }

        // protect access to dl... functions
        [[nodiscard]] static std::shared_ptr<std::recursive_mutex>
        mutex_instance()
        {
            static auto mutex = std::make_shared<std::recursive_mutex>();
            return mutex;
        }

    private:
        std::string dll_name;
        std::string map_name;
        HMODULE dll_handle = nullptr;
        std::shared_ptr<std::recursive_mutex> mtx_;
    };
}    // namespace hpx::util::plugin
