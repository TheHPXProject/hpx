//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:nodeprecatedinclude:boost/shared_ptr.hpp
// hpxinspect:nodeprecatedname:boost::shared_ptr
// hpxinspect:nodeprecatedinclude:boost/intrusive_ptr.hpp
// hpxinspect:nodeprecatedname:boost::intrusive_ptr

#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#if defined(HPX_SERIALIZATION_HAVE_BOOST_TYPES)
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include <memory>
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

void test_shared_null_clears_target()
{
    std::shared_ptr<int> const null_ptr;
    std::shared_ptr<int> target(new int(4));
    {
        std::vector<char> buffer;
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << null_ptr;

        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> target;
    }
    HPX_TEST(!target);
}

void test_unique_null_clears_target()
{
    std::unique_ptr<int> const null_ptr;
    std::unique_ptr<int> target(new int(4));
    {
        std::vector<char> buffer;
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << null_ptr;

        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> target;
    }
    HPX_TEST(!target);
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

int main()
{
    test_shared();
    test_unique();
    test_shared_null_clears_target();
    test_unique_null_clears_target();
#if defined(HPX_SERIALIZATION_HAVE_BOOST_TYPES)
    test_boost_shared();
    test_intrusive();
#endif

    return hpx::util::report_errors();
}
