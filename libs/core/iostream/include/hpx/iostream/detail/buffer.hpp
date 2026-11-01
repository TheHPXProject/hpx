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
#include <hpx/iostream/char_traits.hpp>
#include <hpx/iostream/checked_operations.hpp>
#include <hpx/iostream/read.hpp>
#include <hpx/iostream/traits.hpp>

#include <ios>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::iostream::detail {

    //----------------Buffers-----------------------------------------------------//

    //
    // Template name: buffer
    // Description: Character buffer.
    // Template parameters:
    //     Ch - The character type.
    //     Alloc - The Allocator type.
    //
    HPX_CXX_CORE_EXPORT template <typename Ch,
        typename Alloc = std::allocator<Ch>>
    class basic_buffer
    {
        using allocator_type =
            std::allocator_traits<Alloc>::template rebind_alloc<Ch>;
        using allocator_traits = std::allocator_traits<allocator_type>;

        static Ch* allocate(std::streamsize buffer_size);

    public:
        basic_buffer() = default;
        explicit basic_buffer(std::streamsize buffer_size);

        ~basic_buffer();

        basic_buffer(basic_buffer const&) = delete;
        basic_buffer(basic_buffer&&) = delete;
        basic_buffer& operator=(basic_buffer const&) = delete;
        basic_buffer& operator=(basic_buffer&&) = delete;

        void resize(std::streamsize buffer_size);

        Ch* begin() const
        {
            return buf_;
        }

        Ch* end() const
        {
            return buf_ + size_;
        }

        Ch* data() const
        {
            return buf_;
        }

        [[nodiscard]] std::streamsize size() const
        {
            return size_;
        }

        void swap(basic_buffer& rhs) noexcept;

    private:
        Ch* buf_;
        std::streamsize size_;
    };

    HPX_CXX_CORE_EXPORT template <typename Ch, typename Alloc>
    void swap(
        basic_buffer<Ch, Alloc>& lhs, basic_buffer<Ch, Alloc>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    //
    // Template name: buffer
    // Description: Character buffer with two pointers accessible via ptr() and
    //      eptr().
    // Template parameters:
    //     Ch - A character type.
    //
    HPX_CXX_CORE_EXPORT template <typename Ch,
        typename Alloc = std::allocator<Ch>>
    class buffer : public basic_buffer<Ch, Alloc>
    {
        using base = basic_buffer<Ch, Alloc>;

    public:
        using traits_type = iostream::char_traits<Ch>;
        using const_pointer = Ch* const;

        using base::data;
        using base::resize;
        using base::size;

        explicit buffer(std::streamsize buffer_size);

        Ch*& ptr()
        {
            return ptr_;
        }

        const_pointer& ptr() const
        {
            return ptr_;
        }

        Ch*& eptr()
        {
            return eptr_;
        }

        const_pointer& eptr() const
        {
            return eptr_;
        }

        void set(std::streamsize ptr, std::streamsize end);
        void swap(buffer& rhs) noexcept;

        // Returns an int_type as a status code.
        template <typename Source>
        int_type_of_t<Source> fill(Source& src)
        {
            using namespace std;
            std::streamsize keep = static_cast<std::streamsize>(eptr_ - ptr_);
            if (keep > 0)
            {
                traits_type::move(
                    this->data(), ptr_, static_cast<size_t>(keep));
            }
            set(0, keep);

            std::streamsize const result =
                iostream::read(src, this->data() + keep, this->size() - keep);
            if (result != -1)
                this->set(nullptr, keep + result);
            if (result == -1)
                return traits_type::eof();
            if (result == 0)
                return traits_type::would_block();
            return traits_type::good();
        }

        // Returns true if one or more characters were written.
        template <typename Sink>
        bool flush(Sink& dest)
        {
            using namespace std;
            std::streamsize amt = static_cast<std::streamsize>(eptr_ - ptr_);
            std::streamsize const result = iostream::write_if(dest, ptr_, amt);
            if (result < amt)
            {
                traits_type::move(this->data(),
                    ptr_ + static_cast<size_t>(result),
                    static_cast<size_t>(amt - result));
            }
            this->set(nullptr, amt - result);
            return result != 0;
        }

    private:
        Ch *ptr_ = nullptr, *eptr_ = nullptr;
    };

    HPX_CXX_CORE_EXPORT template <typename Ch, typename Alloc>
    void swap(buffer<Ch, Alloc>& lhs, buffer<Ch, Alloc>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    //--------------Implementation of basic_buffer--------------------------------//
    template <typename Ch, typename Alloc>
    Ch* basic_buffer<Ch, Alloc>::allocate(std::streamsize buffer_size)
    {
        allocator_type alloc;
        return static_cast<Ch*>(allocator_traits::allocate(
            alloc, static_cast<allocator_traits::size_type>(buffer_size)));
    }

    template <typename Ch, typename Alloc>
    basic_buffer<Ch, Alloc>::basic_buffer(std::streamsize const buffer_size)
      : buf_(allocate(buffer_size))
      , size_(buffer_size)
    {
    }

    template <typename Ch, typename Alloc>
    basic_buffer<Ch, Alloc>::~basic_buffer()
    {
        if (buf_)
        {
            allocator_type alloc;
            allocator_traits::deallocate(
                alloc, buf_, static_cast<allocator_traits::size_type>(size_));
        }
    }

    template <typename Ch, typename Alloc>
    void basic_buffer<Ch, Alloc>::resize(std::streamsize const buffer_size)
    {
        if (size_ != buffer_size)
        {
            using std::swap;

            basic_buffer<Ch, Alloc> temp(buffer_size);
            swap(size_, temp.size_);
            swap(buf_, temp.buf_);
        }
    }

    template <typename Ch, typename Alloc>
    void basic_buffer<Ch, Alloc>::swap(basic_buffer& rhs) noexcept
    {
        using std::swap;
        swap(buf_, rhs.buf_);
        swap(size_, rhs.size_);
    }

    //--------------Implementation of buffer--------------------------------------//
    template <typename Ch, typename Alloc>
    buffer<Ch, Alloc>::buffer(std::streamsize const buffer_size)
      : basic_buffer<Ch, Alloc>(buffer_size)
      , ptr_(data())
      , eptr_(data() + buffer_size)
    {
    }

    template <typename Ch, typename Alloc>
    void buffer<Ch, Alloc>::set(
        std::streamsize const ptr, std::streamsize const end)
    {
        ptr_ = data() + ptr;
        eptr_ = data() + end;
    }

    template <typename Ch, typename Alloc>
    void buffer<Ch, Alloc>::swap(buffer& rhs) noexcept
    {
        base::swap(rhs);

        using std::swap;
        swap(ptr_, rhs.ptr_);
        swap(eptr_, rhs.eptr_);
    }
}    // namespace hpx::iostream::detail
