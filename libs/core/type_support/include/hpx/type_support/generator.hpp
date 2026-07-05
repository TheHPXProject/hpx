//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

// clang up to V12 refuses to compile the code below
#if defined(HPX_HAVE_CXX20_COROUTINES) &&                                      \
    (!defined(HPX_CLANG_VERSION) || HPX_CLANG_VERSION >= 130000)
#if defined(HPX_HAVE_CXX23_STD_GENERATOR)

#include <generator>

namespace hpx {

    HPX_CXX_CORE_EXPORT template <typename Ref, typename V = void,
        typename Allocator = void>
    using generator = std::generator<Ref, V, Allocator>;
}

#else

#include <hpx/assert.hpp>
#include <hpx/type_support/coroutines_support.hpp>
#include <hpx/type_support/default_sentinel.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#if defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
#define HPX_STDCPP_DEFAULT_NEW_ALIGNMENT __STDCPP_DEFAULT_NEW_ALIGNMENT__
#else
#define HPX_STDCPP_DEFAULT_NEW_ALIGNMENT 16
#endif

// see: Casey Carter, Lewis Baker, Corentin Jabot. std::generator
// implementation. https://godbolt.org/z/5hcaPcfvP

namespace hpx {

    HPX_CXX_CORE_EXPORT template <typename Ref, typename V = void,
        typename Allocator = void>
    struct generator;

    namespace detail {

        HPX_CXX_CORE_EXPORT struct alignas(HPX_STDCPP_DEFAULT_NEW_ALIGNMENT)
            aligned_block
        {
            unsigned char pad[HPX_STDCPP_DEFAULT_NEW_ALIGNMENT];
        };

        HPX_CXX_CORE_EXPORT template <typename Allocator>
        using rebind = typename std::allocator_traits<
            Allocator>::template rebind_alloc<aligned_block>;

        HPX_CXX_CORE_EXPORT template <typename Allocator>
        concept has_real_pointers = std::is_void_v<Allocator> ||
            std::is_pointer_v<
                typename std::allocator_traits<Allocator>::pointer>;

        // clang-format off
        HPX_CXX_CORE_EXPORT template <typename T, typename U>
        concept common_reference_with =
            std::same_as<std::common_reference_t<T, U>,
                std::common_reference_t<U, T>> &&
            std::is_convertible_v<T, std::common_reference_t<T, U>> &&
            std::is_convertible_v<U, std::common_reference_t<T, U>>;
        // clang-format on

        // statically specified allocator type
        HPX_CXX_CORE_EXPORT template <typename Alloc = void>
        class promise_allocator
        {
            using Allocator = rebind<Alloc>;

            static char* allocate(Allocator alloc, std::size_t const size)
            {
                if constexpr (std::is_default_constructible_v<Allocator> &&
                    std::allocator_traits<Allocator>::is_always_equal::value)
                {
                    // do not store stateless allocator, but store size of block
                    std::size_t const count = (size + sizeof(aligned_block) +
                                                  sizeof(std::size_t) - 1) /
                        sizeof(aligned_block);

                    void* ptr = alloc.allocate(count);

                    char* address = static_cast<char*>(ptr);
                    *reinterpret_cast<std::size_t*>(address) = size;

                    return address;
                }
                else
                {
                    // store stateful allocator and size of block
                    static constexpr std::size_t align =
                        (std::max) (alignof(Allocator), sizeof(aligned_block));

                    std::size_t const count =
                        (size + sizeof(Allocator) + sizeof(std::size_t) +
                            align - 1) /
                        sizeof(aligned_block);

                    void* const ptr = alloc.allocate(count);

                    char* address = static_cast<char*>(ptr);
                    *reinterpret_cast<std::size_t*>(address) = size;

                    auto const alloc_address =
                        (reinterpret_cast<std::uintptr_t>(ptr) + size +
                            sizeof(std::size_t) + alignof(Allocator) - 1) &
                        ~(alignof(Allocator) - 1);

                    ::new (reinterpret_cast<void*>(alloc_address))
                        Allocator(HPX_MOVE(alloc));

                    return address;
                }
            }

            static void dealloc(void* const ptr, std::size_t const size)
            {
                if constexpr (std::is_default_constructible_v<Allocator> &&
                    std::allocator_traits<Allocator>::is_always_equal::value)
                {
                    // make stateless allocator
                    Allocator alloc{};
                    std::size_t const count = (size + sizeof(std::size_t) +
                                                  sizeof(aligned_block) - 1) /
                        sizeof(aligned_block);

                    alloc.deallocate(static_cast<aligned_block*>(ptr), count);
                }
                else
                {
                    // retrieve stateful allocator
                    auto const address =
                        (reinterpret_cast<std::uintptr_t>(ptr) +
                            sizeof(std::size_t) + size + alignof(Allocator) -
                            1) &
                        ~(alignof(Allocator) - 1);

                    auto& stored_allocator =
                        *reinterpret_cast<Allocator*>(address);
                    Allocator alloc{HPX_MOVE(stored_allocator)};
                    stored_allocator.~Allocator();

                    static constexpr std::size_t align =
                        (std::max) (alignof(Allocator), sizeof(aligned_block));
                    std::size_t const count =
                        (size + sizeof(std::size_t) + sizeof(Allocator) +
                            align - 1) /
                        sizeof(aligned_block);

                    alloc.deallocate(static_cast<aligned_block*>(ptr), count);
                }
            }

        public:
            // Allocator support
            // clang-format off
            void* operator new(std::size_t const size)
                requires(std::is_default_constructible_v<Allocator>)
            {
                return allocate(Allocator{}, size) + sizeof(std::size_t);
            }

            template <typename Allocator2, typename... Args>
                requires(std::is_convertible_v<Allocator2 const&, Allocator>)
            void* operator new(std::size_t const size, std::allocator_arg_t,
                Allocator2 const& alloc, Args const&...)
            {
                return allocate(
                           static_cast<Allocator>(static_cast<Alloc>(alloc)),
                           size) +
                    sizeof(std::size_t);
            }

            template <typename This, typename Alloc2, typename... Args>
                requires(std::is_convertible_v<Alloc2 const&, Allocator>)
            void* operator new(std::size_t const size, This const&,
                std::allocator_arg_t, Alloc2 const& alloc, Args const&...)
            {
                return allocate(
                           static_cast<Allocator>(static_cast<Alloc>(alloc)),
                           size) +
                    sizeof(std::size_t);
            }
            // clang-format on

            // some older versions of gcc complain about mismatched-new-delete
            // if this delete overload is missing
            void operator delete(void* const ptr) noexcept
            {
                // the size is stored before the address
                char* address = static_cast<char*>(ptr) - sizeof(std::size_t);
                dealloc(address, *reinterpret_cast<std::size_t*>(address));
            }

            void operator delete(
                void* const ptr, std::size_t const size) noexcept
            {
                char* address = static_cast<char*>(ptr) - sizeof(std::size_t);
                HPX_ASSERT(*reinterpret_cast<std::size_t*>(address) == size);
                dealloc(address, size);
            }
        };

        // type-erased allocator
        template <>
        class promise_allocator<void>
        {
            using dealloc_fn = void (*)(void*, std::size_t);

            template <typename ProtoAlloc>
            static char* allocate(ProtoAlloc const& proto, std::size_t size)
            {
                using Allocator = rebind<ProtoAlloc>;
                auto alloc = static_cast<Allocator>(proto);

                if constexpr (std::is_default_constructible_v<Allocator> &&
                    std::allocator_traits<Allocator>::is_always_equal::value)
                {
                    // don't store stateless allocator, but store size of
                    // allocated block
                    dealloc_fn const dealloc = [](void* const ptr,
                                                   std::size_t const s) {
                        Allocator alloc{};
                        std::size_t const count =
                            (s + sizeof(dealloc_fn) + sizeof(aligned_block) +
                                sizeof(std::size_t) - 1) /
                            sizeof(aligned_block);

                        alloc.deallocate(
                            static_cast<aligned_block*>(ptr), count);
                    };

                    std::size_t const count =
                        (size + sizeof(dealloc_fn) + sizeof(std::size_t) +
                            sizeof(aligned_block) - 1) /
                        sizeof(aligned_block);

                    void* const ptr = alloc.allocate(count);

                    char* address = static_cast<char*>(ptr);
                    *reinterpret_cast<std::size_t*>(address) = size;

                    // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
                    std::memcpy(address + size + sizeof(std::size_t), &dealloc,
                        sizeof(dealloc));

                    return address;
                }
                else
                {
                    // store stateful allocator and size of allocated block
                    static constexpr std::size_t align =
                        (std::max) (alignof(Allocator), sizeof(aligned_block));

                    dealloc_fn const dealloc = [](void* const ptr,
                                                   std::size_t s) {
                        s += sizeof(std::size_t) + sizeof(dealloc_fn);
                        auto const address =
                            (reinterpret_cast<std::uintptr_t>(ptr) + s +
                                alignof(Allocator) - 1) &
                            ~(alignof(Allocator) - 1);

                        auto& stored_allocator =
                            *reinterpret_cast<Allocator const*>(address);
                        Allocator alloc{HPX_MOVE(stored_allocator)};
                        stored_allocator.~Allocator();

                        std::size_t const count =
                            (s + sizeof(alloc) + align - 1) /
                            sizeof(aligned_block);

                        alloc.deallocate(
                            static_cast<aligned_block*>(ptr), count);
                    };

                    std::size_t const count =
                        (size + sizeof(dealloc_fn) + sizeof(alloc) + align +
                            sizeof(std::size_t) - 1) /
                        sizeof(aligned_block);

                    void* const ptr = alloc.allocate(count);

                    // store size
                    char* address = static_cast<char*>(ptr);
                    *reinterpret_cast<std::size_t*>(address) = size;

                    // store deleter

                    // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
                    std::memcpy(address + sizeof(std::size_t) + size, &dealloc,
                        sizeof(dealloc));

                    size += sizeof(std::size_t) + sizeof(dealloc_fn);
                    auto const alloc_address =
                        (reinterpret_cast<uintptr_t>(ptr) + size +
                            alignof(Allocator) - 1) &
                        ~(alignof(Allocator) - 1);

                    ::new (reinterpret_cast<void*>(alloc_address))
                        Allocator{HPX_MOVE(alloc)};

                    return address;
                }
            }

        public:
            // default: new/delete
            void* operator new(std::size_t const size)
            {
                void* const ptr = ::operator new[](
                    size + sizeof(std::size_t) + sizeof(dealloc_fn));

                dealloc_fn const dealloc =
                    [](void* const p, [[maybe_unused]] std::size_t const s) {
#if defined(HPX_HAVE_CXX14_DELETE_OPERATOR_WITH_SIZE)
                        ::operator delete[](
                            p, s + sizeof(std::size_t) + sizeof(dealloc_fn));
#else
                        ::operator delete[](p);
#endif
                    };

                char* address = static_cast<char*>(ptr);
                *reinterpret_cast<std::size_t*>(address) = size;

                // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
                std::memcpy(address + sizeof(std::size_t) + size, &dealloc,
                    sizeof(dealloc_fn));

                return address + sizeof(std::size_t);
            }

            template <typename Allocator, typename... Args>
            void* operator new(std::size_t const size, std::allocator_arg_t,
                Allocator const& alloc, Args const&...)
            {
                return allocate(alloc, size) + sizeof(std::size_t);
            }

            template <typename This, typename Alloc, typename... Args>
            void* operator new(size_t const size, This const&,
                std::allocator_arg_t, Alloc const& alloc, Args const&...)
            {
                static_assert(has_real_pointers<Alloc>,
                    "coroutine allocators must use true pointers");
                return allocate(alloc, size) + sizeof(std::size_t);
            }

            // some older versions of gcc complain about mismatched-new-delete
            // if this delete overload is missing
            void operator delete(void* const ptr) noexcept
            {
                // the size is stored before the address
                char* address = static_cast<char*>(ptr) - sizeof(std::size_t);
                std::size_t const size =
                    *reinterpret_cast<std::size_t*>(address);

                dealloc_fn dealloc;

                // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
                std::memcpy(&dealloc, static_cast<char*>(ptr) + size,
                    sizeof(dealloc_fn));
                dealloc(address, size);
            }

            void operator delete(
                void* const ptr, std::size_t const size) noexcept
            {
                // the size is stored before the address
                char* address = static_cast<char*>(ptr) - sizeof(std::size_t);
                HPX_ASSERT(size == *reinterpret_cast<std::size_t*>(address));

                dealloc_fn dealloc;

                // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
                std::memcpy(&dealloc, static_cast<char*>(ptr) + size,
                    sizeof(dealloc_fn));
                dealloc(address, size);
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Ref, typename V>
        using gen_value_t =
            std::conditional_t<std::is_void_v<V>, std::remove_cvref_t<Ref>, V>;

        HPX_CXX_CORE_EXPORT template <typename Ref, typename V>
        using gen_reference_t =
            std::conditional_t<std::is_void_v<V>, Ref&&, Ref>;

        HPX_CXX_CORE_EXPORT template <typename Ref>
        using gen_yield_t =
            std::conditional_t<std::is_reference_v<Ref>, Ref, Ref const&>;

        HPX_CXX_CORE_EXPORT template <typename Value, typename Ref>
        class gen_iter;

        HPX_CXX_CORE_EXPORT template <typename Yielded>
        class gen_promise_base
        {
        public:
            static_assert(std::is_reference_v<Yielded>);

            static constexpr hpx::suspend_always initial_suspend() noexcept
            {
                return {};
            }

            static constexpr auto final_suspend() noexcept
            {
                return final_awaiter{};
            }

            hpx::suspend_always yield_value(Yielded val) noexcept
            {
                ptr = ::std::addressof(val);
                return {};
            }

            // clang-format off
            auto
            yield_value(std::remove_reference_t<Yielded> const& val) noexcept(
                std::is_nothrow_constructible_v<std::remove_cvref_t<Yielded>,
                    std::remove_reference_t<Yielded> const&>)
                requires(std::is_rvalue_reference_v<Yielded> &&
                    std::is_constructible_v<std::remove_cvref_t<Yielded>,
                        std::remove_reference_t<Yielded> const&>)
            {
                return element_awaiter{val};
            }
            // clang-format on

            void await_transform() = delete;

            static constexpr void return_void() noexcept {}

            void unhandled_exception()
            {
                if (info)
                {
                    info->except = ::std::current_exception();
                }
                else
                {
                    throw;
                }
            }

        private:
            struct element_awaiter
            {
                std::remove_cvref_t<Yielded> val;

                static constexpr bool await_ready() noexcept
                {
                    return false;
                }

                template <typename Promise>
                constexpr void await_suspend(
                    hpx::coroutine_handle<Promise> handle) noexcept
                {
#ifdef __cpp_lib_is_pointer_interconvertible
                    static_assert(std::is_pointer_interconvertible_base_of_v<
                        gen_promise_base, Promise>);
#endif    // __cpp_lib_is_pointer_interconvertible

                    gen_promise_base& current = handle.promise();
                    current.ptr = ::std::addressof(val);
                }

                static constexpr void await_resume() noexcept {}
            };

            struct nest_info
            {
                std::exception_ptr except;
                hpx::coroutine_handle<gen_promise_base> parent;
                hpx::coroutine_handle<gen_promise_base> root;
            };

            struct final_awaiter
            {
                static constexpr bool await_ready() noexcept
                {
                    return false;
                }

                template <typename Promise>
                hpx::coroutine_handle<> await_suspend(
                    hpx::coroutine_handle<Promise> handle) noexcept
                {
#ifdef __cpp_lib_is_pointer_interconvertible
                    static_assert(std::is_pointer_interconvertible_base_of_v<
                        gen_promise_base, Promise>);
#endif    // __cpp_lib_is_pointer_interconvertible

                    gen_promise_base& current = handle.promise();
                    if (!current.info)
                    {
                        return hpx::noop_coroutine();
                    }

                    hpx::coroutine_handle<gen_promise_base> cont =
                        current.info->parent;
                    current.info->root.promise().top = cont;
                    current.info = nullptr;
                    return cont;
                }

                static constexpr void await_resume() noexcept {}
            };

            template <typename Ref, typename V, typename Alloc>
            struct nested_awaitable
            {
                static_assert(std::same_as<gen_yield_t<gen_reference_t<Ref, V>>,
                    Yielded>);

                nest_info nested;
                generator<Ref, V, Alloc> gen;

                explicit nested_awaitable(
                    generator<Ref, V, Alloc>&& gen_) noexcept
                  : gen(HPX_MOVE(gen_))
                {
                }

                constexpr bool await_ready() noexcept
                {
                    return !gen.coro;
                }

                template <typename Promise>
                hpx::coroutine_handle<gen_promise_base> await_suspend(
                    hpx::coroutine_handle<Promise> current) noexcept
                {
#ifdef __cpp_lib_is_pointer_interconvertible
                    static_assert(std::is_pointer_interconvertible_base_of_v<
                        gen_promise_base, Promise>);
#endif    // __cpp_lib_is_pointer_interconvertible

                    auto target =
                        hpx::coroutine_handle<gen_promise_base>::from_address(
                            gen._Coro.address());
                    nested.parent =
                        hpx::coroutine_handle<gen_promise_base>::from_address(
                            current.address());

                    gen_promise_base& parent_promise = nested.parent.promise();
                    if (parent_promise.info)
                    {
                        nested.root = parent_promise.info->root;
                    }
                    else
                    {
                        nested.root = nested.parent;
                    }
                    nested.root.promise().top = target;
                    target.promise().info = ::std::addressof(nested);
                    return target;
                }

                void await_resume()
                {
                    if (nested.except)
                    {
                        ::std::rethrow_exception(HPX_MOVE(nested.except));
                    }
                }
            };

            template <typename, typename>
            friend class gen_iter;

            // top and info are mutually exclusive, and could potentially be merged.
            hpx::coroutine_handle<gen_promise_base> top =
                hpx::coroutine_handle<gen_promise_base>::from_promise(*this);
            std::add_pointer_t<Yielded> ptr = nullptr;
            nest_info* info = nullptr;
        };

        HPX_CXX_CORE_EXPORT struct gen_secret_tag
        {
        };

        HPX_CXX_CORE_EXPORT template <typename Value, typename Ref>
        class gen_iter
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = Value;
            using difference_type = std::ptrdiff_t;
            using pointer = std::add_pointer_t<value_type>;
            using reference = Ref;

            gen_iter(gen_iter const& that) = default;
            gen_iter& operator=(gen_iter const& that) = default;

            gen_iter(gen_iter&& that) noexcept
              : coro{::std::exchange(that.coro, {})}
            {
            }

            ~gen_iter() = default;

            gen_iter& operator=(gen_iter&& that) noexcept
            {
                coro = ::std::exchange(that.coro, {});
                return *this;
            }

            Ref operator*() const noexcept
            {
                HPX_ASSERT_MSG(
                    !coro.done(), "Can't dereference generator end iterator");
                return static_cast<Ref>(*coro.promise().top.promise().ptr);
            }

            gen_iter& operator++()
            {
                HPX_ASSERT_MSG(
                    !coro.done(), "Can't increment generator end iterator");
                coro.promise().top.resume();
                return *this;
            }

            void operator++(int)
            {
                ++*this;
            }

            [[nodiscard]] friend bool operator==(
                gen_iter lhs, hpx::default_sentinel_t) noexcept
            {
                return lhs.coro.done();
            }
            [[nodiscard]] friend bool operator!=(
                gen_iter lhs, hpx::default_sentinel_t rhs) noexcept
            {
                return !(lhs == rhs);
            }

            [[nodiscard]] friend bool operator==(
                hpx::default_sentinel_t, gen_iter rhs) noexcept
            {
                return rhs.coro.done();
            }
            [[nodiscard]] friend bool operator!=(
                hpx::default_sentinel_t lhs, gen_iter rhs) noexcept
            {
                return !(lhs == rhs);
            }

        private:
            template <typename, typename, typename>
            friend struct hpx::generator;

            explicit gen_iter(gen_secret_tag,
                hpx::coroutine_handle<gen_promise_base<gen_yield_t<Ref>>>
                    coro_) noexcept
              : coro{coro_}
            {
            }

            hpx::coroutine_handle<gen_promise_base<gen_yield_t<Ref>>> coro;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename Ref, typename V, typename Allocator>
    struct generator
    {
    private:
        using value = detail::gen_value_t<Ref, V>;
        static_assert(std::is_same_v<std::remove_cvref_t<value>, value> &&
                std::is_object_v<value>,
            "generator's value type must be a cv-unqualified object type");

        using reference = detail::gen_reference_t<Ref, V>;
        static_assert(std::is_reference_v<reference> ||
                (std::is_object_v<reference> &&
                    std::is_same_v<std::remove_cv_t<reference>, reference> &&
                    std::is_copy_constructible_v<reference>),
            "generator's first argument must be a reference type or a "
            "cv-unqualified copy-constructible object type");

        using rv_reference =
            std::conditional_t<std::is_lvalue_reference_v<reference>,
                std::remove_reference_t<reference>&&, reference>;

        static_assert(detail::common_reference_with<reference&&, value&> &&
                detail::common_reference_with<reference&&, rv_reference&&> &&
                detail::common_reference_with<rv_reference&&, value const&>,
            "an iterator with the selected value and reference types cannot "
            "model indirectly_readable");

        static_assert(detail::has_real_pointers<Allocator>,
            "generator allocators must use true pointers");

        friend detail::gen_promise_base<detail::gen_yield_t<reference>>;

    public:
        struct HPX_EMPTY_BASES promise_type
          : detail::promise_allocator<Allocator>
          , detail::gen_promise_base<detail::gen_yield_t<reference>>
        {
            [[nodiscard]] constexpr generator get_return_object() noexcept
            {
                return generator{detail::gen_secret_tag{},
                    hpx::coroutine_handle<promise_type>::from_promise(*this)};
            }
        };
        static_assert(std::is_standard_layout_v<promise_type>);

#ifdef __cpp_lib_is_pointer_interconvertible
        static_assert(std::is_pointer_interconvertible_base_of_v<
            detail::gen_promise_base<detail::gen_yield_t<reference>>,
            promise_type>);
#endif    // __cpp_lib_is_pointer_interconvertible

        generator(generator const& that) = delete;
        generator(generator&& that) noexcept
          : coro(::std::exchange(that.coro, {}))
        {
        }

        ~generator()
        {
            if (coro)
            {
                coro.destroy();
            }
        }

        generator& operator=(generator const& that) = delete;
        generator& operator=(generator&& that) noexcept
        {
            ::std::swap(coro, that.coro);
            return *this;
        }

        [[nodiscard]] detail::gen_iter<value, reference> begin() const
        {
            // Pre: coro is suspended at its initial suspend point
            HPX_ASSERT_MSG(coro, "Can't call begin on moved-from generator");

            coro.resume();

            // clang-format off
            return detail::gen_iter<value, reference>{detail::gen_secret_tag{},
                coroutine_handle<detail::gen_promise_base<
                    detail::gen_yield_t<reference>>>::from_address(coro
                        .address())};
            // clang-format on
        }

        [[nodiscard]] static constexpr hpx::default_sentinel_t end() noexcept
        {
            return hpx::default_sentinel;
        }

    private:
        // for some compilers coro.resume() is not const
        mutable hpx::coroutine_handle<promise_type> coro = nullptr;

        constexpr explicit generator(detail::gen_secret_tag,
            hpx::coroutine_handle<promise_type> coro_) noexcept
          : coro(coro_)
        {
        }
    };
}    // namespace hpx

#undef HPX_STDCPP_DEFAULT_NEW_ALIGNMENT

#endif
#endif
