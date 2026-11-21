//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:nodeprecatedinclude:boost/shared_ptr.hpp
// hpxinspect:nodeprecatedname:boost::shared_ptr
// hpxinspect:nodeprecatedinclude:boost/intrusive_ptr.hpp
// hpxinspect:nodeprecatedname:boost::intrusive_ptr

#include <hpx/modules/memory.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#if defined(HPX_SERIALIZATION_HAVE_BOOST_TYPES)
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

#if defined(HPX_SERIALIZATION_HAVE_BOOST_TYPES)
void test_boost_shared()
{
    boost::shared_ptr<int> ip(new int(7));
    boost::shared_ptr<int> op1;
    boost::shared_ptr<int> op2;
    {
        std::vector<char> buffer;
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << ip << ip;

        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> op1;
        iarchive >> op2;
    }
    HPX_TEST_NEQ(op1.get(), ip.get());
    HPX_TEST_NEQ(op2.get(), ip.get());
    HPX_TEST_EQ(op1.get(), op2.get());
    HPX_TEST_EQ(*op1, *ip);
    op1.reset();
    HPX_TEST_EQ(*op2, *ip);
}
#endif

void test_shared()
{
    std::shared_ptr<int> ip(new int(7));
    std::shared_ptr<int> op1;
    std::shared_ptr<int> op2;
    {
        std::vector<char> buffer;
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << ip << ip;

        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> op1;
        iarchive >> op2;
    }
    HPX_TEST_NEQ(op1.get(), ip.get());
    HPX_TEST_NEQ(op2.get(), ip.get());
    HPX_TEST_EQ(op1.get(), op2.get());
    HPX_TEST_EQ(*op1, *ip);
    op1.reset();
    HPX_TEST_EQ(*op2, *ip);
}

void test_unique()
{
    std::unique_ptr<int> ip(new int(7));
    std::unique_ptr<int> op1;
    std::unique_ptr<int> op2;
    {
        std::vector<char> buffer;
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << ip << ip;

        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> op1;
        iarchive >> op2;
    }
    HPX_TEST_NEQ(op1.get(), ip.get());
    HPX_TEST_NEQ(op2.get(), ip.get());
    HPX_TEST_NEQ(op1.get(), op2.get());    //untracked
    HPX_TEST_EQ(*op1, *ip);
    HPX_TEST_EQ(*op2, *ip);
}

#if defined(HPX_SERIALIZATION_HAVE_BOOST_TYPES)
struct A
{
    A()
      : i(7)
      , count(0)
    {
    }
    int i;

    int count;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & i;
    }
};

void intrusive_ptr_add_ref(A* a) noexcept
{
    ++a->count;
}

void intrusive_ptr_release(A* a) noexcept
{
    if (--a->count == 0)
    {
        delete a;
    }
}

void test_intrusive()
{
    boost::intrusive_ptr<A> ip(new A());
    boost::intrusive_ptr<A> op1;
    boost::intrusive_ptr<A> op2;
    {
        std::vector<char> buffer;
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << ip << ip;

        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> op1;
        iarchive >> op2;
    }
    HPX_TEST_EQ(ip->count, 1);
    HPX_TEST_NEQ(op1.get(), ip.get());
    HPX_TEST_NEQ(op2.get(), ip.get());
    HPX_TEST_EQ(op1.get(), op2.get());
    HPX_TEST_EQ(op1->i, ip->i);
    HPX_TEST_EQ(op1->count, 2);
    HPX_TEST_EQ(op2->count, 2);
    op1.reset();
    HPX_TEST_EQ(op2->count, 1);
    HPX_TEST_EQ(op2->i, ip->i);
}
#endif

// Memory that is handed out again as soon as it was freed: an object created
// after another one died has the same address.
namespace same_address {

    alignas(std::max_align_t) unsigned char storage[256];
    bool in_use = false;

    void* allocate(std::size_t const size)
    {
        if (!in_use && size <= sizeof(storage))
        {
            in_use = true;
            return storage;
        }
        return ::operator new(size);
    }

    void deallocate(void* p) noexcept
    {
        if (p == storage)
        {
            in_use = false;
            return;
        }
        ::operator delete(p);
    }

    template <typename T>
    struct allocator
    {
        using value_type = T;

        allocator() = default;

        // NOLINTNEXTLINE(google-explicit-constructor)
        template <typename U>
        allocator(allocator<U> const&) noexcept
        {
        }

        T* allocate(std::size_t const n)
        {
            return static_cast<T*>(same_address::allocate(n * sizeof(T)));
        }

        void deallocate(T* p, std::size_t) noexcept
        {
            same_address::deallocate(p);
        }

        friend bool operator==(allocator const&, allocator const&) noexcept
        {
            return true;
        }
    };

    struct counted
    {
        counted() = default;

        explicit counted(int const v) noexcept
          : value(v)
        {
        }

        static void* operator new(std::size_t const size)
        {
            return same_address::allocate(size);
        }

        static void operator delete(void* p) noexcept
        {
            same_address::deallocate(p);
        }

        int value = 0;
        int count = 0;
    };

    void intrusive_ptr_add_ref(counted* p) noexcept
    {
        ++p->count;
    }

    void intrusive_ptr_release(counted* p) noexcept
    {
        if (--p->count == 0)
        {
            delete p;
        }
    }

    // Serializes each value through a shared_ptr or intrusive_ptr that it
    // creates on the fly and that dies right after it was written: all of
    // them get the same address unless the archive keeps them alive.
    template <typename Pointer>
    struct temporaries
    {
        std::vector<int> values;

        static Pointer make(int const v)
        {
            if constexpr (std::is_same_v<Pointer, std::shared_ptr<int>>)
            {
                return std::allocate_shared<int>(allocator<int>(), v);
            }
            else
            {
                return Pointer(new counted(v));
            }
        }

        static int value_of(Pointer const& p)
        {
            if constexpr (std::is_same_v<Pointer, std::shared_ptr<int>>)
            {
                return *p;
            }
            else
            {
                return p->value;
            }
        }

        void save(hpx::serialization::output_archive& ar, unsigned) const
        {
            ar << values.size();
            for (int const v : values)
            {
                Pointer const p = make(v);
                ar << p;
            }
        }

        void load(hpx::serialization::input_archive& ar, unsigned)
        {
            std::size_t size = 0;
            ar >> size;
            values.clear();
            for (std::size_t i = 0; i != size; ++i)
            {
                Pointer p;
                ar >> p;
                values.push_back(value_of(p));
            }
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()
    };

    template <typename Archive>
    void serialize(Archive& ar, counted& c, unsigned)
    {
        ar & c.value;
    }
}    // namespace same_address

template <typename Pointer>
void test_temporaries()
{
    same_address::temporaries<Pointer> in;
    in.values = {1, 2, 3};

    same_address::temporaries<Pointer> out;
    {
        std::vector<char> buffer;
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << in;

        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> out;
    }

    // each temporary is written as an object of its own, not as a
    // back-reference to an earlier one at the same address
    HPX_TEST(out.values == in.values);

    // the archive released them
    HPX_TEST(!same_address::in_use);
}

int main()
{
    test_shared();
    test_unique();
    test_temporaries<std::shared_ptr<int>>();
    test_temporaries<hpx::intrusive_ptr<same_address::counted>>();
#if defined(HPX_SERIALIZATION_HAVE_BOOST_TYPES)
    test_boost_shared();
    test_intrusive();
#endif

    return hpx::util::report_errors();
}
