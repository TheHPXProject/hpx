//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/modules/actions.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/parcelset/detail/parcel_await.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parcelset::detail {

    // One preprocessing pass over the parcel. Returns false if the pass found
    // futures that are not ready yet; the caller must then wait for them and
    // run the pass again.
    //
    // We are doing a fixed point iteration until we are sure that the
    // serialization process requires nothing more to wait on ...
    // Things where we need waiting:
    //  - (shared_)future<id_type>: when the future wasn't ready yet, we
    //      need to do another await round for the id splitting
    //  - id_type: we need to await, if and only if, the credit of the
    //      id needs to split.
    //  - data copied for zero-copy chunks (e.g. the host copy of device
    //      memory): we need to await the copies that are not done yet.
    static bool preprocess_parcel(parcelset::parcel& p,
        serialization::output_archive& archive,
        serialization::detail::preprocess_container const& data,
        std::size_t overhead)
    {
        archive.reset();

        // The parcel keeps the buffers zero-copy chunks will refer to alive
        // (serialization creates them only if this exists). Buffers from an
        // earlier preprocessing of this parcel (e.g. by post() before handing
        // it to the parcelport) are reused; the archive keeps those of its
        // earlier rounds (reset() only starts again at the first object).
        // Only the serialization of device memory (the thrust module)
        // creates such buffers.
        serialization::detail::preprocess_keep_alive* buffers = nullptr;
#if defined(HPX_HAVE_THRUST)
        buffers = &archive.get_extra_data<
            serialization::detail::preprocess_keep_alive>();
        if (auto const& keep_alive = p.keep_alive();
            buffers->empty() && !keep_alive.empty())
        {
            buffers->set_buffers(keep_alive);
        }
#endif

        archive << p;

        auto const* handle_futures = archive.try_get_extra_data<
            serialization::detail::preprocess_futures>();
        if (handle_futures && handle_futures->has_futures())
        {
            return false;
        }

        archive.flush();

        p.size() = data.size() + overhead;
        p.num_chunks() = archive.get_num_chunks();

        auto* split_gids = archive.try_get_extra_data<
            serialization::detail::preprocess_gid_types>();
        if (split_gids)
        {
            p.set_split_gids(split_gids->move_split_gids());
        }

        // buffers created during preprocessing that zero-copy chunks of this
        // parcel will refer to; the parcel keeps them alive until it has been
        // sent
        if (buffers != nullptr && !buffers->empty())
        {
            p.set_keep_alive(buffers->move_buffers());
        }

        return true;
    }

    // The serialization failed after it had registered futures (for instance
    // the copy of device memory an earlier argument started, or a future
    // argument that isn't ready): they notify the extra data of the archive
    // once they are ready, so 'owner', which keeps that extra data, is kept
    // until then. The error is passed on right away: a registered future may
    // be one the caller makes ready only after the send has returned (one
    // that never becomes ready keeps the extra data forever). Without an
    // owner (nullptr) this waits for the futures.
    template <typename Owner>
    static void keep_until_registered_futures_are_ready(
        serialization::output_archive& archive, Owner const& owner) noexcept
    {
        auto* handle_futures = archive.try_get_extra_data<
            serialization::detail::preprocess_futures>();
        if (handle_futures == nullptr || !handle_futures->has_futures())
        {
            return;
        }

        if constexpr (!std::is_null_pointer_v<std::decay_t<Owner>>)
        {
            try
            {
                // a copy: if attaching it throws, the caller still holds
                // the owner (and the extra data the wait below uses)
                (*handle_futures)([owner]() {});
                return;
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
            catch (...)
            {
                // the notification couldn't be attached: wait here instead
            }
        }

        try
        {
            hpx::promise<void> all_ready;
            hpx::future<void> f = all_ready.get_future();
            (*handle_futures)([&all_ready]() { all_ready.set_value(); });

            // the waiting thread may be woken before the future is ready
            while (!f.is_ready())
            {
                f.wait();
            }
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {
            // the error of the serialization is passed on instead
        }
    }

    // The extra data of a preprocessing archive whose serialization failed,
    // kept until the futures registered with it are ready.
    struct failed_preprocessing
    {
        explicit failed_preprocessing(serialization::output_archive& from)
          : archive_(data_)
        {
            archive_.move_extra_data_from(from);
        }

        serialization::detail::preprocess_container data_;
        serialization::output_archive archive_;
    };

    template <typename Parcel, typename Handler, typename Derived>
    struct parcel_await_base : std::enable_shared_from_this<Derived>
    {
        using put_parcel_type =
            hpx::move_only_function<void(Parcel&&, Handler&&)>;

        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        parcel_await_base(Parcel&& parcel, Handler&& handler,
            std::uint32_t archive_flags, put_parcel_type pp,
            std::size_t zero_copy_serialization_threshold) noexcept
          : put_parcel_(HPX_MOVE(pp))
          , parcel_(HPX_MOVE(parcel))
          , handler_(HPX_MOVE(handler))
          , archive_(data_, archive_flags, nullptr, nullptr,
                zero_copy_serialization_threshold)
          , overhead_(archive_.bytes_written())
        {
        }

        void done()
        {
            put_parcel_(HPX_MOVE(parcel_), HPX_MOVE(handler_));
        }

        bool apply_single(parcelset::parcel& p)
        {
            bool complete = false;
            std::exception_ptr error;
            try
            {
                complete = preprocess_parcel(p, archive_, data_, overhead_);
            }
            catch (...)
            {
                error = std::current_exception();
            }
            if (error)
            {
                // not inside the handler, see keep_until_registered_futures_
                // are_ready
                keep_until_registered_futures_are_ready(
                    archive_, this->shared_from_this());
                std::rethrow_exception(error);
            }

            if (complete)
            {
                return true;
            }
            wait_for_futures();
            return false;
        }

        // run the next pass when the futures found in the last pass are
        // ready
        void wait_for_futures()
        {
            auto* handle_futures = archive_.try_get_extra_data<
                serialization::detail::preprocess_futures>();
            HPX_ASSERT(handle_futures != nullptr);

            auto this_ = this->shared_from_this();
            (*handle_futures)([this_]() { this_->resume(); });
        }

        // a later pass runs from the continuation of a future: its error
        // goes to the write handler(s) instead of the caller of the send
        void resume()
        {
            // only the preprocessing is guarded: once done() hands the parcel
            // on, its errors are reported by whoever took the parcel
            bool complete = false;
            std::exception_ptr error;
            try
            {
                complete = static_cast<Derived*>(this)->preprocess();
            }
            catch (...)
            {
                error = std::current_exception();
            }

            if (!error)
            {
                if (complete)
                {
                    this->done();
                }
                return;
            }

            error_code const ec = make_error_code(error);
            if constexpr (std::is_same_v<Parcel, parcelset::parcel>)
            {
                if (handler_)
                {
                    handler_(ec, parcel_);
                }
            }
            else
            {
                // nothing has been sent yet, all of them failed
                for (std::size_t i = 0; i != handler_.size(); ++i)
                {
                    if (handler_[i])
                    {
                        handler_[i](ec, parcel_[i]);
                    }
                }
            }
        }

        put_parcel_type put_parcel_;
        Parcel parcel_;
        Handler handler_;
        hpx::serialization::detail::preprocess_container data_;
        hpx::serialization::output_archive archive_;
        std::size_t overhead_;
    };

    struct parcel_await
      : parcel_await_base<parcelset::parcel, write_handler_type, parcel_await>
    {
        using base_type = parcel_await_base<parcelset::parcel,
            write_handler_type, parcel_await>;

        parcel_await(parcelset::parcel&& p, write_handler_type&& f,
            std::uint32_t archive_flags, put_parcel_type pp,
            std::size_t zero_copy_serialization_threshold) noexcept
          : base_type(HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp),
                zero_copy_serialization_threshold)
        {
        }

        // whether the parcel is complete (else it waits for futures)
        bool preprocess()
        {
            return apply_single(parcel_);
        }

        void apply()
        {
            if (preprocess())
            {
                done();
            }
        }
    };

    struct parcels_await
      : parcel_await_base<std::vector<parcelset::parcel>,
            std::vector<write_handler_type>, parcels_await>
    {
        using base_type = parcel_await_base<std::vector<parcelset::parcel>,
            std::vector<write_handler_type>, parcels_await>;

        parcels_await(std::vector<parcelset::parcel>&& p,
            std::vector<write_handler_type>&& f, std::uint32_t archive_flags,
            put_parcel_type pp,
            std::size_t zero_copy_serialization_threshold) noexcept
          : base_type(HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp),
                zero_copy_serialization_threshold)
          , idx_(0)
        {
        }

        // whether all parcels are complete (else one waits for futures)
        bool preprocess()
        {
            for (/*idx_*/; idx_ != parcel_.size(); ++idx_)
            {
                if (!apply_single(parcel_[idx_]))
                    return false;
            }
            return true;
        }

        void apply()
        {
            if (preprocess())
            {
                done();
            }
        }

        std::size_t idx_;
    };

    ///////////////////////////////////////////////////////////////////////////
    void parcel_await_apply(parcelset::parcel&& p, write_handler_type&& f,
        std::uint32_t archive_flags, put_parcel_type pp,
        std::size_t zero_copy_serialization_threshold)
    {
        // Run the first pass on the stack. Most parcels have no futures to
        // wait for, so they need no heap object.
        serialization::detail::preprocess_container data;
        serialization::output_archive archive(data, archive_flags, nullptr,
            nullptr, zero_copy_serialization_threshold);
        bool complete = false;
        std::exception_ptr error;
        try
        {
            complete =
                preprocess_parcel(p, archive, data, archive.bytes_written());
        }
        catch (...)
        {
            error = std::current_exception();
        }
        if (error)
        {
            // Not inside the handler: waiting (if it is needed) may continue
            // on another OS thread, which doesn't know the handled exception.
            // The extra data of the archive goes to the heap if futures still
            // refer to it.
            auto* handle_futures = archive.try_get_extra_data<
                serialization::detail::preprocess_futures>();
            if (handle_futures != nullptr && handle_futures->has_futures())
            {
                std::shared_ptr<failed_preprocessing> failed;
                try
                {
                    failed = std::make_shared<failed_preprocessing>(archive);
                }
                // NOLINTNEXTLINE(bugprone-empty-catch)
                catch (...)
                {
                    // the extra data stays here, waiting keeps it
                }
                if (failed)
                {
                    keep_until_registered_futures_are_ready(
                        failed->archive_, failed);
                }
                else
                {
                    keep_until_registered_futures_are_ready(archive, nullptr);
                }
            }
            std::rethrow_exception(error);
        }
        if (complete)
        {
            pp(HPX_MOVE(p), HPX_MOVE(f));
            return;
        }

        // Some futures are not ready. Their continuations refer to the extra
        // data of the archive, so the heap object takes it over. This must
        // not throw: unwinding would free the extra data while the futures
        // still refer to it.
        auto const ptr = [&]() noexcept {
            auto await = std::make_shared<parcel_await>(HPX_MOVE(p),
                HPX_MOVE(f), archive_flags, HPX_MOVE(pp),
                zero_copy_serialization_threshold);
            await->archive_.move_extra_data_from(archive);
            return await;
        }();
        ptr->wait_for_futures();
    }

    void parcels_await_apply(std::vector<parcelset::parcel>&& p,
        std::vector<write_handler_type>&& f, std::uint32_t archive_flags,
        put_parcels_type pp, std::size_t zero_copy_serialization_threshold)
    {
        auto const ptr =
            std::make_shared<parcels_await>(HPX_MOVE(p), HPX_MOVE(f),
                archive_flags, HPX_MOVE(pp), zero_copy_serialization_threshold);
        ptr->apply();
    }
}    // namespace hpx::parcelset::detail

#endif
