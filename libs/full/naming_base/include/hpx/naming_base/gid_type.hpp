//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2007 Richard D. Guidry Jr.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/naming_base/naming_base.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <mutex>
#include <string>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::naming {

    namespace detail {

        constexpr bool is_locked(gid_type const& gid) noexcept;

        ///////////////////////////////////////////////////////////////////////
        constexpr std::uint64_t strip_internal_bits_from_gid(
            std::uint64_t msb) noexcept;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT gid_type operator+(
        gid_type const& lhs, gid_type const& rhs) noexcept;
    HPX_CXX_EXPORT HPX_EXPORT gid_type operator-(
        gid_type const& lhs, gid_type const& rhs) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// Global identifier for components across the HPX system.
    HPX_CXX_EXPORT struct gid_type
    {
        // These typedefs are for Boost.ICL.
        using size_type = gid_type;
        using difference_type = gid_type;

        static constexpr std::uint64_t credit_base_mask = 0x1full;
        static constexpr std::uint16_t credit_shift = 24;

        static constexpr std::uint64_t credit_mask = credit_base_mask
            << credit_shift;
        static constexpr std::uint64_t was_split_mask =
            0x80000000ull;    //-V112
        static constexpr std::uint64_t has_credits_mask = 0x40000000ull;
        static constexpr std::uint64_t is_locked_mask = 0x20000000ull;

        static constexpr std::uint64_t locality_id_mask = 0xffffffff00000000ull;
        static constexpr std::uint16_t locality_id_shift = 32;    //-V112

        static constexpr std::uint64_t virtual_memory_mask = 0x3fffffull;

        // don't cache this id in the AGAS caches
        static constexpr std::uint64_t dont_cache_mask = 0x800000ull;    //-V112

        // the object is migratable
        static constexpr std::uint64_t is_migratable = 0x400000ull;    //-V112

        // Bit 64 is set for all dynamically assigned ids (if this is not set
        // then the lsb corresponds to the lva of the referenced object).
        static constexpr std::uint64_t dynamically_assigned = 0x1ull;

        // Bits 65-84 are used to store the component type (20 bits) if bit
        // 64 is not set.
        static constexpr std::uint64_t component_type_base_mask = 0xfffffull;
        static constexpr std::uint64_t component_type_shift = 1ull;
        static constexpr std::uint64_t component_type_mask =
            component_type_base_mask << component_type_shift;

        static constexpr std::uint64_t credit_bits_mask =
            credit_mask | was_split_mask | has_credits_mask;
        static constexpr std::uint64_t internal_bits_mask =
            credit_bits_mask | is_locked_mask | dont_cache_mask | is_migratable;
        static constexpr std::uint64_t special_bits_mask =
            locality_id_mask | internal_bits_mask | component_type_mask;

        constexpr gid_type() noexcept = default;

        explicit constexpr gid_type(std::uint64_t const lsb_id) noexcept
          : id_msb_(0)
          , id_lsb_(lsb_id)
        {
        }

        explicit gid_type(void* lsb_id) noexcept
          : id_msb_(0)
          , id_lsb_(reinterpret_cast<std::uint64_t>(lsb_id))
        {
        }

        explicit constexpr gid_type(
            std::uint64_t msb_id, std::uint64_t lsb_id) noexcept;
        explicit inline gid_type(std::uint64_t msb_id, void* lsb_id) noexcept;

        inline constexpr gid_type(gid_type const& rhs) noexcept;
        inline constexpr gid_type(gid_type&& rhs) noexcept;

        ~gid_type() = default;

        gid_type& operator=(std::uint64_t const lsb_id) noexcept
        {
            HPX_ASSERT(!is_locked());
            id_msb_ = 0;
            id_lsb_ = lsb_id;
            return *this;
        }

        inline gid_type& operator=(gid_type const& rhs) noexcept;
        inline gid_type& operator=(gid_type&& rhs) noexcept;

        explicit constexpr operator bool() const noexcept
        {
            return 0 != id_lsb_ || 0 != id_msb_;
        }

        // We support increment, decrement, addition and subtraction
        gid_type& operator++() noexcept    // pre-increment
        {
            *this += 1;
            return *this;
        }
        gid_type operator++(int) noexcept    // post-increment
        {
            gid_type t(*this);
            ++(*this);
            return t;
        }

        gid_type& operator--() noexcept    // pre-decrement
        {
            *this -= 1;
            return *this;
        }
        gid_type operator--(int) noexcept    // post-decrement
        {
            gid_type t(*this);
            --(*this);
            return t;
        }

        // GID + GID
        friend HPX_EXPORT gid_type operator+(
            gid_type const& lhs, gid_type const& rhs) noexcept;
        gid_type operator+=(gid_type const& rhs) noexcept
        {
            return (*this = *this + rhs);
        }

        // GID + std::uint64_t
        friend gid_type operator+(
            gid_type const& lhs, std::uint64_t const rhs) noexcept
        {
            return lhs + gid_type(0, rhs);
        }
        gid_type operator+=(std::uint64_t const rhs) noexcept
        {
            return (*this = *this + rhs);
        }

        // GID - GID
        friend HPX_EXPORT gid_type operator-(
            gid_type const& lhs, gid_type const& rhs) noexcept;
        gid_type operator-=(gid_type const& rhs) noexcept
        {
            return (*this = *this - rhs);
        }

        // GID - std::uint64_t
        friend gid_type operator-(
            gid_type const& lhs, std::uint64_t const rhs) noexcept
        {
            return lhs - gid_type(0, rhs);
        }
        gid_type operator-=(std::uint64_t const rhs) noexcept
        {
            return (*this = *this - rhs);
        }

        friend gid_type operator&(
            gid_type const& lhs, std::uint64_t const rhs) noexcept
        {
            return gid_type(lhs.id_msb_, lhs.id_lsb_ & rhs);
        }

        // comparison is required as well
        friend constexpr bool operator==(
            gid_type const& lhs, gid_type const& rhs) noexcept
        {
            auto const lhs_msb = static_cast<std::int64_t>(
                detail::strip_internal_bits_from_gid(lhs.id_msb_));
            auto const rhs_msb = static_cast<std::int64_t>(
                detail::strip_internal_bits_from_gid(rhs.id_msb_));

            return (lhs_msb == rhs_msb) && (lhs.id_lsb_ == rhs.id_lsb_);
        }

        friend constexpr bool operator!=(
            gid_type const& lhs, gid_type const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        friend HPX_EXPORT bool operator<(
            gid_type const& lhs, gid_type const& rhs) noexcept;
        friend bool operator>=(
            gid_type const& lhs, gid_type const& rhs) noexcept
        {
            return !(lhs < rhs);
        }

        friend HPX_EXPORT bool operator<=(
            gid_type const& lhs, gid_type const& rhs) noexcept;
        friend bool operator>(gid_type const& lhs, gid_type const& rhs) noexcept
        {
            return !(lhs <= rhs);
        }

        constexpr std::uint64_t get_msb() const noexcept
        {
            return id_msb_;
        }
        constexpr void set_msb(std::uint64_t const msb) noexcept
        {
            id_msb_ = msb;
        }
        constexpr std::uint64_t get_lsb() const noexcept
        {
            return id_lsb_;
        }
        constexpr void set_lsb(std::uint64_t const lsb) noexcept
        {
            id_lsb_ = lsb;
        }
        inline void set_lsb(void* lsb) noexcept
        {
            id_lsb_ = reinterpret_cast<std::uint64_t>(lsb);
        }

        std::string to_string() const;

        // this type is at the same time its own mutex type
        using mutex_type = gid_type;

        void lock()
        {
            hpx::tracing::detail::sync_prepare(this);

            while (!acquire_lock())
            {
                util::yield_while<true>([this] { return is_locked(); },
                    "hpx::naming::gid_type::lock");
            }

            util::register_lock(this);

            hpx::tracing::detail::sync_acquired(this);
        }

        bool try_lock()
        {
            hpx::tracing::detail::sync_prepare(this);

            bool r = acquire_lock();
            if (r)
                hpx::tracing::detail::sync_acquired(this);
            else
                hpx::tracing::detail::sync_cancel(this);

            if (r)
            {
                util::register_lock(this);
                return true;
            }

            return false;
        }

        void unlock()
        {
            hpx::tracing::detail::sync_releasing(this);

            relinquish_lock();
            util::unregister_lock(this);

            hpx::tracing::detail::sync_released(this);
        }

        constexpr mutex_type& get_mutex() const noexcept
        {
            return const_cast<mutex_type&>(*this);
        }

    private:
        friend HPX_EXPORT std::ostream& operator<<(
            std::ostream& os, gid_type const& id);

        friend HPX_EXPORT void save(
            serialization::output_archive& ar, gid_type const&, unsigned int);
        friend HPX_EXPORT void load(
            serialization::input_archive& ar, gid_type&, unsigned int);

        // lock implementation
        using spinlock_pool = util::spinlock_pool<gid_type>;

        // returns whether lock has been acquired
        bool acquire_lock()
        {
            std::lock_guard<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));
            bool const was_locked = (id_msb_ & is_locked_mask) ? true : false;
            if (!was_locked)
            {
                id_msb_ |= is_locked_mask;
                return true;
            }
            return false;
        }

        void relinquish_lock()
        {
            util::ignore_lock(this);
            std::lock_guard<hpx::util::detail::spinlock> l(
                spinlock_pool::spinlock_for(this));
            util::reset_ignored(this);

            id_msb_ &= ~is_locked_mask;
        }

        // this is used for assertions only, no need to acquire the lock
        constexpr bool is_locked() const noexcept
        {
            return (id_msb_ & is_locked_mask) ? true : false;
        }

        friend constexpr bool detail::is_locked(gid_type const& gid) noexcept;

        // actual gid
        std::uint64_t id_msb_ = 0;
        std::uint64_t id_lsb_ = 0;
    };

    HPX_CXX_EXPORT HPX_EXPORT void save(
        serialization::output_archive& ar, gid_type const&, unsigned int);
    HPX_CXX_EXPORT HPX_EXPORT void load(
        serialization::input_archive& ar, gid_type&, unsigned int version);

    HPX_SERIALIZATION_SPLIT_FREE(HPX_CXX_EXPORT, gid_type)
}    // namespace hpx::naming

///////////////////////////////////////////////////////////////////////////////
// we know that we can serialize a gid as a byte sequence
HPX_IS_BITWISE_SERIALIZABLE(hpx::naming::gid_type)

namespace hpx::naming {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr bool store_in_cache(
            gid_type const& id) noexcept
        {
            return (id.get_msb() & gid_type::dont_cache_mask) ? false : true;
        }

        HPX_CXX_EXPORT constexpr void set_dont_store_in_cache(
            gid_type& gid) noexcept
        {
            gid.set_msb(gid.get_msb() | gid_type::dont_cache_mask);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    //  Handle conversion to/from locality_id
    HPX_CXX_EXPORT constexpr gid_type get_gid_from_locality_id(
        std::uint32_t const locality_id) noexcept
    {
        return gid_type((static_cast<std::uint64_t>(locality_id) + 1)
                << gid_type::locality_id_shift,
            static_cast<std::uint64_t>(0));
    }

    HPX_CXX_EXPORT constexpr std::uint32_t get_locality_id_from_gid(
        std::uint64_t const msb) noexcept
    {
        return static_cast<std::uint32_t>(msb >> gid_type::locality_id_shift) -
            1;
    }

    HPX_CXX_EXPORT constexpr std::uint32_t get_locality_id_from_gid(
        gid_type const& id) noexcept
    {
        return get_locality_id_from_gid(id.get_msb());
    }

    HPX_CXX_EXPORT constexpr gid_type get_locality_from_gid(
        gid_type const& id) noexcept
    {
        return get_gid_from_locality_id(get_locality_id_from_gid(id));
    }

    HPX_CXX_EXPORT constexpr bool is_locality(gid_type const& gid) noexcept
    {
        return get_locality_from_gid(gid) == gid;
    }

    HPX_CXX_EXPORT constexpr std::uint64_t replace_locality_id(
        std::uint64_t msb, std::uint32_t const locality_id) noexcept
    {
        msb &= ~gid_type::locality_id_mask;
        return msb | get_gid_from_locality_id(locality_id).get_msb();
    }

    HPX_CXX_EXPORT constexpr gid_type replace_locality_id(
        gid_type const& gid, std::uint32_t const locality_id) noexcept
    {
        std::uint64_t msb = gid.get_msb() & ~gid_type::locality_id_mask;
        msb |= get_gid_from_locality_id(locality_id).get_msb();
        return gid_type(msb, gid.get_lsb());
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT constexpr bool refers_to_virtual_memory(
        std::uint64_t const msb) noexcept
    {
        return !(msb & gid_type::virtual_memory_mask);
    }

    HPX_CXX_EXPORT constexpr bool refers_to_virtual_memory(
        gid_type const& gid) noexcept
    {
        return refers_to_virtual_memory(gid.get_msb());
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT constexpr bool refers_to_local_lva(
        gid_type const& gid) noexcept
    {
        return !(gid.get_msb() & gid_type::dynamically_assigned);
    }

    HPX_CXX_EXPORT inline gid_type replace_component_type(
        gid_type const& gid, std::uint32_t const type) noexcept
    {
        std::uint64_t msb = gid.get_msb() & ~gid_type::component_type_mask;

        HPX_ASSERT(!(msb & gid_type::dynamically_assigned));
        msb |= ((static_cast<std::uint64_t>(type)
                    << gid_type::component_type_shift) &
            gid_type::component_type_mask);
        return gid_type(msb, gid.get_lsb());
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr std::uint64_t strip_internal_bits_from_gid(
            std::uint64_t const msb) noexcept
        {
            return msb & ~gid_type::internal_bits_mask;
        }

        HPX_CXX_EXPORT constexpr gid_type& strip_internal_bits_from_gid(
            gid_type& id) noexcept
        {
            id.set_msb(strip_internal_bits_from_gid(id.get_msb()));
            return id;
        }

        HPX_CXX_EXPORT constexpr std::uint64_t
        strip_internal_bits_except_dont_cache_from_gid(
            std::uint64_t const msb) noexcept
        {
            return msb &
                ~(gid_type::credit_bits_mask | gid_type::is_locked_mask);
        }

        HPX_CXX_EXPORT constexpr gid_type&
        strip_internal_bits_except_dont_cache_from_gid(gid_type& id) noexcept
        {
            id.set_msb(
                strip_internal_bits_except_dont_cache_from_gid(id.get_msb()));
            return id;
        }

        HPX_CXX_EXPORT constexpr std::uint64_t
        strip_internal_bits_and_component_type_from_gid(
            std::uint64_t const msb) noexcept
        {
            return msb &
                ~(gid_type::internal_bits_mask | gid_type::component_type_mask);
        }

        HPX_CXX_EXPORT constexpr gid_type&
        strip_internal_bits_and_component_type_from_gid(gid_type& id) noexcept
        {
            id.set_msb(
                strip_internal_bits_and_component_type_from_gid(id.get_msb()));
            return id;
        }

        HPX_CXX_EXPORT constexpr std::uint64_t get_internal_bits(
            std::uint64_t const msb) noexcept
        {
            return msb &
                (gid_type::internal_bits_mask | gid_type::component_type_mask);
        }

        HPX_CXX_EXPORT constexpr std::uint64_t
        strip_internal_bits_and_locality_from_gid(
            std::uint64_t const msb) noexcept
        {
            return msb &
                (~gid_type::special_bits_mask | gid_type::component_type_mask);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr std::uint32_t get_component_type_from_gid(
            std::uint64_t const msb) noexcept
        {
            HPX_ASSERT(!(msb & gid_type::dynamically_assigned));
            return (msb >> gid_type::component_type_shift) &
                gid_type::component_type_base_mask;
        }

        HPX_CXX_EXPORT constexpr std::uint64_t add_component_type_to_gid(
            std::uint64_t const msb, std::uint32_t const type) noexcept
        {
            HPX_ASSERT(!(msb & gid_type::dynamically_assigned));
            return (msb & ~gid_type::component_type_mask) |
                ((static_cast<std::uint64_t>(type)
                     << gid_type::component_type_shift) &
                    gid_type::component_type_mask);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr std::uint64_t strip_lock_from_gid(
            std::uint64_t const msb) noexcept
        {
            return msb & ~gid_type::is_locked_mask;
        }

        HPX_CXX_EXPORT constexpr gid_type& strip_lock_from_gid(
            gid_type& gid) noexcept
        {
            gid.set_msb(strip_lock_from_gid(gid.get_msb()));
            return gid;
        }

        HPX_CXX_EXPORT constexpr bool is_locked(gid_type const& gid) noexcept
        {
            return gid.is_locked();
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr gid_type get_stripped_gid(
            gid_type const& id) noexcept
        {
            std::uint64_t const msb =
                strip_internal_bits_from_gid(id.get_msb());
            std::uint64_t const lsb = id.get_lsb();
            return gid_type(msb, lsb);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr bool has_credits(gid_type const& gid) noexcept
        {
            return (gid.get_msb() & gid_type::has_credits_mask) ? true : false;
        }

        HPX_CXX_EXPORT constexpr bool gid_was_split(
            gid_type const& gid) noexcept
        {
            return (gid.get_msb() & gid_type::was_split_mask) ? true : false;
        }

        HPX_CXX_EXPORT constexpr void set_credit_split_mask_for_gid(
            gid_type& gid) noexcept
        {
            gid.set_msb(gid.get_msb() | gid_type::was_split_mask);
        }

        // We store the log2(credit) in the gid_type
        HPX_CXX_EXPORT constexpr std::int16_t log2(std::int64_t val) noexcept
        {
            std::int16_t ret = -1;
            while (val != 0)
            {
                val >>= 1;
                ++ret;
            }
            return ret;
        }

        HPX_CXX_EXPORT constexpr std::int64_t power2(
            std::int16_t const log2credits) noexcept
        {
            return static_cast<std::int64_t>(1) << log2credits;
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr bool is_migratable(gid_type const& id) noexcept
        {
            return (id.get_msb() & gid_type::is_migratable) ? true : false;
        }

        HPX_CXX_EXPORT constexpr void set_is_migratable(gid_type& gid) noexcept
        {
            gid.set_msb(gid.get_msb() | gid_type::is_migratable);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr gid_type get_stripped_gid_except_dont_cache(
            gid_type const& gid) noexcept
        {
            std::uint64_t const msb =
                strip_internal_bits_except_dont_cache_from_gid(gid.get_msb());
            std::uint64_t const lsb = gid.get_lsb();
            return gid_type(msb, lsb);
        }

        HPX_CXX_EXPORT constexpr std::uint64_t strip_credits_from_gid(
            std::uint64_t const msb) noexcept
        {
            return msb & ~gid_type::credit_bits_mask;
        }

        HPX_CXX_EXPORT constexpr gid_type& strip_credits_from_gid(
            gid_type& gid) noexcept
        {
            gid.set_msb(strip_credits_from_gid(gid.get_msb()));
            return gid;
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT constexpr std::int16_t get_log2credit_from_gid(
            gid_type const& gid) noexcept
        {
            return static_cast<std::int16_t>(
                (gid.get_msb() >> gid_type::credit_shift) &
                gid_type::credit_base_mask);
        }

        HPX_CXX_EXPORT constexpr std::int64_t get_credit_from_gid(
            gid_type const& gid) noexcept
        {
            return has_credits(gid) ?
                detail::power2(get_log2credit_from_gid(gid)) :
                0;
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT inline void set_log2credit_for_gid(
            gid_type& id, std::int16_t const log2credits) noexcept
        {
            // credit should be a clean log2
            HPX_ASSERT(log2credits >= 0);
            HPX_ASSERT(0 == (log2credits & ~gid_type::credit_base_mask));

            id.set_msb((id.get_msb() & ~gid_type::credit_mask) |
                ((static_cast<std::int64_t>(log2credits)
                     << gid_type::credit_shift) &
                    gid_type::credit_mask) |
                gid_type::has_credits_mask);
        }

        HPX_CXX_EXPORT inline void set_credit_for_gid(
            gid_type& id, std::int64_t const credits) noexcept
        {
            if (credits != 0)
            {
                std::int16_t const log2credits = detail::log2(credits);
                HPX_ASSERT(log2credits >= 0);
                HPX_ASSERT(detail::power2(log2credits) == credits);

                set_log2credit_for_gid(id, log2credits);
            }
            else
            {
                strip_credits_from_gid(id);
            }
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT inline constexpr gid_type const invalid_gid{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT std::ostream& operator<<(
        std::ostream& os, gid_type const& id);

    ///////////////////////////////////////////////////////////////////////////
    inline constexpr gid_type::gid_type(
        std::uint64_t const msb_id, std::uint64_t const lsb_id) noexcept
      : id_msb_(naming::detail::strip_lock_from_gid(msb_id))
      , id_lsb_(lsb_id)
    {
    }

    inline gid_type::gid_type(std::uint64_t const msb_id, void* lsb_id) noexcept
      : id_msb_(naming::detail::strip_lock_from_gid(msb_id))
      , id_lsb_(reinterpret_cast<std::uint64_t>(lsb_id))
    {
    }

    inline constexpr gid_type::gid_type(gid_type const& rhs) noexcept
      : id_msb_(naming::detail::strip_lock_from_gid(rhs.get_msb()))
      , id_lsb_(rhs.get_lsb())
    {
    }

    inline constexpr gid_type::gid_type(gid_type&& rhs) noexcept
      : id_msb_(naming::detail::strip_lock_from_gid(rhs.get_msb()))
      , id_lsb_(rhs.get_lsb())
    {
        rhs.id_lsb_ = rhs.id_msb_ = 0;
    }

    inline gid_type& gid_type::operator=(gid_type const& rhs) noexcept
    {
        if (this != &rhs)
        {
            HPX_ASSERT(!is_locked());
            id_msb_ = naming::detail::strip_lock_from_gid(rhs.get_msb());
            id_lsb_ = rhs.get_lsb();
        }
        return *this;
    }
    inline gid_type& gid_type::operator=(gid_type&& rhs) noexcept
    {
        if (this != &rhs)
        {
            HPX_ASSERT(!is_locked());
            id_msb_ = naming::detail::strip_lock_from_gid(rhs.get_msb());
            id_lsb_ = rhs.get_lsb();

            rhs.id_lsb_ = rhs.id_msb_ = 0;
        }
        return *this;
    }
}    // namespace hpx::naming

///////////////////////////////////////////////////////////////////////////////
namespace std {

    // specialize std::hash for hpx::naming::gid_type
    template <>
    struct hash<hpx::naming::gid_type>
    {
        std::size_t operator()(::hpx::naming::gid_type const& gid) const
        {
            std::size_t const h1(std::hash<std::uint64_t>()(gid.get_lsb()));
            std::size_t const h2(std::hash<std::uint64_t>()(
                hpx::naming::detail::strip_internal_bits_from_gid(
                    gid.get_msb())));
            return h1 ^ (h2 << 1);
        }
    };
}    // namespace std

#include <hpx/config/warnings_suffix.hpp>
