///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/compute_local/detail/iterator.hpp>
#include <hpx/compute_local/traits/access_target.hpp>
#include <hpx/compute_local/traits/allocator_traits.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hpx::compute {

    HPX_CXX_CORE_EXPORT template <typename T,
        typename Allocator = std::allocator<T>>
    class vector
    {
        using alloc_traits = traits::allocator_traits<Allocator>;

    public:
        /// Member types (FIXME: add reference to std
        using value_type = T;
        using allocator_type = Allocator;
        using access_target = typename alloc_traits::access_target;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = typename alloc_traits::reference;
        using const_reference = typename alloc_traits::const_reference;
        using pointer = typename alloc_traits::pointer;
        using const_pointer = typename alloc_traits::const_pointer;
        using iterator = detail::iterator<T, Allocator>;
        using const_iterator = detail::iterator<T const, Allocator>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // Default constructor. Constructs an empty container
        explicit vector(Allocator const& alloc = Allocator())
          // C++-14, delegating ctor version:
          // vector() : vector(Allocator()) {}
          // explicit vector(Allocator const& alloc)
          : size_(0)
          , capacity_(0)
          , alloc_(alloc)
          , data_()
        {
        }

        // Constructs the container with count copies of elements with value
        vector(size_type count, T const& value,
            Allocator const& alloc = Allocator())
          : size_(count)
          , capacity_(count)
          , alloc_(alloc)
          , data_(alloc_traits::allocate(alloc_, count))
        {
            alloc_traits::bulk_construct(alloc_, data_, size_, value);
        }

        // Constructs the container with count default-inserted instances of T.
        // No copies are made.
        explicit vector(size_type count, Allocator const& alloc = Allocator())
          : size_(count)
          , capacity_(count)
          , alloc_(alloc)
          , data_(alloc_traits::allocate(alloc_, count))
        {
            alloc_traits::bulk_construct(alloc_, data_, size_);
        }

        template <typename InIter>
        vector(InIter first, InIter last, Allocator const& alloc = Allocator())
#if !defined(DOXYGEN)
            requires hpx::traits::is_input_iterator_v<InIter>
#endif
          : size_(static_cast<size_type>(std::distance(first, last)))
          , capacity_(size_)
          , alloc_(alloc)
          , data_(alloc_traits::allocate(alloc_, size_))
        {
            hpx::parallel::util::copy(first, last, begin());
        }

        vector(vector const& other)
          : size_(other.size_)
          , capacity_(other.capacity_)
          , alloc_(other.alloc_)
          , data_(alloc_traits::allocate(alloc_, capacity_))
        {
            hpx::parallel::util::copy(other.begin(), other.end(), begin());
        }

        vector(vector const& other, Allocator const& alloc)
          : size_(other.size_)
          , capacity_(other.capacity_)
          , alloc_(alloc)
          , data_(alloc_traits::allocate(alloc_, capacity_))
        {
            hpx::parallel::util::copy(other.begin(), other.end(), begin());
        }

        vector(vector&& other) noexcept
          : size_(other.size_)
          , capacity_(other.capacity_)
          , alloc_(HPX_MOVE(other.alloc_))
          , data_(HPX_MOVE(other.data_))
        {
            other.data_ = pointer(nullptr);
            other.size_ = 0;
            other.capacity_ = 0;
        }

        vector(vector&& other, Allocator const& alloc)
          : size_(other.size_)
          , capacity_(other.capacity_)
          , alloc_(alloc)
          , data_(HPX_MOVE(other.data_))
        {
            other.data_ = pointer(nullptr);
            other.size_ = 0;
            other.capacity_ = 0;
        }

        vector(std::initializer_list<T> init, Allocator const& alloc)
          : size_(init.size())
          , capacity_(init.size())
          , alloc_(alloc)
          , data_(alloc_traits::allocate(alloc_, capacity_))
        {
            hpx::parallel::util::copy(init.begin(), init.end(), begin());
        }

        ~vector()
        {
            if (data_ == nullptr)
            {
                return;
            }

#if !defined(__CUDA_ARCH__)
            try
#endif
            {
                alloc_traits::bulk_destroy(alloc_, data_, size_);
                alloc_traits::deallocate(alloc_, data_, capacity_);
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;
            }
#if !defined(__CUDA_ARCH__)
            catch (...)
            {
                // make sure no exception escapes this destructor
                hpx::report_error(std::current_exception());
            }
#endif
        }

        vector& operator=(vector const& other)
        {
            if (this == &other)
                return *this;

            allocator_type tmp_alloc = other.alloc_;

            pointer data = alloc_traits::allocate(tmp_alloc, other.capacity_);
            hpx::parallel::util::copy(other.begin(), other.end(),
                iterator(data, 0, alloc_traits::target(tmp_alloc)));

            if (data_ != nullptr)
            {
                alloc_traits::bulk_destroy(alloc_, data_, size_);
                alloc_traits::deallocate(alloc_, data_, capacity_);
            }

            size_ = other.size_;
            capacity_ = other.capacity_;
            alloc_ = HPX_MOVE(tmp_alloc);
            data_ = HPX_MOVE(data);

            return *this;
        }

        vector& operator=(vector&& other) noexcept
        {
            if (this == &other)
                return *this;

            if (data_ != nullptr)
            {
                alloc_traits::bulk_destroy(alloc_, data_, size_);
                alloc_traits::deallocate(alloc_, data_, capacity_);
            }

            size_ = other.size_;
            capacity_ = other.capacity_;
            alloc_ = HPX_MOVE(other.alloc_);
            data_ = HPX_MOVE(other.data_);

            other.data_ = pointer(nullptr);
            other.size_ = 0;
            other.capacity_ = 0;

            return *this;
        }

        void assign(size_type count, T const& value)
        {
            clear();
            if (capacity_ < count)
            {
                alloc_traits::deallocate(alloc_, data_, capacity_);
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;

                pointer new_data = alloc_traits::allocate(alloc_, count);
                data_ = new_data;
                capacity_ = count;
            }
            size_ = count;
            alloc_traits::bulk_construct(alloc_, data_, size_, value);
        }

        template <typename InIter>
        void assign(InIter first, InIter last)
#if !defined(DOXYGEN)
            requires hpx::traits::is_input_iterator_v<InIter>
#endif
        {
            clear();
            size_type count =
                static_cast<size_type>(std::distance(first, last));
            if (capacity_ < count)
            {
                alloc_traits::deallocate(alloc_, data_, capacity_);
                data_ = nullptr;
                size_ = 0;
                capacity_ = 0;

                pointer new_data = alloc_traits::allocate(alloc_, count);
                data_ = new_data;
                capacity_ = count;
            }
            size_ = count;
            hpx::parallel::util::copy(first, last, begin());
        }

        void assign(std::initializer_list<T> ilist)
        {
            assign(ilist.begin(), ilist.end());
        }
        /// Returns the allocator associated with the container
        allocator_type get_allocator() const noexcept
        {
            return alloc_;
        }

        ///////////////////////////////////////////////////////////////////////
        // Element access

        reference at(size_type pos)
        {
            if (pos >= size_)
            {
                throw std::out_of_range("vector::at");
            }
            return *(data_ + pos);
        }

        const_reference at(size_type pos) const
        {
            if (pos >= size_)
            {
                throw std::out_of_range("vector::at");
            }
            return *(data_ + pos);
        }

        HPX_HOST_DEVICE
        reference operator[](size_type pos)
        {
#if !defined(__CUDA_ARCH__)
            HPX_ASSERT(pos < size_);
#endif
            return *(data_ + pos);
        }

        HPX_HOST_DEVICE
        const_reference operator[](size_type pos) const
        {
#if !defined(__CUDA_ARCH__)
            HPX_ASSERT(pos < size_);
#endif
            return *(data_ + pos);
        }

        /// Returns a reference to the first element in the container.
        /// Calling front on an empty container is undefined.
        HPX_HOST_DEVICE
        reference front()
        {
#if !defined(__CUDA_ARCH__)
            HPX_ASSERT(!empty());
#endif
            return *data_;
        }

        /// \copydoc front()
        HPX_HOST_DEVICE
        const_reference front() const
        {
#if !defined(__CUDA_ARCH__)
            HPX_ASSERT(!empty());
#endif
            return *data_;
        }

        /// Returns a reference to the last element in the container.
        /// Calling back on an empty container is undefined.
        HPX_HOST_DEVICE
        reference back()
        {
#if !defined(__CUDA_ARCH__)
            HPX_ASSERT(!empty());
#endif
            return *(data_ + size_ - 1);
        }

        /// \copydoc back()
        HPX_HOST_DEVICE
        const_reference back() const
        {
#if !defined(__CUDA_ARCH__)
            HPX_ASSERT(!empty());
#endif
            return *(data_ + size_ - 1);
        }

        /// Returns pointer to the underlying array serving as element storage.
        /// The pointer is such that range [data(); data() + size()) is always a
        /// valid range, even if the container is empty (data() is not
        /// dereference-able in that case).
        pointer data() noexcept
        {
            return data_;
        }

        /// \copydoc data()
        const_pointer data() const noexcept
        {
            return data_;
        }

        /// Returns a raw pointer corresponding to the address of the data
        /// allocated on the device.
        T* device_data() const noexcept
        {
#if defined(__NVCC__) || defined(__CUDACC__)
            return data_.device_ptr();
#else
            return data_;
#endif
        }

        std::size_t size() const noexcept
        {
            return size_;
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }

        /// Returns: size() == 0
        bool empty() const noexcept
        {
            return size_ == 0;
        }

        /// Effects: If size <= size(), equivalent to calling pop_back() size()
        /// - size times. If size() < size, appends size - size()
        /// default-inserted elements to the sequence.
        ///
        /// Requires: T shall be MoveInsertable and DefaultInsertable into
        /// *this.
        ///
        /// Remarks: If an exception is thrown other than by the move
        /// constructor of a non-CopyInsertable T there are no effects.
        ///
        void resize(size_type /* size */)
        {
            // TODO: implement this
        }

        /// Effects: If size <= size(), equivalent to calling pop_back()
        /// size() - size times. If size() < size, appends size - size()
        /// copies of val to the sequence.
        ///
        /// Requires: T shall be CopyInsertable into *this.
        ///
        /// Remarks: If an exception is thrown there are no effects.
        ///
        void resize(size_type /* size */, T const& /* val */)
        {
            // TODO: implement this
        }

        ///////////////////////////////////////////////////////////////////////
        // Iterators
        // TODO: debug support
        iterator begin() noexcept
        {
            return iterator(data_, 0, alloc_traits::target(alloc_));
        }

        iterator end() noexcept
        {
            return iterator(data_, size_, alloc_traits::target(alloc_));
        }

        const_iterator cbegin() const noexcept
        {
            return const_iterator(data_, 0, alloc_traits::target(alloc_));
        }

        const_iterator cend() const noexcept
        {
            return const_iterator(data_, size_, alloc_traits::target(alloc_));
        }

        const_iterator begin() const noexcept
        {
            return const_iterator(data_, 0, alloc_traits::target(alloc_));
        }

        const_iterator end() const noexcept
        {
            return const_iterator(data_, size_, alloc_traits::target(alloc_));
        }

        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }

        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return crbegin();
        }

        const_reverse_iterator rend() const noexcept
        {
            return crend();
        }

        /// Effects: Exchanges the contents and capacity() of *this with that
        /// of x.
        ///
        /// Complexity: Constant time.
        ///
        void swap(vector& other) noexcept
        {
            vector tmp = HPX_MOVE(other);
            other = HPX_MOVE(*this);
            *this = HPX_MOVE(tmp);
        }

        /// Effects: Erases all elements in the range [begin(),end()).
        /// Destroys all elements in 'a'. Invalidates all references, pointers,
        /// and iterators referring to the elements of a and may invalidate the
        /// past-the-end iterator.
        ///
        /// Post: a.empty() returns true.
        ///
        /// Complexity: Linear.
        ///
        void clear() noexcept
        {
            alloc_traits::bulk_destroy(alloc_, data_, size_);
            size_ = 0;
        }

    private:
        size_type size_;
        size_type capacity_;
        allocator_type alloc_;
        pointer data_;
    };

    /// Effects: x.swap(y);
    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    HPX_FORCEINLINE void swap(
        vector<T, Allocator>& x, vector<T, Allocator>& y) noexcept
    {
        x.swap(y);
    }
}    // namespace hpx::compute

namespace hpx::traits {

    template <typename T, typename Allocator>
    struct is_contiguous_iterator<hpx::compute::detail::iterator<T, Allocator>>
      : std::true_type
    {
    };
}    // namespace hpx::traits
