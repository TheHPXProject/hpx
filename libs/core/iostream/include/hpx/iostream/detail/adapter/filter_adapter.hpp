//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Defines the class template hpx::iostream::detail::filter_adapter,
// a convenience base class for filter adapters.
//
// File:        hpx/iostream/detail/adapter/filter_adapter.hpp
// Date:        Mon Nov 26 14:35:48 MST 2007
// Copyright:   2007-2008 CodeRage, LLC
// Author:      Jonathan Turkanis
// Contact:     turkanis at coderage dot com
//
// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/traits.hpp>

#include <concepts>
#include <ios>
#include <type_traits>

namespace hpx::iostream::detail {

    HPX_CXX_CORE_EXPORT template <typename T>
    class filter_adapter
    {
        using value_type = value_type<T>::type;

    public:
        template <typename Dev>
            requires(std::same_as<std::decay_t<Dev>, T>)
        explicit filter_adapter(Dev&& t)
          : t_(t)
        {
        }

        T& component()
        {
            return t_;
        }

        template <typename Device>
        void close(Device& dev)
        {
            detail::close_all(t_, dev);
        }

        template <typename Device>
        void close(Device& dev, std::ios_base::openmode which)
        {
            iostream::close(t_, dev, which);
        }

        template <typename Device>
        void flush(Device& dev)
        {
            return iostream::flush(t_, dev);
        }

        template <typename Locale>
        void imbue(Locale const& loc)
        {
            iostream::imbue(t_, loc);
        }

        [[nodiscard]] std::streamsize optimal_buffer_size() const
        {
            return iostream::optimal_buffer_size(t_);
        }

        value_type t_;
    };
}    // namespace hpx::iostream::detail
