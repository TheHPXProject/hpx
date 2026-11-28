//  Copyright (c) 2005-2007 Andre Merzky
//  Copyright (c) 2005-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// System Header Files
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <mutex>
#include <regex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/assert.hpp>
#include <hpx/ini/ini.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/thread_support.hpp>

#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#elif !defined(HPX_WINDOWS)
extern char** environ;
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////////
    // example ini line: line # comment
    inline constexpr char const pattern_comment[] = "^([^#]*)(#.*)$";
    ///////////////////////////////////////////////////////////////////////////////

    namespace detail {

        ///////////////////////////////////////////////////////////////////////////
        inline std::string trim_whitespace(std::string const& s)
        {
            typedef std::string::size_type size_type;

            size_type const first = s.find_first_not_of(" \t\r\n");
            if (std::string::npos == first)
                return (std::string());

            size_type const last = s.find_last_not_of(" \t\r\n");
            return s.substr(first, last - first + 1);
        }

        // MSVC: using std::string::replace triggers ASAN reports
        std::string replace_substr(std::string const& str,
            std::size_t start_pos, std::size_t oldlen, char const* newstr)
        {
            std::string result(str.substr(0, start_pos));
            result += newstr;
            result += str.substr(start_pos + oldlen);
            return result;
        }

        std::string replace_substr(std::string const& str,
            std::size_t start_pos, std::size_t oldlen,
            std::string const& newstr)
        {
            return replace_substr(str, start_pos, oldlen, newstr.c_str());
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////////
    section::section() noexcept
      : root_(this_())
    {
    }

    section::section(std::string const& filename, section* root)
      : root_(nullptr != root ? root : this_())
      , name_(filename)
    {
        read(filename);
    }

    section::section(section const& in)
      : root_(this_())
      , name_(in.get_name())
      , parent_name_(in.get_parent_name())
    {
        entry_map const& e = in.get_entries();
        auto const end = e.end();
        for (auto i = e.begin(); i != end; ++i)
            add_entry(i->first, i->second);

        section_map s = in.get_sections();
        auto const send = s.end();
        for (auto si = s.begin(); si != send; ++si)
            add_section(si->first, si->second, get_root());
    }

    section& section::operator=(section const& rhs)
    {
        if (this != &rhs)
        {
            std::unique_lock<mutex_type> l(mtx_);

            root_ = this;
            parent_name_ = rhs.get_parent_name();
            name_ = rhs.get_name();

            entry_map const& e = rhs.get_entries();
            auto const end = e.end();
            for (auto i = e.begin(); i != end; ++i)
                add_entry(l, i->first, i->first, i->second);

            section_map s = rhs.get_sections();
            auto const send = s.end();
            for (auto si = s.begin(); si != send; ++si)
                add_section(l, si->first, si->second, get_root());
        }
        return *this;
    }

    section& section::clone_from(section const& rhs, section* root)
    {
        if (this != &rhs)
        {
            std::unique_lock<mutex_type> l(mtx_);

            root_ = root ? root : this;
            parent_name_ = rhs.get_parent_name();
            name_ = rhs.get_name();

            entry_map const& e = rhs.get_entries();
            auto const end = e.end();
            for (auto i = e.begin(); i != end; ++i)
                add_entry(l, i->first, i->first, i->second);

            section_map s = rhs.get_sections();
            auto const send = s.end();
            for (auto si = s.begin(); si != send; ++si)
                add_section(l, si->first, si->second, get_root());
        }
        return *this;
    }

    void section::read(std::string const& filename)
    {
#if defined(__AIX__) && defined(__GNUC__)
        // NEVER ask why... seems to be some weird stdlib initialization problem
        // If you don't call getline() here the while(getline...) loop below will
        // crash with a bad_cast exception. Stupid AIX...
        std::string l1;
        std::ifstream i1;
        i1.open(filename.c_str(), std::ios::in);
        std::getline(i1, l1);
        i1.close();
#endif

        // build ini - open file and parse each line
        std::ifstream input(filename.c_str(), std::ios::in);
        if (!input.is_open())
            line_msg("Cannot open file: ", filename);

        // read file
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(input, line))
            lines.push_back(line);

        // parse file
        parse(filename, lines, false);
    }

    bool force_entry(std::string& str)
    {
        std::string::size_type const p = str.find_last_of('!');
        if (p != std::string::npos &&
            str.find_first_not_of(" \t", p + 1) == std::string::npos)
        {
            str = str.substr(0, p);    // remove forcing modifier ('!')
            return true;
        }
        return false;
    }

    // parse file
    void section::parse(std::string const& sourcename,
        std::vector<std::string> const& lines, bool verify_existing,
        bool weed_out_comments, bool replace_existing)
    {
        int linenum = 0;
        section* current = this;

        std::regex regex_comment(pattern_comment, std::regex_constants::icase);

        auto const end = lines.end();
        for (auto it = lines.begin(); it != end; ++it)
        {
            ++linenum;

            // remove trailing new lines and white spaces
            std::string line(detail::trim_whitespace(*it));

            // skip if empty line
            if (line.empty())
                continue;

            // weed out comments
            if (weed_out_comments)
            {
                std::smatch what_comment;
                if (std::regex_match(line, what_comment, regex_comment))
                {
                    HPX_ASSERT(3 == what_comment.size());

                    line = detail::trim_whitespace(what_comment[1]);
                    if (line.empty())
                        continue;
                }
            }
            // no comments anymore: line is either section, key=val, or
            // garbage/empty

            // Check if we have a section. Example: [sec.ssec]
            if (line.front() == '[' && line.back() == ']')
            {
                current = this;    // start adding sections at the root

                // got the section name. It might be hierarchical, so split it
                // up, and for each elem, check if we have it. If not, create
                // it, and add
                std::string sec_name(line.substr(1, line.size() - 2));
                std::string::size_type pos = 0;
                for (std::string::size_type pos1 = sec_name.find_first_of('.');
                    std::string::npos != pos1;
                    pos1 = sec_name.find_first_of('.', pos = pos1 + 1))
                {
                    current = current->add_section_if_new(
                        sec_name.substr(pos, pos1 - pos));
                }

                current = current->add_section_if_new(sec_name.substr(pos));
                continue;
            }

            // Check if we have a key=val entry...
            std::string::size_type assign_pos = line.find('=');
            if (assign_pos != std::string::npos)
            {
                std::string sec_key =
                    detail::trim_whitespace(line.substr(0, assign_pos));

                section* s = current;    // save the section we're in
                current = this;          // start adding sections at the root

                std::string::size_type pos = 0;

                // Check if we have a qualified key name
                // Example: hpx.commandline.allow_unknown
                for (std::string::size_type dot_pos =
                         sec_key.find_first_of('.');
                    std::string::npos != dot_pos;
                    dot_pos = sec_key.find_first_of('.', pos = dot_pos + 1))
                {
                    current = current->add_section_if_new(
                        sec_key.substr(pos, dot_pos - pos));
                }

                // if we don't have section qualifiers, restore current...
                if (current == this)
                {
                    current = s;
                }

                std::string key = sec_key.substr(pos);

                // add key/val to this section
                std::unique_lock<mutex_type> l(current->mtx_);

                if (!force_entry(key) && verify_existing &&
                    !current->has_entry(l, key))
                {
                    line_msg("Attempt to initialize unknown entry: ",
                        sourcename, linenum, line);
                }

                if (replace_existing || !current->has_entry(l, key))
                {
                    std::string value = detail::trim_whitespace(line.substr(
                        assign_pos + 1, line.size() - assign_pos - 1));
                    current->add_entry(l, key, key, value);
                }

                // restore the old section
                current = s;
            }
            else
            {
                // Hmm, is not a section, is not an entry, is not empty - must be
                // an error!
                line_msg("Cannot parse line at: ", sourcename, linenum, line);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    void section::add_section(std::unique_lock<mutex_type>& /* l */,
        std::string const& sec_name, section& sec, section* root)
    {
        // setting name and root
        sec.name_ = sec_name;
        sec.parent_name_ = get_full_name();

        section& newsec = sections_[sec_name];
        newsec.clone_from(sec, (nullptr != root) ? root : get_root());
    }

    ///////////////////////////////////////////////////////////////////////////
    section* section::add_section_if_new(
        std::unique_lock<mutex_type>& l, std::string const& sec_name)
    {
        // do we know this one?
        if (!has_section(l, sec_name))
        {
            // no - add it!
            section sec;
            add_section(l, sec_name, sec, get_root());
        }

        return get_section(l, sec_name);
    }

    bool section::has_section(
        std::unique_lock<mutex_type>& l, std::string const& sec_name) const
    {
        std::string::size_type const i = sec_name.find('.');
        if (i != std::string::npos)
        {
            std::string const cor_sec_name = sec_name.substr(0, i);

            auto const it = sections_.find(cor_sec_name);
            if (it != sections_.end())
            {
                std::string const sub_sec_name = sec_name.substr(i + 1);
                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                return (*it).second.has_section(sub_sec_name);
            }
            return false;
        }
        return sections_.find(sec_name) != sections_.end();
    }

    section* section::get_section(
        std::unique_lock<mutex_type>& l, std::string const& sec_name)
    {
        std::string::size_type const i = sec_name.find('.');
        if (i != std::string::npos)
        {
            std::string const cor_sec_name = sec_name.substr(0, i);
            auto const it = sections_.find(cor_sec_name);
            if (it != sections_.end())
            {
                std::string const sub_sec_name = sec_name.substr(i + 1);
                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                return (*it).second.get_section(sub_sec_name);
            }

            std::string name(get_name());
            if (name.empty())
                name = "<root>";

            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "section::get_section", "No such section ({}) in section: {}",
                sec_name, name);
        }

        auto const it = sections_.find(sec_name);
        if (it != sections_.end())
            return &((*it).second);

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "section::get_section",
            "No such section ({}) in section: {}", sec_name, get_name());
    }

    section const* section::get_section(
        std::unique_lock<mutex_type>& l, std::string const& sec_name) const
    {
        std::string::size_type const i = sec_name.find('.');
        if (i != std::string::npos)
        {
            std::string const cor_sec_name = sec_name.substr(0, i);
            auto const it = sections_.find(cor_sec_name);
            if (it != sections_.end())
            {
                std::string const sub_sec_name = sec_name.substr(i + 1);
                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                return (*it).second.get_section(sub_sec_name);
            }

            std::string name(get_name());
            if (name.empty())
                name = "<root>";

            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "section::get_section", "No such section ({}) in section: {}",
                sec_name, name);
        }

        auto const it = sections_.find(sec_name);
        if (it != sections_.end())
            return &((*it).second);

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "section::get_section",
            "No such section ({}) in section: {}", sec_name, get_name());
    }

    void section::add_entry(std::unique_lock<mutex_type>& l,
        std::string const& fullkey, std::string const& key, std::string val)
    {
        // first expand the full property name in the value (avoids infinite recursion)
        expand_only(l, val, static_cast<std::string::size_type>(-1),
            get_full_name() + "." + key);

        std::string::size_type const i = key.find_last_of('.');
        if (i != std::string::npos)
        {
            section* current = root_;

            // make sure all sections in key exist
            std::string const sec_name = key.substr(0, i);

            std::string::size_type pos = 0;
            for (std::string::size_type pos1 = sec_name.find_first_of('.');
                std::string::npos != pos1;
                pos1 = sec_name.find_first_of('.', pos = pos1 + 1))
            {
                current = current->add_section_if_new(
                    l, sec_name.substr(pos, pos1 - pos));
            }

            current = current->add_section_if_new(l, sec_name.substr(pos));

            // now add this entry to the section
            current->add_entry(l, fullkey, key.substr(i + 1), val);
        }
        else
        {
            auto const it = entries_.find(key);
            if (it != entries_.end())
            {
                auto& e = it->second;
                e.first = HPX_MOVE(val);
                if (!e.second.empty())
                {
                    std::string const value = e.first;
                    entry_changed_func const f = e.second;

                    hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    f(fullkey, value);
                }
            }
            else
            {
                // just add this entry to the section
                entries_[key] = entry_type(val, entry_changed_func());
            }
        }
    }

    void section::add_entry(std::unique_lock<mutex_type>& l,
        std::string const& fullkey, std::string const& key,
        entry_type const& val)
    {
        std::string::size_type const i = key.find_last_of('.');
        if (i != std::string::npos)
        {
            section* current = root_;

            // make sure all sections in key exist
            std::string const sec_name = key.substr(0, i);

            std::string::size_type pos = 0;
            for (std::string::size_type pos1 = sec_name.find_first_of('.');
                std::string::npos != pos1;
                pos1 = sec_name.find_first_of('.', pos = pos1 + 1))
            {
                current = current->add_section_if_new(
                    l, sec_name.substr(pos, pos1 - pos));
            }

            current = current->add_section_if_new(l, sec_name.substr(pos));

            // now add this entry to the section
            current->add_entry(l, fullkey, key.substr(i + 1), val);
        }
        else
        {
            auto const it = entries_.find(key);
            if (it != entries_.end())
            {
                auto& second = it->second;
                second = val;
                if (!second.second.empty())
                {
                    std::string const value = second.first;
                    entry_changed_func const f = second.second;

                    hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    f(fullkey, value);
                }
            }
            else
            {
                // just add this entry to the section
                std::pair<entry_map::iterator, bool> const p =
                    entries_.emplace(key, val);
                HPX_ASSERT(p.second);

                auto const& second = p.first->second;
                if (!second.second.empty())
                {
                    std::string const k = p.first->first;
                    std::string const value = second.first;
                    entry_changed_func const f = second.second;

                    hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    f(k, value);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    template <typename F1, typename F2>
    class compose_callback_impl
    {
    public:
        template <typename A1, typename A2>
        compose_callback_impl(A1&& f1, A2&& f2)
          : f1_(HPX_FORWARD(A1, f1))
          , f2_(HPX_FORWARD(A2, f2))
        {
        }

        void operator()(std::string const& k, std::string const& v) const
        {
            f1_(k, v);
            f2_(k, v);
        }

    private:
        F1 f1_;
        F2 f2_;
    };

    template <typename F1, typename F2>
    static HPX_FORCEINLINE
        hpx::function<void(std::string const&, std::string const&)>
        compose_callback(F1&& f1, F2&& f2)
    {
        if (!f1)
            return HPX_FORWARD(F2, f2);
        else if (!f2)
            return HPX_FORWARD(F1, f1);

        // otherwise create a combined callback
        using result_type =
            compose_callback_impl<std::decay_t<F1>, std::decay_t<F2>>;
        return result_type(HPX_FORWARD(F1, f1), HPX_FORWARD(F2, f2));
    }

    void section::add_notification_callback(std::unique_lock<mutex_type>& l,
        std::string const& key, entry_changed_func const& callback)
    {
        std::string::size_type const i = key.find_last_of('.');
        if (i != std::string::npos)
        {
            section* current = root_;

            // make sure all sections in key exist
            std::string const sec_name = key.substr(0, i);

            std::string::size_type pos = 0;
            for (std::string::size_type pos1 = sec_name.find_first_of('.');
                std::string::npos != pos1;
                pos1 = sec_name.find_first_of('.', pos = pos1 + 1))
            {
                current = current->add_section_if_new(
                    l, sec_name.substr(pos, pos1 - pos));
            }

            current = current->add_section_if_new(l, sec_name.substr(pos));

            // now add this entry to the section
            current->add_notification_callback(l, key.substr(i + 1), callback);
        }
        else
        {
            // just add this entry to the section
            auto const it = entries_.find(key);
            if (it != entries_.end())
            {
                it->second.second =
                    compose_callback(callback, it->second.second);
            }
            else
            {
                entries_[key] = entry_type("", callback);
            }
        }
    }

    bool section::has_entry(
        std::unique_lock<mutex_type>& l, std::string const& key) const
    {
        std::string::size_type const i = key.find('.');
        if (i != std::string::npos)
        {
            std::string const sub_sec = key.substr(0, i);
            if (has_section(l, sub_sec))
            {
                std::string const sub_key = key.substr(i + 1, key.size() - i);
                auto const cit = sections_.find(sub_sec);
                HPX_ASSERT(cit != sections_.end());
                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                return (*cit).second.has_entry(sub_key);    //-V783
            }
            return false;
        }
        return entries_.find(key) != entries_.end();
    }

    std::string section::get_entry(
        std::unique_lock<mutex_type>& l, std::string const& key) const
    {
        if (std::string::size_type const i = key.find('.');
            i != std::string::npos)
        {
            std::string const sub_sec = key.substr(0, i);
            if (has_section(l, sub_sec))
            {
                std::string const sub_key = key.substr(i + 1, key.size() - i);
                auto const cit = sections_.find(sub_sec);
                HPX_ASSERT(cit != sections_.end());
                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                return (*cit).second.get_entry(sub_key);    //-V783
            }

            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "section::get_entry",
                "No such key ({}) in section: {}", key, get_name());
        }

        if (entries_.find(key) != entries_.end())
        {
            auto const cit = entries_.find(key);
            HPX_ASSERT(cit != entries_.end());
            return expand(l, (*cit).second.first);    //-V783
        }

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "section::get_entry",
            "No such section ({}) in section: {}", key, get_name());
    }

    std::string section::get_entry(std::unique_lock<mutex_type>& l,
        std::string const& key, std::string const& default_val) const
    {
        typedef std::vector<std::string> string_vector;

        string_vector split_key;
        hpx::string_util::split(
            split_key, key, hpx::string_util::is_any_of("."));

        std::string const sk = split_key.back();
        split_key.pop_back();

        section const* cur_section = this;
        for (auto const& iter : split_key)
        {
            auto next = cur_section->sections_.find(iter);
            if (cur_section->sections_.end() == next)
                return expand(l, default_val);
            cur_section = &next->second;
        }

        auto const entry = cur_section->entries_.find(sk);
        if (cur_section->entries_.end() == entry)
            return expand(l, default_val);

        return expand(l, entry->second.first);
    }

    inline void indent(int ind, std::ostream& strm)
    {
        for (int i = 0; i < ind; ++i)
            strm << "  ";
    }

    void section::dump(int ind) const
    {
        return dump(ind, std::cout);
    }

    void section::dump(int ind, std::ostream& strm) const
    {
        std::unique_lock<mutex_type> l(mtx_);

        bool header = false;
        if (0 == ind)
            header = true;

        ++ind;
        if (header)
        {
            if (get_root() == this)
            {
                strm << "============================\n";
            }
            else
            {
                strm << "============================[\n"
                     << get_name() << "\n"
                     << "]\n";
            }
        }

        auto const eend = entries_.end();
        for (auto i = entries_.begin(); i != eend; ++i)
        {
            indent(ind, strm);

            auto& second = i->second;
            std::string const expansion = expand(l, second.first);

            // Check if the expanded entry is different from the actual entry.
            if (expansion != second.first)
            {
                // If the expansion is different from the real entry, then print
                // it out.
                strm << "'" << i->first << "' : '" << second.first << "' -> '"
                     << expansion << "'\n";
            }
            else
            {
                strm << "'" << i->first << "' : '" << second.first << "'\n";
            }
        }

        auto const send = sections_.end();
        for (auto i = sections_.begin(); i != send; ++i)
        {
            indent(ind, strm);
            strm << "[" << i->first << "]\n";
            (*i).second.dump(ind, strm);
        }

        if (header)
            strm << "============================\n";

        strm << std::flush;
    }

    void section::merge(std::string const& filename)
    {
        section tmp(filename, root_);
        merge(tmp);
    }

    void section::merge(section& second)
    {
        std::unique_lock<mutex_type> l(mtx_);

        // merge entries: keep own entries, and add other entries
        entry_map const& s_entries = second.get_entries();
        auto const end = s_entries.end();
        for (auto i = s_entries.begin(); i != end; ++i)
            entries_[i->first] = i->second;

        // merge subsection known in first section
        auto const send = sections_.end();
        for (auto i = sections_.begin(); i != send; ++i)
        {
            // is there something to merge with?
            if (second.has_section(l, i->first))
                i->second.merge(second.sections_[i->first]);
        }

        // merge subsection known in second section
        section_map s = second.get_sections();
        auto const secend = s.end();
        for (auto i = s.begin(); i != secend; ++i)
        {
            // if THIS knows the section, we already merged it above
            if (!has_section(l, i->first))
            {
                // it is not known here, so we can't merge, but have to add it.
                add_section(l, i->first, i->second, get_root());
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////
    void section::line_msg(std::string msg, std::string const& file, int lnum,
        std::string const& line) const
    {
        msg += " " + file;
        if (lnum > 0)
            msg += ": line " + std::to_string(lnum);
        if (!line.empty())
            msg += " (offending entry: " + line + ")";

        HPX_THROW_EXCEPTION(hpx::error::no_success, "section::line_msg", msg);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // find the matching closing brace starting from 'begin', escaped braces will
    // be un-escaped
    inline std::string::size_type find_next(char const* ch, std::string& value,
        std::string::size_type begin = static_cast<std::string::size_type>(-1))
    {
        std::string::size_type end = value.find_first_of(ch, begin + 1);
        while (end != std::string::npos)
        {
            if (end != 0 && value[end - 1] != '\\')
                break;
            value = detail::replace_substr(value, end - 1, 2, ch);
            end = value.find_first_of(ch, end);
        }
        return end;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // A '$' directly followed by '{' or '[' starts an expansion; written as "$${"
    // or "$$[" it stands for a literal "${" or "$[" instead. Any other "$$" has
    // no special meaning. While expanding, an escape is skipped and left as it
    // is; the doubled '$' is removed only when the expansion is complete (see
    // the string returning expand below), so that a value inserted from a
    // referenced entry is never expanded a second time.
    namespace {

        // whether value[p] starts an escape ("$${" or "$$[")
        bool is_escape(std::string const& value, std::string::size_type p)
        {
            return p + 2 < value.size() && value[p] == '$' &&
                value[p + 1] == '$' &&
                (value[p + 2] == '{' || value[p + 2] == '[');
        }

        // the text with every '$' that is followed by '{' or '[' doubled, so
        // that the expansion leaves it as it is
        std::string escape_expansions(std::string const& text)
        {
            std::string result;
            result.reserve(text.size());
            for (std::string::size_type i = 0; i != text.size(); ++i)
            {
                if (text[i] == '$' && i + 1 < text.size() &&
                    (text[i + 1] == '{' || text[i + 1] == '['))
                {
                    result += '$';
                }
                result += text[i];
            }
            return result;
        }

        // remove the doubled '$' of every escape after position begin
        void unescape_expansions(
            std::string& value, std::string::size_type begin)
        {
            std::string::size_type p = value.find_first_of('$', begin + 1);
            while (p != std::string::npos)
            {
                if (is_escape(value, p))
                {
                    // "$${" -> "${": continue after the brace
                    value.erase(p, 1);
                    p = value.find_first_of('$', p + 2);
                }
                else
                {
                    p = value.find_first_of('$', p + 1);
                }
            }
        }
    }    // namespace

    void section::expand(std::unique_lock<mutex_type>& l, std::string& value,
        std::string::size_type begin) const
    {
        std::string::size_type p = value.find_first_of('$', begin + 1);
        while (p != std::string::npos && value.size() - 1 != p)
        {
            if (is_escape(value, p))
            {
                // a literal "${" or "$[", left for unescape_expansions
                p = value.find_first_of('$', p + 2);
                continue;
            }
            if ('[' == value[p + 1])
                p = expand_bracket(l, value, p);
            else if ('{' == value[p + 1])
                expand_brace(l, value, p);
            p = value.find_first_of('$', p + 1);
        }
    }

    // Returns the position after which the expansion continues: the last
    // character of the inserted value, which is not looked at again.
    std::string::size_type section::expand_bracket(
        std::unique_lock<mutex_type>& l, std::string& value,
        std::string::size_type begin) const
    {
        // expand all keys embedded inside this key
        expand(l, value, begin);

        // now expand the key itself
        std::string::size_type const end = find_next("]", value, begin + 1);
        if (end != std::string::npos)
        {
            std::string to_expand = value.substr(begin + 2, end - begin - 2);
            std::string::size_type const colon = find_next(":", to_expand);

            // the entry's value is fully expanded; escaped again, so that
            // its literal "${" and "$[" survive the final unescaping
            std::string const inserted =
                escape_expansions(colon == std::string::npos ?
                        root_->get_entry(l, to_expand, std::string()) :
                        root_->get_entry(l, to_expand.substr(0, colon),
                            to_expand.substr(colon + 1)));
            value =
                detail::replace_substr(value, begin, end - begin + 1, inserted);
            return inserted.empty() ? begin : begin + inserted.size() - 1;
        }
        return begin;
    }

    void section::expand_brace(std::unique_lock<mutex_type>& l,
        std::string& value, std::string::size_type begin) const
    {
        // expand all keys embedded inside this key
        expand(l, value, begin);

        // now expand the key itself
        std::string::size_type const end = find_next("}", value, begin + 1);
        if (end != std::string::npos)
        {
            std::string to_expand = value.substr(begin + 2, end - begin - 2);
            std::string::size_type const colon = find_next(":", to_expand);
            if (colon == std::string::npos)
            {
                char const* env = std::getenv(to_expand.c_str());
                value = detail::replace_substr(
                    value, begin, end - begin + 1, nullptr != env ? env : "");
            }
            else
            {
                char* env = std::getenv(to_expand.substr(0, colon).c_str());
                value = detail::replace_substr(value, begin, end - begin + 1,
                    nullptr != env ? std::string(env) :
                                     to_expand.substr(colon + 1));
            }
        }
    }

    std::string section::expand(
        std::unique_lock<mutex_type>& l, std::string value) const
    {
        expand(l, value, static_cast<std::string::size_type>(-1));
        unescape_expansions(value, static_cast<std::string::size_type>(-1));
        return value;
    }

    void section::expand(std::string& str, std::string::size_type len) const
    {
        std::unique_lock<mutex_type> l(mtx_);
        expand(l, str, len);
        unescape_expansions(str, len);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // The expansion of add_entry: only the given key, so that a value can refer
    // to the entry's previous value; the result is stored, so its escapes stay.
    void section::expand_only(std::unique_lock<mutex_type>& l,
        std::string& value, std::string::size_type begin,
        std::string const& expand_this) const
    {
        std::string::size_type p = value.find_first_of('$', begin + 1);
        while (p != std::string::npos && value.size() - 1 != p)
        {
            if (is_escape(value, p))
            {
                p = value.find_first_of('$', p + 2);
                continue;
            }
            if ('[' == value[p + 1])
                p = expand_bracket_only(l, value, p, expand_this);
            else if ('{' == value[p + 1])
                expand_brace_only(l, value, p, expand_this);
            p = value.find_first_of('$', p + 1);
        }
    }

    // Returns the position after which the expansion continues (see
    // expand_bracket).
    std::string::size_type section::expand_bracket_only(
        std::unique_lock<mutex_type>& l, std::string& value,
        std::string::size_type begin, std::string const& expand_this) const
    {
        // expand all keys embedded inside this key
        expand_only(l, value, begin, expand_this);

        // now expand the key itself
        std::string::size_type const end = find_next("]", value, begin + 1);
        if (end != std::string::npos)
        {
            std::string to_expand = value.substr(begin + 2, end - begin - 2);
            std::string::size_type const colon = find_next(":", to_expand);
            if (colon == std::string::npos ?
                    to_expand == expand_this :
                    to_expand.substr(0, colon) == expand_this)
            {
                // stored as it is, so escaped again (see expand_bracket)
                std::string const inserted =
                    escape_expansions(colon == std::string::npos ?
                            root_->get_entry(l, to_expand, std::string()) :
                            root_->get_entry(l, to_expand.substr(0, colon),
                                to_expand.substr(colon + 1)));
                value = detail::replace_substr(
                    value, begin, end - begin + 1, inserted);
                return inserted.empty() ? begin : begin + inserted.size() - 1;
            }
        }
        return begin;
    }

    void section::expand_brace_only(std::unique_lock<mutex_type>& l,
        std::string& value, std::string::size_type begin,
        std::string const& expand_this) const
    {
        // expand all keys embedded inside this key
        expand_only(l, value, begin, expand_this);

        // now expand the key itself
        std::string::size_type const end = find_next("}", value, begin + 1);
        if (end != std::string::npos)
        {
            std::string to_expand = value.substr(begin + 2, end - begin - 2);
            std::string::size_type const colon = find_next(":", to_expand);
            if (colon == std::string::npos)
            {
                char const* env = std::getenv(to_expand.c_str());
                value = detail::replace_substr(
                    value, begin, end - begin + 1, nullptr != env ? env : "");
            }
            else
            {
                char* env = std::getenv(to_expand.substr(0, colon).c_str());
                value = detail::replace_substr(value, begin, end - begin + 1,
                    nullptr != env ? std::string(env) :
                                     to_expand.substr(colon + 1));
            }
        }
    }

    std::string section::expand_only(std::unique_lock<mutex_type>& l,
        std::string value, std::string const& expand_this) const
    {
        expand_only(
            l, value, static_cast<std::string::size_type>(-1), expand_this);
        return value;
    }

    ///////////////////////////////////////////////////////////////////////////////
    template <typename Archive>
    void section::save(Archive& ar, unsigned int const /* version */) const
    {
        ar << name_;
        ar << parent_name_;

        std::uint64_t size = entries_.size();
        ar << size;
        for (auto const& val : entries_)
        {
            ar << val.first;
        }

        ar << sections_;
    }

    template <typename Archive>
    void section::load(Archive& ar, unsigned int const /* version */)
    {
        ar >> name_;
        ar >> parent_name_;

        std::uint64_t size;
        ar >> size;    //-V128

        entries_.clear();
        for (std::size_t i = 0; i < size; ++i)
        {
            using value_type = entry_map::value_type;

            value_type v;
            ar >> const_cast<std::string&>(v.first);
            entries_.insert(entries_.end(), HPX_MOVE(v));
        }

        ar >> sections_;

        set_root(this, true);    // make this the current root
    }

    // explicit instantiation for the correct archive types
    template HPX_CORE_EXPORT void section::save(
        serialization::output_archive&, unsigned int const version) const;

    template HPX_CORE_EXPORT void section::load(
        serialization::input_archive&, unsigned int const version);
}    // namespace hpx::util
