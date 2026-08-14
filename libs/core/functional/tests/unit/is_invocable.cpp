//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/testing.hpp>

struct X
{
    void operator()(int);
};
struct Xc
{
    void operator()(int) const;
};

template <typename T>
struct smart_ptr
{
    T* p;
    T& operator*() const
    {
        return *p;
    }
};

void nullary_function()
{
    typedef void (*f)();
    HPX_TEST_MSG((hpx::is_invocable_v<f> == true), "nullary function");
}

void lambdas()
{
    auto lambda = []() {};

    typedef decltype(lambda) f;
    HPX_TEST_MSG((hpx::is_invocable_v<f> == true), "lambda");
}

void functions_byval_params()
{
    typedef void (*f)(int);
    HPX_TEST_MSG((hpx::is_invocable_v<f, int> == true), "fun-value/value");
    HPX_TEST_MSG((hpx::is_invocable_v<f, int&> == true), "fun-value/lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, int const&> == true), "fun-value/const-lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, int&&> == true), "fun-value/rvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, int const&&> == true), "fun-value/const-rvref");

    typedef void (*fc)(int const);
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int> == true), "fun-const-value/value");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int&> == true), "fun-const-value/lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, int const&> == true),
        "fun-const-value/const-lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int&&> == true), "fun-const-value/rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, int const&&> == true),
        "fun-const-value/const-rvref");
}

void functions_bylvref_params()
{
    typedef void (*f)(int&);
    HPX_TEST_MSG((hpx::is_invocable_v<f, int> == false), "fun-lvref/value");
    HPX_TEST_MSG((hpx::is_invocable_v<f, int&> == true), "fun-lvref/lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, int const&> == false), "fun-lvref/const-lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, int&&> == false), "fun-lvref/rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, int const&&> == false),
        "fun-lvref/const-rvref");

    typedef void (*fc)(int const&);
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int> == true), "fun-const-lvref/value");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int&> == true), "fun-const-lvref/lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, int const&> == true),
        "fun-const-lvref/const-lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int&&> == true), "fun-const-lvref/rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, int const&&> == true),
        "fun-const-lvref/const-rvref");
}

void functions_byrvref_params()
{
    typedef void (*f)(int&&);
    HPX_TEST_MSG((hpx::is_invocable_v<f, int> == true), "fun-rvref/value");
    HPX_TEST_MSG((hpx::is_invocable_v<f, int&> == false), "fun-rvref/lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, int const&> == false), "fun-rvref/const-lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, int&&> == true), "fun-rvref/rvref");
#if !defined(BOOST_INTEL)
    HPX_TEST_MSG((hpx::is_invocable_v<f, int const&&> == false),
        "fun-rvref/const-rvref");
#endif

    typedef void (*fc)(int const&&);
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int> == true), "fun-const-rvref/value");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int&> == false), "fun-const-rvref/lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, int const&> == false),
        "fun-const-rvref/const-lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, int&&> == true), "fun-const-rvref/rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, int const&&> == true),
        "fun-const-rvref/const-rvref");
}

void member_function_pointers()
{
// clang in C++20 mode complains about invalid float->double promotion that
// can't be disabled :/
#if !defined(HPX_CLANG_VERSION)
    typedef int (X::*f)(double);
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, X*, float> == true), "mem-fun-ptr/ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<f, X const*, float> == false),
        "mem-fun-ptr/const-ptr");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, X&, float> == true), "mem-fun-ptr/lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, X const&, float> == false),
        "mem-fun-ptr/const-lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, X&&, float> == true), "mem-fun-ptr/rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, X const&&, float> == false),
        "mem-fun-ptr/const-rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, smart_ptr<X>, float> == true),
        "mem-fun-ptr/smart-ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<f, smart_ptr<X const>, float> == false),
        "mem-fun-ptr/smart-const-ptr");

    typedef int (X::*fc)(double) const;
    HPX_TEST_MSG(
        (hpx::is_invocable_v<fc, X*, float> == true), "const-mem-fun-ptr/ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, X const*, float> == true),
        "const-mem-fun-ptr/const-ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, X&, float> == true),
        "const-mem-fun-ptr/lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, X const&, float> == true),
        "const-mem-fun-ptr/const-lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, X&&, float> == true),
        "const-mem-fun-ptr/rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, X const&&, float> == true),
        "const-mem-fun-ptr/const-rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, smart_ptr<X>, float> == true),
        "const-mem-fun-ptr/smart-ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<fc, smart_ptr<X const>, float> == true),
        "const-mem-fun-ptr/smart-const-ptr");
#endif
}

void member_object_pointers()
{
    typedef int(X::* f);
    HPX_TEST_MSG((hpx::is_invocable_v<f, X*> == true), "mem-obj-ptr/ptr");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, X const*> == true), "mem-obj-ptr/const-ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<f, X&> == true), "mem-obj-ptr/lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, X const&> == true), "mem-obj-ptr/const-lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, X&&> == true), "mem-obj-ptr/rvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<f, X const&&> == true), "mem-obj-ptr/const-rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<f, smart_ptr<X>> == true),
        "mem-obj-ptr/smart-ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<f, smart_ptr<X const>> == true),
        "mem-obj-ptr/smart-const-ptr");
}

void function_objects()
{
    HPX_TEST_MSG((hpx::is_invocable_v<X, int> == true), "fun-obj/value");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<X const, int> == false), "fun-obj/const-value");
    HPX_TEST_MSG((hpx::is_invocable_v<X*, int> == false), "fun-obj/ptr");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<X const*, int> == false), "fun-obj/const-ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<X&, int> == true), "fun-obj/lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<X const&, int> == false), "fun-obj/const-lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<X&&, int> == true), "fun-obj/rvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<X const&&, int> == false), "fun-obj/const-rvref");

    HPX_TEST_MSG((hpx::is_invocable_v<Xc, int> == true), "const-fun-obj/value");
    HPX_TEST_MSG((hpx::is_invocable_v<Xc const, int> == true),
        "const-fun-obj/const-value");
    HPX_TEST_MSG((hpx::is_invocable_v<Xc*, int> == false), "const-fun-obj/ptr");
    HPX_TEST_MSG((hpx::is_invocable_v<Xc const*, int> == false),
        "const-fun-obj/const-ptr");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<Xc&, int> == true), "const-fun-obj/lvref");
    HPX_TEST_MSG((hpx::is_invocable_v<Xc const&, int> == true),
        "const-fun-obj/const-lvref");
    HPX_TEST_MSG(
        (hpx::is_invocable_v<Xc&&, int> == true), "const-fun-obj/rvref");
    HPX_TEST_MSG((hpx::is_invocable_v<Xc const&&, int> == true),
        "const-fun-obj/const-rvref");
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    {
        nullary_function();
        lambdas();
        functions_byval_params();
        functions_bylvref_params();
        functions_byrvref_params();
        member_function_pointers();
        member_object_pointers();
        function_objects();
    }

    return hpx::util::report_errors();
}
