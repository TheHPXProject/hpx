//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/type_support.hpp>

#include <cassert>
#include <memory>
#include <mutex>
#include <type_traits>

using hpx::experimental::is_replaceable;
using hpx::experimental::is_replaceable_v;

// Integral types are replaceable (trivially relocatable)
static_assert(is_replaceable_v<int>);
// Const types are not assignable (thus not replaceable)
static_assert(!is_replaceable_v<int const>);

// Pointer types are replaceable if they are not const-qualified themselves.
static_assert(is_replaceable_v<int*>);
static_assert(is_replaceable_v<int const*>);

// Pointers follow the same rules as other objects. A pointer is replaceable only
// if it is not const-qualified itself, as replacement requires assignment.
// P2786R13 Section 3.2: "const-qualified objects are never replaceable."
static_assert(!is_replaceable_v<int* const>);

// Function pointers are replaceable
static_assert(is_replaceable_v<int (*)()>);

// Arrays are not assignable
static_assert(!is_replaceable_v<int[]>);
static_assert(!is_replaceable_v<int[4]>);

// References are not objects
static_assert(!is_replaceable_v<int&>);
static_assert(!is_replaceable_v<int&&>);

// Void types
static_assert(!is_replaceable_v<void>);

// std::mutex is not move assignable, nor trivially relocatable
static_assert(!is_replaceable_v<std::mutex>);

struct trivial_class
{
    int x;
};
static_assert(is_replaceable_v<trivial_class>);

// Replaceability implies a type can be destroyed and reconstructed.
// Consequently, a type must be destructible to be replaceable.
// Per P2786R13, implicit replaceability also requires trivial relocatability,
// which in turn requires trivial destructibility.
struct not_destructible
{
    not_destructible(not_destructible&&);
    ~not_destructible() = delete;
};
static_assert(!is_replaceable_v<not_destructible>);

struct not_move_assignable
{
    not_move_assignable& operator=(not_move_assignable&&) = delete;
};
static_assert(!is_replaceable_v<not_move_assignable>);

struct not_move_constructible
{
    not_move_constructible(not_move_constructible&&) = delete;
    not_move_constructible& operator=(not_move_constructible&&);
};
static_assert(!is_replaceable_v<not_move_constructible>);

struct move_assignable_but_not_implicitly_replaceable
{
    std::unique_ptr<int> p;
    move_assignable_but_not_implicitly_replaceable(
        move_assignable_but_not_implicitly_replaceable&&) = default;
    move_assignable_but_not_implicitly_replaceable& operator=(
        move_assignable_but_not_implicitly_replaceable&&) = default;
};
static_assert(
    !is_replaceable_v<move_assignable_but_not_implicitly_replaceable>);

// Opt-in example
struct opt_in_replaceable
{
    std::unique_ptr<int> p;
    opt_in_replaceable(opt_in_replaceable&&) = default;
    opt_in_replaceable& operator=(opt_in_replaceable&&) = default;
};

// Specialize is_replaceable for opt_in_replaceable
namespace hpx::experimental {
    template <>
    struct is_replaceable<opt_in_replaceable> : std::true_type
    {
    };
}    // namespace hpx::experimental

static_assert(is_replaceable_v<opt_in_replaceable>);

// volatile types are not replaceable because their state is unstable and
// bitwise move-assignment (replacement) cannot guarantee preservation of
// their specific access semantics.
static_assert(!is_replaceable_v<int volatile>);
static_assert(is_replaceable_v<int volatile*>);     // pointer to volatile is OK
static_assert(!is_replaceable_v<int* volatile>);    // volatile pointer is not

struct has_const_member
{
    int const x;
};
// move assignment is deleted for classes with const members
static_assert(!is_replaceable_v<has_const_member>);

struct has_volatile_member
{
    int volatile x;
};
// volatile members do not necessarily make move assignment non-trivial
static_assert(is_replaceable_v<has_volatile_member>);

int main() {}
