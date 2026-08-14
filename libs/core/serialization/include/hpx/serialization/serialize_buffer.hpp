//  Copyright (c) 2013-2025 Hartmut Kaiser
//  Copyright (c) 2015 Andreas Schaefer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/serialization/array.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/serialization/serialize_buffer_fwd.hpp>

#include <cstddef>
#include <memory>

namespace hpx::serialization {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Allocator>
        struct array_allocator
        {
            auto* operator()(Allocator alloc, std::size_t size) const
            {
                auto* p = alloc.allocate(size);
                std::uninitialized_default_construct(p, p + size);
                return p;
            }
        };

        template <typename T>
        struct array_allocator<std::allocator<T>>
        {
            T* operator()(std::allocator<T>, std::size_t size) const
            {
                return new T[size];
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Deallocator>
        struct array_deleter
        {
            template <typename T>
            void operator()(
                T* p, Deallocator dealloc, std::size_t size) const noexcept
            {
                std::destroy(p, p + size);
                dealloc.deallocate(p, size);
            }
        };

        template <typename T>
        struct array_deleter<std::allocator<T>>
        {
            void operator()(
                T const* p, std::allocator<T>, std::size_t) const noexcept
            {
                delete[] p;
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    class serialize_buffer
    {
    private:
        using allocator_type = Allocator;
        using buffer_type = std::shared_ptr<T[]>;

        static constexpr void no_deleter(T*) noexcept {}

        template <typename Deallocator>
        struct deleter
        {
            void operator()(
                T* p, Deallocator dealloc, std::size_t size) const noexcept
            {
                std::destroy_at(p);
                dealloc.deallocate(p, size);
            }
        };

    public:
        enum init_mode
        {
            copy = 0,          // constructor copies data
            reference = 1,     // constructor does not copy data and does not
                               // manage the lifetime of it
            take = 2,          // constructor does not copy data but takes
                               // ownership of array pointer and manages the
                               // lifetime of it
            take_single = 3    // constructor does not copy data but takes
                               // ownership of pointer to non-array and manages
                               // the lifetime of it
        };

        using value_type = T;

        explicit serialize_buffer(
            allocator_type const& alloc = allocator_type())
          : size_(0)
          , alloc_(alloc)
        {
        }

        explicit serialize_buffer(
            std::size_t size, allocator_type const& alloc = allocator_type())
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            auto* p = detail::array_allocator<allocator_type>()(alloc_, size);
            data_.reset(
                p, [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                    detail::array_deleter<allocator_type>()(p, alloc, size);
                });
        }

        // The default mode is 'copy' which is consistent with the constructor
        // taking a T const * below.
        serialize_buffer(T* data, std::size_t size, init_mode mode = copy,
            allocator_type const& alloc = allocator_type())
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            if (mode == copy)
            {
                auto* p =
                    detail::array_allocator<allocator_type>()(alloc_, size);
                data_.reset(p,
                    [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                        detail::array_deleter<allocator_type>()(p, alloc, size);
                    });
                std::uninitialized_copy(data, data + size, p);
            }
            else if (mode == reference)
            {
                data_ = buffer_type(data, &serialize_buffer::no_deleter);
            }
            else if (mode == take_single)
            {
                // take ownership
                data_ = buffer_type(data,
                    [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                        serialize_buffer::deleter<allocator_type>()(
                            p, alloc, size);
                    });
            }
            else if (mode == take)
            {
                // take ownership
                data_ = buffer_type(data,
                    [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                        detail::array_deleter<allocator_type>()(p, alloc, size);
                    });
            }
        }

        template <typename Deallocator>
        serialize_buffer(T* data, std::size_t size, allocator_type const& alloc,
            Deallocator const& dealloc)
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            // if 2 allocators are specified we assume mode 'take'
            data_ = buffer_type(data, [this, dealloc](T* p) noexcept {
                detail::array_deleter<Deallocator>()(p, dealloc, size_);
            });
        }

        template <typename Deleter>
        serialize_buffer(T* data, std::size_t size, init_mode mode,
            Deleter&& deleter, allocator_type const& alloc = allocator_type())
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            if (mode == copy)
            {
                auto* p =
                    detail::array_allocator<allocator_type>()(alloc_, size);
                data_ = buffer_type(p,
                    [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                        detail::array_deleter<allocator_type>()(p, alloc, size);
                    });
                std::uninitialized_copy(data, data + size, p);
            }
            else
            {
                // reference or take ownership, behavior is defined by deleter
                data_ = buffer_type(data,
                    [deleter = HPX_FORWARD(Deleter, deleter)](
                        T* p) noexcept { deleter(p); });
            }
        }

        template <typename Deleter>
        serialize_buffer(T const* data, std::size_t size,
            init_mode mode,    //-V659
            Deleter&& deleter, allocator_type const& alloc = allocator_type())
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            if (mode == copy)
            {
                auto* p =
                    detail::array_allocator<allocator_type>()(alloc_, size);
                data_ = buffer_type(p,
                    [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                        detail::array_deleter<allocator_type>()(p, alloc, size);
                    });
                std::uninitialized_copy(data, data + size, p);
            }
            else if (mode == reference)
            {
                // reference behavior is defined by deleter
                data_ = buffer_type(const_cast<T*>(data),
                    [deleter = HPX_FORWARD(Deleter, deleter)](
                        T* p) noexcept { deleter(p); });
            }
            else
            {
                // can't take ownership of const buffer
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "serialize_buffer::serialize_buffer",
                    "can't take ownership of const data");
            }
        }

        // Deleter needs to use deallocator
        template <typename Deallocator, typename Deleter>
        serialize_buffer(T* data, std::size_t size, allocator_type const& alloc,
            Deallocator const& /* dealloc */, Deleter const& deleter)
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            // if 2 allocators are specified we assume mode 'take'
            data_ = buffer_type(data, deleter);
        }

        // same set of constructors, but taking const data
        serialize_buffer(T const* data, std::size_t size,
            allocator_type const& alloc = allocator_type())
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            // create from const data implies 'copy' mode
            auto* p = detail::array_allocator<allocator_type>()(alloc_, size);
            data_ = buffer_type(
                p, [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                    detail::array_deleter<allocator_type>()(p, alloc, size);
                });
            std::uninitialized_copy(data, data + size, p);
        }

        template <typename Deleter>
        serialize_buffer(T const* data, std::size_t size, Deleter&&,
            allocator_type const& alloc = allocator_type())
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            // create from const data implies 'copy' mode
            auto* p = detail::array_allocator<allocator_type>()(alloc_, size);
            data_ = buffer_type(
                p, [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                    detail::array_deleter<allocator_type>()(p, alloc, size);
                });
            std::uninitialized_copy(data, data + size, p);
        }

        serialize_buffer(T const* data, std::size_t size, init_mode mode,
            allocator_type const& alloc = allocator_type())
          : data_()
          , size_(size)
          , alloc_(alloc)
        {
            if (mode == copy)
            {
                auto* p =
                    detail::array_allocator<allocator_type>()(alloc_, size);
                data_ = buffer_type(p,
                    [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                        detail::array_deleter<allocator_type>()(p, alloc, size);
                    });
                std::uninitialized_copy(data, data + size, p);
            }
            else if (mode == reference)
            {
                data_ = buffer_type(
                    const_cast<T*>(data), &serialize_buffer::no_deleter);
            }
            else
            {
                // can't take ownership of const buffer
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "serialize_buffer::serialize_buffer",
                    "can't take ownership of const data");
            }
        }

        // accessors enabling data access
        [[nodiscard]] T* data() noexcept
        {
            return data_.get();
        }
        [[nodiscard]] T const* data() const noexcept
        {
            return data_.get();
        }

        [[nodiscard]] T* begin() noexcept
        {
            return data();
        }
        [[nodiscard]] T* end() noexcept
        {
            return data() + size_;
        }

        T& operator[](std::size_t idx)
        {
            return data_[idx];
        }
        T const& operator[](std::size_t idx) const
        {
            return data_[idx];
        }

        [[nodiscard]] constexpr buffer_type data_array() const noexcept
        {
            return data_;
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept
        {
            return size_;
        }

        void resize_norealloc(std::size_t newsize) noexcept
        {
            HPX_ASSERT_MSG(newsize <= size_,
                "serialize_buffer::resize_norealloc: new size shouldn't be "
                "larger than current size");
            if (newsize < size_)
            {
                size_ = newsize;
            }
        }

    private:
        // serialization support
        friend class hpx::serialization::access;

        ///////////////////////////////////////////////////////////////////////
        template <typename Archive>
        void save(Archive& ar, unsigned int const) const
        {
            ar << size_ << alloc_;    // -V128

            if (size_ != 0)
            {
                ar << hpx::serialization::make_array(data_.get(), size_);
            }
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Archive>
        void load(Archive& ar, unsigned int const)
        {
            ar >> size_ >> alloc_;    // -V128

            data_ = buffer_type(
                detail::array_allocator<allocator_type>()(alloc_, size_),
                [alloc = this->alloc_, size = this->size_](T* p) noexcept {
                    detail::array_deleter<allocator_type>()(p, alloc, size);
                });

            if (size_ != 0)
            {
                ar >> hpx::serialization::make_array(data_.get(), size_);
            }
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()

        // this is needed for util::any
        friend bool operator==(
            serialize_buffer const& rhs, serialize_buffer const& lhs) noexcept
        {
            return rhs.data_.get() == lhs.data_.get() && rhs.size_ == lhs.size_;
        }

    private:
        buffer_type data_;
        std::size_t size_;
        Allocator alloc_;
    };
}    // namespace hpx::serialization
