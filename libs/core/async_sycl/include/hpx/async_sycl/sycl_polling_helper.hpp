//  Copyright (c) 2022 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  Note: This file is basically the same as
//  libs/core/async_cuda/include/hpx/async_cuda/cuda_polling_helper.hpp
//  only using the SYCL register/unregister functions
//
//  TODO Maybe it's worthwhile to create a common class for this in the future,
//  however, I feel at this point it's not yet necessary
//
// hpxinspect:noascii

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_sycl/detail/sycl_event_callback.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading_base.hpp>

#include <string>

namespace hpx { namespace sycl { namespace experimental {
    /// This RAII helper class enables polling for a scoped block
    struct [[nodiscard]] enable_user_polling
    {
        explicit enable_user_polling(std::string const& pool_name = {})
          : pool_name_(pool_name)
        {
            if (pool_name_.empty())
            {
                detail::register_polling(hpx::resource::get_thread_pool(0));
            }
            else
            {
                detail::register_polling(
                    hpx::resource::get_thread_pool(pool_name_));
            }
        }

        ~enable_user_polling()
        {
            if (pool_name_.empty())
            {
                detail::unregister_polling(hpx::resource::get_thread_pool(0));
            }
            else
            {
                detail::unregister_polling(
                    hpx::resource::get_thread_pool(pool_name_));
            }
        }

    private:
        std::string pool_name_;
    };

}}}    // namespace hpx::sycl::experimental
