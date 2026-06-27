//  Copyright (c) 2023-2024 Jiakun Yan
//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)

#include <hpx/parcelport_lci/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/parcelport_lci/parcelport_lci.hpp>
#include <hpx/parcelport_lci/receiver_base.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <deque>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::lci {
    struct receiver_connection_sendrecv;
    struct receiver_sendrecv : public receiver_base
    {
        using connection_type = receiver_connection_sendrecv;
        using connection_ptr = std::shared_ptr<connection_type>;

        explicit receiver_sendrecv(parcelport* pp) noexcept
          : receiver_base(pp)
        {
            if (config_t::protocol == config_t::protocol_t::sendrecv)
            {
                HPX_ASSERT(false);
                // for (std::size_t i = 0; i < pp_->devices.size(); ++i)
                // {
                //     auto& device = pp->devices[i];
                //     for (int j = 0; j < config_t::prepost_recv_num; ++j)
                //     {
                //         ::lci::comp_t completion =
                //             device.completion_manager_p->recv_new
                //                 ->alloc_completion();
                //         ::lci::post_recv(device.endpoint_new, LCI_RANK_ANY, 0,
                //             completion, reinterpret_cast<void*>(i));
                //         device.completion_manager_p->recv_new
                //             ->enqueue_completion(completion);
                //     }
                // }
            }
        }

        ~receiver_sendrecv() {}

        connection_ptr create_connection(int dest, parcelset::parcelport* pp);

        bool background_work() noexcept;

    private:
        bool accept_new() noexcept;
        bool followup() noexcept;
    };

}    // namespace hpx::parcelset::policies::lci

#endif
