//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/threading_base/scheduler_tuning.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace hpx::threads::policies {

    scheduler_tuning_parameters::scheduler_tuning_parameters(
        std::size_t const num_workers)
      : values_(num_workers)
    {
        infos_.reserve(max_parameters);
    }

    std::size_t scheduler_tuning_parameters::add(
        tuning_parameter_info const& info, double const value)
    {
        if (infos_.size() == max_parameters)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "scheduler_tuning_parameters::add",
                "too many tunable settings (at most {}), can't add {}",
                max_parameters, info.name);
        }
        if (find(info.name) != npos)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "scheduler_tuning_parameters::add",
                "the tunable setting {} was already added", info.name);
        }

        std::size_t const id = infos_.size();
        infos_.push_back(info);
        set_all(id, value);
        return id;
    }

    std::size_t scheduler_tuning_parameters::find(
        std::string_view const name) const noexcept
    {
        auto const it = std::find_if(infos_.begin(), infos_.end(),
            [&](tuning_parameter_info const& info) {
                return name == info.name;
            });
        return it != infos_.end() ?
            static_cast<std::size_t>(it - infos_.begin()) :
            npos;
    }

    void scheduler_tuning_parameters::set(std::size_t const worker,
        std::size_t const id, double const value) noexcept
    {
        HPX_ASSERT(worker < values_.size() && id < infos_.size());
        tuning_parameter_info const& info = infos_[id];
        HPX_ASSERT(info.per_worker);
        values_[worker].data_[id].store(
            std::clamp(value, info.min_value, info.max_value),
            std::memory_order_relaxed);
    }

    void scheduler_tuning_parameters::set_all(
        std::size_t const id, double const value) noexcept
    {
        HPX_ASSERT(id < infos_.size());
        tuning_parameter_info const& info = infos_[id];
        double const clamped =
            std::clamp(value, info.min_value, info.max_value);
        for (auto& values : values_)
        {
            values.data_[id].store(clamped, std::memory_order_relaxed);
        }
    }

    scheduler_tuner::~scheduler_tuner() = default;

    namespace {

        // keeps the values the settings were configured with
        struct static_tuner final : scheduler_tuner
        {
            void attach(scheduler_tuning_parameters&) override {}
        };

        struct tuner_registry
        {
            tuner_registry()
            {
                factories.emplace(
                    "static", [] { return std::make_unique<static_tuner>(); });
            }

            std::mutex mtx;
            std::map<std::string, scheduler_tuner_factory, std::less<>>
                factories;
        };

        tuner_registry& get_tuner_registry()
        {
            static tuner_registry registry;
            return registry;
        }
    }    // namespace

    void register_scheduler_tuner(
        std::string const& name, scheduler_tuner_factory factory)
    {
        auto& registry = get_tuner_registry();
        std::lock_guard<std::mutex> l(registry.mtx);
        registry.factories.insert_or_assign(name, HPX_MOVE(factory));
    }

    std::unique_ptr<scheduler_tuner> create_scheduler_tuner(
        std::string const& name)
    {
        auto& registry = get_tuner_registry();
        scheduler_tuner_factory factory;
        {
            std::lock_guard<std::mutex> l(registry.mtx);
            auto const it = registry.factories.find(name);
            if (it == registry.factories.end())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "create_scheduler_tuner",
                    "unknown scheduler tuner: {} (see "
                    "hpx.scheduler_tuner)",
                    name);
            }
            factory = it->second;
        }
        return factory();
    }
}    // namespace hpx::threads::policies
