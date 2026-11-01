//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example demonstrates how the HPX serialization archives could be used
// to directly store/load to/from a file.

#include <hpx/init.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct file_wrapper
{
    file_wrapper(std::string const& name, std::ios_base::openmode mode)
      : stream_(name.c_str(), mode)
      , mode_(mode)
    {
        if (!stream_.is_open())
            throw std::runtime_error("Couldn't open file: " + name);

        std::fstream::pos_type fsize = stream_.tellg();
        stream_.seekg(0, std::ios::end);
        data_.resize(static_cast<std::size_t>(stream_.tellg() - fsize));
        stream_.seekg(fsize, std::ios::beg);

        if (mode & std::ios_base::in)
        {
            stream_.read(
                data_.data(), static_cast<std::streamsize>(data_.size()));
        }
    }

    ~file_wrapper()
    {
        if (mode_ & std::ios_base::out)
        {
            stream_.write(
                data_.data(), static_cast<std::streamsize>(data_.size()));
        }
    }

    void read(void* address, std::size_t count, std::size_t current) const
    {
        std::memcpy(address, &data_[current], count);
    }

    void write(void const* address, std::size_t count, std::size_t current)
    {
        std::memcpy(&data_[current], address, count);
    }

    std::size_t size() const
    {
        return data_.size();
    }

    void resize(std::size_t count)
    {
        data_.resize(data_.size() + count);
    }

private:
    std::vector<char> data_;
    std::fstream stream_;
    std::ios_base::openmode mode_;
};

namespace hpx { namespace traits {
    template <>
    struct serialization_access_data<file_wrapper>
      : default_serialization_access_data<file_wrapper>
    {
        static std::size_t size(file_wrapper const& cont)
        {
            return cont.size();
        }

        static void resize(file_wrapper& cont, std::size_t count)
        {
            cont.resize(count);
        }

        static void write(file_wrapper& cont, std::size_t count,
            std::size_t current, void const* address)
        {
            cont.write(address, count, current);
        }

        // functions related to input operations
        static void read(file_wrapper const& cont, std::size_t count,
            std::size_t current, void* address)
        {
            cont.read(address, count, current);
        }
    };
}}    // namespace hpx::traits

int hpx_main()
{
    std::size_t size = 0;
    std::vector<double> os;
    {
        file_wrapper buffer("file_serialization_test.archive",
            std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        hpx::serialization::output_archive oarchive(buffer);
        for (double c = -100.0; c < +100.0; c += 1.3)
        {
            os.push_back(c);
        }
        oarchive << os;
        size = oarchive.bytes_written();
    }

    {
        file_wrapper buffer("file_serialization_test.archive",
            std::ios_base::in | std::ios_base::binary);
        hpx::serialization::input_archive iarchive(buffer, size);
        std::vector<double> is;
        iarchive >> is;
        for (std::size_t i = 0; i < os.size(); ++i)
        {
            if (os[i] != is[i])
            {
                std::cerr << "Mismatch for element " << i << ":" << os[i]
                          << " != " << is[i] << "\n";
            }
        }
    }
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
