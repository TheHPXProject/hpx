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

#include <cstddef>
#include <cstdio>
#include <cstring>
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

    // Tracy can't send a name of 64 KB or more, so a very long description
    // is cut to the cap in tracy.cpp, but the suffix stays whole.
    std::size_t const cap = 256;
    std::size_t const suffix_size = std::strlen(suffix);
    std::string const long_a = std::string(100000, 'y') + "a";
    std::string const long_b = std::string(100000, 'y') + "b";
    void const* la = key(long_a.c_str(), id);
    void const* lb = key(long_b.c_str(), id);
    std::string const text_a = text(la);
    HPX_TEST_LTE(text_a.size(), cap + suffix_size);
    HPX_TEST_EQ(text_a.compare(0, 100, long_a, 0, 100), 0);
    HPX_TEST_EQ(
        text_a.compare(text_a.size() - suffix_size, suffix_size, suffix), 0);

    // Descriptions that differ only after the cut are still different tasks,
    // and a cut name stays the same for the same description.
    HPX_TEST_NEQ(la, lb);
    HPX_TEST_NEQ(text_a, text(lb));
    HPX_TEST_EQ(la, key(long_a.c_str(), id));

    return hpx::util::report_errors();
}
