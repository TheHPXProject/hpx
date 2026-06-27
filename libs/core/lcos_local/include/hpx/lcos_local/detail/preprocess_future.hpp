//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2015-2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <mutex>
#include <type_traits>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
namespace hpx::serialization::detail {

    // This class allows to register futures during serialization preprocessing
    // to ensure each future is ready before serializing it.
    HPX_CXX_CORE_EXPORT class preprocess_futures
    {
        using mutex_type = hpx::spinlock;

    public:
        preprocess_futures() noexcept
          : mtx_()
          , done_(false)
          , num_futures_(0)
          , triggered_futures_(0)
        {
        }

        ~preprocess_futures()
        {
            HPX_ASSERT(done_);
            HPX_ASSERT(num_futures_ == 0);
            HPX_ASSERT(num_futures_ == triggered_futures_);
        }

        preprocess_futures(preprocess_futures&& rhs) noexcept
          : mtx_()
          , done_(rhs.done_)
          , num_futures_(rhs.num_futures_)
          , triggered_futures_(rhs.triggered_futures_)
          , promise_(HPX_MOVE(rhs.promise_))
        {
            rhs.done_ = true;
            rhs.num_futures_ = 0;
            rhs.triggered_futures_ = 0;
        }

        preprocess_futures& operator=(preprocess_futures&& rhs) noexcept
        {
            done_ = rhs.done_;
            num_futures_ = rhs.num_futures_;
            triggered_futures_ = rhs.triggered_futures_;
            promise_ = HPX_MOVE(rhs.promise_);

            rhs.done_ = true;
            rhs.num_futures_ = 0;
            rhs.triggered_futures_ = 0;

            return *this;
        }

        void trigger()
        {
            // hpx::lcos::local::promise<>::set_value() might need to acquire a
            // lock, as such, we check our triggering condition inside a
            // critical section and trigger the promise outside of it.
            bool set_value = false;

            {
                std::lock_guard<mutex_type> l(mtx_);
                ++triggered_futures_;

                // trigger the promise only after the whole serialization
                // operation is done and all futures have become ready
                set_value = (done_ && num_futures_ == triggered_futures_);
            }

            if (set_value)
            {
                promise_.set_value();
            }
        }

        // This is called during serialization of futures. It keeps track of
        // the number of futures encountered. It also attaches a continuation to
        // all futures which triggers this object and eventually invokes the
        // parcel send operation.
        void await_future(
            hpx::lcos::detail::future_data_refcnt_base& future_data,
            bool increment_count = true)
        {
            {
                std::lock_guard<mutex_type> l(mtx_);
                if (num_futures_ == 0)
                {
                    done_ = false;
                }
                if (increment_count)
                {
                    ++num_futures_;
                }
            }

            future_data.set_on_completed([this]() { this->trigger(); });
        }

        void increment_future_count()
        {
            std::lock_guard<mutex_type> l(mtx_);
            if (num_futures_ == 0)
            {
                done_ = false;
            }
            ++num_futures_;
        }

        void decrement_future_count()
        {
            std::lock_guard<mutex_type> l(mtx_);
            HPX_ASSERT(num_futures_ > 0);
            if (--num_futures_ == 0)
            {
                done_ = true;
            }
        }

        void reset()
        {
            std::lock_guard<mutex_type> l(mtx_);

            done_ = true;
            num_futures_ = 0;
            triggered_futures_ = 0;
            promise_ = hpx::promise<void>();
        }

        bool has_futures() const
        {
            std::lock_guard<mutex_type> l(mtx_);
            return num_futures_ > 0;
        }

        // This is called after the full serialization of a parcel. It attaches
        // the supplied function to be invoked as soon as all encountered
        // futures have become ready.
        template <typename F>
        void operator()(F f)
        {
            bool set_promise = false;
            hpx::future<void> fut = promise_.get_future();

            {
                std::lock_guard<mutex_type> l(mtx_);

                // trigger promise if all futures seen during serialization
                // have been made ready by now
                done_ = true;
                if (num_futures_ == triggered_futures_)
                {
                    set_promise = true;
                }
            }

            if (set_promise)
            {
                promise_.set_value();
            }

            // we don't call f directly to avoid possible stack overflow.
            auto& shared_state_ =
                hpx::traits::future_access<hpx::future<void>>::get_shared_state(
                    fut);
            shared_state_->set_on_completed([this, f = HPX_MOVE(f)]() {
                reset();
                f();    // this invokes the next round of the fixed-point
                        // iteration
            });
        }

    private:
        mutable mutex_type mtx_;
        bool done_;
        std::size_t num_futures_;
        std::size_t triggered_futures_;
        hpx::promise<void> promise_;
    };
}    // namespace hpx::serialization::detail

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    template <>
    struct extra_data_helper<hpx::serialization::detail::preprocess_futures>
    {
        HPX_CORE_EXPORT static hpx::util::extra_data_id_type id() noexcept;
        static constexpr void reset(
            hpx::serialization::detail::preprocess_futures*) noexcept
        {
        }
    };
}    // namespace hpx::util
