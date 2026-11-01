//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/detail/adapter/filter_adapter.hpp>
#include <hpx/iostream/detail/execute.hpp>
#include <hpx/iostream/detail/functional.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/traits.hpp>

#include <concepts>
#include <ios>
#include <type_traits>

namespace hpx::iostream {

    //
    // Template name: tee_filter.
    // Template parameters:
    //      Device - A blocking Sink.
    //
    HPX_CXX_CORE_EXPORT template <typename Device>
    class tee_filter : public detail::filter_adapter<Device>
    {
    public:
        using char_type = char_type_of_t<Device>;

        struct category
          : dual_use_filter_tag
          , multichar_tag
          , closable_tag
          , flushable_tag
          , localizable_tag
          , optimally_buffered_tag
        {
        };

        static_assert(is_device<Device>::value);
        static_assert(std::is_convertible_v<category_of_t<Device>, output>);

        template <typename Dev>
            requires(std::same_as<std::decay_t<Dev>, Device>)
        explicit tee_filter(Dev&& dev)
          : detail::filter_adapter<Device>(HPX_FORWARD(Dev, dev))
        {
        }

        template <typename Source>
        std::streamsize read(Source& src, char_type* s, std::streamsize n)
        {
            std::streamsize result = iostream::read(src, s, n);
            if (result != -1)
            {
                [[maybe_unused]] std::streamsize const result2 =
                    iostream::write(this->component(), s, result);
                HPX_ASSERT(result == result2);
            }
            return result;
        }

        template <typename Sink>
        std::streamsize write(Sink& snk, char_type const* s, std::streamsize n)
        {
            std::streamsize result = iostream::write(snk, s, n);
            [[maybe_unused]] std::streamsize const result2 =
                iostream::write(this->component(), s, result);
            HPX_ASSERT(result == result2);
            return result;
        }

        template <typename Next>
        void close(Next&, std::ios_base::openmode)
        {
            detail::close_all(this->component());
        }

        template <typename Sink>
        bool flush(Sink& snk)
        {
            bool const r1 = iostream::flush(snk);
            bool const r2 = iostream::flush(this->component());
            return r1 && r2;
        }
    };

    //
    // Template name: tee_device.
    // Template parameters:
    //      Device - A blocking Device.
    //      Sink - A blocking Sink.
    //
    HPX_CXX_CORE_EXPORT template <typename Device, typename Sink>
    class tee_device
    {
    public:
        using device_value = value_type<Device>::type;
        using sink_value = value_type<Sink>::type;
        using char_type = char_type_of_t<Device>;
        using mode = std::conditional_t<
            std::is_convertible_v<category_of_t<Device>, output>, output,
            input>;

        static_assert(is_device_v<Device>);
        static_assert(is_device_v<Sink>);
        static_assert(std::is_same_v<char_type, char_type_of_t<Sink>>);
        static_assert(std::is_convertible_v<category_of_t<Sink>, output>);

        struct category
          : mode
          , device_tag
          , closable_tag
          , flushable_tag
          , localizable_tag
          , optimally_buffered_tag
        {
        };

        template <typename Dev, typename Snk>
            requires(std::same_as<std::decay_t<Dev>, Device> &&
                        std::same_as<std::decay_t<Snk>, Sink>)
        tee_device(Dev&& device, Snk&& sink)
          : dev_(HPX_FORWARD(Dev, device))
          , sink_(HPX_FORWARD(Snk, sink))
        {
        }

        std::streamsize read(char_type* s, std::streamsize n)
        {
            static_assert(std::is_convertible_v<category_of_t<Device>, input>);

            [[maybe_unused]] std::streamsize result1 =
                iostream::read(dev_, s, n);
            if (result1 != -1)
            {
                [[maybe_unused]] std::streamsize const result2 =
                    iostream::write(sink_, s, result1);
                HPX_ASSERT(result1 == result2);
            }
            return result1;
        }

        std::streamsize write(char_type const* s, std::streamsize n)
        {
            static_assert(std::is_convertible_v<category_of_t<Device>, output>);

            [[maybe_unused]] std::streamsize const result1 =
                iostream::write(dev_, s, n);
            [[maybe_unused]] std::streamsize const result2 =
                iostream::write(sink_, s, n);
            HPX_ASSERT(result1 == n && result2 == n);
            return n;
        }

        void close()
        {
            detail::execute_all(
                detail::call_close_all(dev_), detail::call_close_all(sink_));
        }

        bool flush()
        {
            bool const r1 = iostream::flush(dev_);
            bool const r2 = iostream::flush(sink_);
            return r1 && r2;
        }

        template <typename Locale>
        void imbue(Locale const& loc)
        {
            iostream::imbue(dev_, loc);
            iostream::imbue(sink_, loc);
        }

        [[nodiscard]] std::streamsize optimal_buffer_size() const
        {
            return (std::max) (iostream::optimal_buffer_size(dev_),
                iostream::optimal_buffer_size(sink_));
        }

    private:
        device_value dev_;
        sink_value sink_;
    };

    HPX_CXX_CORE_EXPORT template <typename Sink>
    tee_filter<std::decay_t<Sink>> tee(Sink&& sink)
    {
        return tee_filter<std::decay_t<Sink>>(HPX_FORWARD(Sink, sink));
    }

    HPX_CXX_CORE_EXPORT template <typename Device, typename Sink>
    tee_device<std::decay_t<Device>, std::decay_t<Sink>> tee(
        Device&& dev, Sink&& sink)
    {
        return tee_device<std::decay_t<Device>, std::decay_t<Sink>>(
            HPX_FORWARD(Device, dev), HPX_FORWARD(Sink, sink));
    }
}    // namespace hpx::iostream
