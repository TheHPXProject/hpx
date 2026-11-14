//  Copyright (c) 2017 John Biddiscombe
//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <string>
#include <typeinfo>

// --------------------------------------------------------------------
// Always present regardless of compiler : used by serialization code
// --------------------------------------------------------------------
namespace hpx::util::debug {

    HPX_CXX_CORE_EXPORT template <typename T>
    struct demangle_helper
    {
        [[nodiscard]] static char const* type_id() noexcept
        {
            return typeid(T).name();
        }
    };
}    // namespace hpx::util::debug

#if defined(__GNUG__)

#include <cstdlib>
#include <cxxabi.h>

#include <memory>

// --------------------------------------------------------------------
// if available : demangle an arbitrary c++ type using gnu utility
// --------------------------------------------------------------------
namespace hpx::util::debug {

    HPX_CXX_CORE_EXPORT template <typename T>
    class cxxabi_demangle_helper
    {
    public:
        cxxabi_demangle_helper()
          : demangled_{abi::__cxa_demangle(
                           typeid(T).name(), nullptr, nullptr, nullptr),
                std::free}
        {
        }

        [[nodiscard]] char const* type_id() const noexcept
        {
            return demangled_ ? demangled_.get() : typeid(T).name();
        }

    private:
        std::unique_ptr<char, void (*)(void*)> demangled_;
    };
}    // namespace hpx::util::debug

#else

namespace hpx::util::debug {

    HPX_CXX_CORE_EXPORT template <typename T>
    using cxxabi_demangle_helper = demangle_helper<T>;
}    // namespace hpx::util::debug

#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::debug {

    HPX_CXX_CORE_EXPORT template <typename T>
    char const* type_id()
    {
        static cxxabi_demangle_helper<T> id = cxxabi_demangle_helper<T>();
        return id.type_id();
    }

    // --------------------------------------------------------------------
    // print type information
    // usage : std::cout << print_type<args...>("separator")
    // separator is appended if the number of types > 1
    // --------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT template <typename T = void>
    std::string print_type(char const* = "")
    {
        return std::string(type_id<T>());
    }

    template <>
    inline std::string print_type<>(char const*)
    {
        return "void";
    }

#if !defined(DOXYGEN)
    HPX_CXX_CORE_EXPORT template <typename T, typename... Args>
        requires(sizeof...(Args) != 0)
    std::string print_type(char const* delim = "")
    {
        std::string const temp = type_id<T>();
        return temp + delim + print_type<Args...>(delim);
    }
#endif
}    // namespace hpx::util::debug
