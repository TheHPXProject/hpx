//  Copyright (c)      2014 Thomas Heller
//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/parcelset_base/locality.hpp>
#include <hpx/parcelset_base/locality_interface.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

#if defined(HPX_WINDOWS)
#include <windows.h>
#else
#include <unistd.h>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    namespace {

        // FNV-1a: the same value in every process, unlike std::hash
        std::uint64_t fnv1a(std::uint64_t h, std::string_view const text)
        {
            for (char const c : text)
            {
                h ^= static_cast<unsigned char>(c);
                h *= 0x100000001b3ULL;
            }
            return h;
        }

        std::string host_name()
        {
#if defined(HPX_WINDOWS)
            char name[MAX_COMPUTERNAME_LENGTH + 1] = {};
            DWORD size = sizeof(name);
            if (GetComputerNameA(name, &size))
                return std::string(name, size);
            return {};
#else
            char name[256] = {};
            if (gethostname(name, sizeof(name) - 1) == 0)
                return name;
            return {};
#endif
        }

        // the kernel's boot id tells containers or virtual machines with
        // the same host name apart (and processes on one node share it)
        std::string boot_id()
        {
#if defined(HPX_WINDOWS)
            return {};
#else
            std::ifstream in("/proc/sys/kernel/random/boot_id");
            std::string id;
            std::getline(in, id);
            return id;
#endif
        }
    }    // namespace

    std::uint64_t this_node_identity()
    {
        static std::uint64_t const identity = [] {
            std::uint64_t h = fnv1a(0xcbf29ce484222325ULL, host_name());
            h = fnv1a(h, "/");
            h = fnv1a(h, boot_id());
            return h == 0 ? 1 : h;
        }();
        return identity;
    }

    locality::locality(locality const& other)
      : impl_(other.impl_ ? other.impl_->clone() : nullptr)
      , capabilities_(other.capabilities_)
    {
    }

    locality::locality(locality&& other) noexcept
      : impl_(other.impl_ ? other.impl_->move() : nullptr)
      , capabilities_(HPX_MOVE(other.capabilities_))
    {
    }

    locality& locality::operator=(locality const& other)
    {
        if (this != &other)
        {
            if (other.impl_)
            {
                impl_.reset(other.impl_->clone());
            }
            else
            {
                impl_.reset();
            }
            capabilities_ = other.capabilities_;
        }
        return *this;
    }

    locality& locality::operator=(locality&& other) noexcept
    {
        if (this != &other)
        {
            if (other.impl_)
            {
                impl_.reset(other.impl_->move());
            }
            else
            {
                impl_.reset();
            }
            capabilities_ = HPX_MOVE(other.capabilities_);
        }
        return *this;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool operator==(locality const& lhs, locality const& rhs)
    {
        if (lhs.impl_ == rhs.impl_)
            return true;
        if (!lhs.impl_ || !rhs.impl_)
            return false;
        return lhs.impl_->equal(*rhs.impl_);
    }

    bool operator!=(locality const& lhs, locality const& rhs)
    {
        return !(lhs == rhs);
    }

    bool operator<(locality const& lhs, locality const& rhs)
    {
        if (lhs.impl_ == rhs.impl_)
            return false;
        if (!lhs.impl_ || !rhs.impl_)
            return false;
        return lhs.impl_->less_than(*rhs.impl_);
    }

    bool operator>(locality const& lhs, locality const& rhs)
    {
        if (lhs.impl_ == rhs.impl_)
            return false;
        if (!lhs.impl_ || !rhs.impl_)
            return false;
        return !(lhs < rhs) && lhs != rhs;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& os, locality const& l)
    {
        if (!l.impl_)
            return os;
        return l.impl_->print(os);
    }

    ///////////////////////////////////////////////////////////////////////////
    void locality::save(
        [[maybe_unused]] serialization::output_archive& ar, unsigned int) const
    {
#if defined(HPX_HAVE_NETWORKING)
        std::string const t = type();
        ar << t;
        if (t.empty())
        {
            return;
        }

        impl_->save(ar);
        ar << capabilities_;
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "locality::save",
            "this shouldn't be called if networking is disabled");
#endif
    }

    void locality::load(
        [[maybe_unused]] serialization::input_archive& ar, unsigned int)
    {
#if defined(HPX_HAVE_NETWORKING)
        std::string t;
        ar >> t;
        if (t.empty())
        {
            return;
        }

        impl_ = HPX_MOVE(create_locality(t).impl_);
        impl_->load(ar);
        HPX_ASSERT(impl_->valid());
        ar >> capabilities_;
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "locality::load",
            "this shouldn't be called if networking is disabled");
#endif
    }

    std::ostream& operator<<(std::ostream& os, endpoints_type const& endpoints)
    {
        hpx::util::ios_flags_saver ifs(os);
        os << "[ ";
        for (endpoints_type::value_type const& loc : endpoints)
        {
            os << "(" << loc.first << ":" << loc.second << ") ";
        }
        os << "]";

        return os;
    }
}    // namespace hpx::parcelset
