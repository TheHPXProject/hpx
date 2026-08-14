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
#include <hpx/assert.hpp>
#include <hpx/iostream/detail/error.hpp>
#include <hpx/iostream/detail/execute.hpp>
#include <hpx/iostream/detail/functional.hpp>
#include <hpx/iostream/detail/streambuf/linked_streambuf.hpp>
#include <hpx/iostream/operations.hpp>
#include <hpx/iostream/positioning.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/modules/datastructures.hpp>

#include <cstddef>
#include <iosfwd>
#include <streambuf>
#include <string>
#include <typeinfo>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::detail {

    HPX_CXX_CORE_EXPORT template <typename T,
        typename Tr = std::char_traits<char_type_of_t<T>>>
    class direct_streambuf : public linked_streambuf<char_type_of_t<T>, Tr>
    {
    public:
        using char_type = char_type_of_t<T>;
        using traits_type = Tr;
        using int_type = traits_type::int_type;
        using off_type = traits_type::off_type;
        using pos_type = traits_type::pos_type;

    private:
        using base_type = linked_streambuf<char_type, traits_type>;
        using category = category_of_t<T>;
        using streambuf_type = std::basic_streambuf<char_type, traits_type>;

    public:
        void open(T const& t, std::streamsize buffer_size,
            std::streamsize pback_size);

        [[nodiscard]] bool is_open() const;
        void close();
        [[nodiscard]] bool auto_close() const override
        {
            return auto_close_;
        }
        void set_auto_close(bool const close) override
        {
            auto_close_ = close;
        }
        bool strict_sync() override
        {
            return true;
        }

        // Declared in linked_streambuf.
        T* component()
        {
            if (!storage_.has_value())
                throw std::ios_base::failure("stream buffer not open");
            return &*storage_;
        }

    protected:
        using base_type::eback;
        using base_type::egptr;
        using base_type::epptr;
        using base_type::gbump;
        using base_type::gptr;
        using base_type::overflow;
        using base_type::pbackfail;
        using base_type::pbase;
        using base_type::pbump;
        using base_type::pptr;
        using base_type::seekoff;
        using base_type::seekpos;
        using base_type::setg;
        using base_type::setp;
        using base_type::sync;
        using base_type::underflow;
        using base_type::xsgetn;
        using base_type::xsputn;

        direct_streambuf();

        //--------------Virtual functions-----------------------------------------//

        // Declared in linked_streambuf.
        void close_impl(std::ios_base::openmode which) override;

        [[nodiscard]] std::type_info const& component_type() const override
        {
            return typeid(T);
        }
        void* component_impl() override
        {
            return component();
        }

        // Declared in basic_streambuf.
        int_type underflow() override;
        int_type pbackfail(int_type c) override;
        int_type overflow(int_type c) override;
        pos_type seekoff(off_type off, std::ios_base::seekdir way,
            std::ios_base::openmode which) override;
        pos_type seekpos(pos_type sp, std::ios_base::openmode which) override;

    private:
        pos_type seek_impl(stream_offset off, std::ios_base::seekdir way,
            std::ios_base::openmode which);
        static void init_input(any_tag) {}
        void init_input(input);
        static void init_output(any_tag) {}
        void init_output(output);
        void init_get_area();
        void init_put_area();
        [[nodiscard]] bool one_head() const;
        [[nodiscard]] bool two_head() const;

        optional<T> storage_;
        char_type *ibeg_ = nullptr, *iend_ = nullptr, *obeg_ = nullptr,
                  *oend_ = nullptr;
        bool auto_close_ = true;
    };

    //------------------Implementation of direct_streambuf------------------------//
    template <typename T, typename Tr>
    direct_streambuf<T, Tr>::direct_streambuf()
    {
        this->set_true_eof(true);
    }

    template <typename T, typename Tr>
    void direct_streambuf<T, Tr>::open(
        T const& t, std::streamsize, std::streamsize)
    {
        storage_.emplace(t);
        init_input(category());
        init_output(category());
        setg(nullptr, nullptr, nullptr);
        setp(nullptr, nullptr);
        this->set_needs_close();
    }

    template <typename T, typename Tr>
    bool direct_streambuf<T, Tr>::is_open() const
    {
        return ibeg_ != nullptr || obeg_ != nullptr;
    }

    template <typename T, typename Tr>
    void direct_streambuf<T, Tr>::close()
    {
        base_type* self = this;
        detail::execute_all(detail::call_member_close(*self, std::ios_base::in),
            detail::call_member_close(*self, std::ios_base::out),
            detail::call_reset(storage_));
    }

    template <typename T, typename Tr>
    direct_streambuf<T, Tr>::int_type direct_streambuf<T, Tr>::underflow()
    {
        if (!ibeg_)
            throw cant_read();
        if (!gptr())
            init_get_area();
        if (gptr() != iend_)
            return traits_type::to_int_type(*gptr());
        return traits_type::eof();
    }

    template <typename T, typename Tr>
    direct_streambuf<T, Tr>::int_type direct_streambuf<T, Tr>::pbackfail(
        int_type c)
    {
        using namespace std;
        if (!ibeg_)
            throw cant_read();
        if (gptr() != nullptr && gptr() != ibeg_)
        {
            gbump(-1);
            if (!traits_type::eq_int_type(c, traits_type::eof()))
                *gptr() = traits_type::to_char_type(c);
            return traits_type::not_eof(c);
        }
        throw bad_putback();
    }

    template <typename T, typename Tr>
    direct_streambuf<T, Tr>::int_type direct_streambuf<T, Tr>::overflow(
        int_type c)
    {
        using namespace std;
        if (!obeg_)
            throw std::ios_base::failure("no write access");
        if (!pptr())
            init_put_area();
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            if (pptr() == oend_)
                throw std::ios_base::failure("write area exhausted");
            *pptr() = traits_type::to_char_type(c);
            pbump(1);
            return c;
        }
        return traits_type::not_eof(c);
    }

    template <typename T, typename Tr>
    direct_streambuf<T, Tr>::pos_type direct_streambuf<T, Tr>::seekoff(
        off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
    {
        return seek_impl(off, way, which);
    }

    template <typename T, typename Tr>
    direct_streambuf<T, Tr>::pos_type direct_streambuf<T, Tr>::seekpos(
        pos_type sp, std::ios_base::openmode const which)
    {
        return seek_impl(position_to_offset(sp), std::ios_base::beg, which);
    }

    template <typename T, typename Tr>
    void direct_streambuf<T, Tr>::close_impl(std::ios_base::openmode which)
    {
        if (which == std::ios_base::in && ibeg_ != nullptr)
        {
            setg(nullptr, nullptr, nullptr);
            ibeg_ = iend_ = nullptr;
        }
        if (which == std::ios_base::out && obeg_ != nullptr)
        {
            sync();
            setp(nullptr, nullptr);
            obeg_ = oend_ = nullptr;
        }
        if (!storage_.has_value())
            throw std::ios_base::failure("stream buffer not open");
        iostream::close(*storage_, which);
    }

    template <typename T, typename Tr>
    direct_streambuf<T, Tr>::pos_type direct_streambuf<T, Tr>::seek_impl(
        stream_offset off, std::ios_base::seekdir const way,
        std::ios_base::openmode const which)
    {
        std::ios_base::openmode const both =
            std::ios_base::in | std::ios_base::out;
        if (two_head() && (which & both) == both)
            throw bad_seek();

        stream_offset result = -1;
        bool one = one_head();
        if (one && (pptr() != nullptr || gptr() == nullptr))
            init_get_area();    // Switch to input mode, for code reuse.

        if (one || ((which & std::ios_base::in) != 0 && ibeg_ != nullptr))
        {
            if (!gptr())
                setg(ibeg_, ibeg_, iend_);
            ptrdiff_t next = 0;
            switch (way)
            {
            case std::ios_base::beg:
                next = off;
                break;
            case std::ios_base::cur:
                next = (gptr() - ibeg_) + off;
                break;
            case std::ios_base::end:
                next = (iend_ - ibeg_) + off;
                break;
            default:
                HPX_ASSERT(false);
            }
            if (next < 0 || next > (iend_ - ibeg_))
                throw bad_seek();
            setg(ibeg_, ibeg_ + next, iend_);
            result = next;
        }

        if (!one && (which & std::ios_base::out) != 0 && obeg_ != nullptr)
        {
            if (!pptr())
                setp(obeg_, oend_);
            ptrdiff_t next = 0;
            switch (way)
            {
            case std::ios_base::beg:
                next = off;
                break;
            case std::ios_base::cur:
                next = (pptr() - obeg_) + off;
                break;
            case std::ios_base::end:
                next = (oend_ - obeg_) + off;
                break;
            default:
                HPX_ASSERT(false);
            }
            if (next < 0 || next > (oend_ - obeg_))
                throw bad_seek();
            pbump(static_cast<int>(next - (pptr() - obeg_)));
            result = next;
        }
        return offset_to_position(result);
    }

    template <typename T, typename Tr>
    void direct_streambuf<T, Tr>::init_input(input)
    {
        if (!storage_.has_value())
            throw std::ios_base::failure("stream buffer not open");
        auto p = input_sequence(*storage_);
        ibeg_ = p.data();
        iend_ = p.data() + p.size();
    }

    template <typename T, typename Tr>
    void direct_streambuf<T, Tr>::init_output(output)
    {
        if (!storage_.has_value())
            throw std::ios_base::failure("stream buffer not open");
        auto p = output_sequence(*storage_);
        obeg_ = p.data();
        oend_ = p.data() + p.size();
    }

    template <typename T, typename Tr>
    void direct_streambuf<T, Tr>::init_get_area()
    {
        setg(ibeg_, ibeg_, iend_);
        if (one_head() && pptr())
        {
            gbump(static_cast<int>(pptr() - obeg_));
            setp(nullptr, nullptr);
        }
    }

    template <typename T, typename Tr>
    void direct_streambuf<T, Tr>::init_put_area()
    {
        setp(obeg_, oend_);
        if (one_head() && gptr())
        {
            pbump(static_cast<int>(gptr() - ibeg_));
            setg(nullptr, nullptr, nullptr);
        }
    }

    template <typename T, typename Tr>
    bool direct_streambuf<T, Tr>::one_head() const
    {
        return ibeg_ && obeg_ && ibeg_ == obeg_;
    }

    template <typename T, typename Tr>
    bool direct_streambuf<T, Tr>::two_head() const
    {
        return ibeg_ && obeg_ && ibeg_ != obeg_;
    }
}    // namespace hpx::iostream::detail

#include <hpx/config/warnings_suffix.hpp>
