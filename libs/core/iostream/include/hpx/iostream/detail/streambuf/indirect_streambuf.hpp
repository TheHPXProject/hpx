//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
// See http://www.boost.org/libs/iostreams for documentation.

// This material is heavily indebted to the discussion and code samples in
// A. Langer and K. Kreft, "Standard C++ IOStreams and Locales",
// Addison-Wesley, 2000, pp. 228-43.

// User "GMSB" provided an optimization for small seeks.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/constants.hpp>
#include <hpx/iostream/detail/adapter/concept_adapter.hpp>
#include <hpx/iostream/detail/buffer.hpp>
#include <hpx/iostream/detail/double_object.hpp>
#include <hpx/iostream/detail/error.hpp>
#include <hpx/iostream/detail/execute.hpp>
#include <hpx/iostream/detail/functional.hpp>
#include <hpx/iostream/detail/streambuf/linked_streambuf.hpp>
#include <hpx/iostream/optimal_buffer_size.hpp>
#include <hpx/iostream/positioning.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/modules/datastructures.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <locale>
#include <streambuf>
#include <type_traits>
#include <typeinfo>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::detail {

    //
    // Description: The implementation of basic_streambuf used by chains.
    //
    HPX_CXX_CORE_EXPORT template <typename T, typename Tr, typename Alloc,
        typename Mode>
    class indirect_streambuf : public linked_streambuf<char_type_of_t<T>, Tr>
    {
    public:
        using char_type = char_type_of_t<T>;
        using traits_type = Tr;
        using int_type = traits_type::int_type;
        using off_type = traits_type::off_type;
        using pos_type = traits_type::pos_type;

    private:
        using category = category_of_t<T>;
        using wrapper = concept_adapter<T>;
        using buffer_type = basic_buffer<char_type, Alloc>;
        using my_type = indirect_streambuf<T, Tr, Alloc, Mode>;
        using base_type = linked_streambuf<char_type, traits_type>;
        using streambuf_type = linked_streambuf<char_type, Tr>;

    public:
        indirect_streambuf() = default;

        void open(T const& t, std::streamsize buffer_size = -1,
            std::streamsize pback_size = -1);

        [[nodiscard]] constexpr bool is_open() const noexcept;
        void close();

        [[nodiscard]] bool auto_close() const override;
        void set_auto_close(bool close) override;
        bool strict_sync() noexcept override;

        // Declared in linked_streambuf.
        T* component()
        {
            return &*obj();
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

        //----------virtual functions---------------------------------------------//
        void imbue(std::locale const& loc) override;

        int_type underflow() override;
        int_type pbackfail(int_type c) override;
        int_type overflow(int_type c) override;
        int sync() noexcept override;
        pos_type seekoff(off_type off, std::ios_base::seekdir way,
            std::ios_base::openmode which) override;
        pos_type seekpos(pos_type sp, std::ios_base::openmode which) override;

        // Declared in linked_streambuf.
        void set_next(streambuf_type* next) override;
        void close_impl(std::ios_base::openmode which) override;
        [[nodiscard]] std::type_info const& component_type() const override
        {
            return typeid(T);
        }

        void* component_impl() override
        {
            return component();
        }

    private:
        //----------Accessor functions--------------------------------------------//
        wrapper& obj()
        {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            return *storage_;
        }

        streambuf_type* next() const
        {
            return next_;
        }

        buffer_type& in()
        {
            return buffer_.first();
        }

        buffer_type& out()
        {
            return buffer_.second();
        }

        [[nodiscard]] constexpr bool can_read() const noexcept
        {
            return std::is_convertible_v<Mode, input>;
        }

        [[nodiscard]] constexpr bool can_write() const noexcept
        {
            return std::is_convertible_v<Mode, output>;
        }

        [[nodiscard]] constexpr bool output_buffered() const noexcept
        {
            return (flags_ & flags::output_buffered) != 0;
        }

        [[nodiscard]] constexpr bool shared_buffer() const noexcept
        {
            return std::is_convertible_v<Mode, seekable> ||
                std::is_convertible_v<Mode, dual_seekable>;
        }

        void set_flags(int const f) noexcept
        {
            flags_ = f;
        }

        //----------State changing functions--------------------------------------//
        virtual void init_get_area();
        virtual void init_put_area();

        //----------Utility function----------------------------------------------//
        pos_type seek_impl(stream_offset off, std::ios_base::seekdir way,
            std::ios_base::openmode which);
        void sync_impl();

        enum class flags : std::uint8_t
        {
            open = 1,
            output_buffered = open << 1,
            auto_close = output_buffered << 1
        };

        friend constexpr int operator&(int const lhs, flags rhs) noexcept
        {
            return lhs & static_cast<int>(rhs);
        }

        friend constexpr int operator|(flags lhs, flags rhs) noexcept
        {
            return static_cast<int>(lhs) | static_cast<int>(rhs);
        }

        friend constexpr int operator|(int const lhs, flags rhs) noexcept
        {
            return lhs | static_cast<int>(rhs);
        }

        friend constexpr int operator~(flags rhs) noexcept
        {
            return ~static_cast<int>(rhs);
        }

        optional<wrapper> storage_;
        streambuf_type* next_ = nullptr;
        double_object<buffer_type, std::is_convertible<Mode, two_sequence>>
            buffer_;
        std::streamsize pback_size_ = 0;
        int flags_ = static_cast<int>(flags::auto_close);
    };

    //--------------Implementation of open, is_open and close---------------------//
    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::open(
        T const& t, std::streamsize buffer_size, std::streamsize pback_size)
    {
        using namespace std;

        // Normalize buffer sizes.
        buffer_size = (buffer_size != -1) ? buffer_size :
                                            iostream::optimal_buffer_size(t);
        pback_size =
            (pback_size != -1) ? pback_size : default_pback_buffer_size;

        // Construct input buffer.
        if (can_read())
        {
            pback_size_ =
                (std::max) (static_cast<std::streamsize>(2), pback_size);
            std::streamsize const size = pback_size_ +
                (buffer_size ? buffer_size : static_cast<std::streamsize>(1));
            in().resize(static_cast<int>(size));
            if (!shared_buffer())
                init_get_area();
        }

        // Construct output buffer.
        if (can_write() && !shared_buffer())
        {
            if (buffer_size != static_cast<std::streamsize>(0))
                out().resize(static_cast<int>(buffer_size));
            init_put_area();
        }

        storage_.emplace(t);
        flags_ = flags_ | flags::open;
        if (can_write() && buffer_size > 1)
            flags_ = flags_ | flags::output_buffered;

        this->set_true_eof(false);
        this->set_needs_close();
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    constexpr bool indirect_streambuf<T, Tr, Alloc, Mode>::is_open()
        const noexcept
    {
        return (flags_ & flags::open) != 0;
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::close()
    {
        using namespace std;
        base_type* self = this;
        detail::execute_all(detail::call_member_close(*self, std::ios_base::in),
            detail::call_member_close(*self, std::ios_base::out),
            detail::call_reset(storage_), detail::clear_flags(flags_));
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    bool indirect_streambuf<T, Tr, Alloc, Mode>::auto_close() const
    {
        return (flags_ & flags::auto_close) != 0;
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::set_auto_close(
        bool const close)
    {
        flags_ = (flags_ & ~flags::auto_close) |
            (close ? static_cast<int>(flags::auto_close) : 0);
    }

    //--------------Implementation virtual functions------------------------------//
    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::imbue(std::locale const& loc)
    {
        if (is_open())
        {
            obj().imbue(loc);
            if (next_)
                next_->pubimbue(loc);
        }
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    indirect_streambuf<T, Tr, Alloc, Mode>::int_type
    indirect_streambuf<T, Tr, Alloc, Mode>::underflow()
    {
        using namespace std;
        if (!gptr())
            init_get_area();
        buffer_type& buf = in();
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());

        // Fill putback buffer.
        std::streamsize keep =
            (std::min) (static_cast<std::streamsize>(gptr() - eback()),
                pback_size_);
        if (keep)
        {
            // keep is bounded by pback_size_, which is the size of the
            // putback area, so it always fits
            traits_type::move(buf.data() + (pback_size_ - keep), gptr() - keep,
                static_cast<std::size_t>(keep));
        }

        // Set pointers to reasonable values in case read throws.
        setg(buf.data() + pback_size_ - keep, buf.data() + pback_size_,
            buf.data() + pback_size_);

        // Read from source.
        std::streamsize chars = obj().read(
            buf.data() + pback_size_, buf.size() - pback_size_, next_);
        if (chars == -1)
        {
            this->set_true_eof(true);
            chars = 0;
        }
        setg(eback(), gptr(), buf.data() + pback_size_ + chars);
        return chars != 0 ? traits_type::to_int_type(*gptr()) :
                            traits_type::eof();
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    indirect_streambuf<T, Tr, Alloc, Mode>::int_type
    indirect_streambuf<T, Tr, Alloc, Mode>::pbackfail(int_type c)
    {
        if (gptr() != eback())
        {
            gbump(-1);
            if (!traits_type::eq_int_type(c, traits_type::eof()))
                *gptr() = traits_type::to_char_type(c);
            return traits_type::not_eof(c);
        }
        else
        {
            throw bad_putback();
        }
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    indirect_streambuf<T, Tr, Alloc, Mode>::int_type
    indirect_streambuf<T, Tr, Alloc, Mode>::overflow(int_type c)
    {
        if ((output_buffered() && pptr() == nullptr) ||
            (shared_buffer() && gptr() != nullptr))
        {
            init_put_area();
        }

        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            if (output_buffered())
            {
                if (pptr() == epptr())
                {
                    sync_impl();
                    if (pptr() == epptr())
                        return traits_type::eof();
                }
                *pptr() = traits_type::to_char_type(c);
                pbump(1);
            }
            else
            {
                char_type d = traits_type::to_char_type(c);
                if (obj().write(&d, 1, next_) != 1)
                    return traits_type::eof();
            }
        }
        return traits_type::not_eof(c);
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    int indirect_streambuf<T, Tr, Alloc, Mode>::sync() noexcept
    {
        try
        {
            // sync() is no-throw.
            sync_impl();
            obj().flush(next_);
            return 0;
        }
        catch (...)
        {
            return -1;
        }
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    bool indirect_streambuf<T, Tr, Alloc, Mode>::strict_sync() noexcept
    {
        try
        {
            // sync() is no-throw.
            sync_impl();
            return obj().flush(next_);
        }
        catch (...)
        {
            return false;
        }
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    indirect_streambuf<T, Tr, Alloc, Mode>::pos_type
    indirect_streambuf<T, Tr, Alloc, Mode>::seekoff(
        off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
    {
        return seek_impl(off, way, which);
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    indirect_streambuf<T, Tr, Alloc, Mode>::pos_type
    indirect_streambuf<T, Tr, Alloc, Mode>::seekpos(
        pos_type sp, std::ios_base::openmode const which)
    {
        return seek_impl(position_to_offset(sp), std::ios_base::beg, which);
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    indirect_streambuf<T, Tr, Alloc, Mode>::pos_type
    indirect_streambuf<T, Tr, Alloc, Mode>::seek_impl(stream_offset off,
        std::ios_base::seekdir way, std::ios_base::openmode which)
    {
        if (gptr() != nullptr && way == std::ios_base::cur &&
            which == std::ios_base::in && eback() - gptr() <= off &&
            off <= egptr() - gptr())
        {
            // Small seek optimization
            gbump(static_cast<int>(off));
            return obj().seek(static_cast<stream_offset>(0), std::ios_base::cur,
                       std::ios_base::in, next_) -
                static_cast<off_type>(egptr() - gptr());
        }

        if (pptr() != nullptr)
            this->pubsync();
        if (way == std::ios_base::cur && gptr())
            off -= static_cast<off_type>(egptr() - gptr());

        constexpr bool two_head =
            std::is_convertible_v<category, dual_seekable> ||
            std::is_convertible_v<category, bidirectional_seekable>;
        if constexpr (two_head)
        {
            constexpr std::ios_base::openmode both =
                std::ios_base::in | std::ios_base::out;
            if ((which & both) == both)
                throw bad_seek();
            if (which & std::ios_base::in)
            {
                setg(nullptr, nullptr, nullptr);
            }
            if (which & std::ios_base::out)
            {
                setp(nullptr, nullptr);
            }
        }
        else
        {
            setg(nullptr, nullptr, nullptr);
            setp(nullptr, nullptr);
        }
        return obj().seek(off, way, which, next_);
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::set_next(streambuf_type* next)
    {
        next_ = next;
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::close_impl(
        std::ios_base::openmode which)
    {
        if (which == std::ios_base::in && std::is_convertible_v<Mode, input>)
        {
            setg(nullptr, nullptr, nullptr);
        }
        if (which == std::ios_base::out && std::is_convertible_v<Mode, output>)
        {
            sync();
            setp(nullptr, nullptr);
        }

        if (!std::is_convertible_v<category, dual_use> ||
            std::is_convertible_v<Mode, input> == (which == std::ios_base::in))
        {
            obj().close(which, next_);
        }
    }

    //----------State changing functions------------------------------------------//
    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::sync_impl()
    {
        if (std::streamsize avail =
                static_cast<std::streamsize>(pptr() - pbase());
            avail > 0)
        {
            std::streamsize amt = obj().write(pbase(), avail, next());
            if (amt == avail)
            {
                setp(out().begin(), out().end());
            }
            else
            {
                char_type const* ptr = pptr();
                setp(out().begin() + amt, out().end());
                pbump(static_cast<int>(ptr - pptr()));
            }
        }
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::init_get_area()
    {
        if (shared_buffer() && pptr() != nullptr)
        {
            sync_impl();
            setp(nullptr, nullptr);
        }
        setg(in().begin(), in().begin(), in().begin());
    }

    template <typename T, typename Tr, typename Alloc, typename Mode>
    void indirect_streambuf<T, Tr, Alloc, Mode>::init_put_area()
    {
        using namespace std;
        if (shared_buffer() && gptr() != nullptr)
        {
            obj().seek(static_cast<off_type>(gptr() - egptr()),
                std::ios_base::cur, std::ios_base::in, next_);
            setg(nullptr, nullptr, nullptr);
        }
        if (output_buffered())
            setp(out().begin(), out().end());
        else
            setp(nullptr, nullptr);
    }
}    // namespace hpx::iostream::detail

#include <hpx/config/warnings_suffix.hpp>
