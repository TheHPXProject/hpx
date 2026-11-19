//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/thrust/memory.hpp>
#include <hpx/thrust/serialization/detail/device_staging.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <utility>

namespace hpx::serialization::detail {

    namespace {

        // A few CUDA streams for the staging copies, created on first use
        // (on the device current then; one GPU per locality) and kept for
        // the lifetime of the process: creating a stream per transfer costs
        // more than small and medium transfers take. They are blocking
        // streams, which orders them after the legacy default stream only,
        // not after per-thread default streams or other streams. The data
        // must be complete before it is serialized (see
        // hpx/thrust/serialization/device_vector.hpp).
        cudaStream_t staging_stream()
        {
            struct streams
            {
                streams()
                {
                    for (std::size_t i = 0; i != streams_.size(); ++i)
                    {
                        if (cudaError_t const error =
                                cudaStreamCreate(&streams_[i]);
                            error != cudaSuccess)
                        {
                            // the next call tries again: release the streams
                            // created so far
                            for (std::size_t j = 0; j != i; ++j)
                            {
                                (void) cudaStreamDestroy(streams_[j]);
                            }
                            hpx::cuda::experimental::check_cuda_error(error);
                        }
                    }
                }

                std::array<cudaStream_t, 4> streams_{};
                std::atomic<std::size_t> next_{0};
            };

            static streams pool;
            return pool.streams_[pool.next_++ % pool.streams_.size()];
        }

        // Pinned host memory from the pool (allocating it takes long), the
        // staging area between the network and the device in both
        // directions.
        class pinned_host_buffer
        {
        public:
            explicit pinned_host_buffer(std::size_t const size)
              : data_(hpx::thrust::detail::pool_allocate(
                    size, hpx::thrust::memory_kind::pinned))
              , size_(size)
            {
            }

            pinned_host_buffer(pinned_host_buffer const&) = delete;
            pinned_host_buffer(pinned_host_buffer&&) = delete;
            pinned_host_buffer& operator=(pinned_host_buffer const&) = delete;
            pinned_host_buffer& operator=(pinned_host_buffer&&) = delete;

            ~pinned_host_buffer()
            {
                hpx::thrust::detail::pool_deallocate(data_, size_);
            }

            [[nodiscard]] void* data() const noexcept
            {
                return data_;
            }

        private:
            void* data_;
            std::size_t size_;
        };

        // Pinned host copy of device memory, filled asynchronously on a
        // staging stream. A zero-copy chunk of a serialized device_vector
        // refers to it, and the parcel keeps it alive until the parcel has
        // been sent.
        class device_host_copy
        {
        public:
            device_host_copy(void const* device_data, std::size_t const size)
              : buffer_(size)
              , stream_(staging_stream())
            {
                hpx::cuda::experimental::check_cuda_error(
                    cudaMemcpyAsync(buffer_.data(), device_data, size,
                        cudaMemcpyDeviceToHost, stream_));
                try
                {
                    // the source may be pooled memory, given back (e.g. by a
                    // temporary vector) before the copy has read it
                    hpx::thrust::detail::pool_note_read(device_data, stream_);
                    ready_ = hpx::cuda::experimental::detail::
                        get_future_with_callback(stream_);
                }
                catch (...)
                {
                    // the buffer goes back to the pool, never under a copy
                    (void) cudaStreamSynchronize(stream_);
                    throw;
                }
            }

            device_host_copy(device_host_copy const&) = delete;
            device_host_copy(device_host_copy&&) = delete;
            device_host_copy& operator=(device_host_copy const&) = delete;
            device_host_copy& operator=(device_host_copy&&) = delete;

            ~device_host_copy()
            {
                // normally the copy is done (the parcel waited for it); the
                // buffer goes back to the pool, never under a copy
                if (ready_.valid() && !ready_.is_ready())
                {
                    (void) cudaStreamSynchronize(stream_);
                }
            }

            [[nodiscard]] void const* data() const noexcept
            {
                return buffer_.data();
            }

            [[nodiscard]] hpx::future<void>& ready() noexcept
            {
                return ready_;
            }

            // rethrows the error of a copy that has failed: its buffer must
            // not be sent
            void rethrow_if_failed()
            {
                if (ready_.valid() && ready_.is_ready() &&
                    ready_.has_exception())
                {
                    ready_.get();
                }
            }

        private:
            pinned_host_buffer buffer_;
            cudaStream_t stream_;
            hpx::future<void> ready_;
        };

        // deletes a device_host_copy; tells the keep-alive buffers created
        // here apart from others (std::get_deleter)
        struct device_host_copy_deleter
        {
            void operator()(device_host_copy* copy) const noexcept
            {
                delete copy;
            }
        };

        // the buffer of a copy created here, after checking the copy
        keep_alive_buffer const* checked(keep_alive_buffer const* buffer)
        {
            if (buffer != nullptr &&
                std::get_deleter<device_host_copy_deleter>(buffer->owner) !=
                    nullptr)
            {
                static_cast<device_host_copy*>(buffer->owner.get())
                    ->rethrow_if_failed();
            }
            return buffer;
        }

        // Copies a received staging buffer to device memory asynchronously
        // on a staging stream and calls 'done' once the copy has finished.
        // The CUDA future hands that continuation to an HPX thread.
        void copy_to_device(void* device_data,
            std::shared_ptr<pinned_host_buffer> staging, std::size_t const size,
            post_receive_actions::done_type done)
        {
            cudaStream_t const stream = staging_stream();
            hpx::cuda::experimental::check_cuda_error(
                cudaMemcpyAsync(device_data, staging->data(), size,
                    cudaMemcpyHostToDevice, stream));

            // The continuation gets a copy of 'staging', this function keeps
            // its own until it returns: if handing on the completion fails,
            // the buffer goes back to the pool only after the copy.
            try
            {
                hpx::future<void> f =
                    hpx::cuda::experimental::detail::get_future_with_callback(
                        stream);
                auto state = hpx::traits::future_access<
                    hpx::future<void>>::get_shared_state(f);

                // the continuation holds the state to read a CUDA error; the
                // state drops it after running the continuation
                state->set_on_completed(
                    [state, staging, done = HPX_MOVE(done)]() {
                        if (state->has_exception())
                        {
                            // the arguments are incomplete: the parcel
                            // reports the error instead of running its action
                            done(state->get_exception_ptr());
                            return;
                        }
                        done();
                    });
            }
            catch (...)
            {
                (void) cudaStreamSynchronize(stream);
                throw;
            }
        }
    }    // namespace

    keep_alive_buffer const* device_host_copy_of(output_archive& ar,
        void const* key, void const* device_data, std::size_t const bytes)
    {
        if (bytes < ar.get_zero_copy_serialization_threshold())
        {
            return nullptr;
        }

        if (!ar.is_preprocessing())
        {
            auto const* keep_alive =
                ar.try_get_extra_data<preprocess_keep_alive>();
            return keep_alive ? checked(keep_alive->find(key)) : nullptr;
        }

        // Only the owner of a preprocessing archive that keeps the buffers
        // alive and waits for the futures before the real pass (a parcel)
        // provides the keep-alive data. Others (e.g. the size calculation of
        // a checkpoint) get no copy: its future would outlive their archive.
        auto* keep_alive = ar.try_get_extra_data<preprocess_keep_alive>();
        if (keep_alive == nullptr)
        {
            return nullptr;
        }
        if (auto const* buffer = keep_alive->find(key))
        {
            // an earlier preprocessing round has created it already
            return checked(buffer);
        }

#if defined(HPX_DEBUG)
        warn_if_device_busy();
#endif
        std::shared_ptr<device_host_copy> copy(
            new device_host_copy(device_data, bytes),
            device_host_copy_deleter{});

        // the parcel waits for the copy if it isn't done yet; registered
        // only once the buffer is kept, nothing waits for a copy that's gone
        hpx::traits::detail::shared_state_ptr_t<void> pending;
        if (auto& f = copy->ready(); f.is_ready())
        {
            copy->rethrow_if_failed();
        }
        else
        {
            pending =
                hpx::traits::future_access<hpx::future<void>>::get_shared_state(
                    f);
        }

        void const* data = copy->data();
        keep_alive->add(key, keep_alive_buffer{HPX_MOVE(copy), data, bytes});
        if (pending)
        {
            hpx::lcos::detail::preprocess_future(ar, *pending);
        }
        return keep_alive->find(key);
    }

    void* stage_device_receive(
        input_archive& ar, void* device_data, std::size_t const bytes)
    {
        auto staging = std::make_shared<pinned_host_buffer>(bytes);
        void* data = staging->data();

        ar.get_extra_data<post_receive_actions>().add(
            [staging = HPX_MOVE(staging), device_data, bytes](
                post_receive_actions::done_type done) {
                copy_to_device(device_data, staging, bytes, HPX_MOVE(done));
            });
        return data;
    }

    void warn_if_device_busy() noexcept
    {
#if defined(HPX_HAVE_HIP)
        // HIP has no handle for the legacy default stream, it's the null one
        cudaStream_t const legacy_stream = nullptr;
#else
        cudaStream_t const legacy_stream = cudaStreamLegacy;
#endif
        if (cudaStreamQuery(cudaStreamPerThread) != cudaErrorNotReady &&
            cudaStreamQuery(legacy_stream) != cudaErrorNotReady)
        {
            return;
        }

        static std::atomic<bool> warned{false};
        if (!warned.exchange(true))
        {
            std::fputs("hpx: a thrust::device_vector is serialized while the "
                       "device still has work queued on a default stream; "
                       "wait for the work producing its data to finish "
                       "before sending it\n",
                stderr);
        }
    }
}    // namespace hpx::serialization::detail
