//  Copyright Vladimir Prus 2004.
//  Copyright (c) 2005-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/plugin/abstract_factory.hpp>
#include <hpx/plugin/dll.hpp>
#include <hpx/plugin/macros.hpp>
#include <hpx/plugin/virtual_constructor.hpp>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util::plugin {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename BasePlugin, typename DeleterType>
        std::pair<abstract_factory<BasePlugin>*, dll_handle>
        get_abstract_factory_static(get_plugins_list_type f, DeleterType d,
            std::string const& class_name, std::string const& libname = "",
            error_code& ec = throws)
        {
            using pointed_type = std::remove_pointer_t<get_plugins_list_type>;

            exported_plugins_type& e = *f();

            std::string clsname(class_name);
            std::transform(clsname.begin(), clsname.end(), clsname.begin(),
                [](char c) { return std::tolower(c); });

            if (auto const it = e.find(clsname); it != e.end())
            {
                auto** xw =
                    hpx::any_cast<abstract_factory<BasePlugin>*>(&it->second);

                if (!xw)
                {
                    HPX_THROWS_IF(ec, hpx::error::filesystem_error,
                        "get_abstract_factory_static",
                        "Hpx.Plugin: Can't cast to the right factory type\n");
                    return std::pair<abstract_factory<BasePlugin>*,
                        dll_handle>();
                }

                abstract_factory<BasePlugin>* w = *xw;
                return std::make_pair(w, shared_ptr<pointed_type>(f, d));
            }
            else
            {
                std::ostringstream str;
                hpx::util::format_to(
                    str, "Hpx.Plugin: Class '{}' was not found", class_name);

                if (!libname.empty())
                {
                    hpx::util::format_to(
                        str, " in the shared library '{}'.", libname);
                }

                if (!e.empty())
                {
                    str << " Existing classes: ";

                    bool first = true;
                    auto const end = e.end();
                    for (auto jt = e.begin(); jt != end; ++jt)
                    {
                        if (first)
                        {
                            str << "'" << jt->first << "'";
                            first = false;
                        }
                        else
                        {
                            str << ", '" << jt->first << "'";
                        }
                    }
                    str << ".";
                }
                else
                {
                    str << " No classes exist.";
                }

                HPX_THROWS_IF(ec, hpx::error::filesystem_error,
                    "get_abstract_factory_static", str.str());
                return std::pair<abstract_factory<BasePlugin>*, dll_handle>();
            }
        }

        HPX_CXX_CORE_EXPORT template <typename BasePlugin>
        std::pair<abstract_factory<BasePlugin>*, dll_handle>
        get_abstract_factory(dll const& d, std::string const& class_name,
            std::string const& base_name, error_code& ec = throws)
        {
            using deleter_type = hpx::function<void(get_plugins_list_type)>;

            std::string plugin_entry(HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC_STR
                "_exported_plugins_list_");
            plugin_entry += d.get_mapname();
            plugin_entry += "_" + base_name;

            std::pair<get_plugins_list_type, deleter_type> const f =
                d.get<get_plugins_list_type, deleter_type>(plugin_entry, ec);
            if (ec)
                return std::pair<abstract_factory<BasePlugin>*, dll_handle>();

            return get_abstract_factory_static<BasePlugin>(
                f.first, f.second, class_name, d.get_name(), ec);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT inline void get_abstract_factory_names_static(
            get_plugins_list_type f, std::vector<std::string>& names,
            error_code& /*ec*/ = throws)
        {
            exported_plugins_type& e = *f();

            auto const end = e.end();
            for (auto it = e.begin(); it != end; ++it)
            {
                names.push_back(it->first);
            }
        }

        HPX_CXX_CORE_EXPORT inline void get_abstract_factory_names(dll const& d,
            std::string const& base_name, std::vector<std::string>& names,
            error_code& ec = throws)
        {
            using deleter_type = hpx::function<void(get_plugins_list_type)>;

            std::string plugin_entry(HPX_PLUGIN_SYMBOLS_PREFIX_DYNAMIC_STR
                "_exported_plugins_list_");
            plugin_entry += d.get_mapname();
            plugin_entry += "_" + base_name;

            std::pair<get_plugins_list_type, deleter_type> const f =
                d.get<get_plugins_list_type, deleter_type>(plugin_entry, ec);
            if (ec)
                return;

            get_abstract_factory_names_static(f.first, names, ec);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct HPX_PLUGIN_EXPORT_API
            plugin_factory_item_base
        {
            plugin_factory_item_base(dll& d, std::string basename)
              : m_dll(d)
              , m_basename(HPX_MOVE(basename))
            {
            }

            void create(int******) const;    // dummy placeholder

            void get_names(
                std::vector<std::string>& names, error_code& ec = throws) const
            {
                get_abstract_factory_names(m_dll, m_basename, names, ec);
            }

        protected:
            dll& m_dll;
            std::string m_basename;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename BasePlugin, typename Base,
            typename Parameters>
        struct HPX_PLUGIN_EXPORT_API plugin_factory_item;

        template <typename BasePlugin, typename Base, typename... Parameters>
        struct HPX_PLUGIN_EXPORT_API plugin_factory_item<BasePlugin, Base,
            hpx::util::pack<Parameters...>> : public Base
        {
            plugin_factory_item(dll& d, std::string basename)
              : Base(d, HPX_MOVE(basename))
            {
            }

            [[nodiscard]] BasePlugin* create(
                std::string const& name, Parameters... parameters) const
            {
                std::pair<abstract_factory<BasePlugin>*, dll_handle> r =
                    get_abstract_factory<BasePlugin>(
                        this->m_dll, name, this->m_basename);

                return r.first->create(r.second, parameters...);
            }

            [[nodiscard]] BasePlugin* create(std::string const& name,
                error_code& ec, Parameters... parameters) const
            {
                std::pair<abstract_factory<BasePlugin>*, dll_handle> r =
                    get_abstract_factory<BasePlugin>(
                        this->m_dll, name, this->m_basename, ec);
                if (ec)
                    return nullptr;

                return r.first->create(r.second, parameters...);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // empty deleter for the smart pointer to be used for static
        // plugin_factories
        HPX_CXX_CORE_EXPORT constexpr inline void empty_deleter(
            get_plugins_list_type) noexcept
        {
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct HPX_PLUGIN_EXPORT_API
            static_plugin_factory_item_base
        {
            explicit static_plugin_factory_item_base(
                get_plugins_list_type const& f_) noexcept    //-V835
              : f(f_)
            {
            }

            void create(int******) const;    // dummy placeholder

            void get_names(
                std::vector<std::string>& names, error_code& ec = throws) const
            {
                get_abstract_factory_names_static(f, names, ec);
            }

        protected:
            get_plugins_list_type f;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename BasePlugin, typename Base,
            typename Parameters>
        struct HPX_PLUGIN_EXPORT_API static_plugin_factory_item;

        template <typename BasePlugin, typename Base, typename... Parameters>
        struct HPX_PLUGIN_EXPORT_API static_plugin_factory_item<BasePlugin,
            Base, hpx::util::pack<Parameters...>> : public Base
        {
            explicit static_plugin_factory_item(
                get_plugins_list_type const& f) noexcept    //-V835
              : Base(f)
            {
            }

            [[nodiscard]] BasePlugin* create(
                std::string const& name, Parameters... parameters) const
            {
                std::pair<abstract_factory<BasePlugin>*, dll_handle> r =
                    get_abstract_factory_static<BasePlugin>(
                        this->f, &empty_deleter, name, "");

                return r.first->create(r.second, parameters...);
            }

            [[nodiscard]] BasePlugin* create(std::string const& name,
                error_code& ec, Parameters... parameters) const
            {
                std::pair<abstract_factory<BasePlugin>*, dll_handle> r =
                    get_abstract_factory_static<BasePlugin>(
                        this->f, &empty_deleter, name, "", ec);
                if (ec)
                    return nullptr;

                return r.first->create(r.second, parameters...);
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BasePlugin>
    struct HPX_PLUGIN_EXPORT_API plugin_factory
      : detail::plugin_factory_item<BasePlugin,
            detail::plugin_factory_item_base, virtual_constructor_t<BasePlugin>>
    {
    private:
        using base_type = detail::plugin_factory_item<BasePlugin,
            detail::plugin_factory_item_base,
            virtual_constructor_t<BasePlugin>>;

    public:
        plugin_factory(dll& d, std::string basename)
          : base_type(d, HPX_MOVE(basename))
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BasePlugin>
    struct HPX_PLUGIN_EXPORT_API static_plugin_factory
      : detail::static_plugin_factory_item<BasePlugin,
            detail::static_plugin_factory_item_base,
            virtual_constructor_t<BasePlugin>>
    {
    private:
        using base_type = detail::static_plugin_factory_item<BasePlugin,
            detail::static_plugin_factory_item_base,
            virtual_constructor_t<BasePlugin>>;

    public:
        explicit static_plugin_factory(
            get_plugins_list_type const& f_) noexcept    //-V835
          : base_type(f_)
        {
        }
    };
}    // namespace hpx::util::plugin

#include <hpx/config/warnings_suffix.hpp>
