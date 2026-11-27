//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace hpx::util {

    /// A std::shared_ptr that is constructed only when a pointer is assigned
    /// to it. A class that holds it can be constexpr constructed and
    /// destructed (std::shared_ptr's destructor is not constexpr), also as a
    /// mutable member.
    HPX_CXX_CORE_EXPORT template <typename T>
    class lazy_shared_ptr
    {
    public:
        constexpr lazy_shared_ptr() noexcept = default;

        constexpr lazy_shared_ptr(lazy_shared_ptr const& rhs) noexcept
        {
            if (rhs.engaged_)
            {
                ::new (address()) std::shared_ptr<T>(*rhs.ptr());
                engaged_ = true;
            }
        }

        constexpr lazy_shared_ptr(lazy_shared_ptr&& rhs) noexcept
        {
            if (rhs.engaged_)
            {
                ::new (address()) std::shared_ptr<T>(HPX_MOVE(*rhs.ptr()));
                engaged_ = true;
            }
        }

        constexpr lazy_shared_ptr& operator=(
            lazy_shared_ptr const& rhs) noexcept
        {
            if (this != &rhs)
            {
                reset();
                if (rhs.engaged_)
                {
                    ::new (address()) std::shared_ptr<T>(*rhs.ptr());
                    engaged_ = true;
                }
            }
            return *this;
        }

        constexpr lazy_shared_ptr& operator=(lazy_shared_ptr&& rhs) noexcept
        {
            if (this != &rhs)
            {
                reset();
                if (rhs.engaged_)
                {
                    ::new (address()) std::shared_ptr<T>(HPX_MOVE(*rhs.ptr()));
                    engaged_ = true;
                }
            }
            return *this;
        }

        // A constant expression never engages it. It may not even read
        // engaged_ then, if it is a mutable member.
        constexpr ~lazy_shared_ptr()
        {
            if (!std::is_constant_evaluated())
            {
                reset();
            }
        }

        /// Hold \a p from now on.
        void assign(std::shared_ptr<T> p) noexcept
        {
            reset();
            ::new (address()) std::shared_ptr<T>(HPX_MOVE(p));
            engaged_ = true;
        }

        /// Release the held pointer, if any.
        constexpr void reset() noexcept
        {
            if (engaged_)
            {
                std::destroy_at(ptr());
                engaged_ = false;
            }
        }

        /// The held pointer, nullptr if there is none.
        [[nodiscard]] constexpr T* get() const noexcept
        {
            return engaged_ ? ptr()->get() : nullptr;
        }

        /// A copy of the held std::shared_ptr, an empty one if there is none.
        [[nodiscard]] std::shared_ptr<T> shared() const noexcept
        {
            return engaged_ ? *ptr() : std::shared_ptr<T>();
        }

    private:
        void* address() noexcept
        {
            return static_cast<void*>(storage_);
        }

        std::shared_ptr<T>* ptr() const noexcept
        {
            return std::launder(reinterpret_cast<std::shared_ptr<T>*>(
                const_cast<unsigned char*>(storage_)));
        }

        // holds a std::shared_ptr<T> if engaged_
        alignas(std::shared_ptr<T>) unsigned char storage_[sizeof(
            std::shared_ptr<T>)] = {};
        bool engaged_ = false;
    };
}    // namespace hpx::util
