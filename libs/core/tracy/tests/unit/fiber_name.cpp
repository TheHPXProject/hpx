//  Copyright (c) 2026 The STE||AR Group
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Tracy keys a fiber by the address of its name and reads the text later.
// HPX reuses a thread object for many tasks, so the name of a task on a reused
// object must get its own address, and earlier names must not change. This
// needs no Tracy server.

#include <hpx/modules/testing.hpp>
#include <hpx/modules/tracy.hpp>

#include <cstdio>
#include <string>

int main()
{
    // The address of a local stands in for a reused HPX thread object. Names
    // are compared as addresses, as Tracy does, so a failure prints those.
    int thread_object = 0;
    void const* id = &thread_object;
    auto const key = [](char const* description, void const* object) {
        return static_cast<void const*>(
            hpx::tracy::fiber_name(description, object));
    };
    auto const text = [](void const* name) {
        return std::string(static_cast<char const*>(name));
    };

    char suffix[32];
    std::snprintf(suffix, sizeof(suffix), "_%p", id);

    void const* a = key("task_a", id);
    void const* b = key("task_b", id);

    // Two tasks on the same thread object get different fibers, and each
    // name says which task and thread object it belongs to.
    HPX_TEST_NEQ(a, b);
    HPX_TEST_EQ(text(a), std::string("task_a") + suffix);
    HPX_TEST_EQ(text(b), std::string("task_b") + suffix);

    // The same task name on the same thread object keeps its fiber, so
    // later tasks of that kind share the row instead of adding new ones.
    HPX_TEST_EQ(a, key("task_a", id));

    // Many more names make the set rehash. Earlier names must keep their
    // address and text, as Tracy may read them at any time.
    for (int i = 0; i != 10000; ++i)
    {
        (void) key(std::to_string(i).c_str(), id);
    }
    HPX_TEST_EQ(a, key("task_a", id));
    HPX_TEST_EQ(text(a), std::string("task_a") + suffix);

    // The old 64-byte buffer cut long names short. A long name keeps its
    // suffix now, so it still names one thread object.
    std::string const long_name(200, 'x');
    HPX_TEST_EQ(text(key(long_name.c_str(), id)), long_name + suffix);

    return hpx::util::report_errors();
}
