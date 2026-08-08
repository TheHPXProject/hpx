//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/naming_base/gid_type.hpp>
#include <hpx/naming_base/naming_base.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace naming::detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT HPX_EXPORT void intrusive_ptr_add_ref(
            id_type_impl* p) noexcept;
        HPX_CXX_EXPORT HPX_EXPORT void intrusive_ptr_release(
            id_type_impl* p) noexcept;

        HPX_CXX_EXPORT extern HPX_EXPORT void (*gid_managed_deleter)(
            id_type_impl const* p) noexcept;
        HPX_CXX_EXPORT extern HPX_EXPORT void (*gid_unmanaged_deleter)(
            id_type_impl const* p) noexcept;
    }    // namespace naming::detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT std::ostream& operator<<(
        std::ostream& os, id_type const& id);

    ///////////////////////////////////////////////////////////////////////////
    // the local gid is actually just a wrapper around the real thing
    HPX_CXX_EXPORT struct id_type
    {
    private:
        friend struct naming::detail::id_type_impl;

    public:
        enum class management_type
        {
            unknown_deleter = -1,
            unmanaged = 0,             ///< unmanaged GID
            managed = 1,               ///< managed GID
            managed_move_credit = 2    ///< managed GID that will give up all
                                       ///< credits when sent
        };

        friend constexpr bool operator<(
            management_type lhs, management_type rhs) noexcept
        {
            return static_cast<int>(lhs) < static_cast<int>(rhs);
        }
        friend constexpr bool operator>(
            management_type lhs, management_type rhs) noexcept
        {
            return static_cast<int>(lhs) > static_cast<int>(rhs);
        }

        constexpr id_type() noexcept = default;

        id_type(std::uint64_t lsb_id, management_type t);
        id_type(naming::gid_type const& gid, management_type t);
        id_type(std::uint64_t msb_id, std::uint64_t lsb_id, management_type t);

        id_type(id_type const& o) = default;
        id_type(id_type&& o) noexcept = default;

        id_type& operator=(id_type const& o) = default;
        id_type& operator=(id_type&& o) noexcept = default;

        ~id_type() = default;

        naming::gid_type& get_gid();
        naming::gid_type& get_gid() const;

        // This function is used in AGAS unit tests and application code, do not
        // remove.
        management_type get_management_type() const noexcept;

        id_type& operator++();
        id_type operator++(int) const;

        explicit operator bool() const noexcept;

        // comparison is required as well
        friend HPX_EXPORT bool operator==(
            id_type const& lhs, id_type const& rhs) noexcept;
        friend bool operator!=(id_type const& lhs, id_type const& rhs) noexcept;

        friend HPX_EXPORT bool operator<(
            id_type const& lhs, id_type const& rhs) noexcept;
        friend bool operator<=(id_type const& lhs, id_type const& rhs) noexcept;
        friend bool operator>(id_type const& lhs, id_type const& rhs) noexcept;
        friend bool operator>=(id_type const& lhs, id_type const& rhs) noexcept;

        // access the internal parts of the gid
        std::uint64_t get_msb() const;
        void set_msb(std::uint64_t msb) const;

        std::uint64_t get_lsb() const;
        void set_lsb(std::uint64_t lsb) const;
        void set_lsb(void* lsb) const;

        // Convert this id into an unmanaged one (in-place) - Use with maximum
        // care, or better, don't use this at all.
        void make_unmanaged();
        hpx::id_type make_unmanaged() const;

        hpx::intrusive_ptr<naming::detail::id_type_impl>& impl() noexcept
        {
            return gid_;
        }
        constexpr hpx::intrusive_ptr<naming::detail::id_type_impl> const& impl()
            const noexcept
        {
            return gid_;
        }

    private:
        friend HPX_EXPORT std::ostream& operator<<(
            std::ostream& os, id_type const& id);

        hpx::intrusive_ptr<naming::detail::id_type_impl> gid_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT inline id_type const invalid_id = id_type();

    namespace naming {

        ///////////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT HPX_EXPORT char const* get_management_type_name(
            id_type::management_type m) noexcept;

        ///////////////////////////////////////////////////////////////////////
        // Handle conversion to/from locality_id
        // FIXME: these names are confusing, 'id' appears in identifiers far too
        // frequently.
        HPX_CXX_EXPORT inline id_type get_id_from_locality_id(
            std::uint32_t locality_id) noexcept
        {
            return {(static_cast<std::uint64_t>(locality_id) + 1)
                    << naming::gid_type::locality_id_shift,
                0, id_type::management_type::unmanaged};
        }

        HPX_CXX_EXPORT inline std::uint32_t get_locality_id_from_id(
            id_type const& id) noexcept
        {
            return static_cast<std::uint32_t>(
                       id.get_msb() >> naming::gid_type::locality_id_shift) -
                1;
        }

        HPX_CXX_EXPORT inline id_type get_locality_from_id(
            id_type const& id) noexcept
        {
            return get_id_from_locality_id(get_locality_id_from_id(id));
        }

        HPX_CXX_EXPORT inline bool is_locality(id_type const& id) noexcept
        {
            return id && is_locality(id.get_gid());
        }
    }    // namespace naming

    ///////////////////////////////////////////////////////////////////////////
    namespace naming::detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT inline void set_dont_store_in_cache(
            id_type const& id) noexcept
        {
            id.set_msb(id.get_msb() | gid_type::dont_cache_mask);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT struct id_type_impl : gid_type
        {
        public:
            id_type_impl(id_type_impl const&) = delete;
            id_type_impl(id_type_impl&&) = delete;
            id_type_impl& operator=(id_type_impl const&) = delete;
            id_type_impl& operator=(id_type_impl&&) = delete;

        private:
            using deleter_type = void (*)(detail::id_type_impl const*) noexcept;
            static deleter_type get_deleter(
                id_type::management_type t) noexcept;

        public:
            // This is a tag type used to convey the information that the caller is
            // _not_ going to addref the future_data instance
            struct init_no_addref
            {
            };

            // called by serialization, needs to start off with a reference
            // count of zero
            id_type_impl() noexcept
              : count_(0)
            {
            }

            id_type_impl(init_no_addref, std::uint64_t lsb_id,
                id_type::management_type t) noexcept
              : gid_type(0, lsb_id)
              , count_(1)
              , type_(t)
            {
            }

            explicit id_type_impl(init_no_addref, std::uint64_t msb_id,
                std::uint64_t lsb_id, id_type::management_type t) noexcept
              : gid_type(msb_id, lsb_id)
              , count_(1)
              , type_(t)
            {
            }

            explicit id_type_impl(init_no_addref, gid_type const& gid,
                id_type::management_type t) noexcept
              : gid_type(gid)
              , count_(1)
              , type_(t)
            {
            }

            ~id_type_impl() = default;

            constexpr id_type::management_type get_management_type()
                const noexcept
            {
                return type_;
            }
            constexpr void set_management_type(
                id_type::management_type type) noexcept
            {
                type_ = type;
            }

            // custom allocator support
            static void* operator new(std::size_t size)
            {
                if (size != sizeof(id_type_impl))
                {
                    return ::operator new(size);
                }
                return alloc_.allocate(1);
            }

            static void operator delete(void* p, std::size_t size) noexcept
            {
                if (p == nullptr)
                {
                    return;
                }

                if (size != sizeof(id_type_impl))
                {
                    return ::operator delete(p);
                }

                return alloc_.deallocate(static_cast<id_type_impl*>(p), 1);
            }

        private:
            // reference counting
            friend HPX_EXPORT void intrusive_ptr_add_ref(
                id_type_impl* p) noexcept;
            friend HPX_EXPORT void intrusive_ptr_release(
                id_type_impl* p) noexcept;

            util::atomic_count count_;
            id_type::management_type type_ =
                id_type::management_type::unknown_deleter;

            static util::internal_allocator<id_type_impl> alloc_;
        };
    }    // namespace naming::detail

    ///////////////////////////////////////////////////////////////////////////
    // the local gid is actually just a wrapper around the real thing
    inline id_type::id_type(std::uint64_t lsb_id, management_type t)
      : gid_(new naming::detail::id_type_impl(
                 naming::detail::id_type_impl::init_no_addref{}, 0, lsb_id, t),
            false)
    {
    }

    inline id_type::id_type(naming::gid_type const& gid, management_type t)
      : gid_(new naming::detail::id_type_impl(
                 naming::detail::id_type_impl::init_no_addref{}, gid, t),
            false)
    {
        if (t == management_type::unmanaged)
        {
            naming::detail::strip_internal_bits_except_dont_cache_from_gid(
                *gid_);
        }
    }

    inline id_type::id_type(
        std::uint64_t msb_id, std::uint64_t lsb_id, management_type t)
      : gid_(new naming::detail::id_type_impl(
                 naming::detail::id_type_impl::init_no_addref{}, msb_id, lsb_id,
                 t),
            false)
    {
        if (t == management_type::unmanaged)
        {
            naming::detail::strip_internal_bits_except_dont_cache_from_gid(
                *gid_);
        }
    }

    inline naming::gid_type& id_type::get_gid()
    {
        return *gid_;
    }
    inline naming::gid_type& id_type::get_gid() const
    {
        return *gid_;
    }

    // This function is used in AGAS unit tests and application code, do not
    // remove.
    inline id_type::management_type id_type::get_management_type()
        const noexcept
    {
        return gid_ ? gid_->get_management_type() :
                      management_type::unknown_deleter;
    }

    inline id_type::operator bool() const noexcept
    {
        return gid_ && *gid_;
    }

    // comparison is required as well
    HPX_CXX_EXPORT inline bool operator!=(
        id_type const& lhs, id_type const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    HPX_CXX_EXPORT inline bool operator<=(
        id_type const& lhs, id_type const& rhs) noexcept
    {
        // Deduced from <.
        return !(rhs < lhs);
    }

    HPX_CXX_EXPORT inline bool operator>(
        id_type const& lhs, id_type const& rhs) noexcept
    {
        // Deduced from <.
        return rhs < lhs;
    }

    HPX_CXX_EXPORT inline bool operator>=(
        id_type const& lhs, id_type const& rhs) noexcept
    {
        // Deduced from <.
        return !(lhs < rhs);
    }

    // access the internal parts of the gid
    inline std::uint64_t id_type::get_msb() const
    {
        return gid_->get_msb();
    }
    inline void id_type::set_msb(std::uint64_t msb) const
    {
        gid_->set_msb(msb);
    }

    inline std::uint64_t id_type::get_lsb() const
    {
        return gid_->get_lsb();
    }
    inline void id_type::set_lsb(std::uint64_t lsb) const
    {
        gid_->set_lsb(lsb);
    }
    inline void id_type::set_lsb(void* lsb) const
    {
        gid_->set_lsb(lsb);
    }

    inline void id_type::make_unmanaged()
    {
        gid_->set_management_type(management_type::unmanaged);
    }

    inline hpx::id_type id_type::make_unmanaged() const
    {
        return {*impl(), management_type::unmanaged};
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    template <>
    struct get_remote_result<hpx::id_type, naming::gid_type>
    {
        HPX_EXPORT static hpx::id_type call(naming::gid_type const& rhs);
    };

    template <>
    struct promise_local_result<naming::gid_type>
    {
        using type = hpx::id_type;
    };

    // we need to specialize this template to allow for automatic conversion of
    // the vector<naming::gid_type> to a vector<hpx::id_type>
    template <>
    struct get_remote_result<std::vector<hpx::id_type>,
        std::vector<naming::gid_type>>
    {
        HPX_EXPORT static std::vector<hpx::id_type> call(
            std::vector<naming::gid_type> const& rhs);
    };

    template <>
    struct promise_local_result<std::vector<naming::gid_type>>
    {
        using type = std::vector<hpx::id_type>;
    };
}    // namespace hpx::traits

///////////////////////////////////////////////////////////////////////////////
namespace std {

    // specialize std::hash for hpx::id_type
    template <>
    struct hash<::hpx::id_type>
    {
        std::size_t operator()(::hpx::id_type const& id) const noexcept
        {
            return id ? hash<::hpx::naming::gid_type>()(id.get_gid()) : 0;
        }
    };
}    // namespace std

#include <hpx/config/warnings_suffix.hpp>
