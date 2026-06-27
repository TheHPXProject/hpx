//  Copyright (c) 2011-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components/basename_registration.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {

        std::string name_from_basename(
            std::string const& basename, std::size_t idx)
        {
            HPX_ASSERT(!basename.empty());

            std::string name;
            if (basename[0] != '/')
            {
                name = '/';
            }

            name += basename;
            if (name[name.size() - 1] != '/')
            {
                name += '/';
            }
            name += std::to_string(idx);

            return name;
        }

        std::string name_from_basename(std::string&& basename, std::size_t idx)
        {
            HPX_ASSERT(!basename.empty());

            std::string name;
            if (basename[0] != '/')
            {
                name = '/';
                name += HPX_MOVE(basename);
            }
            else
            {
                name = HPX_MOVE(basename);
            }

            if (name[name.size() - 1] != '/')
            {
                name += '/';
            }
            name += std::to_string(idx);

            return name;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    std::vector<hpx::future<hpx::id_type>> find_all_from_basename(
        std::string basename, std::size_t num_ids)
    {
        if (basename.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::find_all_from_basename", "no basename specified");
        }

        std::vector<hpx::future<hpx::id_type>> results;
        for (std::size_t i = 0; i != num_ids; ++i)
        {
            std::string name;
            if (i == num_ids - 1)
            {
                // NOLINTNEXTLINE(bugprone-use-after-move)
                name = detail::name_from_basename(HPX_MOVE(basename), i);
            }
            else
            {
                name = detail::name_from_basename(basename, i);
            }

            results.push_back(
                agas::on_symbol_namespace_event(HPX_MOVE(name), true));
        }
        return results;
    }

    std::vector<hpx::future<hpx::id_type>> find_all_from_basename(
        hpx::launch::sync_policy, std::string base_name, std::size_t num_ids)
    {
        std::vector<hpx::future<hpx::id_type>> results =
            find_all_from_basename(HPX_MOVE(base_name), num_ids);
        hpx::wait_all(results);
        return results;
    }

    std::vector<hpx::future<hpx::id_type>> find_from_basename(
        std::string basename, std::vector<std::size_t> const& ids)
    {
        if (basename.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::find_from_basename", "no basename specified");
        }

        // 26800: Use of a moved from object: ''basename''
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26800)
#endif
        std::vector<hpx::future<hpx::id_type>> results;
        for (std::size_t i = 0; i != ids.size(); ++i)
        {
            std::string name;
            if (i == ids.size() - 1)
            {
                // NOLINTNEXTLINE(bugprone-use-after-move)
                name = detail::name_from_basename(HPX_MOVE(basename), i);
            }
            else
            {
                name = detail::name_from_basename(basename, i);
            }

            results.emplace_back(
                agas::on_symbol_namespace_event(HPX_MOVE(name), true));
        }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
        return results;
    }

    std::vector<hpx::future<hpx::id_type>> find_from_basename(
        hpx::launch::sync_policy, std::string base_name,
        std::vector<std::size_t> const& ids)
    {
        std::vector<hpx::future<hpx::id_type>> results =
            find_from_basename(HPX_MOVE(base_name), ids);
        hpx::wait_all(results);
        return results;
    }

    hpx::future<hpx::id_type> find_from_basename(
        std::string basename, std::size_t sequence_nr)
    {
        if (basename.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::find_from_basename", "no basename specified");
        }

        if (sequence_nr == ~static_cast<std::size_t>(0))
        {
            sequence_nr = static_cast<std::size_t>(agas::get_locality_id());
        }

        std::string name =
            detail::name_from_basename(HPX_MOVE(basename), sequence_nr);
        return agas::on_symbol_namespace_event(HPX_MOVE(name), true);
    }

    hpx::id_type find_from_basename(
        hpx::launch::sync_policy, std::string basename, std::size_t sequence_nr)
    {
        if (basename.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::find_from_basename", "no basename specified");
        }

        if (sequence_nr == ~static_cast<std::size_t>(0))
        {
            sequence_nr = static_cast<std::size_t>(agas::get_locality_id());
        }

        std::string name =
            detail::name_from_basename(HPX_MOVE(basename), sequence_nr);
        return agas::on_symbol_namespace_event(HPX_MOVE(name), true).get();
    }

    hpx::future<bool> register_with_basename(
        std::string basename, hpx::id_type const& id, std::size_t sequence_nr)
    {
        if (basename.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::register_with_basename", "no basename specified");
        }

        if (sequence_nr == ~static_cast<std::size_t>(0))
        {
            sequence_nr = static_cast<std::size_t>(agas::get_locality_id());
        }

        std::string name =
            detail::name_from_basename(HPX_MOVE(basename), sequence_nr);
        return agas::register_name(HPX_MOVE(name), id);
    }

    bool register_with_basename(hpx::launch::sync_policy, std::string basename,
        hpx::id_type const& id, std::size_t sequence_nr, error_code& ec)
    {
        if (basename.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::register_with_basename", "no basename specified");
        }

        if (sequence_nr == ~static_cast<std::size_t>(0))
        {
            sequence_nr = static_cast<std::size_t>(agas::get_locality_id());
        }

        std::string name =
            detail::name_from_basename(HPX_MOVE(basename), sequence_nr);
        return agas::register_name(hpx::launch::sync, HPX_MOVE(name), id, ec);
    }

    hpx::future<bool> register_with_basename(std::string base_name,
        hpx::future<hpx::id_type> f, std::size_t sequence_nr)
    {
        return f.then(hpx::launch::sync,
            [sequence_nr, base_name = HPX_MOVE(base_name)](
                hpx::future<hpx::id_type>&& f) mutable -> hpx::future<bool> {
                return register_with_basename(
                    HPX_MOVE(base_name), f.get(), sequence_nr);
            });
    }

    hpx::future<hpx::id_type> unregister_with_basename(
        std::string basename, std::size_t sequence_nr)
    {
        if (basename.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::unregister_with_basename", "no basename specified");
        }

        if (sequence_nr == ~static_cast<std::size_t>(0))
        {
            sequence_nr = static_cast<std::size_t>(agas::get_locality_id());
        }

        std::string name =
            detail::name_from_basename(HPX_MOVE(basename), sequence_nr);
        return agas::unregister_name(HPX_MOVE(name));
    }
}    // namespace hpx
