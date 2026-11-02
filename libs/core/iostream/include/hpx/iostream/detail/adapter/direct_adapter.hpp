//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/detail/double_object.hpp>
#include <hpx/iostream/detail/error.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/traits.hpp>

#include <algorithm>
#include <cstddef>
#include <ios>
#include <memory>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::detail {

    //------------------Definition of direct_adapter_base-------------------------//

    // Put all initialization in base class to facilitate forwarding.
    HPX_CXX_CORE_EXPORT template <typename Direct>
    class direct_adapter_base
    {
    public:
        using char_type = char_type_of_t<Direct>;
        using mode_type = mode_of_t<Direct>;

        struct category
          : mode_type
          , device_tag
          , closable_tag
          , localizable_tag
        {
        };

    protected:
        explicit direct_adapter_base(Direct const& d);

        using is_double = std::is_convertible<category, two_sequence>;

        struct pointers
        {
            pointers() = default;
            char_type *beg = nullptr, *ptr = nullptr, *end = nullptr;
        };

        void init_input(std::true_type);
        static constexpr void init_input(std::false_type) noexcept {}

        void init_output(std::true_type);
        static constexpr void init_output(std::false_type) noexcept {}

        double_object<pointers, is_double> ptrs_;
        Direct d_;
    };

    HPX_CXX_CORE_EXPORT template <typename Direct>
    class direct_adapter : private direct_adapter_base<Direct>
    {
        using base_type = direct_adapter_base<Direct>;
        using pointers = base_type::pointers;
        using is_double = base_type::is_double;
        using base_type::d_;
        using base_type::ptrs_;

    public:
        using char_type = base_type::char_type;
        using category = base_type::category;

        // Constructors
        explicit direct_adapter(Direct const& d)
          : base_type(d)
        {
        }

        template <typename T, typename T1, typename... Ts>
        explicit direct_adapter(T&& t, T1&& t1, Ts&&... ts)
          : base_type(Direct(
                HPX_FORWARD(T, t), HPX_FORWARD(T1, t1), HPX_FORWARD(Ts, ts)...))
        {
        }

        direct_adapter(direct_adapter const& d) = default;
        direct_adapter(direct_adapter&& d) = default;
        direct_adapter& operator=(direct_adapter const&) = default;
        direct_adapter& operator=(direct_adapter&&) = default;

        ~direct_adapter() = default;

        // Device interface.
        std::streamsize read(char_type* s, std::streamsize n);
        std::streamsize write(char_type const* s, std::streamsize n);
        std::streampos seek(stream_offset, std::ios_base::seekdir,
            std::ios_base::openmode = std::ios_base::in | std::ios_base::out);

        void close();
        void close(std::ios_base::openmode which);

        void imbue(std::locale const&);

        // Direct device access.
        Direct& operator*()
        {
            return d_;
        }
        Direct* operator->()
        {
            return std::addressof(d_);
        }
    };

    //--------------Definition of wrap_direct and unwrap_direct-------------------//
    HPX_CXX_CORE_EXPORT template <typename Device>
    struct wrap_direct_traits
      : std::conditional<is_direct_v<Device>, direct_adapter<Device>, Device>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Device>
    wrap_direct_traits<Device>::type wrap_direct(Device&& dev)
    {
        using type = wrap_direct_traits<std::decay_t<Device>>::type;
        return type(HPX_FORWARD(Device, dev));
    }

    HPX_CXX_CORE_EXPORT template <typename Device>
    Device& unwrap_direct(Device& d)
    {
        return d;
    }

    HPX_CXX_CORE_EXPORT template <typename Device>
    Device& unwrap_direct(direct_adapter<Device>& d)
    {
        return *d;
    }

    //--------------Implementation of direct_adapter_base-------------------------//
    template <typename Direct>
    direct_adapter_base<Direct>::direct_adapter_base(Direct const& d)
      : d_(d)
    {
        init_input(std::is_convertible<category, input>());
        init_output(std::is_convertible<category, output>());
    }

    template <typename Direct>
    void direct_adapter_base<Direct>::init_input(std::true_type)
    {
        auto seq = iostream::input_sequence(d_);
        ptrs_.first().beg = seq.data();
        ptrs_.first().ptr = seq.data();
        ptrs_.first().end = seq.data() + seq.size();
    }

    template <typename Direct>
    void direct_adapter_base<Direct>::init_output(std::true_type)
    {
        auto seq = iostream::output_sequence(d_);
        ptrs_.first().beg = seq.data();
        ptrs_.first().ptr = seq.data();
        ptrs_.first().end = seq.data() + seq.size();
    }

    //--------------Implementation of direct_adapter------------------------------//
    template <typename Direct>
    std::streamsize direct_adapter<Direct>::read(
        char_type* s, std::streamsize n)
    {
        pointers& get = ptrs_.first();
        std::streamsize avail = static_cast<std::streamsize>(get.end - get.ptr);
        std::streamsize result = (std::min) (n, avail);
        std::copy(get.ptr, get.ptr + result, s);
        get.ptr += result;
        return result != 0 ? result : -1;
    }

    template <typename Direct>
    std::streamsize direct_adapter<Direct>::write(
        char_type const* s, std::streamsize n)
    {
        pointers& put = ptrs_.second();
        if (n > static_cast<std::streamsize>(put.end - put.ptr))
            throw write_area_exhausted();
        std::copy(s, s + n, put.ptr);
        put.ptr += n;
        return n;
    }

    template <typename Direct>
    std::streampos direct_adapter<Direct>::seek(stream_offset off,
        std::ios_base::seekdir way, std::ios_base::openmode which)
    {
        pointers& get = ptrs_.first();
        pointers& put = ptrs_.second();
        if (way == std::ios_base::cur && get.ptr != put.ptr)
            throw bad_seek();

        std::ptrdiff_t next = 0;
        if constexpr (!is_double::value)
        {
            if (which & std::ios_base::in)
            {
                if (way == std::ios_base::beg)
                    next = off;
                else if (way == std::ios_base::cur)
                    next = get.ptr - get.beg + off;
                else
                    next = get.end - get.beg + off;
                if (next >= 0 && next <= get.end - get.beg)
                    get.ptr = get.beg + next;
                else
                    throw bad_seek();
            }
        }
        else
        {
            if (which & std::ios_base::out)
            {
                if (way == std::ios_base::beg)
                    next = off;
                else if (way == std::ios_base::cur)
                    next = put.ptr - put.beg + off;
                else
                    next = put.end - put.beg + off;
                if (next >= 0 && next <= put.end - put.beg)
                    put.ptr = put.beg + next;
                else
                    throw bad_seek();
            }
        }
        return offset_to_position(next);
    }

    template <typename Direct>
    void direct_adapter<Direct>::close()
    {
        static_assert(!std::is_convertible_v<category, two_sequence>);
        detail::close_all(d_);
    }

    template <typename Direct>
    void direct_adapter<Direct>::close(std::ios_base::openmode which)
    {
        static_assert(std::is_convertible_v<category, two_sequence>);
        iostream::close(d_, which);
    }

    template <typename Direct>
    void direct_adapter<Direct>::imbue(std::locale const& loc)
    {
        iostream::imbue(d_, loc);
    }
}    // namespace hpx::iostream::detail

#include <hpx/config/warnings_suffix.hpp>
