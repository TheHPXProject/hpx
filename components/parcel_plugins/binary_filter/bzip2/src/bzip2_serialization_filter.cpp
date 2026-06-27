//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_COMPRESSION_BZIP2)
#include <hpx/modules/actions.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/iostream.hpp>

#include <hpx/binary_filter/bzip2_serialization_filter.hpp>
#include <hpx/modules/plugin_factories.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_PLUGIN_MODULE();
HPX_REGISTER_BINARY_FILTER_FACTORY(
    hpx::plugins::compression::bzip2_serialization_filter,
    bzip2_serialization_filter);

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins::compression {

    namespace detail {

        class bzip2_compdecomp
          : public hpx::iostream::detail::bzip2_base
          , public hpx::iostream::detail::bzip2_allocator<std::allocator<char>>
        {
            using allocator_type =
                hpx::iostream::detail::bzip2_allocator<std::allocator<char>>;

        public:
            bzip2_compdecomp();    // used for decompression
            explicit bzip2_compdecomp(bool compress,
                hpx::iostream::bzip2_params const& params =
                    hpx::iostream::bzip2_params());
            ~bzip2_compdecomp();

            bool save(char const*& src_begin, char const* src_end,
                char*& dest_begin, char* dest_end, bool flush = false);
            bool load(char const*& begin_in, char const* end_in,
                char*& begin_out, char* end_out);

            void close();

            bool eof() const noexcept
            {
                return eof_;
            }

        protected:
            void init()
            {
                hpx::iostream::detail::bzip2_base::init(
                    compress_, static_cast<allocator_type&>(*this));
            }

        private:
            bool compress_;
            bool eof_;
        };

        bzip2_compdecomp::bzip2_compdecomp()
          : hpx::iostream::detail::bzip2_base(hpx::iostream::bzip2_params(
                hpx::iostream::bzip2::default_small))
          , compress_(false)
          , eof_(false)
        {
        }

        bzip2_compdecomp::bzip2_compdecomp(
            bool compress, hpx::iostream::bzip2_params const& params)
          : hpx::iostream::detail::bzip2_base(params)
          , compress_(compress)
          , eof_(false)
        {
        }

        bzip2_compdecomp::~bzip2_compdecomp()
        {
            close();
        }

        bool bzip2_compdecomp::save(char const*& src_begin, char const* src_end,
            char*& dest_begin, char* dest_end, bool flush)
        {
            using namespace hpx::iostream;
            using namespace hpx::iostream::bzip2;

            if (!ready())
                init();
            if (eof_)
                return false;

            before(src_begin, src_end, dest_begin, dest_end);
            int result = compress(flush ? finish : run);
            after(src_begin, dest_begin);
            (bzip2_error::check)(result);
            return !(eof_ = (result == stream_end));
        }

        bool bzip2_compdecomp::load(char const*& src_begin, char const* src_end,
            char*& dest_begin, char* dest_end)
        {
            using namespace hpx::iostream;
            using namespace hpx::iostream::bzip2;

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
            if (result == ok)
                result = check_end(src_begin, dest_begin);
            after(src_begin, dest_begin);
            (bzip2_error::check)(result);
            eof_ = (result == stream_end);
            return true;
        }

        void bzip2_compdecomp::close()
        {
            try
            {
                end(compress_);
            }
            catch (...)
            {
                eof_ = false;
                throw;
            }
            eof_ = false;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    bzip2_serialization_filter::bzip2_serialization_filter() noexcept
      : current_(0)
    {
    }

    bzip2_serialization_filter::bzip2_serialization_filter(
        bool compress, serialization::binary_filter* next_filter) noexcept
      : compdecomp_(std::make_unique<detail::bzip2_compdecomp>(compress))
      , current_(0)
    {
    }

    void bzip2_serialization_filter::set_max_length(std::size_t size)
    {
        buffer_.reserve(size);
    }

    std::size_t bzip2_serialization_filter::load_impl(void* dst,
        std::size_t dst_count, void const* src, std::size_t src_count)
    {
        char const* src_begin = static_cast<char const*>(src);
        char* dst_begin = static_cast<char*>(dst);
        compdecomp_->load(
            src_begin, src_begin + src_count, dst_begin, dst_begin + dst_count);
        return src_begin - static_cast<char const*>(src);
    }

    std::size_t bzip2_serialization_filter::init_data(
        void const* buffer, std::size_t size, std::size_t buffer_size)
    {
        buffer_.resize(buffer_size);
        std::size_t s = load_impl(buffer_.data(), buffer_size, buffer, size);
        if (s > size)
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "bzip2_serialization_filter::load",
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
    void bzip2_serialization_filter::load(void* dst, std::size_t dst_count)
    {
        if (current_ + dst_count > buffer_.size())
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "bzip2_serialization_filter::load",
                "archive data bstream is too short");
            return;
        }

        std::memcpy(dst, &buffer_[current_], dst_count);
        current_ += dst_count;
    }

    ///////////////////////////////////////////////////////////////////////////
    void bzip2_serialization_filter::save(
        void const* src, std::size_t src_count)
    {
        char const* src_begin = static_cast<char const*>(src);
        std::copy(
            src_begin, src_begin + src_count, std::back_inserter(buffer_));
    }

    ///////////////////////////////////////////////////////////////////////////
    bool bzip2_serialization_filter::flush(
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
