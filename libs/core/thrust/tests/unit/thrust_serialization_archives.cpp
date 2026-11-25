//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The archives of thrust_serialization.cu, see thrust_serialization.hpp.

#include <hpx/config.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include "thrust_serialization.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

// A filter that passes the data through unchanged (see filtered_round_trip).
struct identity_filter : hpx::serialization::binary_filter
{
    void set_max_length(std::size_t const size) override
    {
        buffer_.reserve(size);
    }

    void save(void const* src, std::size_t const src_count) override
    {
        char const* begin = static_cast<char const*>(src);
        buffer_.insert(buffer_.end(), begin, begin + src_count);
    }

    bool flush(
        void* dst, std::size_t const dst_count, std::size_t& written) override
    {
        written = (std::min) (dst_count, buffer_.size() - current_);
        std::memcpy(dst, buffer_.data() + current_, written);
        current_ += written;
        return current_ == buffer_.size();
    }

    std::size_t init_data(void const* buffer, std::size_t const size,
        std::size_t const buffer_size) override
    {
        char const* begin = static_cast<char const*>(buffer);
        buffer_.assign(begin, begin + size);
        current_ = 0;
        return buffer_size;
    }

    void load(void* dst, std::size_t const dst_count) override
    {
        HPX_TEST(current_ + dst_count <= buffer_.size());
        if (current_ + dst_count > buffer_.size())
        {
            throw std::runtime_error("identity_filter: too little data");
        }
        std::memcpy(dst, buffer_.data() + current_, dst_count);
        current_ += dst_count;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }

    HPX_SERIALIZATION_POLYMORPHIC(identity_filter, override);

private:
    std::vector<char> buffer_;
    std::size_t current_ = 0;
};

namespace test {

    using hpx::serialization::chunk_type;
    using hpx::serialization::serialization_chunk;

    std::uint32_t archive_flags(bool const chunks, bool const device_chunks)
    {
        using hpx::serialization::archive_flags;

        archive_flags flags = hpx::endian::native == hpx::endian::big ?
            archive_flags::endian_big :
            archive_flags::endian_little;
        if (!chunks)
        {
            flags = flags | archive_flags::disable_data_chunking;
        }
        if (device_chunks)
        {
            flags = flags | archive_flags::enable_device_chunks;
        }
        return static_cast<std::uint32_t>(flags);
    }

    namespace {

        // the preprocessing passes, until nothing is left to wait for;
        // returns the buffers the zero-copy chunks will refer to
        hpx::serialization::detail::keep_alive_list preprocess(
            std::uint32_t const flags, save_type const& save)
        {
            // one archive for all rounds, as parcel_await uses: its extra
            // data keeps the buffers and the futures that were pending
            // between the rounds
            struct state
            {
                explicit state(std::uint32_t const flags)
                  : archive(data, flags, nullptr, nullptr,
                        HPX_ZERO_COPY_SERIALIZATION_THRESHOLD)
                {
                }

                hpx::serialization::detail::preprocess_container data;
                hpx::serialization::output_archive archive;
            };
            auto s = std::make_unique<state>(flags);
            hpx::serialization::output_archive& archive = s->archive;

            // as a parcel does: keeps the buffers alive
            auto& keep_alive = archive.get_extra_data<
                hpx::serialization::detail::preprocess_keep_alive>();
            while (true)
            {
                archive.reset();

                // What waits for the futures a round registers is set up
                // before they are registered. If waiting can't be set up
                // nevertheless, the archive the futures notify is kept
                // (lost) instead of destroyed.
                hpx::promise<void> ready;
                hpx::future<void> f = ready.get_future();
                auto const wait = [&]() {
                    try
                    {
                        auto* futures = archive.try_get_extra_data<
                            hpx::serialization::detail::preprocess_futures>();
                        if (futures == nullptr || !futures->has_futures())
                        {
                            return false;
                        }
                        (*futures)([&ready]() { ready.set_value(); });
                    }
                    catch (...)
                    {
                        (void) s.release();
                        throw;
                    }

                    // the waiting thread may be woken before the future is
                    // ready
                    while (!f.is_ready())
                    {
                        f.wait();
                    }
                    return true;
                };

                // not inside a handler while waiting: this thread may
                // continue on another OS thread, which doesn't know the
                // handled exception (as parcel_await_apply does)
                std::exception_ptr error;
                try
                {
                    save(archive);
                }
                catch (...)
                {
                    error = std::current_exception();
                }
                if (error)
                {
                    // futures registered before the error notify the
                    // archive's extra data: keep the archive until they are
                    // ready, then pass the error of save() on
                    try
                    {
                        (void) wait();
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                    catch (...)
                    {
                    }
                    std::rethrow_exception(error);
                }

                // another round once the futures are ready
                if (!wait())
                {
                    return keep_alive.move_buffers();
                }
                f.get();
            }
        }

        // copies between any memory (host, pinned or device)
        void copy(void* to, void const* from, std::size_t const bytes)
        {
            hpx::cuda::experimental::check_cuda_error(
                cudaMemcpy(to, from, bytes, cudaMemcpyDefault));
        }
    }    // namespace

    message send(
        std::uint32_t const flags, bool const preprocess, save_type const& save)
    {
        hpx::serialization::detail::keep_alive_list buffers;
        if (preprocess)
        {
            buffers = test::preprocess(flags, save);
        }

        message m;
        m.keep_alive = buffers.size();
        {
            hpx::serialization::output_archive archive(m.data, flags, &m.chunks,
                nullptr, HPX_ZERO_COPY_SERIALIZATION_THRESHOLD);
            if (!buffers.empty())
            {
                archive
                    .get_extra_data<
                        hpx::serialization::detail::preprocess_keep_alive>()
                    .set_buffers(buffers);
            }
            save(archive);
            archive.flush();
            m.data.resize(archive.bytes_written());
        }

        // the chunk contents are sent while the buffers are alive
        for (serialization_chunk const& c : m.chunks)
        {
            if (c.type_ == chunk_type::chunk_type_index)
            {
                m.chunk_data.emplace_back();
                continue;
            }
            m.chunk_data.emplace_back(c.size());
            copy(m.chunk_data.back().data(), c.data(), c.size());
            m.sources.push_back(c.data());
        }
        return m;
    }

    preprocessed preprocess_only(
        std::uint32_t const flags, save_type const& save)
    {
        hpx::serialization::detail::preprocess_container data;
        hpx::serialization::output_archive archive(data, flags, nullptr,
            nullptr, HPX_ZERO_COPY_SERIALIZATION_THRESHOLD);
        save(archive);
        archive.flush();

        preprocessed result;
        auto* futures = archive.try_get_extra_data<
            hpx::serialization::detail::preprocess_futures>();
        result.futures = futures != nullptr && futures->has_futures();
        result.size = data.size();
        return result;
    }

    received receive(message& m, bool const zero_copy_receive,
        load_type const& load, bool const device_receive,
        int const receive_device)
    {
        // the receive buffers: zero-copy receive lets load() choose the
        // destinations, otherwise the data has arrived already
        std::vector<serialization_chunk> chunks;
        for (std::size_t i = 0; i != m.chunks.size(); ++i)
        {
            serialization_chunk const& c = m.chunks[i];
            if (c.type_ == chunk_type::chunk_type_index)
            {
                chunks.push_back(c);
            }
            else if (zero_copy_receive)
            {
                chunks.push_back(hpx::serialization::create_pointer_chunk(
                    nullptr, c.size()));
            }
            else
            {
                chunks.push_back(hpx::serialization::create_pointer_chunk(
                    static_cast<void*>(m.chunk_data[i].data()), c.size()));
            }
        }

        received r;
        hpx::serialization::detail::post_receive_actions::actions_type steps;
        {
            hpx::serialization::input_archive archive(
                m.data, m.data.size(), &chunks);
            if (zero_copy_receive)
            {
                archive.get_extra_data<
                    hpx::serialization::detail::allow_zero_copy_receive>();
                if (device_receive)
                {
                    archive.get_extra_data<
                        hpx::serialization::detail::allow_device_receive>();
                }
            }
            if (receive_device >= 0)
            {
                archive
                    .get_extra_data<
                        hpx::serialization::detail::receive_device>()
                    .device = receive_device;
            }

            load(archive);

            if (auto* actions = archive.try_get_extra_data<
                    hpx::serialization::detail::post_receive_actions>())
            {
                steps = actions->move_actions();
            }
        }

        if (!zero_copy_receive)
        {
            r.steps = steps.size();
            return r;
        }

        // the networking layer places the data now
        for (std::size_t i = 0; i != chunks.size(); ++i)
        {
            serialization_chunk& c = chunks[i];
            if (c.type_ == chunk_type::chunk_type_index)
            {
                continue;
            }
            r.destinations.push_back(c.data());
            if (c.memory_ == hpx::serialization::chunk_memory::device)
            {
                ++r.device_destinations;
            }
            copy(c.data(), m.chunk_data[i].data(), c.size());
        }

        // then the deferred steps run, the parcel's action would run after
        // the last of them
        r.steps = steps.size();
        if (!steps.empty())
        {
            std::atomic<std::size_t> pending(steps.size());
            hpx::promise<void> done;
            hpx::future<void> f = done.get_future();

            // a step that fails to start doesn't call its callback, nor do
            // the steps after it; the started ones still have to finish
            // before the error is passed on, as the error of a step that
            // fails later
            std::exception_ptr error;
            std::mutex step_mtx;
            std::exception_ptr step_error;
            for (std::size_t i = 0; i != steps.size(); ++i)
            {
                // each step finishes once: by its callback or by throwing
                // (as post_receive_actions' run_all counts them), also if
                // setting it up throws
                std::shared_ptr<std::atomic<bool>> once;
                try
                {
                    once = std::make_shared<std::atomic<bool>>(false);
                    steps[i]([&pending, &done, &step_mtx, &step_error, once](
                                 std::exception_ptr e) {
                        if (once->exchange(true))
                        {
                            return;
                        }
                        if (e)
                        {
                            std::lock_guard<std::mutex> l(step_mtx);
                            if (!step_error)
                            {
                                step_error = HPX_MOVE(e);
                            }
                        }
                        if (--pending == 0)
                        {
                            done.set_value();
                        }
                    });
                }
                catch (...)
                {
                    error = std::current_exception();

                    // this step (unless its callback ran) and the ones not
                    // started
                    std::size_t const finished = steps.size() - i -
                        (once && once->exchange(true) ? 1 : 0);
                    if (finished != 0 && (pending -= finished) == 0)
                    {
                        done.set_value();
                    }
                    break;
                }
            }
            f.get();
            if (error)
            {
                std::rethrow_exception(error);
            }
            if (step_error)
            {
                std::rethrow_exception(step_error);
            }
        }
        return r;
    }

    filtered filtered_round_trip(
        std::uint32_t const flags, save_type const& save, load_type const& load)
    {
        filtered result;

        std::vector<char> buffer;
        std::vector<serialization_chunk> chunks;
        identity_filter filter;
        std::size_t written = 0;
        {
            hpx::serialization::output_archive archive(buffer, flags, &chunks,
                &filter, HPX_ZERO_COPY_SERIALIZATION_THRESHOLD);
            result.saved_filtered = archive.is_filtered();
            save(archive);
            archive.flush();
            written = archive.bytes_written();
        }

        for (serialization_chunk const& c : chunks)
        {
            if (c.is_device_memory())
            {
                ++result.device_chunks;
            }
        }

        hpx::serialization::input_archive archive(buffer, written, &chunks);
        result.loaded_filtered = archive.is_filtered();
        load(archive);
        return result;
    }
}    // namespace test
