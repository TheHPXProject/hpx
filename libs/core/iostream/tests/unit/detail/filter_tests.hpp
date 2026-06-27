//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/iostream.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

namespace hpx::iostream {

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_string
      : std::bool_constant<
            detail::is_iostreams_trait3<T, std::basic_string>::value>
    {
    };
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_string_v = is_string<T>::value;

    constexpr std::streamsize const default_increment = 5;

    std::streamsize rand(std::streamsize inc)
    {
        static std::mt19937 random_gen;
        static std::uniform_int_distribution<int> random_dist(
            0, static_cast<int>(inc));
        return random_dist(random_gen);
    }

    class non_blocking_source
    {
    public:
        typedef char char_type;

        struct category
          : source_tag
          , peekable_tag
        {
        };

        explicit non_blocking_source(
            std::string const& data, std::streamsize inc = default_increment)
          : data_(data)
          , inc_(inc)
          , pos_(0)
        {
        }

        std::streamsize read(char* s, std::streamsize n)
        {
            using namespace std;
            if (pos_ == static_cast<streamsize>(data_.size()))
                return -1;
            streamsize avail =
                (std::min) (n, static_cast<streamsize>(data_.size() - pos_));
            streamsize amt = (std::min) (rand(inc_), avail);
            if (amt)
                memcpy(s, data_.c_str() + pos_, static_cast<size_t>(amt));
            pos_ += amt;
            return amt;
        }

        bool putback(char c)
        {
            if (pos_ > 0)
            {
                data_[static_cast<std::string::size_type>(--pos_)] = c;
                return true;
            }
            return false;
        }

    private:
        std::string data_;
        std::streamsize inc_, pos_;
    };

    class non_blocking_sink : public sink
    {
    public:
        non_blocking_sink(
            std::string& dest, std::streamsize inc = default_increment)
          : dest_(dest)
          , inc_(inc)
        {
        }

        std::streamsize write(char const* s, std::streamsize n)
        {
            std::streamsize amt = (std::min) (rand(inc_), n);

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
            dest_.insert(dest_.end(), s, s + amt);
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif

            return amt;
        }

    private:
        std::string& dest_;
        std::streamsize inc_;
    };

    //--------------Definition of test_input_filter-------------------------------//
    template <typename Filter, typename Source1, typename Source2>
    bool test_input_filter(
        Filter filter, Source1 const& input, Source2 const& output)
    {
        if constexpr (is_string_v<Source1>)
        {
            for (int inc = default_increment; inc < default_increment * 40;
                inc += default_increment)
            {
                non_blocking_source src(input, inc);
                std::string dest;
                iostream::copy(
                    compose(filter, src), iostream::back_inserter(dest));
                if (dest != output)
                    return false;
            }
            return true;
        }
        else
        {
            std::string in;
            std::string out;
            iostream::copy(input, iostream::back_inserter(in));
            iostream::copy(output, iostream::back_inserter(out));
            return test_input_filter(filter, in, out);
        }
    }

    //--------------Definition of test_output_filter------------------------------//
    template <typename Filter, typename Source1, typename Source2>
    bool test_output_filter(
        Filter filter, Source1 const& input, Source2 const& output)
    {
        if constexpr (is_string_v<Source1>)
        {
            for (int inc = default_increment; inc < default_increment * 40;
                inc += default_increment)
            {
                array_source<char> src(
                    input.data(), input.data() + input.size());
                std::string dest;
                iostream::copy(
                    src, compose(filter, non_blocking_sink(dest, inc)));
                if (dest != output)
                    return false;
            }
            return true;
        }
        else
        {
            std::string in;
            std::string out;
            iostream::copy(input, iostream::back_inserter(in));
            iostream::copy(output, iostream::back_inserter(out));
            return test_output_filter(filter, in, out);
        }
    }

    //--------------Definition of test_filter_pair--------------------------------//
    template <typename OutputFilter, typename InputFilter, typename Source>
    bool test_filter_pair(OutputFilter out, InputFilter in, Source const& data)
    {
        if constexpr (is_string_v<Source>)
        {
            for (int inc = default_increment; inc <= default_increment * 40;
                inc += default_increment)
            {
                {
                    array_source<char> src(
                        data.data(), data.data() + data.size());
                    std::string temp;
                    std::string dest;
                    iostream::copy(
                        src, compose(out, non_blocking_sink(temp, inc)));
                    iostream::copy(compose(in, non_blocking_source(temp, inc)),
                        iostream::back_inserter(dest));
                    if (dest != data)
                        return false;
                }
                {
                    array_source<char> src(
                        data.data(), data.data() + data.size());
                    std::string temp;
                    std::string dest;
                    iostream::copy(
                        src, compose(out, non_blocking_sink(temp, inc)));

                    // truncate the file, this should not loop, it may throw
                    // std::ios_base::failure, which we swallow.
                    try
                    {
                        temp.resize(temp.size() / 2);
                        iostream::copy(
                            compose(in, non_blocking_source(temp, inc)),
                            iostream::back_inserter(dest));
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                    catch (std::ios_base::failure&)
                    {
                    }
                }

                if constexpr (std::is_convertible_v<OutputFilter, input> &&
                    std::is_convertible_v<InputFilter, output>)
                {
                    {
                        array_source<char> src(
                            data.data(), data.data() + data.size());
                        std::string temp;
                        std::string dest;
                        iostream::copy(
                            compose(out, src), non_blocking_sink(temp, inc));
                        iostream::copy(non_blocking_source(temp, inc),
                            compose(in, iostream::back_inserter(dest)));
                        if (dest != data)
                            return false;
                    }
                    {
                        array_source<char> src(
                            data.data(), data.data() + data.size());
                        std::string temp;
                        std::string dest;
                        iostream::copy(
                            compose(out, src), non_blocking_sink(temp, inc));

                        // truncate the file, this should not loop, it may throw
                        // std::ios_base::failure, which we swallow.
                        try
                        {
                            temp.resize(temp.size() / 2);
                            iostream::copy(non_blocking_source(temp, inc),
                                compose(in, iostream::back_inserter(dest)));
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                        catch (std::ios_base::failure&)
                        {
                        }
                    }
                }
            }
            return true;
        }
        else
        {
            std::string str;
            iostream::copy(data, iostream::back_inserter(str));
            return test_filter_pair(out, in, str);
        }
    }
}    // namespace hpx::iostream
