//  Copyright (c) 2005-2007 Andre Merzky
//  Copyright (c) 2005-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/serialization.hpp>

#include <iosfwd>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// suppress warnings about dependent classes not being exported from the dll
#if defined(HPX_MSVC_WARNING_PRAGMA)
#pragma warning(push)
#pragma warning(disable : 4091 4251 4231 4275 4660)
#endif

///////////////////////////////////////////////////////////////////////////////
//  section serialization format version
#define HPX_SECTION_VERSION 0x10

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT section
    {
    public:
        using entry_changed_func =
            hpx::function<void(std::string const&, std::string const&)>;
        using entry_type = std::pair<std::string, entry_changed_func>;
        using entry_map = std::map<std::string, entry_type>;
        using section_map = std::map<std::string, section>;

    private:
        section* this_() noexcept
        {
            return this;
        }

        using mutex_type = util::spinlock;

        section* root_;
        entry_map entries_;
        section_map sections_;
        std::string name_;
        std::string parent_name_;

        mutable mutex_type mtx_;

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void save(Archive& ar, unsigned int version) const;

        template <typename Archive>
        void load(Archive& ar, unsigned int version);

        HPX_SERIALIZATION_SPLIT_MEMBER()

    protected:
        void line_msg(std::string msg, std::string const& file, int lnum = 0,
            std::string const& line = "") const;

        section& clone_from(section const& rhs, section* root = nullptr);

    private:
        void add_section(std::unique_lock<mutex_type>& l,
            std::string const& sec_name, section& sec, section* root = nullptr);
        bool has_section(
            std::unique_lock<mutex_type>& l, std::string const& sec_name) const;

        section* get_section(
            std::unique_lock<mutex_type>& l, std::string const& sec_name);
        section const* get_section(
            std::unique_lock<mutex_type>& l, std::string const& sec_name) const;

        ///////////////////////////////////////////////////////////////////////////
        section* add_section_if_new(
            std::unique_lock<mutex_type>& l, std::string const& sec_name);

        void add_entry(std::unique_lock<mutex_type>& l,
            std::string const& fullkey, std::string const& key,
            std::string val);
        void add_entry(std::unique_lock<mutex_type>& l,
            std::string const& fullkey, std::string const& key,
            entry_type const& val);

        bool has_entry(
            std::unique_lock<mutex_type>& l, std::string const& key) const;
        std::string get_entry(
            std::unique_lock<mutex_type>& l, std::string const& key) const;
        std::string get_entry(std::unique_lock<mutex_type>& l,
            std::string const& key, std::string const& default_val) const;

        void add_notification_callback(std::unique_lock<mutex_type>& l,
            std::string const& key, entry_changed_func const& callback);

    public:
        section() noexcept;
        explicit section(std::string const& filename, section* root = nullptr);
        section(section const& in);
        ~section() = default;

        section& operator=(section const& rhs);

        void parse(std::string const& sourcename,
            std::vector<std::string> const& lines, bool verify_existing = true,
            bool weed_out_comments = true, bool replace_existing = true);

        void parse(std::string const& sourcename, std::string const& line,
            bool verify_existing = true, bool weed_out_comments = true,
            bool replace_existing = true)
        {
            std::vector<std::string> lines;
            lines.push_back(line);
            parse(sourcename, lines, verify_existing, weed_out_comments,
                replace_existing);
        }

        void read(std::string const& filename);
        void merge(std::string const& filename);
        void merge(section& second);
        void dump(int ind = 0) const;
        void dump(int ind, std::ostream& strm) const;

        void add_section(
            std::string const& sec_name, section& sec, section* root = nullptr)
        {
            std::unique_lock<mutex_type> l(mtx_);
            add_section(l, sec_name, sec, root);
        }

        section* add_section_if_new(std::string const& sec_name)
        {
            std::unique_lock<mutex_type> l(mtx_);
            return add_section_if_new(l, sec_name);
        }

        bool has_section(std::string const& sec_name) const
        {
            std::unique_lock<mutex_type> l(mtx_);
            return has_section(l, sec_name);
        }

        section* get_section(std::string const& sec_name)
        {
            std::unique_lock<mutex_type> l(mtx_);
            return get_section(l, sec_name);
        }

        section const* get_section(std::string const& sec_name) const
        {
            std::unique_lock<mutex_type> l(mtx_);
            return get_section(l, sec_name);
        }

        section_map& get_sections() noexcept
        {
            return sections_;
        }
        section_map const& get_sections() const noexcept
        {
            return sections_;
        }

        void add_entry(std::string const& key, entry_type const& val)
        {
            std::unique_lock<mutex_type> l(mtx_);
            add_entry(l, key, key, val);
        }

        void add_entry(std::string const& key, std::string const& val)
        {
            std::unique_lock<mutex_type> l(mtx_);
            add_entry(l, key, key, val);
        }

        bool has_entry(std::string const& key) const
        {
            std::unique_lock<mutex_type> l(mtx_);
            return has_entry(l, key);
        }

        std::string get_entry(std::string const& key) const
        {
            std::unique_lock<mutex_type> l(mtx_);
            return get_entry(l, key);
        }

        std::string get_entry(
            std::string const& key, std::string const& dflt) const
        {
            std::unique_lock<mutex_type> l(mtx_);
            return get_entry(l, key, dflt);
        }

        template <typename T>
        std::string get_entry(std::string const& key, T dflt) const
        {
            std::unique_lock<mutex_type> l(mtx_);
            return get_entry(l, key, hpx::util::to_string(dflt));
        }

        void add_notification_callback(
            std::string const& key, entry_changed_func const& callback)
        {
            std::unique_lock<mutex_type> l(mtx_);
            add_notification_callback(l, key, callback);
        }

        entry_map const& get_entries() const noexcept
        {
            return entries_;
        }

    private:
        std::string expand(
            std::unique_lock<mutex_type>& l, std::string value) const;

        void expand(std::unique_lock<mutex_type>& l, std::string&,
            std::string::size_type) const;
        std::string::size_type expand_bracket(std::unique_lock<mutex_type>& l,
            std::string&, std::string::size_type) const;
        void expand_brace(std::unique_lock<mutex_type>& l, std::string&,
            std::string::size_type) const;

        std::string expand_only(std::unique_lock<mutex_type>& l,
            std::string value, std::string const& expand_this) const;

        void expand_only(std::unique_lock<mutex_type>& l, std::string&,
            std::string::size_type, std::string const& expand_this) const;
        std::string::size_type expand_bracket_only(
            std::unique_lock<mutex_type>& l, std::string&,
            std::string::size_type, std::string const& expand_this) const;
        void expand_brace_only(std::unique_lock<mutex_type>& l, std::string&,
            std::string::size_type, std::string const& expand_this) const;

    public:
        /// Expands the references to other entries ($[key] or
        /// $[key:default]) and to environment variables (${name} or
        /// ${name:default}) in \p str. Written as $${ or $$[, a '$' followed by
        /// '{' or '[' is a literal one: "$${HOME}" expands to "${HOME}". Any
        /// other "$$" is left as it is.
        std::string expand(std::string const& str) const
        {
            std::unique_lock<mutex_type> l(mtx_);
            return expand(l, str);
        }

        /// Expands \p str in place as expand(str) does, the characters after
        /// position \p len only (all for std::string::npos).
        void expand(std::string& str, std::string::size_type len) const;

        void set_root(section* r, bool recursive = false)
        {
            root_ = r;
            if (recursive)
            {
                auto const send = sections_.end();
                for (auto si = sections_.begin(); si != send; ++si)
                    si->second.set_root(r, true);
            }
        }
        section* get_root() const noexcept
        {
            return root_;
        }
        std::string get_name() const
        {
            return name_;
        }
        std::string get_parent_name() const
        {
            return parent_name_;
        }
        std::string get_full_name() const
        {
            if (!parent_name_.empty())
                return parent_name_ + "." + name_;
            return name_;
        }

        void set_name(std::string const& name)
        {
            name_ = name;
        }
    };
}    // namespace hpx::util
