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

#include <ios>
#include <memory>
#include <new>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream {

    namespace bzip2 {

        // Typedefs.

        HPX_CXX_CORE_EXPORT using alloc_func = void * (*) (void*, int, int);
        HPX_CXX_CORE_EXPORT using free_func = void (*)(void*, void*);

        // Status codes
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const ok;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const run_ok;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const flush_ok;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const finish_ok;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const stream_end;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const sequence_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const param_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const mem_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const data_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const data_error_magic;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const io_error;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const unexpected_eof;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const outbuff_full;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const config_error;

        // Action codes
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const finish;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern int const run;

        // Default values
        HPX_CXX_CORE_EXPORT inline constexpr int default_block_size = 9;
        HPX_CXX_CORE_EXPORT inline constexpr int default_work_factor = 30;
        HPX_CXX_CORE_EXPORT inline constexpr bool default_small = false;
    }    // End namespace bzip2.

    //
    // Class name: bzip2_params.
    // Description: Encapsulates the parameters passed to deflateInit2
    //      to customize compression.
    //
    HPX_CXX_CORE_EXPORT struct bzip2_params
    {
        // Non-explicit constructor for compression.
        explicit bzip2_params(int const block_size = bzip2::default_block_size,
            int const work_factor = bzip2::default_work_factor)
          : block_size(block_size)
          , work_factor(work_factor)
        {
        }

        // Constructor for decompression.
        explicit bzip2_params(bool const small_)
          : small_(small_)
          , work_factor(0)
        {
        }

        union
        {
            int block_size;    // For compression.
            bool small_;       // For decompression.
        };
        int work_factor;
    };

    //
    // Class name: bzip2_error.
    // Description: Subclass of std::ios_base::failure thrown to indicate
    //     bzip2 errors other than out-of-memory conditions.
    //
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT bzip2_error
      : public std::ios::failure
    {
    public:
        explicit bzip2_error(int error);

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
        struct bzip2_allocator_traits
        {
            using type =
                std::allocator_traits<Alloc>::template rebind_alloc<char>;
        };

        HPX_CXX_CORE_EXPORT template <typename Alloc>
        struct bzip2_allocator : private bzip2_allocator_traits<Alloc>::type
        {
        private:
            using base_type = bzip2_allocator_traits<Alloc>::type;
            using size_type = std::allocator_traits<base_type>::size_type;

        public:
            static constexpr bool custom =
                !std::is_same_v<std::allocator<char>, base_type>;
            using allocator_type = bzip2_allocator_traits<Alloc>::type;

            static void* allocate(void* self, int items, int size);
            static void deallocate(void* self, void* address);
        };

        HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT bzip2_base
        {
        public:
            using char_type = char;

        protected:
            explicit bzip2_base(bzip2_params const& params);
            ~bzip2_base();

            bzip2_params& params()
            {
                return params_;
            }

            bool& ready()
            {
                return ready_;
            }

            template <typename Alloc>
            void init(bool const compress, bzip2_allocator<Alloc>& alloc)
            {
                bool custom = bzip2_allocator<Alloc>::custom;
                do_init(compress, custom ? bzip2_allocator<Alloc>::allocate : 0,
                    custom ? bzip2_allocator<Alloc>::deallocate : 0,
                    custom ? &alloc : 0);
            }

            void before(char const*& src_begin, char const* src_end,
                char*& dest_begin, char const* dest_end) const;
            void after(char const*& src_begin, char*& dest_begin) const;

            int check_end(char const* src_begin, char const* dest_begin) const;

            int compress(int action) const;
            int decompress() const;

            int end(bool compress, std::nothrow_t);
            void end(bool compress);

        private:
            void do_init(bool compress, bzip2::alloc_func, bzip2::free_func,
                void* derived);
            bzip2_params params_;
            void* stream_;    // Actual type: bz_stream*.
            bool ready_;
        };

        //
        // Template name: bzip2_compressor_impl
        // Description: Model of SymmetricFilter implementing compression by
        //      delegating to the libbzip2 function BZ_bzCompress.
        //
        HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
        class bzip2_compressor_impl
          : public bzip2_base
          , bzip2_allocator<Alloc>
        {
        public:
            explicit bzip2_compressor_impl(bzip2_params const&);
            ~bzip2_compressor_impl();

            bool filter(char const*& src_begin, char const* src_end,
                char*& dest_begin, char const* dest_end, bool flush);
            void close();

        private:
            void init();

            // Guard to make sure filter() isn't called after it returns false.
            bool eof_;
        };

        //
        // Template name: bzip2_compressor
        // Description: Model of SymmetricFilter implementing decompression by
        //      delegating to the libbzip2 function BZ_bzDecompress.
        //
        HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
        class bzip2_decompressor_impl
          : public bzip2_base
          , bzip2_allocator<Alloc>
        {
        public:
            explicit bzip2_decompressor_impl(
                bool small_ = bzip2::default_small);
            ~bzip2_decompressor_impl();

            bool filter(char const*& src_begin, char const* src_end,
                char*& dest_begin, char const* dest_end, bool flush);
            void close();

        private:
            void init();

            // Guard to make sure filter() isn't called after it returns false.
            bool eof_;
        };
    }    // End namespace detail.

    //
    // Template name: bzip2_compressor
    // Description: Model of InputFilter and OutputFilter implementing
    //      compression using libbzip2.
    //
    HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
    struct basic_bzip2_compressor
      : symmetric_filter<detail::bzip2_compressor_impl<Alloc>, Alloc>
    {
    private:
        using impl_type = detail::bzip2_compressor_impl<Alloc>;
        using base_type = symmetric_filter<impl_type, Alloc>;

    public:
        using char_type = base_type::char_type;
        using category = base_type::category;

        explicit basic_bzip2_compressor(
            bzip2_params const& = bzip2_params(bzip2::default_block_size),
            std::streamsize buffer_size = default_device_buffer_size);
    };

    HPX_CXX_CORE_EXPORT using bzip2_compressor = basic_bzip2_compressor<>;

    //
    // Template name: bzip2_decompressor
    // Description: Model of InputFilter and OutputFilter implementing
    //      decompression using libbzip2.
    //
    HPX_CXX_CORE_EXPORT template <typename Alloc = std::allocator<char>>
    struct basic_bzip2_decompressor
      : symmetric_filter<detail::bzip2_decompressor_impl<Alloc>, Alloc>
    {
    private:
        using impl_type = detail::bzip2_decompressor_impl<Alloc>;
        using base_type = symmetric_filter<impl_type, Alloc>;

    public:
        using char_type = base_type::char_type;
        using category = base_type::category;

        explicit basic_bzip2_decompressor(bool small_ = bzip2::default_small,
            std::streamsize buffer_size = default_device_buffer_size);
    };

    HPX_CXX_CORE_EXPORT using bzip2_decompressor = basic_bzip2_decompressor<>;

    //------------------Implementation of bzip2_allocator-------------------------//
    namespace detail {

        template <typename Alloc>
        void* bzip2_allocator<Alloc>::allocate(
            void* self, int const items, int const size)
        {
            size_type len = items * size;
            char* ptr = static_cast<allocator_type*>(self)->allocate(
                len + sizeof(size_type));
            *reinterpret_cast<size_type*>(ptr) = len;
            return ptr + sizeof(size_type);
        }

        template <typename Alloc>
        void bzip2_allocator<Alloc>::deallocate(void* self, void* address)
        {
            char* ptr = static_cast<char*>(address) - sizeof(size_type);
            size_type len =
                *reinterpret_cast<size_type*>(ptr) + sizeof(size_type);
            static_cast<allocator_type*>(self)->deallocate(ptr, len);
        }

        //------------------Implementation of bzip2_compressor_impl-------------------//
        template <typename Alloc>
        bzip2_compressor_impl<Alloc>::bzip2_compressor_impl(
            bzip2_params const& p)
          : bzip2_base(p)
          , eof_(false)
        {
        }

        template <typename Alloc>
        bzip2_compressor_impl<Alloc>::~bzip2_compressor_impl()
        {
            (void) bzip2_base::end(true, std::nothrow);
        }

        template <typename Alloc>
        bool bzip2_compressor_impl<Alloc>::filter(char const*& src_begin,
            char const* src_end, char*& dest_begin, char const* dest_end,
            bool const flush)
        {
            if (!ready())
                init();
            if (eof_)
                return false;
            before(src_begin, src_end, dest_begin, dest_end);
            int const result = compress(flush ? bzip2::finish : bzip2::run);
            after(src_begin, dest_begin);
            (bzip2_error::check)(result);
            eof_ = result == bzip2::stream_end;
            return !eof_;
        }

        template <typename Alloc>
        void bzip2_compressor_impl<Alloc>::close()
        {
            try
            {
                end(true);
            }
            catch (...)
            {
                eof_ = false;
                throw;
            }
            eof_ = false;
        }

        template <typename Alloc>
        void bzip2_compressor_impl<Alloc>::init()
        {
            bzip2_base::init(true, static_cast<bzip2_allocator<Alloc>&>(*this));
        }

        //------------------Implementation of bzip2_decompressor_impl-----------------//

        template <typename Alloc>
        bzip2_decompressor_impl<Alloc>::bzip2_decompressor_impl(bool small_)
          : bzip2_base(bzip2_params(small_))
          , eof_(false)
        {
        }

        template <typename Alloc>
        bzip2_decompressor_impl<Alloc>::~bzip2_decompressor_impl()
        {
            (void) bzip2_base::end(false, std::nothrow);
        }

        template <typename Alloc>
        bool bzip2_decompressor_impl<Alloc>::filter(char const*& src_begin,
            char const* src_end, char*& dest_begin, char const* dest_end,
            bool const flush)
        {
            do
            {
                if (eof_)
                {
                    // reset the stream if there are more characters
                    if (src_begin == src_end)
                        return false;
                    else
                        close();
                }
                if (!ready())
                    init();
                before(src_begin, src_end, dest_begin, dest_end);
                int result = decompress();
                if (result == bzip2::ok && flush)
                    result = check_end(src_begin, dest_begin);
                after(src_begin, dest_begin);
                (bzip2_error::check)(result);
                eof_ = result == bzip2::stream_end;
            } while (eof_ && src_begin != src_end && dest_begin != dest_end);
            return true;
        }

        template <typename Alloc>
        void bzip2_decompressor_impl<Alloc>::close()
        {
            try
            {
                end(false);
            }
            catch (...)
            {
                eof_ = false;
                throw;
            }
            eof_ = false;
        }

        template <typename Alloc>
        void bzip2_decompressor_impl<Alloc>::init()
        {
            bzip2_base::init(
                false, static_cast<bzip2_allocator<Alloc>&>(*this));
        }
    }    // End namespace detail.

    //------------------Implementation of bzip2_decompressor----------------------//
    template <typename Alloc>
    basic_bzip2_compressor<Alloc>::basic_bzip2_compressor(
        bzip2_params const& p, std::streamsize buffer_size)
      : base_type(buffer_size, p)
    {
    }

    //------------------Implementation of bzip2_decompressor----------------------//
    template <typename Alloc>
    basic_bzip2_decompressor<Alloc>::basic_bzip2_decompressor(
        bool small_, std::streamsize buffer_size)
      : base_type(buffer_size, small_)
    {
    }
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
