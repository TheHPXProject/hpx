//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

#include "constants.hpp"

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::test {

    template <typename Ch, typename Tr>
    bool compare_streams_in_chars(
        std::basic_istream<Ch, Tr>& first, std::basic_istream<Ch, Tr>& second)
    {
        for (std::size_t z = 0; z < data_reps; ++z)
            for (int w = 0; w < static_cast<int>(data_length()); ++w)
                if (first.eof() != second.eof() || first.get() != second.get())
                    return false;
        return true;
    }

    template <typename Ch, typename Tr>
    bool compare_streams_in_chunks(
        std::basic_istream<Ch, Tr>& first, std::basic_istream<Ch, Tr>& second)
    {
        do
        {
            Ch buf_one[chunk_size];
            Ch buf_two[chunk_size];
            first.read(buf_one, chunk_size);
            second.read(buf_two, chunk_size);
            std::streamsize amt = first.gcount();
            if (amt != static_cast<std::streamsize>(second.gcount()) ||
                std::char_traits<Ch>::compare(
                    buf_one, buf_two, static_cast<std::size_t>(amt)) != 0)
                return false;
        } while (!first.eof());
        return true;
    }

    bool compare_files(std::string const& first, std::string const& second)
    {
        using namespace std;
        ifstream one(first.c_str(), std::ios_base::in | std::ios_base::binary);
        ifstream two(second.c_str(), std::ios_base::in | std::ios_base::binary);
        return compare_streams_in_chunks(one, two);
    }

    template <typename Container, typename Ch, typename Tr>
    bool compare_container_and_stream(
        Container& cnt, std::basic_istream<Ch, Tr>& is)
    {
        typename Container::iterator first = cnt.begin();
        typename Container::iterator last = cnt.end();
        do
        {
            if ((first == last) != is.eof())
                return false;
            if (first != last && *first != is.get())
                return false;
            if (first != last)
                ++first;
        } while (first != last);
        return true;
    }

    template <typename Container>
    bool compare_container_and_file(Container& cnt, std::string const& file)
    {
        std::ifstream fstrm(
            file.c_str(), std::ios_base::in | std::ios_base::binary);
        return compare_container_and_stream(cnt, fstrm);
    }

    template <typename Ch, typename Tr>
    void write_data_in_chars(std::basic_ostream<Ch, Tr>& os)
    {
        for (std::size_t z = 0; z < data_reps; ++z)
            for (int w = 0; w < static_cast<int>(data_length()); ++w)
                os.put(detail::data((Ch*) nullptr)[w]);
        os.flush();
    }

    template <typename Ch, typename Tr>
    void write_data_in_chunks(std::basic_ostream<Ch, Tr>& os)
    {
        Ch const* buf = detail::data((Ch*) nullptr);
        for (std::size_t z = 0; z < data_reps; ++z)
            os.write(buf, data_length());
        os.flush();
    }

    bool test_seekable_in_chars(std::iostream& io)
    {
        std::size_t i;    // old 'for' scope workaround.

        // Test seeking with ios::cur
        for (i = 0; i < data_reps; ++i)
        {
            int j;
            for (j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(-chunk_size, std::ios_base::cur);
            for (j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(-chunk_size, std::ios_base::cur);
            for (j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
        }

        // Test seeking with ios::beg
        std::streamoff off = 0;
        io.seekp(0, std::ios_base::beg);
        for (i = 0; i < data_reps; ++i, off += chunk_size)
        {
            int j;
            for (j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(off, std::ios_base::beg);
            for (j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(off, std::ios_base::beg);
            for (j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
        }

        // Test seeking with ios::end
        io.seekp(0, std::ios_base::end);
        off = io.tellp();
        io.seekp(-off, std::ios_base::end);
        for (i = 0; i < data_reps; ++i, off -= chunk_size)
        {
            int j;
            for (j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(-off, std::ios_base::end);
            for (j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(-off, std::ios_base::end);
            for (j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
        }
        return true;
    }

    bool test_seekable_in_chunks(std::iostream& io)
    {
        std::size_t i;    // old 'for' scope workaround.

        // Test seeking with ios::cur
        for (i = 0; i < data_reps; ++i)
        {
            io.write(narrow_data(), chunk_size);
            io.seekp(-chunk_size, std::ios_base::cur);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
            io.seekg(-chunk_size, std::ios_base::cur);
            io.write(narrow_data(), chunk_size);
        }

        // Test seeking with ios::beg
        std::streamoff off = 0;
        io.seekp(0, std::ios_base::beg);
        for (i = 0; i < data_reps; ++i, off += chunk_size)
        {
            io.write(narrow_data(), chunk_size);
            io.seekp(off, std::ios_base::beg);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
            io.seekg(off, std::ios_base::beg);
            io.write(narrow_data(), chunk_size);
        }

        // Test seeking with ios::end
        io.seekp(0, std::ios_base::end);
        off = io.tellp();
        io.seekp(-off, std::ios_base::end);
        for (i = 0; i < data_reps; ++i, off -= chunk_size)
        {
            io.write(narrow_data(), chunk_size);
            io.seekp(-off, std::ios_base::end);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
            io.seekg(-off, std::ios_base::end);
            io.write(narrow_data(), chunk_size);
        }
        return true;
    }

    bool test_input_seekable(std::istream& io)
    {
        std::size_t i;    // old 'for' scope workaround.

        // Test seeking with ios::cur
        for (i = 0; i < data_reps; ++i)
        {
            for (int j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(-chunk_size, std::ios_base::cur);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
        }

        // Test seeking with ios::beg
        std::streamoff off = 0;
        io.seekg(0, std::ios_base::beg);
        for (i = 0; i < data_reps; ++i, off += chunk_size)
        {
            for (int j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(off, std::ios_base::beg);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
        }

        // Test seeking with ios::end
        io.seekg(0, std::ios_base::end);
        off = io.tellg();
        io.seekg(-off, std::ios_base::end);
        for (i = 0; i < data_reps; ++i, off -= chunk_size)
        {
            for (int j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(-off, std::ios_base::end);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
        }
        return true;
    }

    bool test_output_seekable(std::ostream& io)
    {
        std::size_t i;    // old 'for' scope workaround.

        // Test seeking with ios::cur
        for (i = 0; i < data_reps; ++i)
        {
            for (int j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(-chunk_size, std::ios_base::cur);
            io.write(narrow_data(), chunk_size);
        }

        // Test seeking with ios::beg
        std::streamoff off = 0;
        io.seekp(0, std::ios_base::beg);
        for (i = 0; i < data_reps; ++i, off += chunk_size)
        {
            for (int j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(off, std::ios_base::beg);
            io.write(narrow_data(), chunk_size);
        }

        // Test seeking with ios::end
        io.seekp(0, std::ios_base::end);
        off = io.tellp();
        io.seekp(-off, std::ios_base::end);
        for (i = 0; i < data_reps; ++i, off -= chunk_size)
        {
            for (int j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(-off, std::ios_base::end);
            io.write(narrow_data(), chunk_size);
        }
        return true;
    }

    bool test_dual_seekable(std::iostream& io)
    {
        std::size_t i;    // old 'for' scope workaround.

        // Test seeking with ios::cur
        for (i = 0; i < data_reps; ++i)
        {
            for (int j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(-chunk_size, std::ios_base::cur);
            for (int j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(-chunk_size, std::ios_base::cur);
            io.write(narrow_data(), chunk_size);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
        }

        // Test seeking with ios::beg
        std::streamoff off = 0;
        io.seekp(0, std::ios_base::beg);
        io.seekg(0, std::ios_base::beg);
        for (i = 0; i < data_reps; ++i, off += chunk_size)
        {
            for (int j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(off, std::ios_base::beg);
            for (int j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(off, std::ios_base::beg);
            io.write(narrow_data(), chunk_size);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
        }

        // Test seeking with ios::end
        io.seekp(0, std::ios_base::end);
        io.seekg(0, std::ios_base::end);
        off = io.tellp();
        io.seekp(-off, std::ios_base::end);
        io.seekg(-off, std::ios_base::end);
        for (i = 0; i < data_reps; ++i, off -= chunk_size)
        {
            for (int j = 0; j < chunk_size; ++j)
                io.put(narrow_data()[j]);
            io.seekp(-off, std::ios_base::end);
            for (int j = 0; j < chunk_size; ++j)
                if (io.get() != narrow_data()[j])
                    return false;
            io.seekg(-off, std::ios_base::end);
            io.write(narrow_data(), chunk_size);
            char buf[chunk_size];
            io.read(buf, chunk_size);
            if (strncmp(buf, narrow_data(), chunk_size) != 0)
                return false;
        }
        return true;
    }
}    // namespace hpx::iostream::test

#include <hpx/config/warnings_suffix.hpp>
