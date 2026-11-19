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

#include "thrust_serialization.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <utility>
#include <vector>

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
        hpx::serialization::detail::keep_alive_map preprocess(
            std::uint32_t const flags, save_type const& save)
        {
            hpx::serialization::detail::keep_alive_map buffers;
            while (true)
            {
                hpx::serialization::detail::preprocess_container data;
                hpx::serialization::output_archive archive(data, flags, nullptr,
                    nullptr, HPX_ZERO_COPY_SERIALIZATION_THRESHOLD);

                // as a parcel does: keeps the buffers alive
                auto& keep_alive = archive.get_extra_data<
                    hpx::serialization::detail::preprocess_keep_alive>();
                if (!buffers.empty())
                {
                    keep_alive.add_buffers(buffers);
                }

                save(archive);

                if (!keep_alive.empty())
                {
                    buffers = keep_alive.move_buffers();
                }

                auto* futures = archive.try_get_extra_data<
                    hpx::serialization::detail::preprocess_futures>();
                if (futures == nullptr || !futures->has_futures())
                {
                    return buffers;
                }

                // another round once the futures are ready
                hpx::promise<void> ready;
                hpx::future<void> f = ready.get_future();
                (*futures)([&ready]() { ready.set_value(); });
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
        hpx::serialization::detail::keep_alive_map buffers;
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
                    .add_buffers(buffers);
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

    received receive(
        message& m, bool const zero_copy_receive, load_type const& load)
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
                try
                {
                    steps[i]([&pending, &done, &step_mtx, &step_error](
                                 std::exception_ptr e) {
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
                    if ((pending -= steps.size() - i) == 0)
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
}    // namespace test
