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

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#include "constants.hpp"

namespace hpx::iostream::test {

    // Represents a temp file, deleted upon destruction.
    class temp_file
    {
        static std::string get_unique_path()
        {
            auto temp_path = std::filesystem::temp_directory_path();
            std::string unique_filename =
                "file_" + std::to_string(std::random_device{}()) + ".tmp";
            return (temp_path / unique_filename).string();
        }

    public:
        // Constructs a temp file which does not initially exist.
        temp_file()
          : name_(get_unique_path())
        {
        }

        ~temp_file()
        {
            (void) std::filesystem::remove(name_.c_str());
        }

        std::string const name() const
        {
            return name_;
        }

        operator std::string const() const
        {
            return name_;
        }

    private:
        std::string name_;
    };

    struct test_file : public temp_file
    {
        test_file()
        {
            std::ios_base::openmode mode =
                std::ios_base::out | std::ios_base::binary;
            std::ofstream f(name().c_str(), mode);

            char const* buf = narrow_data();
            for (std::size_t z = 0; z < data_reps; ++z)
                f.write(buf, static_cast<long>(data_length()));
        }
    };

    struct uppercase_file : public temp_file
    {
        uppercase_file()
        {
            std::ios_base::openmode mode =
                std::ios_base::out | std::ios_base::binary;
            std::ofstream f(name().c_str(), mode);
            char const* buf = narrow_data();
            for (std::size_t z = 0; z < data_reps; ++z)
                for (std::size_t w = 0; w < data_length(); ++w)
                    f.put((char) std::toupper(buf[w]));
        }
    };

    struct lowercase_file : public temp_file
    {
        lowercase_file()
        {
            std::ios_base::openmode mode =
                std::ios_base::out | std::ios_base::binary;
            std::ofstream f(name().c_str(), mode);
            char const* buf = narrow_data();
            for (std::size_t z = 0; z < data_reps; ++z)
                for (std::size_t w = 0; w < data_length(); ++w)
                    f.put((char) std::tolower(buf[w]));
        }
    };
}    // namespace hpx::iostream::test
