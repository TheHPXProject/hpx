//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/iostream.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/components/iostreams/manipulators.hpp>
#include <hpx/components/iostreams/server/output_stream.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <ios>
#include <iterator>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hpx::iostreams {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename Char = char>
        struct buffer_sink;
    }

    template <typename Char = char, typename Sink = detail::buffer_sink<Char>>
    struct ostream;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        // Tag types to be used to identify standard ostream objects
        struct cout_tag
        {
        };

        struct cerr_tag
        {
        };

        struct consolestream_tag
        {
        };

        ///////////////////////////////////////////////////////////////////////
        /// This is a HPX.IoStreams Sink that can be used to create an
        /// [io]stream on top of a detail::buffer.
        template <typename Char>
        struct buffer_sink
        {
            typedef Char char_type;

            struct category
              : hpx::iostream::sink_tag
              , hpx::iostream::flushable_tag
            {
            };

            explicit buffer_sink(ostream<Char, buffer_sink>& os)
              : os_(os)
            {
            }

            // Write up to n characters to the underlying data sink into the
            // buffer s, returning the number of characters written.
            inline std::streamsize write(char_type const* s, std::streamsize n);

            // Make sure all content is sent to console
            inline bool flush();

        private:
            ostream<Char, buffer_sink>& os_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Char = char, typename Sink = buffer_sink<Char>>
        struct ostream_creator
        {
            typedef std::back_insert_iterator<std::vector<Char>> iterator_type;
            typedef Sink device_type;
            typedef hpx::iostream::stream<device_type> stream_type;
        };

        ///////////////////////////////////////////////////////////////////////
        std::ostream& get_coutstream() noexcept;

        inline std::ostream& get_outstream(cout_tag)
        {
            return get_coutstream();
        }

        std::ostream& get_cerrstream() noexcept;

        inline std::ostream& get_outstream(cerr_tag)
        {
            return get_cerrstream();
        }

        std::stringstream& get_consolestream();

        inline std::ostream& get_outstream(consolestream_tag)
        {
            return get_consolestream();
        }

        inline char const* get_outstream_name(cout_tag)
        {
            return "/locality#console/output_stream#cout";
        }

        inline char const* get_outstream_name(cerr_tag)
        {
            return "/locality#console/output_stream#cerr";
        }

        inline char const* get_outstream_name(consolestream_tag)
        {
            return "/locality#console/output_stream#consolestream";
        }

        ///////////////////////////////////////////////////////////////////////
        hpx::future<hpx::id_type> create_ostream(
            char const* name, std::ostream& strm);

        template <typename Tag>
        hpx::future<hpx::id_type> create_ostream(Tag tag)
        {
            return create_ostream(
                get_outstream_name(tag), detail::get_outstream(tag));
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_IOSTREAMS_EXPORT void release_ostream(
            char const* name, hpx::id_type const& id);

        template <typename Tag>
        void release_ostream(Tag tag, hpx::id_type const& id)
        {
            release_ostream(get_outstream_name(tag), id);
        }

        ///////////////////////////////////////////////////////////////////////
        void register_ostreams();
        void unregister_ostreams();
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Sink>
    struct ostream
      : components::client_base<ostream<Char, Sink>, server::output_stream>
      , detail::data_buffer
      , detail::ostream_creator<Char, Sink>::stream_type
    {
        ostream(ostream const&) = delete;
        ostream(ostream&&) = delete;
        ostream& operator=(ostream const&) = delete;
        ostream& operator=(ostream&&) = delete;

    private:
        typedef components::client_base<ostream, server::output_stream>
            base_type;

        typedef detail::ostream_creator<Char, Sink> ostream_creator;
        typedef typename ostream_creator::stream_type stream_base_type;
        typedef typename ostream_creator::iterator_type iterator_type;

        typedef typename stream_base_type::traits_type stream_traits_type;
        typedef std::basic_ostream<Char, stream_traits_type> std_stream_type;
        typedef detail::data_buffer::mutex_type mutex_type;

    private:
        using detail::data_buffer::mtx_;
        std::atomic<std::uint64_t> generational_count_;
        Sink sink;

        // Performs a lazy streaming operation.
        template <typename T>
        ostream& streaming_operator_lazy(T const& subject)
        {
            // apply the subject to the local stream
            *static_cast<stream_base_type*>(this) << subject;
            return *this;
        }

        // Performs an asynchronous streaming operation.
        template <typename T, typename Lock>
        ostream& streaming_operator_async(
            [[maybe_unused]] T const& subject, [[maybe_unused]] Lock& l)
        {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
            // apply the subject to the local stream
            *static_cast<stream_base_type*>(this) << subject;

            // If the buffer isn't empty, send it asynchronously to the
            // destination.
            if (!this->detail::data_buffer::empty_locked())
            {
                // Create the next buffer, returns the previous buffer
                data_buffer next = this->detail::data_buffer::init_locked();

                // Unlock the mutex before we cleanup.
                l.unlock();

                // Perform the write operation, then destroy the old buffer and
                // stream.
                typedef server::output_stream::write_async_action action_type;
                hpx::post<action_type>(this->get_id(), hpx::get_locality_id(),
                    generational_count_++, next);
            }
#else
            HPX_ASSERT(false);
#endif
            return *this;
        }

        // Performs a synchronous streaming operation.
        template <typename T, typename Lock>
        ostream& streaming_operator_sync(
            [[maybe_unused]] T const& subject, [[maybe_unused]] Lock& l)
        {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
            // apply the subject to the local stream
            *static_cast<stream_base_type*>(this) << subject;

            // Send even empty buffer to flush the data buffered server-side.

            // Create the next buffer, returns the previous buffer
            data_buffer next = this->detail::data_buffer::init_locked();

            // 26110: Caller failing to hold lock 'l'
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif
            // Unlock the mutex before we clean up.
            l.unlock();
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

            // Perform the write operation, then destroy the old buffer and
            // stream.
            typedef server::output_stream::write_sync_action action_type;
            hpx::async<action_type>(this->get_id(), hpx::get_locality_id(),
                generational_count_++, next)
                .get();
#else
            HPX_ASSERT(false);
#endif
            return *this;
        }

        ///////////////////////////////////////////////////////////////////////
        friend struct detail::buffer_sink<char>;

        bool flush()
        {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
            std::unique_lock<mutex_type> l(*mtx_);
            if (!this->detail::data_buffer::empty_locked())
            {
                // Create the next buffer, returns the previous buffer
                data_buffer next = this->detail::data_buffer::init_locked();

                // Unlock the mutex before we clean up.
                l.unlock();

                // since mtx_ is recursive and apply will do an AGAS lookup,
                // we need to ignore the lock here in case we are called
                // recursively
                [[maybe_unused]] hpx::util::ignore_while_checking il(&l);

                // Perform the write operation, then destroy the old buffer and
                // stream.
                typedef server::output_stream::write_async_action action_type;
                hpx::post<action_type>(this->get_id(), hpx::get_locality_id(),
                    generational_count_++, next);
            }
            return true;
#else
            HPX_ASSERT(false);
            return false;
#endif
        }

        ///////////////////////////////////////////////////////////////////////
        friend void detail::register_ostreams();
        friend void detail::unregister_ostreams();

    public:
        // late initialization during runtime system startup
        template <typename Tag>
        void initialize(Tag tag)
        {
            *static_cast<base_type*>(this) = detail::create_ostream(tag);
        }

        // reset this object during runtime system shutdown
        template <typename Tag>
        void uninitialize(Tag tag)
        {
            std::unique_lock<mutex_type> l(*mtx_, std::try_to_lock);
            if (l)
            {
                streaming_operator_sync(
                    hpx::iostreams::flush_type(), l);    // unlocks
            }

            // FIXME: find a later spot to invoke this
            detail::release_ostream(tag, this->get_id());
            this->base_type::free();
        }

    public:
        ostream()
          : base_type()
          , data_buffer()
          , stream_base_type()
          , generational_count_(0)
          , sink(*this)
        {
            stream_base_type::open(sink);
        }

        // hpx::flush manipulator
        ostream& operator<<(hpx::iostreams::flush_type const& m)
        {
            std::unique_lock<mutex_type> l(*mtx_);
            return streaming_operator_sync(m, l);
        }

        // hpx::endl manipulator
        ostream& operator<<(hpx::iostreams::endl_type const& m)
        {
            std::unique_lock<mutex_type> l(*mtx_);
            return streaming_operator_sync(m, l);
        }

        // hpx::async_flush manipulator
        ostream& operator<<(hpx::iostreams::async_flush_type const& m)
        {
            std::unique_lock<mutex_type> l(*mtx_);
            return streaming_operator_async(m, l);
        }

        // hpx::async_endl manipulator
        ostream& operator<<(hpx::iostreams::async_endl_type const& m)
        {
            std::unique_lock<mutex_type> l(*mtx_);
            return streaming_operator_async(m, l);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        ostream& operator<<(T const& subject)
        {
            std::lock_guard<mutex_type> l(*mtx_);
            return streaming_operator_lazy(subject);
        }

        ///////////////////////////////////////////////////////////////////////
        ostream& operator<<(std_stream_type& (*manip_fun)(std_stream_type&) )
        {
            std::unique_lock l(*mtx_);
            [[maybe_unused]] util::ignore_while_checking ignore(&l);

            return streaming_operator_lazy(manip_fun);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename Char>
        inline std::streamsize buffer_sink<Char>::write(
            Char const* s, std::streamsize n)
        {
            return static_cast<data_buffer&>(os_).write(s, n);
        }

        template <typename Char>
        inline bool buffer_sink<Char>::flush()
        {
            return os_.flush();
        }
    }    // namespace detail
}    // namespace hpx::iostreams

namespace hpx::util {

    // TODO: This overload should not be needed. See #3175.
    template <typename Char, typename Sink, typename... Args>
    hpx::iostreams::ostream<Char, Sink>& format_to(
        hpx::iostreams::ostream<Char, Sink>& os, std::string const& format_str,
        Args const&... args)
    {
        return os << hpx::util::format(format_str, args...);
    }
}    // namespace hpx::util
