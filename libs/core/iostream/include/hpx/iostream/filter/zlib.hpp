//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/constants.hpp>
#include <hpx/iostream/filter/symmetric.hpp>

#include <cstdint>
#include <ios>
#include <memory>
#include <new>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    namespace zlib {

        // Typedefs
        HPX_CXX_CORE_EXPORT using uint = std::uint32_t;
        HPX_CXX_CORE_EXPORT using byte = std::uint8_t;
        HPX_CXX_CORE_EXPORT using ulong = std::uint32_t;

        // Prefix 'x' prevents symbols from being redefined when Z_PREFIX is defined
        HPX_CXX_CORE_EXPORT using xalloc_func =
            void * (*) (void*, zlib::uint, zlib::uint);
        HPX_CXX_CORE_EXPORT using xfree_func = void (*)(void*, void*);

        // Compression levels
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const no_compression;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const best_speed;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const best_compression;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const
            default_compression;

        // Compression methods
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const deflated;

        // Compression strategies
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const default_strategy;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const filtered;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const huffman_only;

        // Status codes
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const okay;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const stream_end;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const stream_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const version_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const data_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const mem_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const buf_error;

        // Flush codes
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const finish;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const no_flush;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const sync_flush;

        // Null pointer constant.
        HPX_CXX_CORE_EXPORT constexpr int null = 0;

        // Default values
        HPX_CXX_CORE_EXPORT inline constexpr int default_window_bits = 15;
        HPX_CXX_CORE_EXPORT inline constexpr int default_mem_level = 8;
        HPX_CXX_CORE_EXPORT inline constexpr bool default_crc = false;
        HPX_CXX_CORE_EXPORT inline constexpr bool default_noheader = false;
    }    // End namespace zlib.

    //
    // Class name: zlib_params.
    // Description: Encapsulates the parameters passed to deflateInit2
    //      and inflateInit2 to customize compression and decompression.
    //
    HPX_CXX_CORE_EXPORT struct zlib_params
    {
        // Non-explicit constructor.
        explicit zlib_params(int const level_ = zlib::default_compression,
            int const method_ = zlib::deflated,
            int const window_bits_ = zlib::default_window_bits,
            int const mem_level_ = zlib::default_mem_level,
            int const strategy_ = zlib::default_strategy,
            bool const noheader_ = zlib::default_noheader,
            bool const calculate_crc_ = zlib::default_crc)
          : level(level_)
          , method(method_)
          , window_bits(window_bits_)
          , mem_level(mem_level_)
          , strategy(strategy_)
          , noheader(noheader_)
          , calculate_crc(calculate_crc_)
        {
        }

        int level;
        int method;
        int window_bits;
        int mem_level;
        int strategy;
        bool noheader;
        bool calculate_crc;
    };

    //
    // Class name: zlib_error.
    // Description: Subclass of std::ios::failure thrown to indicate
    //     zlib errors other than out-of-memory conditions.
    //
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT zlib_error
      : public std::ios_base::failure
    {
    public:
        explicit zlib_error(int error);

        constexpr int error() const noexcept
        {
            return error_;
        }

        static void(check)(int error);

    private:
        int error_;
    };

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Alloc>
        struct zlib_allocator_traits
        {
            using type =
                std::allocator_traits<Alloc>::template rebind_alloc<char>;
        };

        HPX_CXX_CORE_EXPORT template <typename Alloc>
        struct zlib_allocator : private zlib_allocator_traits<Alloc>::type
        {
        private:
            using base_type = zlib_allocator_traits<Alloc>::type;
            using size_type = std::allocator_traits<base_type>::size_type;

        public:
            static constexpr bool custom =
                !std::is_same_v<std::allocator<char>, base_type>;

            using allocator_type = zlib_allocator_traits<Alloc>::type;

            static void* allocate(
                void* self, zlib::uint items, zlib::uint size);
            static void deallocate(void* self, void* address);
        };

        HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT zlib_base
        {
        public:
            using char_type = char;

        protected:
            zlib_base();
            ~zlib_base();

            void* stream() const
            {
                return stream_;
            }

            template <typename Alloc>
            void init(zlib_params const& p, bool const compress,
                zlib_allocator<Alloc>& zalloc)
            {
                bool custom = zlib_allocator<Alloc>::custom;
                do_init(p, compress,
                    custom ? zlib_allocator<Alloc>::allocate : 0,
                    custom ? zlib_allocator<Alloc>::deallocate : 0, &zalloc);
            }

            void before(char const*& src_begin, char const* src_end,
                char*& dest_begin, char const* dest_end) const;
            void after(
                char const*& src_begin, char*& dest_begin, bool compress);

            // Prefix 'x' prevents symbols from being redefined when Z_PREFIX is
            // defined
            int xdeflate(int flush) const;
            int xinflate(int flush) const;

            void reset(bool compress, bool realloc);

        public:
            [[nodiscard]] zlib::ulong crc() const
            {
                return crc_;
            }

            [[nodiscard]] int total_in() const
            {
                return total_in_;
            }

            [[nodiscard]] int total_out() const
            {
                return total_out_;
            }

        private:
            void do_init(zlib_params const& p, bool compress, zlib::xalloc_func,
                zlib::xfree_func, void* derived);

            void* stream_;    // Actual type: z_stream*.
            bool calculate_crc_;
            zlib::ulong crc_;
            zlib::ulong crc_imp_;
            zlib::ulong total_in_;
            zlib::ulong total_out_;
        };

        //
        // Template name: zlib_compressor_impl
        // Description: Model of C-Style Filter implementing compression by
        //      delegating to the zlib function deflate.
        //
        HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
        class zlib_compressor_impl
          : public zlib_base
          , public zlib_allocator<Alloc>
        {
        public:
            explicit zlib_compressor_impl(
                zlib_params const& = zlib_params(zlib::default_compression));
            ~zlib_compressor_impl();

            bool filter(char const*& src_begin, char const* src_end,
                char*& dest_begin, char* dest_end, bool flush);
            void close();
        };

        //
        // Template name: zlib_compressor
        // Description: Model of C-Style Filter implementing decompression by
        //      delegating to the zlib function inflate.
        //
        HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
        class zlib_decompressor_impl
          : public zlib_base
          , public zlib_allocator<Alloc>
        {
        public:
            explicit zlib_decompressor_impl(zlib_params const&);
            explicit zlib_decompressor_impl(
                int window_bits = zlib::default_window_bits);
            ~zlib_decompressor_impl();

            bool filter(char const*& src_begin, char const* src_end,
                char*& dest_begin, char* dest_end, bool flush);
            void close();

            [[nodiscard]] bool eof() const
            {
                return eof_;
            }

        private:
            bool eof_;
        };
    }    // namespace detail

    //
    // Template name: zlib_compressor
    // Description: Model of InputFilter and OutputFilter implementing
    //      compression using zlib.
    //
    HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
    struct basic_zlib_compressor
      : symmetric_filter<detail::zlib_compressor_impl<Alloc>, Alloc>
    {
    private:
        using impl_type = detail::zlib_compressor_impl<Alloc>;
        using base_type = symmetric_filter<impl_type, Alloc>;

    public:
        using char_type = base_type::char_type;
        using category = base_type::category;

        explicit basic_zlib_compressor(
            zlib_params const& = zlib_params(zlib::default_compression),
            std::streamsize buffer_size = default_device_buffer_size);
        zlib::ulong crc()
        {
            return this->filter().crc();
        }
        int total_in()
        {
            return this->filter().total_in();
        }
    };

    HPX_CXX_CORE_EXPORT using zlib_compressor = basic_zlib_compressor<>;

    //
    // Template name: zlib_decompressor
    // Description: Model of InputFilter and OutputFilter implementing
    //      decompression using zlib.
    //
    HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
    struct basic_zlib_decompressor
      : symmetric_filter<detail::zlib_decompressor_impl<Alloc>, Alloc>
    {
    private:
        using impl_type = detail::zlib_decompressor_impl<Alloc>;
        using base_type = symmetric_filter<impl_type, Alloc>;

    public:
        using char_type = base_type::char_type;
        using category = base_type::category;

        basic_zlib_decompressor(int window_bits = zlib::default_window_bits,
            std::streamsize buffer_size = default_device_buffer_size);
        basic_zlib_decompressor(zlib_params const& p,
            std::streamsize buffer_size = default_device_buffer_size);

        zlib::ulong crc()
        {
            return this->filter().crc();
        }

        int total_out()
        {
            return this->filter().total_out();
        }

        bool eof()
        {
            return this->filter().eof();
        }
    };

    HPX_CXX_CORE_EXPORT using zlib_decompressor = basic_zlib_decompressor<>;

    //------------------Implementation of zlib_allocator--------------------------//
    namespace detail {

        template <typename Alloc>
        void* zlib_allocator<Alloc>::allocate(
            void* self, zlib::uint const items, zlib::uint const size)
        {
            size_type len = items * size;
            char* ptr = static_cast<allocator_type*>(self)->allocate(
                len + sizeof(size_type));
            *reinterpret_cast<size_type*>(ptr) = len;
            return ptr + sizeof(size_type);
        }

        template <typename Alloc>
        void zlib_allocator<Alloc>::deallocate(void* self, void* address)
        {
            char* ptr = static_cast<char*>(address) - sizeof(size_type);
            size_type len =
                *reinterpret_cast<size_type*>(ptr) + sizeof(size_type);
            static_cast<allocator_type*>(self)->deallocate(ptr, len);
        }

        //------------------Implementation of zlib_compressor_impl--------------------//
        template <typename Alloc>
        zlib_compressor_impl<Alloc>::zlib_compressor_impl(zlib_params const& p)
        {
            init(p, true, static_cast<zlib_allocator<Alloc>&>(*this));
        }

        template <typename Alloc>
        zlib_compressor_impl<Alloc>::~zlib_compressor_impl()
        {
            reset(true, false);
        }

        template <typename Alloc>
        bool zlib_compressor_impl<Alloc>::filter(char const*& src_begin,
            char const* src_end, char*& dest_begin, char* dest_end,
            bool const flush)
        {
            before(src_begin, src_end, dest_begin, dest_end);
            int const result = xdeflate(flush ? zlib::finish : zlib::no_flush);
            after(src_begin, dest_begin, true);
            (zlib_error::check)(result);
            return result != zlib::stream_end;
        }

        template <typename Alloc>
        void zlib_compressor_impl<Alloc>::close()
        {
            reset(true, true);
        }

        //------------------Implementation of zlib_decompressor_impl------------------//
        template <typename Alloc>
        zlib_decompressor_impl<Alloc>::zlib_decompressor_impl(
            zlib_params const& p)
          : eof_(false)
        {
            init(p, false, static_cast<zlib_allocator<Alloc>&>(*this));
        }

        template <typename Alloc>
        zlib_decompressor_impl<Alloc>::~zlib_decompressor_impl()
        {
            reset(false, false);
        }

        template <typename Alloc>
        zlib_decompressor_impl<Alloc>::zlib_decompressor_impl(
            int const window_bits)
        {
            zlib_params p;
            p.window_bits = window_bits;
            init(p, false, static_cast<zlib_allocator<Alloc>&>(*this));
        }

        template <typename Alloc>
        bool zlib_decompressor_impl<Alloc>::filter(char const*& src_begin,
            char const* src_end, char*& dest_begin, char* dest_end,
            bool /* flush */)
        {
            before(src_begin, src_end, dest_begin, dest_end);
            int const result = xinflate(zlib::sync_flush);
            after(src_begin, dest_begin, false);
            (zlib_error::check)(result);
            eof_ = result == zlib::stream_end;
            return !eof_;
        }

        template <typename Alloc>
        void zlib_decompressor_impl<Alloc>::close()
        {
            eof_ = false;
            reset(false, true);
        }

    }    // namespace detail

    //------------------Implementation of zlib_decompressor-----------------------//
    template <typename Alloc>
    basic_zlib_compressor<Alloc>::basic_zlib_compressor(
        zlib_params const& p, std::streamsize buffer_size)
      : base_type(buffer_size, p)
    {
    }

    //------------------Implementation of zlib_decompressor-----------------------//
    template <typename Alloc>
    basic_zlib_decompressor<Alloc>::basic_zlib_decompressor(
        int window_bits, std::streamsize buffer_size)
      : base_type(buffer_size, window_bits)
    {
    }

    template <typename Alloc>
    basic_zlib_decompressor<Alloc>::basic_zlib_decompressor(
        zlib_params const& p, std::streamsize buffer_size)
      : base_type(buffer_size, p)
    {
    }
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
