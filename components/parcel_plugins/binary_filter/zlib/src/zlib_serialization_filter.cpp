//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_COMPRESSION_ZLIB)
#include <hpx/modules/actions.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/iostream.hpp>

#include <hpx/binary_filter/zlib_serialization_filter.hpp>
#include <hpx/modules/plugin_factories.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_PLUGIN_MODULE();
HPX_REGISTER_BINARY_FILTER_FACTORY(
    hpx::plugins::compression::zlib_serialization_filter,
    zlib_serialization_filter);

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins::compression {

    namespace detail {

        class zlib_compdecomp
          : public hpx::iostream::detail::zlib_base
          , public hpx::iostream::detail::zlib_allocator<std::allocator<char>>
        {
            using allocator_type =
                hpx::iostream::detail::zlib_allocator<std::allocator<char>>;

        public:
            explicit zlib_compdecomp(bool compress = false,
                hpx::iostream::zlib_params const& params =
                    hpx::iostream::zlib_params{
                        hpx::iostream::zlib::default_compression});
            ~zlib_compdecomp();

            bool save(char const*& src_begin, char const* src_end,
                char*& dest_begin, char* dest_end, bool flush = false);
            bool load(char const*& begin_in, char const* end_in,
                char*& begin_out, char* end_out);

            void close();

            bool eof() const noexcept
            {
                return eof_;
            }

        private:
            bool compress_;
            bool eof_;
        };

        zlib_compdecomp::zlib_compdecomp(
            bool compress, hpx::iostream::zlib_params const& params)
          : compress_(compress)
          , eof_(false)
        {
            init(params, compress_, static_cast<allocator_type&>(*this));
        }

        zlib_compdecomp::~zlib_compdecomp()
        {
            reset(compress_, false);
        }

        bool zlib_compdecomp::save(char const*& src_begin, char const* src_end,
            char*& dest_begin, char* dest_end, bool flush)
        {
            using namespace hpx::iostream;
            using namespace hpx::iostream::zlib;

            before(src_begin, src_end, dest_begin, dest_end);
            int result = this->xdeflate(flush ? finish : no_flush);
            after(src_begin, dest_begin, true);
            (zlib_error::check)(result);
            return !(eof_ = (result == stream_end));
        }

        bool zlib_compdecomp::load(char const*& src_begin, char const* src_end,
            char*& dest_begin, char* dest_end)
        {
            using namespace hpx::iostream;
            using namespace hpx::iostream::zlib;

            before(src_begin, src_end, dest_begin, dest_end);
            int result = this->xinflate(sync_flush);
            after(src_begin, dest_begin, false);
            (zlib_error::check)(result);
            return !(eof_ = (result == stream_end));
        }

        void zlib_compdecomp::close()
        {
            eof_ = false;
            reset(compress_, true);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    zlib_serialization_filter::zlib_serialization_filter(
        bool compress, serialization::binary_filter* next_filter) noexcept
      : compdecomp_(std::make_unique<detail::zlib_compdecomp>(compress))
      , current_(0)
    {
    }

    void zlib_serialization_filter::set_max_length(std::size_t size)
    {
        buffer_.reserve(size);
    }

    std::size_t zlib_serialization_filter::load_impl(void* dst,
        std::size_t dst_count, void const* src, std::size_t src_count)
    {
        char const* src_begin = static_cast<char const*>(src);
        char* dst_begin = static_cast<char*>(dst);
        compdecomp_->load(
            src_begin, src_begin + src_count, dst_begin, dst_begin + dst_count);
        return src_begin - static_cast<char const*>(src);
    }

    std::size_t zlib_serialization_filter::init_data(
        void const* buffer, std::size_t size, std::size_t buffer_size)
    {
        buffer_.resize(buffer_size);
        std::size_t s = load_impl(buffer_.data(), buffer_size, buffer, size);
        if (s > size)
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "zlib_serialization_filter::load",
                hpx::util::format(
                    "decompression failure, number of "
                    "bytes expected: {}, number of bytes decoded: {}",
                    size, s));
            return 0;
        }

        current_ = 0;
        return buffer_.size();
    }

    ///////////////////////////////////////////////////////////////////////////
    void zlib_serialization_filter::load(void* dst, std::size_t dst_count)
    {
        if (current_ + dst_count > buffer_.size())
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "zlib_serialization_filter::load",
                "archive data bstream is too short");
            return;
        }

        std::memcpy(dst, &buffer_[current_], dst_count);
        current_ += dst_count;
    }

    ///////////////////////////////////////////////////////////////////////////
    void zlib_serialization_filter::save(void const* src, std::size_t src_count)
    {
        char const* src_begin = static_cast<char const*>(src);
        std::copy(
            src_begin, src_begin + src_count, std::back_inserter(buffer_));
    }

    ///////////////////////////////////////////////////////////////////////////
    bool zlib_serialization_filter::flush(
        void* dst, std::size_t dst_count, std::size_t& written)
    {
        // compress everything in one go
        char* dst_begin = static_cast<char*>(dst);
        char const* src_begin = buffer_.data();
        bool eof = compdecomp_->save(src_begin, src_begin + buffer_.size(),
            dst_begin, dst_begin + dst_count, true);

        written = dst_begin - static_cast<char*>(dst);
        return !eof;
    }
}    // namespace hpx::plugins::compression

#endif
