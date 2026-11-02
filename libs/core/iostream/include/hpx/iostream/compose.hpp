//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// Note: bidirectional streams are not supported.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/detail/adapter/direct_adapter.hpp>
#include <hpx/iostream/detail/execute.hpp>
#include <hpx/iostream/detail/functional.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <functional>
#include <ios>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    namespace detail {

        // clang-format off
        HPX_CXX_CORE_EXPORT template <typename First, typename Second,
            typename FirstMode = mode_of_t<First>,
            typename SecondMode = mode_of_t<Second>>
        struct composite_mode
          : util::select<
                std::is_convertible<SecondMode, FirstMode>, FirstMode,
                std::is_convertible<FirstMode, SecondMode>, SecondMode,
                std::is_convertible<SecondMode, input>, input,
                util::else_t, output>
        {
        };
        // clang-format on

        //
        // Template name: composite_device.
        // Description: Provides a Device view of a Filter, Device pair.
        // Template parameters:
        //      Filter - A model of Filter.
        //      Device - An indirect model of Device.
        //
        HPX_CXX_CORE_EXPORT template <typename Filter, typename Device,
            typename Mode = typename composite_mode<Filter, Device>::type>
        class composite_device
        {
            using filter_mode = mode_of_t<Filter>;
            using device_mode = mode_of_t<Device>;

            // clang-format off
            using value_type =
                util::select_t<
                    is_direct<Device>, direct_adapter<Device>,
                    is_std_io<Device>, Device&,
                    util::else_t, Device>;
            // clang-format on

            static_assert(is_filter_v<Filter>);
            static_assert(is_device_v<Device>);

        public:
            using char_type = char_type_of_t<Filter>;

            struct category
              : Mode
              , device_tag
              , closable_tag
              , flushable_tag
              , localizable_tag
              , optimally_buffered_tag
            {
            };

            template <typename Param>
            composite_device(Filter const& flt, Param&& dev);

            std::streamsize read(char_type* s, std::streamsize n);
            std::streamsize write(char_type const* s, std::streamsize n);
            std::streampos seek(stream_offset off, std::ios_base::seekdir way,
                std::ios_base::openmode which = std::ios_base::in |
                    std::ios_base::out);

            void close();
            void close(std::ios_base::openmode which);
            bool flush();
            [[nodiscard]] std::streamsize optimal_buffer_size() const;

            template <typename Locale>    // Avoid dependency on <locale>
            void imbue(Locale const& loc)
            {
                iostream::imbue(filter_, loc);
                iostream::imbue(device_, loc);
            }

            Filter& first()
            {
                return filter_;
            }
            Device& second()
            {
                return device_;
            }

        private:
            Filter filter_;
            value_type device_;
        };

        //
        // Template name: composite_device.
        // Description: Provides a Device view of a Filter, Device pair.
        // Template parameters:
        //      Filter - A model of Filter.
        //      Device - An indirect model of Device.
        //
        HPX_CXX_CORE_EXPORT template <typename Filter1, typename Filter2,
            typename Mode = typename composite_mode<Filter1, Filter2>::type>
        class composite_filter
        {
        private:
            using filter_ref = std::reference_wrapper<Filter2>;
            using first_mode = mode_of_t<Filter1>;
            using second_mode = mode_of_t<Filter2>;

            // A dual-use filter cannot be composed with a read-write filter
            static_assert(!(std::is_convertible_v<first_mode, dual_use>) ||
                !(std::is_convertible_v<second_mode, input>) ||
                !(std::is_convertible_v<second_mode, output>) ||
                (std::is_convertible_v<second_mode, dual_use>) );
            static_assert(!(std::is_convertible_v<second_mode, dual_use>) ||
                !(std::is_convertible_v<first_mode, input>) ||
                !(std::is_convertible_v<first_mode, output>) ||
                (std::is_convertible_v<first_mode, dual_use>) );
            static_assert(is_filter_v<Filter1>);
            static_assert(is_filter_v<Filter2>);

        public:
            using char_type = char_type_of_t<Filter1>;

            struct category
              : Mode
              , filter_tag
              , multichar_tag
              , closable_tag
              , flushable_tag
              , localizable_tag
              , optimally_buffered_tag
            {
            };

            composite_filter(Filter1 const& filter1, Filter2 const& filter2)
              : filter1_(filter1)
              , filter2_(filter2)
            {
            }

            template <typename Source>
            std::streamsize read(Source& src, char_type* s, std::streamsize n)
            {
                composite_device<filter_ref, Source> cmp(
                    std::ref(filter2_), src);
                return iostream::read(filter1_, cmp, s, n);
            }

            template <typename Sink>
            std::streamsize write(
                Sink& snk, char_type const* s, std::streamsize n)
            {
                composite_device<filter_ref, Sink> cmp(std::ref(filter2_), snk);
                return iostream::write(filter1_, cmp, s, n);
            }

            template <typename Device>
            std::streampos seek(Device& dev, stream_offset off,
                std::ios_base::seekdir way,
                std::ios_base::openmode which = std::ios_base::in |
                    std::ios_base::out)
            {
                composite_device<filter_ref, Device> cmp(
                    std::ref(filter2_), dev);
                return iostream::seek(filter1_, cmp, off, way, which);
            }

            template <typename Device>
            void close(Device& dev)
            {
                static_assert(
                    (!std::is_convertible_v<category, two_sequence>) );
                static_assert((!std::is_convertible_v<category, dual_use>) );

                // Create a new device by composing the second filter2_ with dev.
                composite_device<filter_ref, Device> cmp(
                    std::ref(filter2_), dev);

                // Close input sequences in reverse order and output sequences in
                // forward order
                if constexpr (!std::is_convertible_v<first_mode, dual_use>)
                {
                    detail::execute_all(
                        detail::call_close(filter2_, dev, std::ios_base::in),
                        detail::call_close(filter1_, cmp, std::ios_base::in),
                        detail::call_close(filter1_, cmp, std::ios_base::out),
                        detail::call_close(filter2_, dev, std::ios_base::out));
                }
                else if constexpr (std::is_convertible_v<second_mode, input>)
                {
                    detail::execute_all(
                        detail::call_close(filter2_, dev, std::ios_base::in),
                        detail::call_close(filter1_, cmp, std::ios_base::in));
                }
                else
                {
                    detail::execute_all(
                        detail::call_close(filter1_, cmp, std::ios_base::out),
                        detail::call_close(filter2_, dev, std::ios_base::out));
                }
            }

            template <typename Device>
            void close(Device& dev, std::ios_base::openmode const which)
            {
                static_assert((std::is_convertible_v<category, two_sequence>) ||
                    (std::is_convertible_v<category, dual_use>) );

                // Create a new device by composing the second filter2_ with dev.
                composite_device<filter_ref, Device> cmp(
                    std::ref(filter2_), dev);

                // Close input sequences in reverse order
                if constexpr (!std::is_convertible_v<first_mode, dual_use> ||
                    std::is_convertible_v<second_mode, input>)
                {
                    if (which == std::ios_base::in)
                    {
                        detail::execute_all(detail::call_close(filter2_, dev,
                                                std::ios_base::in),
                            detail::call_close(
                                filter1_, cmp, std::ios_base::in));
                    }
                }

                // Close output sequences in forward order
                if constexpr (!std::is_convertible_v<first_mode, dual_use> ||
                    std::is_convertible_v<second_mode, output>)
                {
                    if (which == std::ios_base::out)
                    {
                        detail::execute_all(detail::call_close(filter1_, cmp,
                                                std::ios_base::out),
                            detail::call_close(
                                filter2_, dev, std::ios_base::out));
                    }
                }
            }

            template <typename Device>
            bool flush(Device& dev)
            {
                composite_device<Filter2, Device> cmp(filter2_, dev);
                return iostream::flush(filter1_, cmp);
            }

            [[nodiscard]] std::streamsize optimal_buffer_size() const
            {
                std::streamsize const first =
                    iostream::optimal_buffer_size(filter1_);
                std::streamsize const second =
                    iostream::optimal_buffer_size(filter2_);
                return first < second ? second : first;
            }

            template <typename Locale>    // Avoid dependency on <locale>
            void imbue(Locale const& loc)
            {
                iostream::imbue(filter1_, loc);
                iostream::imbue(filter2_, loc);
            }

            Filter1& first()
            {
                return filter1_;
            }

            Filter2& second()
            {
                return filter2_;
            }

        private:
            Filter1 filter1_;
            Filter2 filter2_;
        };

        HPX_CXX_CORE_EXPORT template <typename Filter, typename FilterOrDevice>
        struct composite_traits
          : std::conditional<is_device_v<FilterOrDevice>,
                composite_device<Filter, FilterOrDevice>,
                composite_filter<Filter, FilterOrDevice>>
        {
        };
    }    // End namespace detail.

    HPX_CXX_CORE_EXPORT template <typename Filter, typename FilterOrDevice>
    struct composite : detail::composite_traits<Filter, FilterOrDevice>::type
    {
        using base = detail::composite_traits<Filter, FilterOrDevice>::type;

        template <typename Param>
        composite(Filter const& flt, Param&& dev)
          : base(flt, HPX_FORWARD(Param, dev))
        {
        }
    };

    //--------------Implementation of compose-------------------------------------//

    // Note: The following workarounds are patterned after resolve.hpp. It has not
    // yet been confirmed that they are necessary.

    HPX_CXX_CORE_EXPORT template <typename Filter, typename FilterOrDevice>
        requires(!is_std_io_v<FilterOrDevice>)
    composite<Filter, FilterOrDevice> compose(
        Filter const& filter, FilterOrDevice const& fod)
    {
        return composite<Filter, FilterOrDevice>(filter, fod);
    }

    HPX_CXX_CORE_EXPORT template <typename Filter, typename Ch, typename Tr>
    composite<Filter, std::basic_streambuf<Ch, Tr>> compose(
        Filter const& filter, std::basic_streambuf<Ch, Tr>& sb)
    {
        return composite<Filter, std::basic_streambuf<Ch, Tr>>(filter, sb);
    }

    HPX_CXX_CORE_EXPORT template <typename Filter, typename Ch, typename Tr>
    composite<Filter, std::basic_istream<Ch, Tr>> compose(
        Filter const& filter, std::basic_istream<Ch, Tr>& is)
    {
        return composite<Filter, std::basic_istream<Ch, Tr>>(filter, is);
    }

    HPX_CXX_CORE_EXPORT template <typename Filter, typename Ch, typename Tr>
    composite<Filter, std::basic_ostream<Ch, Tr>> compose(
        Filter const& filter, std::basic_ostream<Ch, Tr>& os)
    {
        return composite<Filter, std::basic_ostream<Ch, Tr>>(filter, os);
    }

    HPX_CXX_CORE_EXPORT template <typename Filter, typename Ch, typename Tr>
    composite<Filter, std::basic_iostream<Ch, Tr>> compose(
        Filter const& filter, std::basic_iostream<Ch, Tr>& io)
    {
        return composite<Filter, std::basic_iostream<Ch, Tr>>(filter, io);
    }

    //----------------------------------------------------------------------------//
    namespace detail {

        //--------------Implementation of composite_device---------------------------//
        template <typename Filter, typename Device, typename Mode>
        template <typename Param>
        composite_device<Filter, Device, Mode>::composite_device(
            Filter const& flt, Param&& dev)
          : filter_(flt)
          , device_(HPX_FORWARD(Param, dev))
        {
        }

        template <typename Filter, typename Device, typename Mode>
        std::streamsize composite_device<Filter, Device, Mode>::read(
            char_type* s, std::streamsize n)
        {
            return iostream::read(filter_, device_, s, n);
        }

        template <typename Filter, typename Device, typename Mode>
        std::streamsize composite_device<Filter, Device, Mode>::write(
            char_type const* s, std::streamsize n)
        {
            return iostream::write(filter_, device_, s, n);
        }

        template <typename Filter, typename Device, typename Mode>
        std::streampos composite_device<Filter, Device, Mode>::seek(
            stream_offset off, std::ios_base::seekdir way,
            std::ios_base::openmode which)
        {
            return iostream::seek(filter_, device_, off, way, which);
        }

        template <typename Filter, typename Device, typename Mode>
        void composite_device<Filter, Device, Mode>::close()
        {
            static_assert(!std::is_convertible_v<Mode, two_sequence>);
            static_assert(!std::is_convertible_v<filter_mode, dual_use> ||
                !std::is_convertible_v<device_mode, input> ||
                !std::is_convertible_v<device_mode, output>);

            // Close input sequences in reverse order and output sequences
            // in forward order
            if constexpr (!std::is_convertible_v<filter_mode, dual_use>)
            {
                detail::execute_all(
                    detail::call_close(device_, std::ios_base::in),
                    detail::call_close(filter_, device_, std::ios_base::in),
                    detail::call_close(filter_, device_, std::ios_base::out),
                    detail::call_close(device_, std::ios_base::out));
            }
            else if constexpr (std::is_convertible_v<device_mode, input>)
            {
                detail::execute_all(
                    detail::call_close(device_, std::ios_base::in),
                    detail::call_close(filter_, device_, std::ios_base::in));
            }
            else
            {
                detail::execute_all(
                    detail::call_close(filter_, device_, std::ios_base::out),
                    detail::call_close(device_, std::ios_base::out));
            }
        }

        template <typename Filter, typename Device, typename Mode>
        void composite_device<Filter, Device, Mode>::close(
            std::ios_base::openmode const which)
        {
            static_assert(std::is_convertible_v<Mode, two_sequence>);
            static_assert(!std::is_convertible_v<filter_mode, dual_use>);

            // Close input sequences in reverse order
            if (which == std::ios_base::in)
            {
                detail::execute_all(
                    detail::call_close(device_, std::ios_base::in),
                    detail::call_close(filter_, device_, std::ios_base::in));
            }

            // Close output sequences in forward order
            if (which == std::ios_base::out)
            {
                detail::execute_all(
                    detail::call_close(filter_, device_, std::ios_base::out),
                    detail::call_close(device_, std::ios_base::out));
            }
        }

        template <typename Filter, typename Device, typename Mode>
        bool composite_device<Filter, Device, Mode>::flush()
        {
            bool const r1 = iostream::flush(filter_, device_);
            bool const r2 = iostream::flush(device_);
            return r1 && r2;
        }

        template <typename Filter, typename Device, typename Mode>
        std::streamsize
        composite_device<Filter, Device, Mode>::optimal_buffer_size() const
        {
            return iostream::optimal_buffer_size(device_);
        }
    }    // End namespace detail.
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
