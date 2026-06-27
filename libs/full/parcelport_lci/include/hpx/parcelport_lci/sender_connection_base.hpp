//  Copyright (c) 2023-2024 Jiakun Yan
//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)

#include <hpx/modules/parcelset.hpp>
#include <hpx/parcelport_lci/parcelport_lci.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::lci {
    struct sender_connection_base
      : public parcelset::parcelport_connection<sender_connection_base>
    {
    protected:
        using write_handler_type =
            hpx::function<void(std::error_code const&, parcel const&)>;
        using base_type =
            parcelset::parcelport_connection<sender_connection_base>;
        using handler_type = hpx::move_only_function<void(error_code const&)>;
        using postprocess_handler_type = hpx::move_only_function<void(
            error_code const&, parcelset::locality const&,
            std::shared_ptr<sender_connection_base>)>;

    public:
        enum class return_status_t
        {
            done,     // This connection is done. No need to call send anymore.
            retry,    // Need to call send on this connection to make progress.
            wait      // Wait for the completion object and then call send to
                      // make progress.
        };
        struct return_t
        {
            return_status_t status;
            ::lci::comp_t completion;
        };
        sender_connection_base(int dst, parcelset::parcelport* pp)
          : dst_rank(dst)
          , pp_((lci::parcelport*) pp)
          , there_(parcelset::locality(locality(dst_rank)))
          , device_p(nullptr)
        {
        }

        virtual ~sender_connection_base() {}

        parcelset::locality const& destination() const noexcept
        {
            return there_;
        }

        constexpr void verify_(
            parcelset::locality const& /* parcel_locality_id */) const noexcept
        {
        }

        void async_write(handler_type&& handler,
            postprocess_handler_type&& parcel_postprocess);
        virtual void load(handler_type&& handler,
            postprocess_handler_type&& parcel_postprocess) = 0;
        return_t send(bool in_bg_work);
        virtual return_t send_nb() = 0;
        virtual void done() = 0;
        void profile_start_hook(header const& header_);
        void profile_end_hook();

        int dst_rank;
        handler_type handler_;
        postprocess_handler_type postprocess_handler_;
        parcelport* pp_;
        parcelset::locality there_;
        parcelport::device_t* device_p;
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        parcelset::data_point data_point_;
#endif
    };
}    // namespace hpx::parcelset::policies::lci

#endif
