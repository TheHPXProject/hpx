//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/naming_base/gid_type.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::naming {

    ///////////////////////////////////////////////////////////////////////////
    bool operator<(gid_type const& lhs, gid_type const& rhs) noexcept
    {
        auto const lhs_msb = static_cast<std::int64_t>(
            detail::strip_internal_bits_from_gid(lhs.id_msb_));
        auto const rhs_msb = static_cast<std::int64_t>(
            detail::strip_internal_bits_from_gid(rhs.id_msb_));

        if (lhs_msb < rhs_msb)
        {
            return true;
        }
        if (lhs_msb > rhs_msb)
        {
            return false;
        }
        return lhs.id_lsb_ < rhs.id_lsb_;
    }

    bool operator<=(gid_type const& lhs, gid_type const& rhs) noexcept
    {
        auto const lhs_msb = static_cast<std::int64_t>(
            detail::strip_internal_bits_from_gid(lhs.id_msb_));
        auto const rhs_msb = static_cast<std::int64_t>(
            detail::strip_internal_bits_from_gid(rhs.id_msb_));

        if (lhs_msb < rhs_msb)
        {
            return true;
        }
        if (lhs_msb > rhs_msb)
        {
            return false;
        }
        return lhs.id_lsb_ <= rhs.id_lsb_;
    }

    ///////////////////////////////////////////////////////////////////////////
    gid_type operator+(gid_type const& lhs, gid_type const& rhs) noexcept
    {
        std::uint64_t const lsb = lhs.id_lsb_ + rhs.id_lsb_;
        std::uint64_t msb = lhs.id_msb_ + rhs.id_msb_;

#if defined(HPX_DEBUG)
        // make sure we're using the operator+ in proper contexts only
        std::uint64_t const lhs_internal_bits =
            detail::get_internal_bits(lhs.id_msb_);

        std::uint64_t const msb_test =
            detail::strip_internal_bits_and_component_type_from_gid(
                lhs.id_msb_) +
            detail::strip_internal_bits_and_locality_from_gid(rhs.id_msb_);

        HPX_ASSERT(msb == (msb_test | lhs_internal_bits));
#endif

        if (lsb < lhs.id_lsb_ || lsb < rhs.id_lsb_)
            ++msb;

        return gid_type(msb, lsb);
    }

    gid_type operator-(gid_type const& lhs, gid_type const& rhs) noexcept
    {
        std::uint64_t const lsb = lhs.id_lsb_ - rhs.id_lsb_;
        std::uint64_t msb = lhs.id_msb_ - rhs.id_msb_;

        if (lsb > lhs.id_lsb_)
            --msb;

        return gid_type(msb, lsb);
    }

    std::string gid_type::to_string() const
    {
        return hpx::util::format("{:016llx}{:016llx}", id_msb_, id_lsb_);
    }

    std::ostream& operator<<(std::ostream& os, gid_type const& id)
    {
        hpx::util::ios_flags_saver ifs(os);
        if (id != naming::invalid_gid)
        {
            hpx::util::format_to(
                os, "{{{:016llx}, {:016llx}}}", id.id_msb_, id.id_lsb_);
        }
        else
        {
            os << "{invalid}";
        }
        return os;
    }

    ///////////////////////////////////////////////////////////////////////////
    void save(
        serialization::output_archive& ar, gid_type const& gid, unsigned int)
    {
        ar << gid.id_msb_ << gid.id_lsb_;
    }

    void load(serialization::input_archive& ar, gid_type& gid, unsigned int)
    {
        ar >> gid.id_msb_ >> gid.id_lsb_;

        // strip lock-bit upon receive
        gid.id_msb_ &= ~gid_type::is_locked_mask;
    }
}    // namespace hpx::naming
