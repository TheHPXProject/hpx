//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The code below was taken and adapted from:
//  https://github.com/martinus/svector.
//
// The original file was licensed under the MIT license:
//
// Copyright (c) 2022 Martin Leitner-Ankerl
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/contracts.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hpx::detail {

    // Note we implement is_iterator here instead of pulling the trait from
    // the iterator_support module to avoid circular dependencies between
    // the modules.
    //
    // This implementation of is_iterator seems to work fine even for VS2013
    // which has an implementation of std::iterator_traits that is
    // SFINAE-unfriendly.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_argument_iterator
    {
#if defined(HPX_MSVC) && defined(__CUDACC__)
        template <typename U>
        static typename U::iterator_category* test(U);    // iterator

        template <typename U>
        static void* test(U*);    // pointer
#else
        template <typename U,
            typename = typename std::iterator_traits<U>::pointer>
        static void* test(U&&);
#endif

        static char test(...);

        enum
        {
            value = sizeof(test(std::declval<T>())) == sizeof(void*)
        };
    };

    HPX_CXX_CORE_EXPORT template <typename Iter>
    inline constexpr bool is_argument_iterator_v =
        is_argument_iterator<Iter>::value;

    HPX_CXX_CORE_EXPORT constexpr auto round_up(
        std::size_t n, std::size_t multiple) noexcept -> std::size_t
    {
        return ((n + (multiple - 1)) / multiple) * multiple;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr auto cx_min(T a, T b) noexcept -> T
    {
        return a < b ? a : b;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr auto cx_max(T a, T b) noexcept -> T
    {
        return a > b ? a : b;
    }

    HPX_CXX_CORE_EXPORT class header
    {
        std::size_t m_size{};
        std::size_t const m_capacity;

    public:
        inline explicit constexpr header(std::size_t capacity) noexcept
          : m_capacity{capacity}
        {
        }

        [[nodiscard]] inline constexpr auto size() const noexcept -> std::size_t
        {
            return m_size;
        }

        [[nodiscard]] inline constexpr auto capacity() const noexcept
            -> std::size_t
        {
            return m_capacity;
        }

        inline void size(std::size_t s) noexcept
        {
            m_size = s;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    struct storage : public header
    {
        static constexpr auto alignment_of_t = std::alignment_of_v<T>;
        static constexpr auto max_Alignment =
            (std::max) (std::alignment_of_v<header>, std::alignment_of_v<T>);
        static constexpr auto offset_to_data =
            round_up(sizeof(header), alignment_of_t);
        static_assert(max_Alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

        explicit constexpr storage(std::size_t capacity) noexcept
          : header(capacity)
        {
        }

        auto data() noexcept -> T*
        {
            auto ptr_to_data =
                reinterpret_cast<std::byte*>(this) + offset_to_data;
            return std::launder(reinterpret_cast<T*>(ptr_to_data));
        }

        static auto alloc(std::size_t capacity) -> storage<T>*
        {
            // make sure we don't overflow!
            auto mem = sizeof(T) * capacity;
            if (mem < capacity)
            {
                throw std::bad_alloc();
            }

            if (offset_to_data + mem < mem)
            {
                throw std::bad_alloc();
            }

            mem += offset_to_data;
            if (static_cast<std::ptrdiff_t>(mem) >
                (std::numeric_limits<std::ptrdiff_t>::max)())
            {
                throw std::bad_alloc();
            }

            // only void* is allowed to be converted to uintptr_t
            void* ptr = ::operator new(mem);
            if (nullptr == ptr)
            {
                throw std::bad_alloc();
            }
            return hpx::construct_at(static_cast<storage<T>*>(ptr), capacity);
        }

        static void dealloc(storage* strg)
        {
            if (strg == nullptr)
                return;
            std::destroy_at(strg);
            ::operator delete(strg);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr auto alignment_of_small_vector() noexcept -> std::size_t
    {
        return cx_max(sizeof(void*), std::alignment_of_v<T>);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr auto size_of_small_vector(
        std::size_t min_inline_capacity) noexcept -> std::size_t
    {
        // + 1 for one byte size in direct mode
        return round_up(sizeof(T) * min_inline_capacity + 1,
            alignment_of_small_vector<T>());
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr auto automatic_capacity(std::size_t min_inline_capacity) noexcept
        -> std::size_t
    {
        return cx_min(
            (size_of_small_vector<T>(min_inline_capacity) - 1U) / sizeof(T),
            static_cast<std::size_t>(127));
    }

    // note: Allocator is currently unused
    HPX_CXX_CORE_EXPORT template <typename T, std::size_t MinInlineCapacity,
        typename Allocator = std::allocator<T>>
    class small_vector
    {
        static_assert(MinInlineCapacity <= 127,
            "sorry, can't have more than 127 direct elements");
        static constexpr auto N = automatic_capacity<T>(MinInlineCapacity);

        enum class direction
        {
            direct,
            indirect
        };

        // A buffer to hold the data of the small_vector Depending on
        // direct/indirect mode, the content it holds is like so:
        //
        // direct:
        //  m_data[0] & 1:  lowest bit is 1 for direct mode.
        //  m_data[0] >> 1: size for direct mode
        //  Then 0-X bytes unused (padding), and then the actual inline T data.
        //
        // indirect:
        //  m_data[0] & 1: lowest bit is 0 for indirect mode
        //  m_data[0..7]:  stores an uintptr_t, which points to the indirect
        //                 data.

        alignas(alignment_of_small_vector<T>()) std::array<std::uint8_t,
            size_of_small_vector<T>(MinInlineCapacity)> m_data;

        [[nodiscard]] constexpr auto is_direct() const noexcept -> bool
        {
            return (m_data[0] & 1U) != 0U;
        }

        [[nodiscard]] auto indirect() noexcept -> storage<T>*
        {
            // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
            storage<T>* ptr;

            // NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
            // NOLINTNEXTLINE(bugprone-sizeof-expression)
            std::memcpy(&ptr, m_data.data(), sizeof(ptr));
            // NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)
            return ptr;
        }

        [[nodiscard]] auto indirect() const noexcept -> storage<T> const*
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<small_vector*>(this)->indirect();
        }

        void set_indirect(storage<T>* ptr) noexcept
        {
            // NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)
            // NOLINTNEXTLINE(bugprone-sizeof-expression)
            std::memcpy(m_data.data(), &ptr, sizeof(ptr));
            // NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)

            // safety check to guarantee the lowest bit is 0
            HPX_ASSERT(!is_direct());
        }

        [[nodiscard]] constexpr auto direct_size() const noexcept -> std::size_t
        {
            return m_data[0] >> 1U;
        }

        // sets size of direct mode and mode to direct too.
        constexpr void set_direct_and_size(std::size_t s) noexcept
        {
            m_data[0] = static_cast<std::uint8_t>((s << 1U) | 1U);
        }

        [[nodiscard]] auto direct_data() noexcept -> T*
        {
            return std::launder(
                reinterpret_cast<T*>(m_data.data() + std::alignment_of_v<T>));
        }

        static void uninitialized_move_and_destroy(
            T* source_ptr, T* target_ptr, std::size_t size) noexcept
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(target_ptr, source_ptr, size * sizeof(T));
            }
            else
            {
                std::uninitialized_move_n(source_ptr, size, target_ptr);
                std::destroy_n(source_ptr, size);
            }
        }

        void realloc(std::size_t new_capacity)
        {
            if (new_capacity <= N)
            {
                // put everything into direct storage

                // direct -> direct: nothing to do!
                if (!is_direct())
                {
                    // indirect -> direct
                    auto* storage = indirect();
                    auto const data_size = storage->size();
                    if (data_size != 0)
                    {
                        uninitialized_move_and_destroy(
                            storage->data(), direct_data(), data_size);
                    }
                    set_direct_and_size(data_size);
                    detail::storage<T>::dealloc(storage);
                }
            }
            else
            {
                // put everything into indirect storage
                auto* storage = detail::storage<T>::alloc(new_capacity);
                if (is_direct())
                {
                    // direct -> indirect
                    auto const data_size = size<direction::direct>();
                    if (data_size != 0)
                    {
                        uninitialized_move_and_destroy(
                            data<direction::direct>(), storage->data(),
                            data_size);
                    }
                    storage->size(data_size);
                }
                else
                {
                    // indirect -> indirect
                    auto const data_size = size<direction::indirect>();
                    if (data_size != 0)
                    {
                        uninitialized_move_and_destroy(
                            data<direction::indirect>(), storage->data(),
                            data_size);
                    }
                    storage->size(data_size);
                    detail::storage<T>::dealloc(indirect());
                }
                set_indirect(storage);
            }
        }

        [[nodiscard]] static constexpr auto calculate_new_capacity(
            std::size_t size_to_fit, std::size_t starting_capacity) noexcept
            -> std::size_t
        {
            if (size_to_fit == 0)
            {
                // special handling for 0 so N==0 works
                return starting_capacity;
            }

            // start with at least 1, so N==0 works
            auto new_capacity = std::max<std::size_t>(1, starting_capacity);

            // double capacity until its large enough, but make sure we don't
            // overflow
            while (
                new_capacity < size_to_fit && new_capacity * 2 > new_capacity)
            {
                new_capacity *= 2;
            }

            if (new_capacity < size_to_fit)
            {
                // got an overflow, set capacity to max
                new_capacity = max_size();
            }
            return (std::min) (new_capacity, max_size());
        }

        template <direction D>
        [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t
        {
            if constexpr (D == direction::direct)
            {
                return N;
            }
            else
            {
                return indirect()->capacity();
            }
        }

        template <direction D>
        [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
        {
            if constexpr (D == direction::direct)
            {
                return direct_size();
            }
            else
            {
                return indirect()->size();
            }
        }

        void set_size(std::size_t s) noexcept
        {
            if (is_direct())
            {
                set_size<direction::direct>(s);
            }
            else
            {
                set_size<direction::indirect>(s);
            }
        }

        template <direction D>
        void set_size(std::size_t s) noexcept
        {
            if constexpr (D == direction::direct)
            {
                set_direct_and_size(s);
            }
            else
            {
                indirect()->size(s);
            }
        }

        template <direction D>
        [[nodiscard]] auto data() noexcept -> T*
        {
            if constexpr (D == direction::direct)
            {
                return direct_data();
            }
            else
            {
                return indirect()->data();
            }
        }

        template <direction D>
        [[nodiscard]] auto data() const noexcept -> T const*
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<small_vector*>(this)->data<D>();
        }

        template <direction D>
        void pop_back() noexcept
        {
            if constexpr (std::is_trivially_destructible_v<T>)
            {
                set_size<D>(size<D>() - 1);
            }
            else
            {
                auto s = size<D>() - 1;
                std::destroy_at(data<D>() + s);
                set_size<D>(s);
            }
        }

        // \brief We need variadic arguments so we can either use copy ctor or
        // default ctor
        template <direction D, typename... Args>
        void resize_after_reserve(std::size_t count, Args&&... args)
        {
            auto current_size = size<D>();
            if (current_size > count)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    auto* d = data<D>();
                    std::destroy(d + count, d + current_size);
                }
            }
            else
            {
                auto* d = data<D>();
                for (auto ptr = d + current_size, end = d + count; ptr != end;
                    ++ptr)
                {
                    hpx::construct_at(
                        // NOLINTNEXTLINE(bugprone-use-after-move)
                        static_cast<T*>(ptr), HPX_FORWARD(Args, args)...);
                }
            }
            set_size<D>(count);
        }

        // Makes sure that to is not past the end iterator
        template <direction D>
        auto erase_checked_end(T const* cfrom, T const* to) -> T*
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            auto* const erase_begin = const_cast<T*>(cfrom);
            auto* const container_end = data<D>() + size<D>();
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            auto* const erase_end =
                (std::min) (const_cast<T*>(to), container_end);

            std::move(erase_end, container_end, erase_begin);
            auto const num_erased = std::distance(erase_begin, erase_end);
            std::destroy(container_end - num_erased, container_end);
            set_size<D>(size<D>() - num_erased);
            return erase_begin;
        }

        template <typename It>
        void assign(It first, It last, std::input_iterator_tag /*unused*/)
        {
            clear();

            // TODO this can be made faster, e.g. by setting size only when
            // finished.
            while (first != last)
            {
                push_back(*first);
                ++first;
            }
        }

        template <typename It>
        void assign(It first, It last, std::forward_iterator_tag /*unused*/)
        {
            clear();

            auto s = std::distance(first, last);
            reserve(s);
            std::uninitialized_copy(first, last, data());
            set_size(s);
        }

        // precondition: all uninitialized
        void do_move_assign(small_vector&& other) noexcept
        {
            if (!other.is_direct())
            {
                // take other's memory, even when empty
                set_indirect(other.indirect());
            }
            else
            {
                auto* other_ptr = other.data<direction::direct>();
                auto s = other.size<direction::direct>();
                auto* other_end = other_ptr + s;

                std::uninitialized_move(
                    other_ptr, other_end, data<direction::direct>());
                std::destroy(other_ptr, other_end);
                set_size(s);
            }
            other.set_direct_and_size(0);
        }

        // \brief Shifts data [source_begin, source_end) to the right, starting
        // on target_begin.
        //
        // Preconditions:
        // * contiguous memory
        // * source_begin <= target_begin
        // * source_end onwards is uninitialized memory
        //
        // Destroys then empty elements in [source_begin, source_end)
        auto shift_right(
            T* source_begin, T* source_end, T* target_begin) noexcept
        {
            // 1. uninitialized moves
            auto const num_moves = std::distance(source_begin, source_end);
            auto const target_end = target_begin + num_moves;
            auto const num_uninitialized_move =
                (std::min) (num_moves, std::distance(source_end, target_end));
            std::uninitialized_move(source_end - num_uninitialized_move,
                source_end, target_end - num_uninitialized_move);
            std::move_backward(source_begin,
                source_end - num_uninitialized_move,
                target_end - num_uninitialized_move);
            std::destroy(source_begin, (std::min) (source_end, target_begin));
        }

        // makes space for uninitialized data of cout elements. Also updates
        // size.
        template <direction D>
        [[nodiscard]] auto make_uninitialized_space_new(
            std::size_t s, T* p, std::size_t count) -> T*
        {
            auto target = small_vector();

            // we know target is indirect because we're increasing capacity
            target.reserve(s + count);

            // move everything [begin, pos[
            auto* target_pos = std::uninitialized_move(
                data<D>(), p, target.template data<direction::indirect>());

            // move everything [pos, end]
            std::uninitialized_move(p, data<D>() + s, target_pos + count);

            target.template set_size<direction::indirect>(s + count);
            *this = HPX_MOVE(target);
            return target_pos;
        }

        template <direction D>
        [[nodiscard]] auto make_uninitialized_space(
            T const* pos, std::size_t count) -> T*
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            auto* const p = const_cast<T*>(pos);
            auto s = size<D>();
            if (s + count > capacity<D>())
            {
                return make_uninitialized_space_new<D>(s, p, count);
            }

            shift_right(p, data<D>() + s, p + count);
            set_size<D>(s + count);
            return p;
        }

        // makes space for uninitialized data of cout elements. Also updates size.
        [[nodiscard]] auto make_uninitialized_space(
            T const* pos, std::size_t count) -> T*
        {
            if (is_direct())
            {
                return make_uninitialized_space<direction::direct>(pos, count);
            }
            return make_uninitialized_space<direction::indirect>(pos, count);
        }

        void destroy() noexcept
        {
            auto const is_dir = is_direct();
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                T* ptr;
                std::size_t s;
                if (is_dir)
                {
                    ptr = data<direction::direct>();
                    s = size<direction::direct>();
                }
                else
                {
                    ptr = data<direction::indirect>();
                    s = size<direction::indirect>();
                }
                std::destroy_n(ptr, s);
            }
            if (!is_dir)
            {
                detail::storage<T>::dealloc(indirect());
            }
            set_direct_and_size(0);
        }

        // performs a const_cast, so we don't need this implementation twice
        template <direction D>
        [[nodiscard]] auto at(std::size_t idx) const -> T&
        {
            if (idx >= size<D>())
            {
                throw std::out_of_range{"small_vector: idx out of range"};
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            auto* ptr = const_cast<T*>(data<D>() + idx);
            return *ptr;
        }    // LCOV_EXCL_LINE why is this single '}' marked as not covered? gcov bug?

    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = value_type const&;
        using pointer = T*;
        using const_pointer = T const*;
        using iterator = T*;
        using const_iterator = T const*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using allocator_type = Allocator;

        static constexpr std::size_t static_capacity = N;

        constexpr small_vector(Allocator const& = Allocator()) noexcept
        {
            set_direct_and_size(0);
        }

        small_vector(
            std::size_t count, T const& value, Allocator const& = Allocator())
          : small_vector()
        {
            resize(count, value);
        }

        explicit small_vector(std::size_t count, Allocator const& = Allocator())
          : small_vector()
        {
            reserve(count);
            if (is_direct())
            {
                resize_after_reserve<direction::direct>(count);
            }
            else
            {
                resize_after_reserve<direction::indirect>(count);
            }
        }

        template <typename InputIt,
            typename =
                std::enable_if_t<detail::is_argument_iterator_v<InputIt>>>
        small_vector(
            InputIt first, InputIt last, Allocator const& = Allocator())
          : small_vector()
        {
            assign(first, last);
        }

        small_vector(small_vector const& other)
          : small_vector()
        {
            auto s = other.size();
            reserve(s);
            std::uninitialized_copy(other.begin(), other.end(), begin());
            set_size(s);
        }

        small_vector(small_vector&& other) noexcept
          : small_vector()
        {
            do_move_assign(HPX_MOVE(other));
        }

        small_vector(
            std::initializer_list<T> init, Allocator const& = Allocator())
          : small_vector(init.begin(), init.end())
        {
        }

        ~small_vector()
        {
            destroy();
        }

        void assign(std::size_t count, T const& value)
        {
            clear();
            resize(count, value);
        }

        template <typename InputIt,
            typename =
                std::enable_if_t<detail::is_argument_iterator_v<InputIt>>>
        void assign(InputIt first, InputIt last)
        {
            assign(first, last,
                typename std::iterator_traits<InputIt>::iterator_category());
        }

        void assign(std::initializer_list<T> l)
        {
            assign(l.begin(), l.end());
        }

        auto operator=(small_vector const& other) -> small_vector&
        {
            if (&other == this)
            {
                return *this;
            }

            assign(other.begin(), other.end());
            return *this;
        }

        auto operator=(small_vector&& other) noexcept -> small_vector&
        {
            if (&other == this)
            {
                // It doesn't seem to be required to do self-check, but let's do
                // it anyway to be safe
                return *this;
            }

            destroy();
            do_move_assign(HPX_MOVE(other));
            return *this;
        }

        auto operator=(std::initializer_list<T> l) -> small_vector&
        {
            assign(l.begin(), l.end());
            return *this;
        }

        void resize(std::size_t count)
        {
            if (count > capacity())
            {
                reserve(count);
            }

            if (is_direct())
            {
                resize_after_reserve<direction::direct>(count);
            }
            else
            {
                resize_after_reserve<direction::indirect>(count);
            }
        }

        void resize(std::size_t count, value_type const& value)
        {
            if (count > capacity())
            {
                reserve(count);
            }

            if (is_direct())
            {
                resize_after_reserve<direction::direct>(count, value);
            }
            else
            {
                resize_after_reserve<direction::indirect>(count, value);
            }
        }

        auto reserve(std::size_t s)
        {
            auto const old_capacity = capacity();
            auto const new_capacity = calculate_new_capacity(s, old_capacity);
            if (new_capacity > old_capacity)
            {
                realloc(new_capacity);
            }
        }

        [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t
        {
            if (is_direct())
            {
                return capacity<direction::direct>();
            }
            return capacity<direction::indirect>();
        }

        [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
        {
            if (is_direct())
            {
                return size<direction::direct>();
            }
            return size<direction::indirect>();
        }

        [[nodiscard]] auto data() noexcept -> T*
        {
            if (is_direct())
            {
                return direct_data();
            }
            return indirect()->data();
        }

        [[nodiscard]] auto data() const noexcept -> T const*
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<small_vector*>(this)->data();
        }

        template <typename... Args>
        auto emplace_back(Args&&... args) -> T&
        {
            std::size_t s;    // NOLINT(cppcoreguidelines-init-variables)
            if (is_direct())
            {
                s = direct_size();
                if (s < N)
                {
                    set_direct_and_size(s + 1);
                    return *hpx::construct_at(
                        static_cast<T*>(direct_data() + s),
                        HPX_FORWARD(Args, args)...);
                }
                realloc(calculate_new_capacity(N + 1, N));
            }
            else
            {
                s = size<direction::indirect>();
                if (s == capacity<direction::indirect>())
                {
                    realloc(calculate_new_capacity(s + 1, s));
                }
            }

            set_size<direction::indirect>(s + 1);
            return *hpx::construct_at(
                static_cast<T*>(data<direction::indirect>() + s),
                HPX_FORWARD(Args, args)...);
        }

        void push_back(T const& value)
        {
            emplace_back(value);
        }

        void push_back(T&& value)
        {
            emplace_back(HPX_MOVE(value));
        }

        [[nodiscard]] auto operator[](std::size_t idx) const noexcept
            HPX_PRE(idx < this->size()) -> T const&
        {
            return *(data() + idx);
        }

        [[nodiscard]] auto operator[](std::size_t idx) noexcept
            HPX_PRE(idx < this->size()) -> T&
        {
            return *(data() + idx);
        }

        [[nodiscard]] auto at(std::size_t idx) const -> T const&
        {
            if (is_direct())
            {
                return at<direction::direct>(idx);
            }
            return at<direction::indirect>(idx);
        }

        auto at(std::size_t idx) -> T&
        {
            if (is_direct())
            {
                return at<direction::direct>(idx);
            }
            return at<direction::indirect>(idx);
        }

        [[nodiscard]] auto begin() const noexcept -> T const*
        {
            return data();
        }

        [[nodiscard]] auto cbegin() const noexcept -> T const*
        {
            return begin();
        }

        [[nodiscard]] auto begin() noexcept -> T*
        {
            return data();
        }

        [[nodiscard]] auto end() const noexcept -> T const*
        {
            if (is_direct())
            {
                return data<direction::direct>() + size<direction::direct>();
            }
            return data<direction::indirect>() + size<direction::indirect>();
        }

        [[nodiscard]] auto cend() const noexcept -> T const*
        {
            return end();
        }

        [[nodiscard]] auto end() noexcept -> T*
        {
            if (is_direct())
            {
                return data<direction::direct>() + size<direction::direct>();
            }
            return data<direction::indirect>() + size<direction::indirect>();
        }

        [[nodiscard]] auto rbegin() noexcept -> reverse_iterator
        {
            return reverse_iterator{end()};
        }

        [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator
        {
            return crbegin();
        }

        [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator
        {
            return const_reverse_iterator{end()};
        }

        [[nodiscard]] auto rend() noexcept -> reverse_iterator
        {
            return reverse_iterator{begin()};
        }

        [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator
        {
            return crend();
        }

        [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator
        {
            return const_reverse_iterator{begin()};
        }

        [[nodiscard]] auto front() const noexcept HPX_PRE(!this->empty())
            -> T const&
        {
            return *data();
        }

        [[nodiscard]] auto front() noexcept HPX_PRE(!this->empty()) -> T&
        {
            return *data();
        }

        [[nodiscard]] auto back() const noexcept HPX_PRE(!this->empty())
            -> T const&
        {
            if (is_direct())
            {
                return *(
                    data<direction::direct>() + size<direction::direct>() - 1);
            }
            return *(
                data<direction::indirect>() + size<direction::indirect>() - 1);
        }

        [[nodiscard]] auto back() noexcept HPX_PRE(!this->empty()) -> T&
        {
            if (is_direct())
            {
                return *(
                    data<direction::direct>() + size<direction::direct>() - 1);
            }
            return *(
                data<direction::indirect>() + size<direction::indirect>() - 1);
        }

        void clear() noexcept
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                std::destroy(begin(), end());
            }

            if (is_direct())
            {
                set_size<direction::direct>(0);
            }
            else
            {
                set_size<direction::indirect>(0);
            }
        }

        [[nodiscard]] constexpr auto empty() const noexcept -> bool
        {
            return 0U == size();
        }

        void pop_back()
        {
            if (is_direct())
            {
                pop_back<direction::direct>();
            }
            else
            {
                pop_back<direction::indirect>();
            }
        }

        [[nodiscard]] static constexpr auto max_size() -> std::size_t
        {
            return (std::numeric_limits<std::size_t>::max)();
        }

        void swap(small_vector& other) noexcept
        {
            // TODO we could try to do the minimum number of moves
            std::swap(*this, other);
        }

        void shrink_to_fit()
        {
            // per the standard we wouldn't need to do anything here. But since
            // we are so nice, let's do the shrink.
            auto const c = capacity();
            auto const s = size();
            if (s >= c)
            {
                return;
            }

            auto new_capacity = calculate_new_capacity(s, N);
            if (new_capacity == c)
            {
                // nothing change!
                return;
            }

            realloc(new_capacity);
        }

        template <typename... Args>
        auto emplace(const_iterator pos, Args&&... args) -> iterator
        {
            auto* p = make_uninitialized_space(pos, 1);
            return hpx::construct_at(
                static_cast<T*>(p), HPX_FORWARD(Args, args)...);
        }

        auto insert(const_iterator pos, T const& value) -> iterator
        {
            return emplace(pos, value);
        }

        auto insert(const_iterator pos, T&& value) -> iterator
        {
            return emplace(pos, HPX_MOVE(value));
        }

        auto insert(const_iterator pos, std::size_t count, T const& value)
            -> iterator
        {
            auto* p = make_uninitialized_space(pos, count);
            std::uninitialized_fill_n(p, count, value);
            return p;
        }

        template <typename It>
        auto insert(const_iterator pos, It first, It last,
            std::input_iterator_tag /*unused*/)
        {
            if (!(first != last))
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                return const_cast<T*>(pos);
            }

            // just input_iterator_tag makes this very slow. Let's do the same
            // as the STL.
            if (pos == end())
            {
                auto s = size();
                while (first != last)
                {
                    emplace_back(*first);
                    ++first;
                }
                return begin() + s;
            }

            auto tmp = small_vector(first, last);
            return insert(pos, std::make_move_iterator(tmp.begin()),
                std::make_move_iterator(tmp.end()));
        }

        template <typename It>
        auto insert(const_iterator pos, It first, It last,
            std::forward_iterator_tag /*unused*/)
        {
            auto* p = make_uninitialized_space(pos, std::distance(first, last));
            std::uninitialized_copy(first, last, p);
            return p;
        }

        template <typename InputIt,
            typename =
                std::enable_if_t<detail::is_argument_iterator_v<InputIt>>>
        auto insert(const_iterator pos, InputIt first, InputIt last) -> iterator
        {
            return insert(pos, first, last,
                typename std::iterator_traits<InputIt>::iterator_category());
        }

        auto insert(const_iterator pos, std::initializer_list<T> l) -> iterator
        {
            return insert(pos, l.begin(), l.end());
        }

        auto erase(const_iterator pos) -> iterator
        {
            return erase(pos, pos + 1);
        }

        auto erase(const_iterator first, const_iterator last) -> iterator
        {
            if (is_direct())
            {
                return erase_checked_end<direction::direct>(first, last);
            }
            return erase_checked_end<direction::indirect>(first, last);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t NA, std::size_t NB>
    [[nodiscard]] constexpr auto operator==(small_vector<T, NA> const& a,
        small_vector<T, NB> const& b) noexcept -> bool
    {
        return std::equal(a.begin(), a.end(), b.begin(), b.end());
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t NA, std::size_t NB>
    [[nodiscard]] constexpr auto operator!=(small_vector<T, NA> const& a,
        small_vector<T, NB> const& b) noexcept -> bool
    {
        return !(a == b);
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t NA, std::size_t NB>
    [[nodiscard]] constexpr auto operator<(small_vector<T, NA> const& a,
        small_vector<T, NB> const& b) noexcept -> bool
    {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end());
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t NA, std::size_t NB>
    [[nodiscard]] constexpr auto operator>=(small_vector<T, NA> const& a,
        small_vector<T, NB> const& b) noexcept -> bool
    {
        return !(a < b);
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t NA, std::size_t NB>
    [[nodiscard]] constexpr auto operator>(small_vector<T, NA> const& a,
        small_vector<T, NB> const& b) noexcept -> bool
    {
        return std::lexicographical_compare(
            b.begin(), b.end(), a.begin(), a.end());
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t NA, std::size_t NB>
    [[nodiscard]] constexpr auto operator<=(small_vector<T, NA> const& a,
        small_vector<T, NB> const& b) noexcept -> bool
    {
        return !(a > b);
    }
}    // namespace hpx::detail

// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {    //-V1061

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N, typename U>
    constexpr auto erase(hpx::detail::small_vector<T, N>& sv, U const& value) ->
        typename hpx::detail::small_vector<T, N>::size_type
    {
        auto* removed_begin = std::remove(sv.begin(), sv.end(), value);
        auto num_removed = std::distance(removed_begin, sv.end());
        sv.erase(removed_begin, sv.end());
        return num_removed;
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N, typename Pred>
    constexpr auto erase_if(hpx::detail::small_vector<T, N>& sv, Pred pred) ->
        typename hpx::detail::small_vector<T, N>::size_type
    {
        auto* removed_begin = std::remove_if(sv.begin(), sv.end(), pred);
        auto num_removed = std::distance(removed_begin, sv.end());
        sv.erase(removed_begin, sv.end());
        return num_removed;
    }
}    // namespace std
