//  Copyright (c) 2018 Thomas Heller
//  Copyright (c) 2019-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/coroutines/thread_id_type.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/thread_support.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string_view>
#include <utility>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    // same as below, just not holding a reference count
    HPX_CXX_CORE_EXPORT struct thread_id
    {
    private:
        using thread_id_repr = void*;

    public:
        constexpr thread_id() noexcept = default;

        thread_id(thread_id const&) noexcept = default;
        thread_id& operator=(thread_id const&) noexcept = default;

        ~thread_id() = default;

        constexpr thread_id(thread_id&& rhs) noexcept
          : thrd_(rhs.thrd_)
        {
            rhs.thrd_ = nullptr;
        }
        constexpr thread_id& operator=(thread_id&& rhs) noexcept
        {
            if (this != &rhs)
            {
                thrd_ = rhs.thrd_;
                rhs.thrd_ = nullptr;
            }
            return *this;
        }

        explicit constexpr thread_id(thread_id_repr const thrd) noexcept
          : thrd_(thrd)
        {
        }
        constexpr thread_id& operator=(thread_id_repr const rhs) noexcept
        {
            thrd_ = rhs;
            return *this;
        }

        explicit constexpr operator bool() const noexcept
        {
            return nullptr != thrd_;
        }

        constexpr thread_id_repr get() const noexcept
        {
            return thrd_;
        }

        constexpr void reset() noexcept
        {
            thrd_ = nullptr;
        }

        friend constexpr bool operator==(
            std::nullptr_t, thread_id const& rhs) noexcept
        {
            return nullptr == rhs.thrd_;
        }

        friend constexpr bool operator!=(
            std::nullptr_t, thread_id const& rhs) noexcept
        {
            return nullptr != rhs.thrd_;
        }

        friend constexpr bool operator==(
            thread_id const& lhs, std::nullptr_t) noexcept
        {
            return nullptr == lhs.thrd_;
        }

        friend constexpr bool operator!=(
            thread_id const& lhs, std::nullptr_t) noexcept
        {
            return nullptr != lhs.thrd_;
        }

        friend constexpr bool operator==(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return lhs.thrd_ == rhs.thrd_;
        }

        friend constexpr bool operator!=(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return lhs.thrd_ != rhs.thrd_;
        }

        friend constexpr bool operator<(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return std::less<thread_id_repr>{}(lhs.thrd_, rhs.thrd_);
        }

        friend constexpr bool operator>(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return std::less<thread_id_repr>{}(rhs.thrd_, lhs.thrd_);
        }

        friend constexpr bool operator<=(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return !(rhs > lhs);
        }

        friend constexpr bool operator>=(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return !(rhs < lhs);
        }

        HPX_CORE_EXPORT friend std::ostream& operator<<(
            std::ostream& os, thread_id const& id);
        HPX_CORE_EXPORT friend void format_value(
            std::ostream& os, std::string_view spec, thread_id const& id);

    private:
        thread_id_repr thrd_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT enum class thread_id_addref : std::uint8_t { yes, no };

    namespace detail {

        HPX_CXX_CORE_EXPORT struct thread_data_reference_counting;

        HPX_CORE_EXPORT void intrusive_ptr_add_ref(
            thread_data_reference_counting* p) noexcept;
        HPX_CORE_EXPORT void intrusive_ptr_release(
            thread_data_reference_counting* p) noexcept;

        struct thread_data_reference_counting
        {
            // the initial reference count is one by default as each newly
            // created thread will be held alive by the variable returned from
            // the creation function;
            explicit constexpr thread_data_reference_counting(
                thread_id_addref const addref = thread_id_addref::yes) noexcept
              : count_(addref == thread_id_addref::yes ? 1 : 0)
            {
            }

            thread_data_reference_counting(
                thread_data_reference_counting const&) = delete;
            thread_data_reference_counting(
                thread_data_reference_counting&&) = delete;
            thread_data_reference_counting& operator=(
                thread_data_reference_counting const&) = delete;
            thread_data_reference_counting& operator=(
                thread_data_reference_counting&&) = delete;

            virtual ~thread_data_reference_counting() = default;
            virtual void destroy_thread() = 0;

            // reference counting
            HPX_CORE_EXPORT friend void intrusive_ptr_add_ref(
                thread_data_reference_counting* p) noexcept;
            HPX_CORE_EXPORT friend void intrusive_ptr_release(
                thread_data_reference_counting* p) noexcept;

            util::atomic_count count_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct thread_id_ref
    {
    private:
        using thread_id_repr =
            hpx::intrusive_ptr<detail::thread_data_reference_counting>;

    public:
        thread_id_ref() noexcept = default;

        thread_id_ref(thread_id_ref const&) = default;
        thread_id_ref& operator=(thread_id_ref const&) = default;

        thread_id_ref(thread_id_ref&& rhs) noexcept = default;
        thread_id_ref& operator=(thread_id_ref&& rhs) noexcept = default;

        ~thread_id_ref() = default;

        explicit thread_id_ref(thread_id_repr const& thrd) noexcept
          : thrd_(thrd)
        {
        }
        explicit thread_id_ref(thread_id_repr&& thrd) noexcept
          : thrd_(HPX_MOVE(thrd))
        {
        }

        thread_id_ref& operator=(thread_id_repr const& rhs) noexcept
        {
            thrd_ = rhs;
            return *this;
        }
        thread_id_ref& operator=(thread_id_repr&& rhs) noexcept
        {
            thrd_ = HPX_MOVE(rhs);
            return *this;
        }

        using thread_repr = detail::thread_data_reference_counting;

        explicit thread_id_ref(thread_repr* thrd,
            thread_id_addref const addref = thread_id_addref::yes) noexcept
          : thrd_(thrd, addref == thread_id_addref::yes)
        {
        }

        thread_id_ref& operator=(thread_repr* rhs) noexcept
        {
            thrd_.reset(rhs);
            return *this;
        }

        thread_id_ref(thread_id const& noref)
          : thrd_(static_cast<thread_repr*>(noref.get()))
        {
        }

        thread_id_ref(thread_id&& noref, bool const add_ref = true) noexcept
          : thrd_(static_cast<thread_repr*>(noref.get()), add_ref)
        {
            noref.reset();
        }

        thread_id_ref& operator=(thread_id const& noref)
        {
            thrd_.reset(static_cast<thread_repr*>(noref.get()));
            return *this;
        }

        thread_id_ref& operator=(thread_id&& noref) noexcept
        {
            thrd_.reset(static_cast<thread_repr*>(noref.get()));
            noref.reset();
            return *this;
        }

        explicit constexpr operator bool() const noexcept
        {
            return nullptr != thrd_;
        }

        [[nodiscard]] constexpr thread_id noref() const noexcept
        {
            return thread_id(thrd_.get());
        }

        [[nodiscard]] constexpr thread_id_repr& get() & noexcept
        {
            return thrd_;
        }
        [[nodiscard]] thread_id_repr&& get() && noexcept
        {
            return HPX_MOVE(thrd_);
        }

        [[nodiscard]] constexpr thread_id_repr const& get() const& noexcept
        {
            return thrd_;
        }

        void reset() noexcept
        {
            thrd_.reset();
        }

        void reset(thread_repr* thrd, bool const add_ref = true) noexcept
        {
            thrd_.reset(thrd, add_ref);
        }

        constexpr thread_repr* detach() noexcept
        {
            return thrd_.detach();
        }

        friend constexpr bool operator==(
            std::nullptr_t, thread_id_ref const& rhs) noexcept
        {
            return nullptr == rhs.thrd_;
        }

        friend constexpr bool operator!=(
            std::nullptr_t, thread_id_ref const& rhs) noexcept
        {
            return nullptr != rhs.thrd_;
        }

        friend constexpr bool operator==(
            thread_id_ref const& lhs, std::nullptr_t) noexcept
        {
            return nullptr == lhs.thrd_;
        }

        friend constexpr bool operator!=(
            thread_id_ref const& lhs, std::nullptr_t) noexcept
        {
            return nullptr != lhs.thrd_;
        }

        friend constexpr bool operator==(
            thread_id_ref const& lhs, thread_id_ref const& rhs) noexcept
        {
            return lhs.thrd_ == rhs.thrd_;
        }

        friend constexpr bool operator!=(
            thread_id_ref const& lhs, thread_id_ref const& rhs) noexcept
        {
            return lhs.thrd_ != rhs.thrd_;
        }

        friend constexpr bool operator<(
            thread_id_ref const& lhs, thread_id_ref const& rhs) noexcept
        {
            return std::less<thread_repr const*>{}(
                lhs.thrd_.get(), rhs.thrd_.get());
        }

        friend constexpr bool operator>(
            thread_id_ref const& lhs, thread_id_ref const& rhs) noexcept
        {
            return std::less<thread_repr const*>{}(
                rhs.thrd_.get(), lhs.thrd_.get());
        }

        friend constexpr bool operator<=(
            thread_id_ref const& lhs, thread_id_ref const& rhs) noexcept
        {
            return !(rhs > lhs);
        }

        friend constexpr bool operator>=(
            thread_id_ref const& lhs, thread_id_ref const& rhs) noexcept
        {
            return !(rhs < lhs);
        }

        HPX_CORE_EXPORT friend std::ostream& operator<<(
            std::ostream& os, thread_id_ref const& id);
        HPX_CORE_EXPORT friend void format_value(
            std::ostream& os, std::string_view spec, thread_id_ref const& id);

    private:
        thread_id_repr thrd_;
    };

#if defined(HPX_COMPUTE_DEVICE_CODE)
    // We can't actually refer to this in device code. This is only to satisfy
    // the compiler.
    extern HPX_DEVICE thread_id invalid_thread_id;
#else
    HPX_CXX_CORE_EXPORT inline constexpr thread_id const invalid_thread_id;
#endif

    // This data type allows to optionally keep the provided thread_id alive
    struct keep_alive_thread_id
    {
        explicit keep_alive_thread_id(
            hpx::threads::thread_id id, bool const add_ref) noexcept
          : add_ref(add_ref)
          , id(HPX_MOVE(id), add_ref)
        {
        }

        keep_alive_thread_id(keep_alive_thread_id const& other) = delete;
        keep_alive_thread_id(keep_alive_thread_id&& other) = default;

        keep_alive_thread_id& operator=(
            keep_alive_thread_id const& other) = delete;
        keep_alive_thread_id& operator=(keep_alive_thread_id&& other) = default;

        ~keep_alive_thread_id()
        {
            // prevent for the reference count to be decremented if it was
            // not incremented in the constructor
            if (!add_ref)
            {
                [[maybe_unused]] auto* p = id.get().detach();
            }
        }

        bool add_ref;
        hpx::threads::thread_id_ref id;
    };
}    // namespace hpx::threads

namespace std {

    template <>
    struct hash<::hpx::threads::thread_id>
    {
        std::size_t operator()(
            ::hpx::threads::thread_id const& v) const noexcept
        {
            constexpr std::hash<std::size_t> hasher_;
            return hasher_(reinterpret_cast<std::size_t>(v.get()));
        }
    };

    template <>
    struct hash<::hpx::threads::thread_id_ref>
    {
        std::size_t operator()(
            ::hpx::threads::thread_id_ref const& v) const noexcept
        {
            constexpr std::hash<std::size_t> hasher_;
            return hasher_(reinterpret_cast<std::size_t>(v.get().get()));
        }
    };
}    // namespace std
