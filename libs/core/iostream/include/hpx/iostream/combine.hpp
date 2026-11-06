//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

// To do: add support for random-access.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/detail/wrap_unwrap.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/traits.hpp>

#include <ios>
#include <locale>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    namespace detail {

        //
        // Template name: combined_device.
        // Description: Model of Device defined in terms of a Source/Sink pair.
        // Template parameters:
        //      Source - A model of Source, with the same char_type and traits_type
        //          as Sink.
        //      Sink - A model of Sink, with the same char_type and traits_type
        //          as Source.
        //
        HPX_CXX_CORE_EXPORT template <typename Source, typename Sink>
        class combined_device
        {
            using in_category = category_of_t<Source>;
            using out_category = category_of_t<Sink>;
            using sink_char_type = char_type_of_t<Sink>;

        public:
            using char_type = char_type_of_t<Source>;

            struct category
              : bidirectional
              , device_tag
              , closable_tag
              , localizable_tag
            {
            };

            static_assert(is_device_v<Source>);
            static_assert(is_device_v<Sink>);
            static_assert(std::is_convertible_v<in_category, input>);
            static_assert(std::is_convertible_v<out_category, output>);
            static_assert(std::is_same_v<char_type, sink_char_type>);

            combined_device(Source const& src, Sink const& snk);

            std::streamsize read(char_type* s, std::streamsize n);
            std::streamsize write(char_type const* s, std::streamsize n);
            void close(std::ios_base::openmode);
            void imbue(std::locale const& loc);

        private:
            Source src_;
            Sink sink_;
        };

        //
        // Template name: combined_filter.
        // Description: Model of Device defined in terms of a Source/Sink pair.
        // Template parameters:
        //      InputFilter - A model of InputFilter, with the same char_type as
        //          OutputFilter.
        //      OutputFilter - A model of OutputFilter, with the same char_type as
        //          InputFilter.
        //
        HPX_CXX_CORE_EXPORT template <typename InputFilter,
            typename OutputFilter>
        class combined_filter
        {
            using in_category = category_of_t<InputFilter>;
            using out_category = category_of_t<OutputFilter>;
            using output_char_type = char_type_of_t<OutputFilter>;

        public:
            using char_type = char_type_of_t<InputFilter>;

            struct category
              : multichar_bidirectional_filter_tag
              , closable_tag
              , localizable_tag
            {
            };

            static_assert(is_filter_v<InputFilter>);
            static_assert(is_filter_v<OutputFilter>);
            static_assert(std::is_convertible_v<in_category, input>);
            static_assert(std::is_convertible_v<out_category, output>);
            static_assert(std::is_same_v<char_type, output_char_type>);

            combined_filter(InputFilter const& in, OutputFilter const& out);

            template <typename Source>
            std::streamsize read(Source& src, char_type* s, std::streamsize n)
            {
                return iostream::read(in_, src, s, n);
            }

            template <typename Sink>
            std::streamsize write(
                Sink& snk, char_type const* s, std::streamsize n)
            {
                return iostream::write(out_, snk, s, n);
            }

            template <typename Sink>
            void close(Sink& snk, std::ios_base::openmode const which)
            {
                if (which == std::ios_base::in)
                {
                    if constexpr (std::is_convertible_v<in_category, dual_use>)
                    {
                        iostream::close(in_, snk, std::ios_base::in);
                    }
                    else
                    {
                        detail::close_all(in_, snk);
                    }
                }
                if (which == std::ios_base::out)
                {
                    if constexpr (std::is_convertible_v<out_category, dual_use>)
                    {
                        iostream::close(out_, snk, std::ios_base::out);
                    }
                    else
                    {
                        detail::close_all(out_, snk);
                    }
                }
            }

            void imbue(std::locale const& loc);

        private:
            InputFilter in_;
            OutputFilter out_;
        };

        HPX_CXX_CORE_EXPORT template <typename In, typename Out>
        struct combination_traits
          : std::conditional<is_device_v<In>,
                combined_device<typename wrapped_type<In>::type,
                    typename wrapped_type<Out>::type>,
                combined_filter<typename wrapped_type<In>::type,
                    typename wrapped_type<Out>::type>>
        {
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename In, typename Out>
    using combination = detail::combination_traits<In, Out>::type;

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename In, typename Out>
        struct combine_traits
        {
            using type = combination<typename unwrapped_type<In>::type,
                typename unwrapped_type<Out>::type>;
        };
    }    // namespace detail

    //
    // Template name: combine.
    // Description: Takes a Source/Sink pair or InputFilter/OutputFilter pair and
    //      returns a Source or Filter which performs input using the first member
    //      of the pair and output using the second member of the pair.
    // Template parameters:
    //      In - A model of Source or InputFilter, with the same char_type as Out.
    //      Out - A model of Sink or OutputFilter, with the same char_type as In.
    //
    HPX_CXX_CORE_EXPORT template <typename In, typename Out>
    detail::combine_traits<In, Out>::type combine(In const& in, Out const& out)
    {
        return {in, out};
    }

    //----------------------------------------------------------------------------//
    namespace detail {

        //--------------Implementation of combined_device-----------------------------//
        template <typename Source, typename Sink>
        combined_device<Source, Sink>::combined_device(
            Source const& src, Sink const& snk)
          : src_(src)
          , sink_(snk)
        {
        }

        template <typename Source, typename Sink>
        std::streamsize combined_device<Source, Sink>::read(
            char_type* s, std::streamsize n)
        {
            return iostream::read(src_, s, n);
        }

        template <typename Source, typename Sink>
        std::streamsize combined_device<Source, Sink>::write(
            char_type const* s, std::streamsize n)
        {
            return iostream::write(sink_, s, n);
        }

        template <typename Source, typename Sink>
        void combined_device<Source, Sink>::close(
            std::ios_base::openmode const which)
        {
            if (which == std::ios_base::in)
                detail::close_all(src_);
            if (which == std::ios_base::out)
                detail::close_all(sink_);
        }

        template <typename Source, typename Sink>
        void combined_device<Source, Sink>::imbue(std::locale const& loc)
        {
            iostream::imbue(src_, loc);
            iostream::imbue(sink_, loc);
        }

        //--------------Implementation of filter_pair---------------------------------//
        template <typename InputFilter, typename OutputFilter>
        combined_filter<InputFilter, OutputFilter>::combined_filter(
            InputFilter const& in, OutputFilter const& out)
          : in_(in)
          , out_(out)
        {
        }

        template <typename InputFilter, typename OutputFilter>
        void combined_filter<InputFilter, OutputFilter>::imbue(
            std::locale const& loc)
        {
            iostream::imbue(in_, loc);
            iostream::imbue(out_, loc);
        }
    }    // namespace detail
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
