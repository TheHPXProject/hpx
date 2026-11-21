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
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/modules/actions.hpp>
#include <hpx/parcelset/detail/parcel_await.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
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
    static bool preprocess_parcel(parcelset::parcel& p,
        serialization::output_archive& archive,
        serialization::detail::preprocess_container const& data,
        std::size_t overhead)
    {
        archive.reset();

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

        return true;
    }

    template <typename Parcel, typename Handler, typename Derived>
    struct parcel_await_base : std::enable_shared_from_this<Derived>
    {
        using put_parcel_type =
            hpx::move_only_function<void(Parcel&&, Handler&&)>;

        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        parcel_await_base(Parcel&& parcel, Handler&& handler,
            std::uint32_t archive_flags, put_parcel_type pp) noexcept
          : put_parcel_(HPX_MOVE(pp))
          , parcel_(HPX_MOVE(parcel))
          , handler_(HPX_MOVE(handler))
          , archive_(data_, archive_flags)
          , overhead_(archive_.bytes_written())
        {
        }

        void done()
        {
            put_parcel_(HPX_MOVE(parcel_), HPX_MOVE(handler_));
        }

        bool apply_single(parcelset::parcel& p)
        {
            if (preprocess_parcel(p, archive_, data_, overhead_))
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
            (*handle_futures)([this_]() { this_->apply(); });
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
            std::uint32_t archive_flags, put_parcel_type pp) noexcept
          : base_type(HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp))
        {
        }

        void apply()
        {
            if (apply_single(parcel_))
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
            put_parcel_type pp) noexcept
          : base_type(HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp))
          , idx_(0)
        {
        }

        void apply()
        {
            for (/*idx_*/; idx_ != parcel_.size(); ++idx_)
            {
                if (!apply_single(parcel_[idx_]))
                    return;
            }
            done();
        }

        std::size_t idx_;
    };

    ///////////////////////////////////////////////////////////////////////////
    void parcel_await_apply(parcelset::parcel&& p, write_handler_type&& f,
        std::uint32_t archive_flags, put_parcel_type pp)
    {
        // Run the first pass on the stack. Most parcels have no futures to
        // wait for, so they need no heap object.
        serialization::detail::preprocess_container data;
        serialization::output_archive archive(data, archive_flags);
        if (preprocess_parcel(p, archive, data, archive.bytes_written()))
        {
            pp(HPX_MOVE(p), HPX_MOVE(f));
            return;
        }

        // Some futures are not ready. Their continuations refer to the extra
        // data of the archive, so the heap object takes it over. This must
        // not throw: unwinding would free the extra data while the futures
        // still refer to it.
        auto const ptr = [&]() noexcept {
            auto await = std::make_shared<parcel_await>(
                HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp));
            await->archive_.move_extra_data_from(archive);
            return await;
        }();
        ptr->wait_for_futures();
    }

    void parcels_await_apply(std::vector<parcelset::parcel>&& p,
        std::vector<write_handler_type>&& f, std::uint32_t archive_flags,
        put_parcels_type pp)
    {
        auto const ptr = std::make_shared<parcels_await>(
            HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp));
        ptr->apply();
    }
}    // namespace hpx::parcelset::detail

#endif
