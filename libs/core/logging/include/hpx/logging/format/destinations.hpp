// destination_defaults.hpp

// Boost Logging library
//
// Author: John Torjo, www.torjo.com
//
// Copyright (C) 2007 John Torjo (see www.torjo.com for email)
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.
// See http://www.torjo.com/log2/ for more details

#pragma once

#include <hpx/config.hpp>
#include <hpx/logging/manipulator.hpp>

#include <ios>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>

namespace hpx::util::logging::destination {

    /**
    @brief Writes the string to console
    */
    HPX_CXX_CORE_EXPORT struct cout : manipulator
    {
        [[nodiscard]] HPX_CORE_EXPORT static std::unique_ptr<cout> make();

        HPX_CORE_EXPORT ~cout() override;

    protected:
        cout() = default;
    };

    /**
    @brief Writes the string to cerr
    */
    HPX_CXX_CORE_EXPORT struct cerr : manipulator
    {
        [[nodiscard]] HPX_CORE_EXPORT static std::unique_ptr<cerr> make();

        HPX_CORE_EXPORT ~cerr() override;

    protected:
        cerr() = default;
    };

    /**
    @brief writes to stream.

    @note:
    The stream must outlive this object! Or, clear() the stream,
    before the stream is deleted.
    */
    HPX_CXX_CORE_EXPORT struct stream : manipulator
    {
        [[nodiscard]] HPX_CORE_EXPORT static std::unique_ptr<stream> make(
            std::ostream* stream_ptr);

        HPX_CORE_EXPORT ~stream() override;

        /**
         @brief resets the stream. Further output will be written to this stream
         */
        void set_stream(std::ostream* stream_ptr)
        {
            ptr = stream_ptr;
        }

        /**
         @brief clears the stream. Further output will be ignored
         */
        void clear()
        {
            ptr = nullptr;
        }

    protected:
        explicit stream(std::ostream* stream_ptr)
          : ptr(stream_ptr)
        {
        }

    protected:
        std::ostream* ptr;
    };

    /**
    @brief Writes the string to output debug window

    For non-Windows systems, this is the console.
     */
    HPX_CXX_CORE_EXPORT struct dbg_window : manipulator
    {
        [[nodiscard]] HPX_CORE_EXPORT static std::unique_ptr<dbg_window> make();

        HPX_CORE_EXPORT ~dbg_window() override;

    protected:
        dbg_window() = default;
    };

    /**
    @brief Writes the string to a file
    */
    HPX_CXX_CORE_EXPORT struct file : manipulator
    {
        /**
         @brief Settings for constructing a file destination.
        */
        struct file_settings
        {
            file_settings()
              : flush_each_time(true)
              , initial_overwrite(false)
              , do_append(true)
              , extra_flags(std::ios_base::out)
            {
            }

            /// if true (default), flushes after each write
            bool flush_each_time : 1;
            // if true it initially overwrites the file; default = false
            bool initial_overwrite : 1;
            // if true (default), opens the file for appending
            bool do_append : 1;
            /// just in case you have some extra flags to pass, when opening the file
            std::ios_base::openmode extra_flags;
        };

        /**
        @brief constructs the file destination

        @param file_name name of the file
        @param set [optional] file settings - see the file_settings class.
        */
        [[nodiscard]] HPX_CORE_EXPORT static std::unique_ptr<file> make(
            std::string const& file_name, file_settings set = {});

        HPX_CORE_EXPORT ~file() override;

    protected:
        file(std::string file_name, file_settings set)
          : name(HPX_MOVE(file_name))
          , settings(set)
        {
        }

        std::string name;
        file_settings settings;
    };
}    // namespace hpx::util::logging::destination
