//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Event layout shared between the recording ITT collector and the test that
// inspects what it recorded. The collector is a plain shared library loaded by
// the ittnotify static part, so this header must not depend on HPX.

#pragma once

#include <cstddef>
#include <cstdint>

namespace hpx_itt_test {

    enum class event_kind : std::uint32_t
    {
        id_create,
        id_destroy,
        task_begin,
        task_end,
        marker,
        metadata,
        note
    };

    struct event
    {
        event_kind kind;
        std::uint64_t d1;
        std::uint64_t d2;
        std::uint64_t value;
        std::size_t thread;
        char name[64];
    };

    // Copies all recorded events into 'out' if 'capacity' is enough, and
    // returns the total, so a result above 'capacity' means try again larger.
    using copy_events_fn = std::size_t (*)(event* out, std::size_t capacity);
    using note_fn = std::size_t (*)(char const*);
}    // namespace hpx_itt_test
