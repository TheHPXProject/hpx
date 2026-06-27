//  Copyright Vladimir Prus 2002-2004.
//  Copyright Bertolt Mildner 2004.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/program_options/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/program_options/cmdline.hpp>
#include <hpx/program_options/errors.hpp>
#include <hpx/program_options/options_description.hpp>

#include <climits>
#include <cstdarg>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hpx::program_options {

    namespace {

        template <typename Char>
        std::basic_string<Char> tolower_(std::basic_string<Char> const& str)
        {
            std::basic_string<Char> result;
            for (typename std::basic_string<Char>::size_type i = 0;
                i < str.size(); ++i)
            {
                result.append(1, static_cast<Char>(std::tolower(str[i])));
            }
            return result;
        }

    }    // unnamed namespace

    option_description::option_description() {}

    option_description::option_description(
        char const* names, value_semantic const* s)
      : m_value_semantic(s)
    {
        this->set_names(names);
    }

    option_description::option_description(
        char const* name, std::unique_ptr<value_semantic const> s)
      : m_value_semantic(s.release())
    {
        this->set_names(name);
    }

    option_description::option_description(
        char const* names, value_semantic const* s, char const* description)
      : m_description(description)
      , m_value_semantic(s)
    {
        this->set_names(names);
    }

    option_description::option_description(char const* names,
        std::unique_ptr<value_semantic const> s, char const* description)
      : m_description(description)
      , m_value_semantic(s.release())
    {
        this->set_names(names);
    }

    option_description::~option_description() {}

    option_description::match_result option_description::match(
        std::string const& option, bool approx, bool long_ignore_case,
        bool short_ignore_case) const
    {
        match_result result = no_match;
        std::string const local_option =
            long_ignore_case ? tolower_(option) : option;

        for (auto const& long_name : m_long_names)
        {
            std::string local_long_name(
                long_ignore_case ? tolower_(long_name) : long_name);

            if (!local_long_name.empty())
            {
                if (result == no_match && *local_long_name.rbegin() == '*')
                {
                    // The name ends with '*'. Any specified name with the given
                    // prefix is OK.
                    if (local_option.find(local_long_name.substr(
                            0, local_long_name.length() - 1)) == 0)
                        result = approximate_match;
                }

                if (local_long_name == local_option)
                {
                    result = full_match;
                    break;
                }
                if (approx)
                {
                    if (local_long_name.find(local_option) == 0)
                    {
                        result = approximate_match;
                    }
                }
            }
        }

        if (result != full_match)
        {
            std::string const local_short_name(
                short_ignore_case ? tolower_(m_short_name) : m_short_name);

            if (local_short_name == local_option)
            {
                result = full_match;
            }
        }

        return result;
    }

    std::string const& option_description::key(std::string const& option) const
    {
        // We make the arbitrary choice of using the first long
        // name as the key, regardless of anything else
        if (!m_long_names.empty())
        {
            std::string const& first_long_name = *m_long_names.begin();
            if (first_long_name.find('*') != std::string::npos)
            {
                // The '*' character means we're long_name
                // matches only part of the input. So, returning
                // long name will remove some of the information,
                // and we have to return the option as specified
                // in the source.
                return option;    // NOLINT(bugprone-return-const-ref-from-parameter)
            }
            return first_long_name;
        }
        else
            return m_short_name;
    }

    std::string option_description::canonical_display_name(
        int prefix_style) const
    {
        // We prefer the first long name over any others
        if (!m_long_names.empty())
        {
            if (prefix_style == command_line_style::allow_long)
                return "--" + *m_long_names.begin();
            if (prefix_style == command_line_style::allow_long_disguise)
                return "-" + *m_long_names.begin();
        }
        // sanity check: m_short_name[0] should be '-' or '/'
        if (m_short_name.length() == 2)
        {
            if (prefix_style == command_line_style::allow_slash_for_short)
                return std::string("/") + m_short_name[1];
            if (prefix_style == command_line_style::allow_dash_for_short)
                return std::string("-") + m_short_name[1];
        }
        if (!m_long_names.empty())
            return *m_long_names.begin();
        else
            return m_short_name;
    }

    std::string const& option_description::long_name() const
    {
        static std::string empty_string("");
        return m_long_names.empty() ? empty_string : *m_long_names.begin();
    }

    std::pair<std::string const*, std::size_t> option_description::long_names()
        const
    {
        // reinterpret_cast is to please msvc 10.
        return m_long_names.empty() ?
            std::pair<std::string const*, size_t>(
                reinterpret_cast<std::string const*>(0), 0) :
            std::pair<std::string const*, size_t>(
                &*m_long_names.begin(), m_long_names.size());
    }

    option_description& option_description::set_names(char const* _names)
    {
        m_long_names.clear();
        std::istringstream iss(_names);
        std::string name;

        while (std::getline(iss, name, ','))
        {
            m_long_names.push_back(name);
        }
        HPX_ASSERT(!m_long_names.empty() && "No option names were specified");

        if (m_long_names.size() > 1)
        {
            std::string const& last_name = *m_long_names.rbegin();
            if (last_name.length() == 1)
            {
                m_short_name = '-' + last_name;
                m_long_names.pop_back();

                // The following caters to the (valid) input of ",c" for some
                // character c, where the caller only wants this option to have
                // a short name.
                if (m_long_names.size() == 1 && (*m_long_names.begin()).empty())
                {
                    m_long_names.clear();
                }
            }
        }
        // We could theoretically also ensure no remaining long names
        // are empty, or that none of them have length 1
        return *this;
    }

    std::string const& option_description::description() const
    {
        return m_description;
    }

    std::shared_ptr<value_semantic const> option_description::semantic() const
    {
        return m_value_semantic;
    }

    std::string option_description::format_name() const
    {
        if (!m_short_name.empty())
        {
            return m_long_names.empty() ? m_short_name :
                                          std::string(m_short_name)
                                              .append(" [ --")
                                              .append(*m_long_names.begin())
                                              .append(" ]");
        }
        return std::string("--").append(*m_long_names.begin());
    }

    std::string option_description::format_parameter() const
    {
        if (m_value_semantic->max_tokens() != 0)
            return m_value_semantic->name();
        else
            return "";
    }

    options_description_easy_init::options_description_easy_init(
        options_description* owner)
      : owner(owner)
    {
    }

    options_description_easy_init& options_description_easy_init::operator()(
        char const* name, char const* description)
    {
        // Create untyped semantic which accepts zero tokens: i.e.
        // no value can be specified on command line.
        //
        // By creating a unique_ptr first, and passing it to the new
        // option_description constructor that accepts a unique_ptr, we avoid
        // any possibility of a memory leak if allocation fails.
        std::unique_ptr<value_semantic const> semantic =
            std::make_unique<untyped_value>(true);
        std::shared_ptr<option_description> d =
            std::make_shared<option_description>(
                name, HPX_MOVE(semantic), description);

        owner->add(HPX_MOVE(d));
        return *this;
    }

    options_description_easy_init& options_description_easy_init::operator()(
        char const* name, value_semantic const* s)
    {
        std::shared_ptr<option_description> d =
            std::make_shared<option_description>(name, s);
        owner->add(HPX_MOVE(d));
        return *this;
    }

    options_description_easy_init& options_description_easy_init::operator()(
        char const* name, value_semantic const* s, char const* description)
    {
        std::shared_ptr<option_description> d =
            std::make_shared<option_description>(name, s, description);

        owner->add(HPX_MOVE(d));
        return *this;
    }

    options_description::options_description(
        unsigned line_length, unsigned min_description_length)
      : m_line_length(line_length)
      , m_min_description_length(min_description_length)
    {
        // we require a space between the option and description parts, so add 1.
        HPX_ASSERT(m_min_description_length < m_line_length - 1);
    }

    options_description::options_description(std::string const& caption,
        unsigned line_length, unsigned min_description_length)
      : m_caption(caption)
      , m_line_length(line_length)
      , m_min_description_length(min_description_length)
    {
        // we require a space between the option and description parts, so add 1.
        HPX_ASSERT(m_min_description_length < m_line_length - 1);
    }

    void options_description::add(std::shared_ptr<option_description> desc)
    {
        m_options.push_back(HPX_MOVE(desc));
        belong_to_group.push_back(0);
    }

    options_description& options_description::add(
        options_description const& desc)
    {
        groups.push_back(std::make_shared<options_description>(desc));

        for (auto const& option : desc.m_options)
        {
            add(option);
            belong_to_group.back() = 1;
        }

        return *this;
    }

    options_description_easy_init options_description::add_options()
    {
        return options_description_easy_init(this);
    }

    option_description const& options_description::find(std::string const& name,
        bool approx, bool long_ignore_case, bool short_ignore_case) const
    {
        option_description const* d =
            find_nothrow(name, approx, long_ignore_case, short_ignore_case);
        if (!d)
            throw unknown_option();
        return *d;
    }

    std::vector<std::shared_ptr<option_description>> const&
    options_description::options() const
    {
        return m_options;
    }

    option_description const* options_description::find_nothrow(
        std::string const& name, bool approx, bool long_ignore_case,
        bool short_ignore_case) const
    {
        std::shared_ptr<option_description> found;
        bool had_full_match = false;
        std::vector<std::string> approximate_matches;
        std::vector<std::string> full_matches;

        // We use linear search because matching specified option
        // name with the declared option name need to take care about
        // case sensitivity and trailing '*' and so we can't use simple map.
        for (auto const& option : m_options)
        {
            option_description::match_result r = option->match(
                name, approx, long_ignore_case, short_ignore_case);

            if (r == option_description::no_match)
                continue;

            if (r == option_description::full_match)
            {
                full_matches.push_back(option->key(name));
                found = option;
                had_full_match = true;
            }
            else
            {
                // FIXME: the use of 'key' here might not be the best approach.
                approximate_matches.push_back(option->key(name));
                if (!had_full_match)
                    found = option;
            }
        }
        if (full_matches.size() > 1)
            throw ambiguous_option(full_matches);

        // If we have a full match, and an approximate match, ignore approximate
        // match instead of reporting error. Say, if we have options "all" and
        // "all-chroots", then "--all" on the command line should select the
        // first one, without ambiguity.
        if (full_matches.empty() && approximate_matches.size() > 1)
            throw ambiguous_option(approximate_matches);

        return found.get();
    }

    HPX_CORE_EXPORT
    std::ostream& operator<<(std::ostream& os, options_description const& desc)
    {
        desc.print(os);
        return os;
    }

    namespace {

        /* Given a string 'par', that contains no newline characters
           outputs it to 'os' with word wrapping, that is, as several
           line.

           Each output line starts with 'indent' space characters,
           following by characters from 'par'. The total length of
           line is no longer than 'line_length'.

        */
        void format_paragraph(std::ostream& os, std::string par,
            std::size_t indent, std::size_t line_length)
        {
            // Through reminder of this function, 'line_length' will
            // be the length available for characters, not including
            // indent.
            HPX_ASSERT(indent < line_length);
            line_length -= indent;

            // index of tab (if present) is used as additional indent relative
            // to first_column_width if paragraph is spanned over multiple
            // lines if tab is not on first line it is ignored
            std::string::size_type par_indent = par.find('\t');

            if (par_indent == std::string::npos)
            {
                par_indent = 0;
            }
            else
            {
                // only one tab per paragraph allowed
                if (std::count(par.begin(), par.end(), '\t') > 1)
                {
                    throw program_options::error(
                        "Only one tab per paragraph is allowed in the options "
                        "description");
                }

                // erase tab from string
                par.erase(par_indent, 1);

                // this HPX_ASSERT may fail due to user error or
                // environment conditions!
                HPX_ASSERT(par_indent < line_length);

                // ignore tab if not on first line
                if (par_indent >= line_length)
                {
                    par_indent = 0;
                }
            }

            if (par.size() < line_length)
            {
                os << par;
            }
            else
            {
                std::string::const_iterator line_begin = par.begin();
                std::string::const_iterator const par_end = par.end();

                bool first_line = true;    // of current paragraph!

                while (line_begin < par_end)    // paragraph lines
                {
                    if (!first_line)
                    {
                        // If line starts with space, but second character is
                        // not space, remove the leading space. We don't remove
                        // double spaces because those might be intentional.
                        if (*line_begin == ' ' &&
                            (line_begin + 1 < par_end &&
                                *(line_begin + 1) != ' '))
                        {
                            line_begin += 1;    // line_begin != line_end
                        }
                    }

                    // Take care to never increment the iterator past the end,
                    // since MSVC 8.0 (brokenly), assumes that doing that, even
                    // if no access happens, is a bug.
                    auto remaining = static_cast<std::size_t>(
                        std::distance(line_begin, par_end));
                    auto line_end = line_begin +
                        static_cast<std::ptrdiff_t>(
                            remaining < line_length ? remaining : line_length);

                    // prevent chopped words
                    // Is line_end between two non-space characters?
                    if (*(line_end - 1) != ' ' &&
                        (line_end < par_end && *line_end != ' '))
                    {
                        // find last ' ' in the second half of the current paragraph line
                        auto last_space = find(std::reverse_iterator(line_end),
                            std::reverse_iterator(line_begin), ' ')
                                              .base();

                        if (last_space != line_begin)
                        {
                            // is last_space within the second half ot the
                            // current line
                            if (static_cast<std::size_t>(std::distance(
                                    last_space, line_end)) < line_length / 2)
                            {
                                line_end = last_space;
                            }
                        }
                    }    // prevent chopped words

                    // write line to stream
                    copy(line_begin, line_end, std::ostream_iterator<char>(os));

                    if (first_line)
                    {
                        indent += par_indent;    //-V101

                        // there's less to work with now
                        line_length -= par_indent;
                        first_line = false;
                    }

                    // more lines to follow?
                    if (line_end != par_end)
                    {
                        os << '\n';

                        for (std::size_t pad = indent; pad > 0; --pad)
                        {
                            os.put(' ');
                        }
                    }

                    // next line starts after of this line
                    line_begin = line_end;
                }    // paragraph lines
            }
        }

        void format_description(std::ostream& os, std::string const& desc,
            std::size_t first_column_width, std::size_t line_length)
        {
            // we need to use one char less per line to work correctly if actual
            // console has longer lines
            HPX_ASSERT(line_length > 1);
            if (line_length > 1)
            {
                --line_length;
            }

            // line_length must be larger than first_column_width
            // this HPX_ASSERT may fail due to user error or environment
            // conditions!
            HPX_ASSERT(line_length > first_column_width);

            hpx::string_util::tokenizer paragraphs(desc,
                hpx::string_util::char_separator(
                    "\n", "", hpx::string_util::empty_token_policy::keep));

            auto par_iter = paragraphs.begin();
            auto const par_end = paragraphs.end();

            while (par_iter != par_end)    // paragraphs
            {
                format_paragraph(
                    os, *par_iter, first_column_width, line_length);

                ++par_iter;

                // prepare next line if any
                if (par_iter != par_end)
                {
                    os << '\n';

                    for (std::size_t pad = first_column_width; pad > 0; --pad)
                    {
                        os.put(' ');
                    }
                }
            }    // paragraphs
        }

        void format_one(std::ostream& os, option_description const& opt,
            std::size_t first_column_width, std::size_t line_length)
        {
            std::stringstream ss;
            ss << "  " << opt.format_name() << ' ' << opt.format_parameter();

            // Don't use ss.rdbuf() since g++ 2.96 is buggy on it.
            os << ss.str();

            if (!opt.description().empty())
            {
                if (ss.str().size() >= first_column_width)
                {
                    // first column is too long, lets put description in new line
                    os.put('\n');
                    for (std::size_t pad = first_column_width; pad > 0; --pad)
                    {
                        os.put(' ');
                    }
                }
                else
                {
                    for (std::size_t pad = first_column_width - ss.str().size();
                        pad > 0; --pad)
                    {
                        os.put(' ');
                    }
                }

                format_description(
                    os, opt.description(), first_column_width, line_length);
            }
        }
    }    // namespace

    std::size_t options_description::get_option_column_width() const
    {
        /* Find the maximum width of the option column */
        std::size_t width(23);
        for (std::size_t i = 0; i < m_options.size(); ++i)
        {
            option_description const& opt = *m_options[i];
            std::stringstream ss;
            ss << "  " << opt.format_name() << ' ' << opt.format_parameter();
            width = (std::max) (width, ss.str().size());
        }

        /* Get width of groups as well*/
        for (auto const& group : groups)
            width = (std::max) (width, group->get_option_column_width());

        /* this is the column were description should start, if first
           column is longer, we go to a new line */
        std::size_t const start_of_description_column =
            m_line_length - m_min_description_length;

        width = (std::min) (width, start_of_description_column - 1);

        ++width;
        return width;
    }

    void options_description::print(std::ostream& os, std::size_t width) const
    {
        if (!m_caption.empty())
            os << m_caption << ":\n";

        if (!width)
            width = get_option_column_width();

        /* The options formatting style is stolen from Subversion. */
        for (std::size_t i = 0; i < m_options.size(); ++i)
        {
            if (belong_to_group[i])
                continue;

            option_description const& opt = *m_options[i];

            format_one(os, opt, width, m_line_length);

            os << "\n";
        }

        for (auto const& group : groups)
        {
            os << "\n";
            group->print(os, width);
        }
    }
}    // namespace hpx::program_options
