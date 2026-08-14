//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/performance_counters.hpp>

#include <hpx/distribution_policies/binpacking_distribution_policy.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::components {

    binpacking_distribution_policy const binpacked{};
}

namespace hpx::components::detail {

    std::vector<std::size_t> get_items_count(
        std::size_t count, std::vector<std::uint64_t> const& values)
    {
        std::size_t maxcount = 0;

        for (std::uint64_t value : values)
        {
            maxcount = (std::max) (maxcount, std::size_t(value));
        }

        // distribute the number of components to create in a way, so that
        // the overall number of component instances on all localities is
        // approximately the same
        std::size_t num_localities = values.size();

        // calculate the number of new instances to create on each of the
        // localities
        std::vector<std::size_t> to_create(num_localities, 0);

        bool even_fill = true;
        while (even_fill)
        {
            even_fill = false;
            for (std::size_t i = 0; i != num_localities; ++i)
            {
                if (values[i] + to_create[i] >= maxcount)
                    continue;
                even_fill = true;

                ++to_create[i];
                --count;
                if (count == 0)
                    break;
            }
        }

        std::size_t i = 0;
        while (count != 0)
        {
            ++to_create[i];
            i = (i + 1) % num_localities;
            --count;
        }

        return to_create;
    }

    hpx::future<std::vector<std::uint64_t>> retrieve_counter_values(
        std::vector<performance_counters::performance_counter>&& counters)
    {
        using namespace hpx::performance_counters;

        std::vector<hpx::future<std::uint64_t>> values;
        values.reserve(counters.size());

        for (performance_counter const& counter : counters)
            values.emplace_back(counter.get_value<std::uint64_t>());

        return hpx::dataflow(
            hpx::launch::sync,
            [](std::vector<hpx::future<std::uint64_t>>&& values)
                -> std::vector<std::uint64_t> { return hpx::unwrap(values); },
            HPX_MOVE(values));
    }

    hpx::future<std::vector<std::uint64_t>> get_counter_values(
        std::string const& component_name, std::string const& counter_name,
        std::vector<hpx::id_type> const& localities)
    {
        using namespace hpx::performance_counters;

        // create performance counters on all localities
        std::vector<performance_counter> counters;
        counters.reserve(localities.size());

        if (counter_name[counter_name.size() - 1] == '@')
        {
            std::string name(counter_name + component_name);

            for (hpx::id_type const& id : localities)
                counters.emplace_back(name, id);
        }
        else
        {
            for (hpx::id_type const& id : localities)
                counters.emplace_back(counter_name, id);
        }

        return hpx::dataflow(&retrieve_counter_values, HPX_MOVE(counters));
    }

    hpx::id_type const& get_best_locality(
        hpx::future<std::vector<std::uint64_t>>&& f,
        std::vector<hpx::id_type> const& localities)
    {
        std::vector<std::uint64_t> values = f.get();

        std::size_t best_locality = 0;
        std::uint64_t min_value = (std::numeric_limits<std::uint64_t>::max)();

        for (std::size_t i = 0; i != values.size(); ++i)
        {
            if (min_value > values[i])
            {
                min_value = values[i];
                best_locality = i;
            }
        }

        return localities[best_locality];
    }
}    // namespace hpx::components::detail
