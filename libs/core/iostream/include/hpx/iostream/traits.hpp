//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

//
// Contains metafunctions char_type_of, category_of and mode_of used for
// deducing the i/o category and i/o mode of a model of Filter or Device.
//
// Also contains several utility metafunctions, functions and macros.
//

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/categories.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <iosfwd>
#include <iterator>
#include <memory>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    //------------------Definitions of char_type_of-------------------------------//
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct member_char_type
        {
            using type = typename T::char_type;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    struct char_type_of : detail::member_char_type<util::unwrap_reference_t<T>>
    {
    };

    template <typename Iter>
    struct char_type_of<util::iterator_range<Iter>>
    {
        using type = typename std::iterator_traits<Iter>::value_type;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using char_type_of_t = char_type_of<T>::type;

    //------------------Definition of int_type_of---------------------------------//
    HPX_CXX_CORE_EXPORT template <typename T>
    struct int_type_of
    {
        using traits_type = std::char_traits<char_type_of_t<T>>;
        using type = traits_type::int_type;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using int_type_of_t = int_type_of<T>::type;

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T,
            template <class, class> typename Base>
        struct is_iostreams_trait2
        {
        private:
            template <typename T1, typename T2>
            static std::true_type test(Base<T1, T2> const volatile*);
            static std::false_type test(...);

        public:
            static constexpr bool value =
                decltype(test(static_cast<T*>(nullptr)))::value;
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            template <class, class, class> typename Base>
        struct is_iostreams_trait3
        {
        private:
            template <typename T1, typename T2, typename T3>
            static std::true_type test(Base<T1, T2, T3> const volatile*);
            static std::false_type test(...);

        public:
            static constexpr bool value =
                decltype(test(static_cast<T*>(nullptr)))::value;
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            template <class, class, class, class> typename Base>
        struct is_iostreams_trait4
        {
        private:
            template <typename T1, typename T2, typename T3, typename T4>
            static std::true_type test(Base<T1, T2, T3, T4> const volatile*);
            static std::false_type test(...);

        public:
            static constexpr bool value =
                decltype(test(static_cast<T*>(nullptr)))::value;
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            template <class, class, class, class, class> typename Base>
        struct is_iostreams_trait5
        {
        private:
            template <typename T1, typename T2, typename T3, typename T4,
                typename T5>
            static std::true_type test(
                Base<T1, T2, T3, T4, T5> const volatile*);
            static std::false_type test(...);

        public:
            static constexpr bool value =
                decltype(test(static_cast<T*>(nullptr)))::value;
        };
    }    // namespace detail

    //----------Definitions of predicates for streams and stream buffers----------//
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_istream
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_istream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_istream_v = is_istream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_ostream
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_ostream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_ostream_v = is_ostream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_iostream
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_iostream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_iostream_v = is_iostream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_streambuf
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_streambuf>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_streambuf_v = is_streambuf<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_filebuf
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_filebuf>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_filebuf_v = is_filebuf<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_ifstream
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_ifstream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_ifstream_v = is_ifstream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_ofstream
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_ofstream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_ofstream_v = is_ofstream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_fstream
      : std::bool_constant<
            detail::is_iostreams_trait2<T, std::basic_fstream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_fstream_v = is_fstream<T>::value;

    // clang-format off
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_istringstream
      : std::bool_constant<
            detail::is_iostreams_trait3<T, std::basic_istringstream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_istringstream_v = is_istringstream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_ostringstream
      : std::bool_constant<
            detail::is_iostreams_trait3<T, std::basic_ostringstream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_ostringstream_v = is_ostringstream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_stringstream
      : std::bool_constant<
            detail::is_iostreams_trait3<T, std::basic_stringstream>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_stringstream_v = is_stringstream<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_stringbuf
      : std::bool_constant<
            detail::is_iostreams_trait3<T, std::basic_stringbuf>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_stringbuf_v = is_stringbuf<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_std_io
      : std::disjunction<is_istream<T>, is_ostream<T>, is_streambuf<T>>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_std_io_v = is_std_io<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_std_file_device
      : std::disjunction<is_ifstream<T>, is_ofstream<T>, is_fstream<T>,
            is_filebuf<T>>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_std_file_device_v = is_std_file_device<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_std_string_device
      : std::disjunction<is_istringstream<T>, is_ostringstream<T>,
            is_stringstream<T>, is_stringbuf<T>>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_std_string_device_v =
        is_std_string_device<T>::value;

    HPX_CXX_CORE_EXPORT template <typename Device,
        typename Tr = std::char_traits<char_type_of_t<Device>>,
        typename Alloc = std::allocator<char_type_of_t<Device>>>
    struct stream;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct mode_of;

    HPX_CXX_CORE_EXPORT template <typename T,
        typename Tr = std::char_traits<char_type_of_t<T>>,
        typename Alloc = std::allocator<char_type_of_t<T>>,
        typename Mode = typename mode_of<T>::type>
    class stream_buffer;

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Tr,
        typename Alloc, typename Access>
    class filtering_stream;

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Tr,
        typename Alloc, typename Access>
    class filtering_streambuf;

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Tr>
        class linked_streambuf;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_iostreams_stream
          : std::bool_constant<
                is_iostreams_trait3<T, stream>::value>
        {
        };
        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool is_iostreams_stream_v =
            is_iostreams_stream<T>::value;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_iostreams_stream_buffer
          : std::bool_constant<
                is_iostreams_trait4<T, stream_buffer>::value>
        {
        };
        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool is_iostreams_stream_buffer_v =
            is_iostreams_stream_buffer<T>::value;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_filtering_stream
          : std::bool_constant<
                is_iostreams_trait5<T, filtering_stream>::value>
        {
        };
        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool is_filtering_stream_v =
            is_filtering_stream<T>::value;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_filtering_streambuf
          : std::bool_constant<
                is_iostreams_trait5<T, filtering_streambuf>::value>
        {
        };
        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool is_filtering_streambuf_v =
            is_filtering_streambuf<T>::value;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_linked
          : std::bool_constant<
                is_iostreams_trait2<T, linked_streambuf>::value>
        {
        };
        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool is_linked_v = is_linked<T>::value;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_iostreams
          : std::disjunction<is_iostreams_stream<T>,
                is_iostreams_stream_buffer<T>, is_filtering_stream<T>,
                is_filtering_streambuf<T>>
        {
        };
        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool is_iostreams_v = is_iostreams<T>::value;
    }    // namespace detail

    //------------------Definitions of category_of--------------------------------//
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct member_category
        {
            using type = T::category;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    struct category_of
    {
        using U = util::unwrap_reference_t<T>;

        // clang-format off
        using type =
            util::lazy_conditional<
                is_std_io_v<U> && !detail::is_iostreams_v<U>,
                util::select<
                    is_filebuf<U>, filebuf_tag,
                    is_ifstream<U>, ifstream_tag,
                    is_ofstream<U>, ofstream_tag,
                    is_fstream<U>, fstream_tag,
                    is_stringbuf<U>, stringbuf_tag,
                    is_istringstream<U>, istringstream_tag,
                    is_ostringstream<U>, ostringstream_tag,
                    is_stringstream<U>, stringstream_tag,
                    is_streambuf<U>, generic_streambuf_tag,
                    is_iostream<U>, generic_iostream_tag,
                    is_istream<U>, generic_istream_tag,
                    is_ostream<U>, generic_ostream_tag
                >,
                detail::member_category<U>
            >::type;
        // clang-format on
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using category_of_t = category_of<T>::type;

    //------------------Definition of get_category--------------------------------//
    //
    // Returns an object of type category_of<T>::type.
    //
    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr category_of_t<T> get_category(T const&) noexcept
    {
        return category_of_t<T>();
    }

    //------------------Definition of mode_of-------------------------------------//
    namespace detail {

        HPX_CXX_CORE_EXPORT template <int N>
        struct int_case : std::integral_constant<int, N>
        {
        };

        // clang-format off
        HPX_CXX_CORE_EXPORT int_case<1> io_mode(input);
        HPX_CXX_CORE_EXPORT int_case<2> io_mode(output);
        HPX_CXX_CORE_EXPORT int_case<3> io_mode(bidirectional);
        HPX_CXX_CORE_EXPORT int_case<4> io_mode(input_seekable);
        HPX_CXX_CORE_EXPORT int_case<5> io_mode(output_seekable);
        HPX_CXX_CORE_EXPORT int_case<6> io_mode(seekable);
        HPX_CXX_CORE_EXPORT int_case<7> io_mode(dual_seekable);
        HPX_CXX_CORE_EXPORT int_case<8> io_mode(bidirectional_seekable);
        HPX_CXX_CORE_EXPORT int_case<9> io_mode(dual_use);

        HPX_CXX_CORE_EXPORT template <int N>
        struct io_mode_impl;

        template <> struct io_mode_impl<1> { using type = input; };
        template <> struct io_mode_impl<2> { using type = output; };
        template <> struct io_mode_impl<3> { using type = bidirectional; };
        template <> struct io_mode_impl<4> { using type = input_seekable; };
        template <> struct io_mode_impl<5> { using type = output_seekable; };
        template <> struct io_mode_impl<6> { using type = seekable; };
        template <> struct io_mode_impl<7> { using type = dual_seekable; };
        template <> struct io_mode_impl<8> { using type = bidirectional_seekable; };
        template <> struct io_mode_impl<9> { using type = dual_use; };
        // clang-format on

        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr int io_mode_id =
            decltype(io_mode(category_of_t<T>()))::value;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    struct mode_of
      : detail::io_mode_impl<detail::io_mode_id<util::unwrap_reference_t<T>>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using mode_of_t = mode_of<T>::type;

    //------------------Definition of is_device, is_filter and is_direct----------//
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Tag>
        struct has_trait : std::is_convertible<category_of_t<T>, Tag>
        {
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_device : detail::has_trait<T, device_tag>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_device_v = is_device<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_filter : detail::has_trait<T, filter_tag>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_filter_v = is_filter<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_direct : detail::has_trait<T, direct_tag>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_direct_v = is_direct<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    struct value_type
    {
        using type = std::conditional_t<is_std_io_v<T>, T&, T>;
    };
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
