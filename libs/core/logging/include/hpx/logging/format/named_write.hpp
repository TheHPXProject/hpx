// named_write.hpp

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
#include <hpx/logging/format/destinations.hpp>
#include <hpx/logging/format/formatters.hpp>

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hpx::util::logging::detail {

    HPX_CXX_CORE_EXPORT template <typename T>
    struct named
    {
        std::string name;
        T value;
    };

    HPX_CXX_CORE_EXPORT template <typename C, typename S>
    typename C::iterator find_named(C& c, S const& name)
    {
        for (auto iter = c.begin(), end = c.end(); iter != end; ++iter)
        {
            if (iter->name == name)
                return iter;
        }
        return c.end();
    }

    /**
@brief Allows you to contain multiple formatters,
and specify a %spacer between them. You have a %spacer string, and within it,
you can escape your contained formatters.

@code
#include <hpx/logging/format/named_write.hpp>
@endcode

This allows you:
- to hold multiple formatters
- each formatter is given a name, when being added
- you have a %spacer string, which contains what is to be prepended or
appended to the string (by default, prepended)
- a formatter is escaped with @c '\%' chars, like this @c "%name%"
- if you want to write the @c '\%', just double it,
like this: <tt>"this %% gets written"</tt>

Example:

@code
#define L_ HPX_LOG_USE_LOG_IF_FILTER(g_l(), g_log_filter()->is_enabled() )

g_l()->writer().add_formatter( formatter::named("[%index%] %time% (T%thread%) ")
        .add( "index", formatter::idx())
        .add( "thread", formatter::thread_id())
        .add( "time", formatter::time("$mm")) );
@endcode

Assuming you'd use the above in code
@code
int i = 1;
L_ << "this is so cool " << i++;
L_ << "this is so cool again " << i++;
@endcode

You could have an output like this:

@code
[1] 53 (T3536) this is so cool 1
[2] 54 (T3536) this is so cool again 2
@endcode

*/
    HPX_CXX_CORE_EXPORT struct named_formatters
    {
        named_formatters(named_formatters const&) = delete;
        named_formatters(named_formatters&&) = delete;
        named_formatters& operator=(named_formatters const&) = delete;
        named_formatters& operator=(named_formatters&&) = delete;

        using ptr_type = std::unique_ptr<formatter::manipulator>;

        named_formatters() = default;

        named_formatters& string(std::string const& str)
        {
            format_string = str;
            compute_write_steps();
            return *this;
        }

        void add(std::string const& name, ptr_type p)
        {
            auto iter = find_named(formatters, name);
            if (iter != formatters.end())
                iter->value = HPX_MOVE(p);
            else
                formatters.push_back(named<ptr_type>{name, HPX_MOVE(p)});
            compute_write_steps();
        }

        void configure(
            std::string const& name, std::string const& configure_str)
        {
            auto iter = find_named(formatters, name);
            if (iter != formatters.end())
                iter->value->configure(configure_str);
        }

        void operator()(std::stringstream& out, message const& msg) const
        {
            for (auto const& step : write_steps)
            {
                out << step.prefix;
                if (step.fmt)
                {
                    if (step.fmt == (formatter::manipulator*) -1)
                        out << msg;
                    else
                        (*step.fmt)(out);
                }
            }
        }

    private:
        // recomputes the write steps - note that this takes place after
        // each operation for instance, the user might have first set the
        // string and later added the formatters
        HPX_CORE_EXPORT void compute_write_steps();

    private:
        struct write_step
        {
            write_step(std::string const& prefix_, formatter::manipulator* fmt_)
              : prefix(prefix_)
              , fmt(fmt_)
            {
            }
            std::string prefix;
            // could be null - in case formatter not found by name, or it's
            // the last step
            formatter::manipulator* fmt;
        };

        std::vector<named<ptr_type>> formatters;
        std::vector<write_step> write_steps;
        std::string format_string;
    };

    /**
@brief Allows you to contain multiple destinations,
give each such destination a name.
Then, at run-time, you can specify a format string which will specify which
destinations to be called, and on what order.

This allows you:
- to hold multiple destinations
- each destination is given a name, when being added.
The name <b>must not</b> contain spaces and must not start with '+'/'-' signs
- you have a %format string, which contains what destinations to be called,
and on which order

The %format string contains destination names, separated by space.

When a message is written to this destination,
I parse the format string. When a name is encountered, if there's a destination
corresponding to this name, I will call it.

Example:

@code
g_l()->writer().add_destination(
    destination::named_destinations("cout out debug")
        .add( "cout", destination::cout())
        .add( "debug", destination::dbg_window() )
        .add( "out", destination::file("out.txt"))
     );
@endcode

In the above code, we'll write to 3 destinations, in the following order:
- first, to the console
- second, to the out.txt file
- third, to the debug window



@section If you deal with config files

As an extra feature:
- if a name starts with '-' is ignored
- if a name starts with '+', is included.

This is useful if you want to set this format string in a config file.
The good thing is that this way you can easily turn on/off
certain destinations, while seeing all the available destinations as well.

Example: \n <tt>+out_file -debug_window +console</tt> \n
In the above example, I know that the available destinations are @c out_file,
@c debug_window and @c console, but I'm not writing to @c debug_window.

*/
    HPX_CXX_CORE_EXPORT struct named_destinations
    {
        HPX_NON_COPYABLE(named_destinations);

        using ptr_type = std::unique_ptr<destination::manipulator>;

        named_destinations() = default;

        named_destinations& string(std::string const& str)
        {
            format_string = str;
            compute_write_steps();
            return *this;
        }

        void add(std::string const& name, ptr_type p)
        {
            auto iter = find_named(destinations, name);
            if (iter != destinations.end())
                iter->value = HPX_MOVE(p);
            else
                destinations.push_back(named<ptr_type>{name, HPX_MOVE(p)});
            compute_write_steps();
        }

        void configure(
            std::string const& name, std::string const& configure_str)
        {
            auto iter = find_named(destinations, name);
            if (iter != destinations.end())
                iter->value->configure(configure_str);
        }

        void operator()(message const& msg) const
        {
            for (auto const& step : write_steps)
                (*step)(msg);
        }

    private:
        // recomputes the write steps - note that this takes place after
        // each operation for instance, the user might have first set the
        // string and later added the formatters
        HPX_CORE_EXPORT void compute_write_steps();

    private:
        std::vector<named<ptr_type>> destinations;
        std::vector<destination::manipulator*> write_steps;
        std::string format_string;
    };
}    // namespace hpx::util::logging::detail

namespace hpx::util::logging::writer {

    /**
    @brief Composed of a named formatter and a named destinations.
    Thus, you can specify the formatting and destinations as strings

    @code
    #include <hpx/logging/format/named_write.hpp>
    @endcode


    Contains a very easy interface for using
    @ref hpx::util::logging::formatter::manipulator "formatters" and
    @ref hpx::util::logging::destination::manipulator "destinations":
    - at construction, specify 2 params: the %formatter string and the
      destinations string

    Setting the @ref hpx::util::logging::formatter::manipulator "formatters"
    and @ref hpx::util::logging::destination::manipulator "destinations" to
    write to is extremely simple:

    @code
    // Set the formatters (first param) and destinatins (second step) in one
    step g_l()->writer().write("%time%($hh:$mm.$ss.$mili) [%idx%] |\n",
    "cout file(out.txt) debug");

    // set the formatter(s)
    g_l()->writer().format("%time%($hh:$mm.$ss.$mili) [%idx%] |\n");

    // set the destination(s)
    g_l()->writer().destination("cout file(out.txt) debug");
    @endcode


    @section format_string_syntax The syntax of the format string

    - The format string specifies how the message is to be logged
    - Every formatter is escaped using <tt>%</tt><em>fmt</em><tt>%</tt>
      - Available formatters:
        - <tt>"%idx%"</tt> - writes the index of the message (formatter::idx)
        - <tt>"%time%"</tt> - writes the time (formatter::high_precision_time)
        - <tt>"%thread_id%"</tt> - writes the thread id (formatter::thread_id)
        - if you want to write @c "%", double it, like this: @c "%%"
    - @c "|" is used to specify the original message. What is before it,
    is prepended to the message, what is after, is appended to the message
    - If a formatter is configurable, append @em (params) to it
      - For now, only @c "%time%" is configurable. For instance,
      @c "%time%($hh:$mm.$ss.$mili)" writes time like @c "21:14.24.674"

    Example:
    @code
    "%time%($hh:$mm.$ss.$mili) [%idx%] |\n"
    @endcode

    The output can look like:

    @code
    21:03.17.243 [1] this is so cool
    21:03.17.243 [2] first error
    21:03.17.243 [3] hello, world
    @endcode


    @section dest_string_syntax The syntax of the destinations string

    - The syntax of the destination string specifies where the message is to be logged
      - Every destination is specified by name
      - Separate destinations by space (' ')
    - Available destinations
      - <tt>"cout"</tt> - writes to std::cout (destination::cout)
      - <tt>"cerr"</tt> - writes to std::cerr (destination::cerr)
      - <tt>"debug"</tt> - writes to the debug window: OutputDebugString in Windows,
      console on Linux (destination::dbg_window)
      - <tt>"file"</tt> - writes to a file (destination::file)
    - If a destination is configurable, append @em (params) to it
      - Right now, @c "file" is configurable
        - Append <tt>(</tt><em>filename</em><tt>)</tt> to them to specify the file name.
        Example: @c "file(out.txt)" will write to the out.txt file

    Examples:
    - <tt>"file(out.txt) cout"</tt> - will write to a file called out.txt and to cout
    - <tt>"cout debug"</tt> - will write to cout and debug window (see above)

    @note
    If you want to output to 2 files, don't use "file(one.txt) file(two.txt)".
    This will just configure "file" twice, ending up with writing only to "two.txt" file.

    @param format_write_ the underlying format writer

*/
    HPX_CXX_CORE_EXPORT struct named_write
    {
        HPX_CORE_EXPORT named_write();

        /** @brief sets the format string: what should be before,
            and what after the original message, separated by "|"

            Example: \n
            "[%idx%] |\n" - this writes "[%idx%] " before the message,
            and "\n" after the message

            If "|" is not present, the whole message is prepended to the message
         */
        void format(std::string const& format_str)
        {
            m_format_str = format_str;
            configure_formatter(format_str);
        };

        /** @brief sets the destinations string - where should logged messages
         * be outputted
         */
        void destination(std::string const& destination_str)
        {
            m_destination_str = destination_str;
            configure_destination(destination_str);
        }

        /** @brief Specifies the formats and destinations in one step
         */
        void write(
            std::string const& format_str, std::string const& destination_str)
        {
            format(format_str);
            destination(destination_str);
        }

        void operator()(message const& msg) const
        {
            std::stringstream out;
            m_format(out, msg);

#if defined(HPX_COMPUTE_HOST_CODE)
            message const formatted(HPX_MOVE(out));
            m_destination(formatted);
#endif
        }

        /** @brief Replaces a formatter from the named formatter.

            You can use this, for instance, when you want to share
            a formatter between multiple named writers.
         */
        template <typename Formatter>
        void set_formatter(std::string const& name, Formatter fmt)
        {
            m_format.add(
                name, detail::named_formatters::ptr_type(new Formatter(fmt)));
        }

        template <typename Formatter, typename... Args>
        void set_formatter(std::string const& name, Args&&... args)
        {
            m_format.add(name, Formatter::make(HPX_FORWARD(Args, args)...));
        }

        /** @brief Replaces a destination from the named destination.

            You can use this, for instance, when you want to share a
            destination between multiple named writers.
         */
        template <typename Destination>
        void set_destination(std::string const& name, Destination dest)
        {
            m_destination.add(name,
                detail::named_destinations::ptr_type(new Destination(dest)));
        }

        template <typename Destination, typename... Args>
        void set_destination(std::string const& name, Args&&... args)
        {
            m_destination.add(
                name, Destination::make(HPX_FORWARD(Args, args)...));
        }

    private:
        HPX_CORE_EXPORT void configure_formatter(std::string const& format);
        HPX_CORE_EXPORT void configure_destination(std::string const& format);

    private:
        detail::named_formatters m_format;
        detail::named_destinations m_destination;

        std::string m_format_str;
        std::string m_destination_str;
    };
}    // namespace hpx::util::logging::writer
