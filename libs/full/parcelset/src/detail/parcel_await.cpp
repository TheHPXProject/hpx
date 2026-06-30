//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/functional.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/modules/actions.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <hpx/parcelset/detail/parcel_await.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace hpx::parcelset::detail {

    template <typename Parcel, typename Handler, typename Derived>
    struct parcel_await_base : std::enable_shared_from_this<Derived>
    {
        using put_parcel_type =
            hpx::move_only_function<void(Parcel&&, Handler&&)>;

        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        parcel_await_base(Parcel&& parcel, Handler&& handler,
            std::uint32_t const archive_flags, put_parcel_type pp) noexcept
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
            archive_.reset();

            archive_ << p;

            auto* handle_futures = archive_.try_get_extra_data<
                serialization::detail::preprocess_futures>();

            // We are doing a fixed point iteration until we are sure that the
            // serialization process requires nothing more to wait on ...
            // Things where we need waiting:
            //  - (shared_)future<id_type>: when the future wasn't ready yet, we
            //      need to do another await round for the id splitting
            //  - id_type: we need to await, if and only if, the credit of the
            //      id needs to split.
            if (handle_futures && handle_futures->has_futures())
            {
                auto this_ = this->shared_from_this();
                (*handle_futures)([this_]() { this_->apply(); });
                return false;
            }

            archive_.flush();

            p.size() = data_.size() + overhead_;
            p.num_chunks() = archive_.get_num_chunks();

            auto* split_gids = archive_.try_get_extra_data<
                serialization::detail::preprocess_gid_types>();
            if (split_gids)
            {
                p.set_split_gids(split_gids->move_split_gids());
            }

            return true;
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
            std::uint32_t const archive_flags, put_parcel_type pp) noexcept
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
            std::vector<write_handler_type>&& f,
            std::uint32_t const archive_flags, put_parcel_type pp) noexcept
          : base_type(HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp))
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

        std::size_t idx_ = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    void parcel_await_apply(parcelset::parcel&& p, write_handler_type&& f,
        std::uint32_t archive_flags, put_parcel_type pp)
    {
        auto const ptr = std::make_shared<parcel_await>(
            HPX_MOVE(p), HPX_MOVE(f), archive_flags, HPX_MOVE(pp));
        ptr->apply();
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
