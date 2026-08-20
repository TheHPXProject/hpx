////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <hpx/config.hpp>
#include <hpx/functional/bind_front.hpp>

#include <hpx/io_service/io_service_pool.hpp>
#include <hpx/runtime_distributed/runtime_fwd.hpp>
#include <hpx/runtime_local/runtime_local_fwd.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/serialization/shared_ptr.hpp>
#include <hpx/serialization/vector.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_helpers.hpp>

#include <hpx/components/iostreams/server/buffer.hpp>
#include <hpx/components/iostreams/server/output_stream.hpp>

#include <asio/io_context.hpp>
#include <asio/version.hpp>
#if ASIO_VERSION >= 103400
#include <asio/post.hpp>
#endif

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

namespace hpx::iostreams::detail {

    void buffer::save(serialization::output_archive& ar, unsigned) const
    {
        bool const valid = (data_.get() && !data_->empty());
        ar << valid;
        if (valid)
        {
            ar & data_;
        }
    }

    void buffer::load(serialization::input_archive& ar, unsigned)
    {
        bool valid = false;
        ar >> valid;
        if (valid)
        {
            ar & data_;
        }
    }
}    // namespace hpx::iostreams::detail

namespace hpx::iostreams::server {
    ///////////////////////////////////////////////////////////////////////////
    void output_stream::call_write_async(std::uint32_t locality_id,
        std::uint64_t count, detail::buffer const& in, hpx::id_type /*this_id*/)
    {    // {{{
        // Perform the IO operation.
        pending_output_.output(locality_id, count, in, write_f, mtx_);
    }    // }}}

    void output_stream::write_async(std::uint32_t locality_id,
        std::uint64_t count, detail::buffer const& buf_in)
    {
        // Perform the IO in another OS thread.
        detail::buffer in(buf_in);
        // we need to capture the GID of the component to keep it alive long
        // enough.
        hpx::id_type this_id = this->get_id();
#if ASIO_VERSION >= 103400
        ::asio::post(hpx::get_thread_pool("io_pool")->get_io_service(),
            hpx::bind_front(&output_stream::call_write_async, this, locality_id,
                count, HPX_MOVE(in), HPX_MOVE(this_id)));
#else
        hpx::get_thread_pool("io_pool")->get_io_service().post(
            hpx::bind_front(&output_stream::call_write_async, this, locality_id,
                count, HPX_MOVE(in), HPX_MOVE(this_id)));
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    void output_stream::call_write_sync(std::uint32_t locality_id,
        std::uint64_t count, detail::buffer const& in,
        threads::thread_id_ref_type caller)
    {
        // Perform the IO operation.
        pending_output_.output(locality_id, count, in, write_f, mtx_);

        // Wake up caller.
        threads::set_thread_state(
            caller.noref(), threads::thread_schedule_state::pending);
    }

    void output_stream::write_sync(std::uint32_t locality_id,
        std::uint64_t count, detail::buffer const& buf_in)
    {
        // Perform the IO in another OS thread.
        detail::buffer in(buf_in);
#if ASIO_VERSION >= 103400
        ::asio::post(hpx::get_thread_pool("io_pool")->get_io_service(),
            hpx::bind_front(&output_stream::call_write_sync, this, locality_id,
                count, std::ref(in),
                threads::thread_id_ref_type(threads::get_self_id())));
#else
        hpx::get_thread_pool("io_pool")->get_io_service().post(hpx::bind_front(
            &output_stream::call_write_sync, this, locality_id, count,
            std::ref(in), threads::thread_id_ref_type(threads::get_self_id())));
#endif
        // Sleep until the worker thread wakes us up.
        this_thread::suspend(threads::thread_schedule_state::suspended,
            "output_stream::write_sync");
    }
}    // namespace hpx::iostreams::server
