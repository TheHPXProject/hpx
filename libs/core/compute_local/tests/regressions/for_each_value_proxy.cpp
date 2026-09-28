//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/compute.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename T>
class test_value_proxy
{
public:
    test_value_proxy(T& p)
      : p_(&p)
    {
    }

    test_value_proxy(test_value_proxy const& other)
      : p_(other.p_)
    {
    }

    test_value_proxy& operator=(T const& t)
    {
        *p_ = t;
        return *this;
    }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    test_value_proxy& operator=(test_value_proxy const& other)
    {
        p_ = other.p_;
        return *this;
    }

    operator T() const
    {
        return *p_;
    }

private:
    T* p_;
};

namespace hpx::traits {
    template <typename T>
    struct is_value_proxy<test_value_proxy<T>> : std::true_type
    {
    };

    template <typename T>
    struct proxy_value<test_value_proxy<T>>
    {
        using type = T;
    };
}    // namespace hpx::traits

template <typename T>
struct test_allocator
{
    typedef T value_type;
    typedef T* pointer;
    typedef T const* const_pointer;
    typedef test_value_proxy<T> reference;
    typedef test_value_proxy<T const> const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template <typename U>
    struct rebind
    {
        typedef test_allocator<U> other;
    };

    typedef std::false_type is_always_equal;
    typedef std::true_type propagate_on_container_move_assignment;

    test_allocator() {}

    template <typename U>
    test_allocator(test_allocator<U> const&)
    {
    }

    template <typename U>
    test_allocator(test_allocator<U>&&)
    {
    }

    // Returns the actual address of x even in presence of overloaded
    // operator&
    pointer address(reference x) const
    {
        return &x;
    }

    const_pointer address(const_reference x) const
    {
        return &x;
    }

    // Allocates n * sizeof(T) bytes of uninitialized storage by calling
    // topo.allocate(). The pointer hint may be used to provide locality of
    // reference: the allocator, if supported by the implementation, will
    // attempt to allocate the new memory block as close as possible to hint.
    pointer allocate(size_type n, void const* /* hint */ = nullptr)
    {
        return new T[n];
    }

    // Deallocates the storage referenced by the pointer p, which must be a
    // pointer obtained by an earlier call to allocate(). The argument n
    // must be equal to the first argument of the call to allocate() that
    // originally produced p; otherwise, the behavior is undefined.
    void deallocate(pointer p, size_type /* n */) noexcept
    {
        delete[] p;
    }

    // Returns the maximum theoretically possible value of n, for which the
    // call allocate(n, 0) could succeed. In most implementations, this
    // returns std::numeric_limits<size_type>::max() / sizeof(value_type).
    size_type max_size() const noexcept
    {
        return (std::numeric_limits<size_type>::max)();
    }

public:
    // Constructs count objects of type T in allocated uninitialized
    // storage pointed to by p, using placement-new. This will use the
    // underlying executors to distribute the memory according to
    // first touch memory placement.
    template <typename U, typename... Args>
    void bulk_construct(U*, std::size_t, Args&&...)
    {
    }

    // Constructs an object of type T in allocated uninitialized storage
    // pointed to by p, using placement-new
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        new (p) T(std::forward<Args>(args)...);
    }

    // Calls the destructor of count objects pointed to by p
    template <typename U>
    void bulk_destroy(U*, std::size_t)
    {
    }

    // Calls the destructor of the object pointed to by p
    template <typename U>
    void destroy(U* p)
    {
        std::destroy_at(p);
    }
};

///////////////////////////////////////////////////////////////////////////////
struct set_42
{
    // Accept both ordinary references and proxies returned by value.
    template <typename T>
    void operator()(T&& val)
    {
        val = 42;
    }
};

int hpx_main()
{
    hpx::compute::vector<int, test_allocator<int>> v(100);

    hpx::ranges::for_each(hpx::execution::par, v.begin(), v.end(), set_42());

    HPX_TEST_EQ(std::count(v.begin(), v.end(), 42), std::ptrdiff_t(v.size()));
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
