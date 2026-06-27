//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) COPYRIGHT 2017 ARM Limited

#include <hpx/hpx_main.hpp>
#include <hpx/modules/iostream.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <ios>

using namespace hpx::iostream;

struct limit_device
{
    using char_type = char;
    using category = sink_tag;

    std::streamsize written;
    int overflow_count, limit;

    explicit limit_device(int limit = 20)
      : written(0)
      , overflow_count(0)
      , limit(limit)
    {
    }

    std::streamsize write(char_type const*, std::streamsize n)
    {
        if (written > limit)
        {
            // first return partial writes, then an error
            ++overflow_count;
            if (overflow_count > 2 || n < 2)
            {
                return static_cast<std::streamsize>(-1);
            }
            n /= 2;
        }
        written += n;
        return n;
    }
};

static void disk_full_test()
{
    // non_blocking_adapter used to handle write returning -1 correctly, usually
    // hanging (see ticket 2557). As non_blocking_adapter is used for ofstream,
    // this would happen for ordinary files when reaching quota, disk full or
    // rlimits.
    limit_device outdev;
    non_blocking_adapter<limit_device> nonblock_outdev(outdev);
    filtering_ostream out;
    out.push(nonblock_outdev);
    write(out, "teststring0123456789", 20);
    out.flush();
    write(out, "secondwrite123456789", 20);
    close(out);
}

int main(int, char*[])
{
    disk_full_test();
    return hpx::util::report_errors();
}
