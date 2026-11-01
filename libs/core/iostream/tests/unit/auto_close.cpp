//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#include <hpx/hpx_main.hpp>
#include <hpx/modules/iostream.hpp>
#include <hpx/modules/testing.hpp>

#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

#include <cstdio>
#include <ios>
#include <memory>

using namespace std;
using namespace hpx;
using namespace hpx::iostream;
using namespace hpx::iostream::test;

class closable_source : public source
{
public:
    closable_source()
      : open_(new bool(true))
    {
    }
    std::streamsize read(char*, std::streamsize)
    {
        return 0;
    }
    void open()
    {
        *open_ = true;
    }
    void close()
    {
        *open_ = false;
    }
    bool is_open() const
    {
        return *open_;
    }

private:
    std::shared_ptr<bool> open_;
};

class closable_input_filter : public input_filter
{
public:
    closable_input_filter()
      : open_(new bool(true))
    {
    }

    template <typename Source>
    int get(Source&)
    {
        return EOF;
    }

    void open()
    {
        *open_ = true;
    }

    template <typename Source>
    void close(Source&)
    {
        *open_ = false;
    }

    bool is_open() const
    {
        return *open_;
    }

private:
    std::shared_ptr<bool> open_;
};

void auto_close_source()
{
    // Rely on auto_close to close source.
    closable_source src;
    {
        stream<closable_source> in(src);
        HPX_TEST(src.is_open());
        HPX_TEST(in.auto_close());
    }
    HPX_TEST(!src.is_open());

    // Use close() to close components.
    src.open();
    {
        stream<closable_source> in(src);
        HPX_TEST(src.is_open());
        HPX_TEST(in.auto_close());
        in.close();
        HPX_TEST(!src.is_open());
    }

    // Use close() to close components, with auto_close disabled.
    src.open();
    {
        stream<closable_source> in(src);
        HPX_TEST(src.is_open());
        in.set_auto_close(false);
        in.close();
        HPX_TEST(!src.is_open());
    }

    // Disable auto_close.
    src.open();
    {
        stream<closable_source> in(src);
        HPX_TEST(src.is_open());
        in.set_auto_close(false);
        HPX_TEST(!in.auto_close());
    }
    HPX_TEST(src.is_open());
}

void auto_close_filter()
{
    closable_source src;
    closable_input_filter flt;

    // Rely on auto_close to close components.
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        HPX_TEST(flt.is_open());
        HPX_TEST(src.is_open());
        HPX_TEST(in.auto_close());
    }
    HPX_TEST(!flt.is_open());
    HPX_TEST(!src.is_open());

    // Use reset() to close components.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        HPX_TEST(flt.is_open());
        HPX_TEST(src.is_open());
        HPX_TEST(in.auto_close());
        in.reset();
        HPX_TEST(!flt.is_open());
        HPX_TEST(!src.is_open());
    }

    // Use reset() to close components, with auto_close disabled.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        HPX_TEST(flt.is_open());
        HPX_TEST(src.is_open());
        in.set_auto_close(false);
        in.reset();
        HPX_TEST(!flt.is_open());
        HPX_TEST(!src.is_open());
    }

    // Disable auto_close.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        HPX_TEST(flt.is_open());
        HPX_TEST(src.is_open());
        in.set_auto_close(false);
        HPX_TEST(!in.auto_close());
        in.pop();
        HPX_TEST(flt.is_open());
        HPX_TEST(src.is_open());
    }
    HPX_TEST(!flt.is_open());
    HPX_TEST(src.is_open());

    // Disable auto_close; disconnect and reconnect resource.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        HPX_TEST(flt.is_open());
        HPX_TEST(src.is_open());
        in.set_auto_close(false);
        HPX_TEST(!in.auto_close());
        in.pop();
        HPX_TEST(flt.is_open());
        HPX_TEST(src.is_open());
        in.push(src);
    }
    HPX_TEST(!flt.is_open());
    HPX_TEST(!src.is_open());
}

int main(int, char*[])
{
    auto_close_source();
    auto_close_filter();
    return hpx::util::report_errors();
}
