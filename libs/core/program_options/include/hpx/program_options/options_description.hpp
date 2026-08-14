// Copyright Vladimir Prus 2002-2004.
// Copyright Bertolt Mildner 2004.
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/program_options/config.hpp>
#include <hpx/program_options/errors.hpp>
#include <hpx/program_options/value_semantic.hpp>

#include <cstddef>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::program_options {

    /** Describes one possible command line/config file option. There are two
        kinds of properties of an option. First describe it syntactically and
        are used only to validate input. Second affect interpretation of the
        option, for example default value for it or function that should be
        called  when the value is finally known. Routines which perform parsing
        never use second kind of properties \-- they are side effect free.
        @sa options_description
    */
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT option_description
    {
    public:
        option_description();

        /** Initializes the object with the passed data.

            Note: it is generally preferred to use the std::unique_ptr
            overloads to explicitly pass ownership. We retain the raw pointer
            overloads for backward compatibility with legacy code that
            relies on implicit derived-to-base conversions which were historically
            problematic with std::auto_ptr.

            The 'name' parameter is interpreted by the following rules:
            - if there's no "," character in 'name', it specifies long name
            - otherwise, the part before "," specifies long name and the part
            after \-- short name.
        */
        option_description(char const* name, value_semantic const* s);

        /** Initializes the class with the passed data.
         */
        option_description(
            char const* name, std::unique_ptr<value_semantic const> s);

        /** Initializes the class with the passed data.
         */
        option_description(
            char const* name, value_semantic const* s, char const* description);

        /** Initializes the class with the passed data.
         */
        option_description(char const* name,
            std::unique_ptr<value_semantic const> s, char const* description);

        virtual ~option_description();

        enum match_result
        {
            no_match,
            full_match,
            approximate_match
        };

        /** Given 'option', specified in the input source,
            returns 'true' if 'option' specifies *this.
        */
        match_result match(std::string const& option, bool approx,
            bool long_ignore_case, bool short_ignore_case) const;

        /** Returns the key that should identify the option, in
            particular in the variables_map class.
            The 'option' parameter is the option spelling from the
            input source.
            If option name contains '*', returns 'option'.
            If long name was specified, it's the long name, otherwise
            it's a short name with pre-pended '-'.
        */
        std::string const& key(std::string const& option) const;

        /** Returns the canonical name for the option description to enable the user to
            recognized a matching option.
            1) For short options ('-', '/'), returns the short name prefixed.
            2) For long options ('--' / '-') returns the first long name prefixed
            3) All other cases, returns the first long name (if present) or the short
               name, un-prefixed.
        */
        std::string canonical_display_name(
            int canonical_option_style = 0) const;

        std::string const& long_name() const;

        std::pair<std::string const*, std::size_t> long_names() const;

        /// Explanation of this option
        std::string const& description() const;

        /// Semantic of option's value
        std::shared_ptr<value_semantic const> semantic() const;

        /// Returns the option name, formatted suitably for usage message.
        std::string format_name() const;

        /** Returns the parameter name and properties, formatted suitably for
            usage message. */
        std::string format_parameter() const;

    private:
        option_description& set_names(char const* name);

        /**
         * a one-character "switch" name - with its prefix,
         * so that this is either empty or has length 2 (e.g. "-c"
         */
        std::string m_short_name;

        /**
         *  one or more names by which this option may be specified
         *  on a command-line or in a config file, which are not
         *  a single-letter switch. The names here are _without_
         * any prefix.
         */
        std::vector<std::string> m_long_names;

        std::string m_description;

        // shared_ptr is needed to simplify memory management in
        // copy ctor and destructor.
        std::shared_ptr<value_semantic const> m_value_semantic;
    };

    HPX_CXX_CORE_EXPORT class options_description;

    /** Class which provides convenient creation syntax to option_description.
     */
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT options_description_easy_init
    {
    public:
        explicit options_description_easy_init(options_description* owner);

        options_description_easy_init& operator()(
            char const* name, char const* description);

        options_description_easy_init& operator()(
            char const* name, value_semantic const* s);

        options_description_easy_init& operator()(
            char const* name, value_semantic const* s, char const* description);

    private:
        options_description* owner;
    };

    /** A set of option descriptions. This provides convenient interface for
        adding new option (the add_options) method, and facilities to search
        for options by name.

        See @ref a_adding_options "here" for option adding interface discussion.
        @sa option_description
    */
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT options_description
    {
    public:
        static constexpr unsigned const m_default_line_length = 80;

        /** Creates the instance. */
        options_description(unsigned line_length = m_default_line_length,
            unsigned min_description_length = m_default_line_length / 2);
        /** Creates the instance. The 'caption' parameter gives the name of
            this 'options_description' instance. Primarily useful for output.
            The 'description_length' specifies the number of columns that
            should be reserved for the description text; if the option text
            encroaches into this, then the description will start on the next
            line.
        */
        options_description(std::string const& caption,
            unsigned line_length = m_default_line_length,
            unsigned min_description_length = m_default_line_length / 2);
        /** Adds new variable description. Throws duplicate_variable_error if
            either short or long name matches that of already present one.
        */
        void add(std::shared_ptr<option_description> desc);
        /** Adds a group of option description. This has the same
            effect as adding all option_descriptions in 'desc'
            individually, except that output operator will show
            a separate group.
            Returns *this.
        */
        options_description& add(options_description const& desc);

        /** Find the maximum width of the option column, including options
            in groups. */
        std::size_t get_option_column_width() const;

    public:
        /** Returns an object of implementation-defined type suitable for adding
            options to options_description. The returned object will
            have overloaded operator() with parameter type matching
            'option_description' constructors. Calling the operator will create
            new option_description instance and add it.
        */
        options_description_easy_init add_options();

        option_description const& find(std::string const& name, bool approx,
            bool long_ignore_case = false,
            bool short_ignore_case = false) const;

        option_description const* find_nothrow(std::string const& name,
            bool approx, bool long_ignore_case = false,
            bool short_ignore_case = false) const;

        std::vector<std::shared_ptr<option_description>> const& options() const;

        /** Produces a human readable output of 'desc', listing options,
            their descriptions and allowed parameters. Other options_description
            instances previously passed to add will be output separately. */
        friend HPX_CORE_EXPORT std::ostream& operator<<(
            std::ostream& os, options_description const& desc);

        /** Outputs 'desc' to the specified stream, calling 'f' to output each
            option_description element. */
        void print(std::ostream& os, std::size_t width = 0) const;

    private:
        using name2index_iterator = std::map<std::string, int>::const_iterator;
        using approximation_range =
            std::pair<name2index_iterator, name2index_iterator>;

        // approximation_range find_approximation(const std::string& prefix) const;

        std::string m_caption;
        std::size_t const m_line_length;
        std::size_t const m_min_description_length;

        // Data organization is chosen because:
        // - there could be two names for one option
        // - option_add_proxy needs to know the last added option
        std::vector<std::shared_ptr<option_description>> m_options;

        // Whether the option comes from one of declared groups.
        // vector<bool> is buggy there, see
        // http://support.microsoft.com/default.aspx?scid=kb;en-us;837698
        std::vector<char> belong_to_group;

        std::vector<std::shared_ptr<options_description>> groups;
    };

    /** Class thrown when duplicate option description is found. */
    HPX_CXX_CORE_EXPORT class HPX_ALWAYS_EXPORT duplicate_option_error
      : public error
    {
    public:
        explicit duplicate_option_error(std::string const& xwhat)
          : error(xwhat)
        {
        }
    };
}    // namespace hpx::program_options

#include <hpx/config/warnings_suffix.hpp>
