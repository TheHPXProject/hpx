//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example is meant for inclusion in the documentation.

//[scheduler_tuner_docs
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace policies = hpx::threads::policies;

// Halves the idle spin time of a worker thread whose spins mostly run out
// without finding work, and doubles it while its spins mostly find work.
class spin_tuner final : public policies::scheduler_tuner
{
public:
    explicit spin_tuner(policies::scheduler_tuner_context const& context)
      // the tuner's own entry: hpx.scheduler_tuner.spin_period
      : period_(std::stoi(context.get_entry("spin_period", "100")))
    {
    }

    // called once, before the worker threads start
    void attach(policies::scheduler_tuning_parameters& parameters) override
    {
        spin_time_ = parameters.find("hpx.min_idle_backoff_time");
        found_.assign(parameters.num_workers(), 0);
        ran_out_.assign(parameters.num_workers(), 0);

        // start with at least one spin step (see update), also if the spin
        // time was configured shorter
        if (spin_time_ != policies::scheduler_tuning_parameters::npos)
        {
            for (std::size_t w = 0; w != parameters.num_workers(); ++w)
            {
                parameters.set(w, spin_time_,
                    (std::max) (parameters.get<float>(w, spin_time_), 2.0f));
            }
        }
    }

    // how often update is called
    [[nodiscard]] std::chrono::milliseconds update_period() const override
    {
        return period_;
    }

    // called every period on an HPX thread of the scheduler's pool
    void update(policies::scheduler_tuning_counters const& counters,
        policies::scheduler_tuning_parameters& parameters) override
    {
        // the setting exists only with HPX_WITH_THREAD_MANAGER_IDLE_BACKOFF
        if (spin_time_ == policies::scheduler_tuning_parameters::npos)
            return;

        using counter = policies::scheduler_counter;
        for (std::size_t w = 0; w != counters.num_workers(); ++w)
        {
            // the counters wrap around, compare with the last reading
            std::uint32_t const found =
                counters.get(w, counter::idle_spin_own_work) +
                counters.get(w, counter::idle_spin_work_to_steal);
            std::uint32_t const ran_out =
                counters.get(w, counter::idle_spin_ran_out);

            std::uint32_t const new_found = found - found_[w];
            std::uint32_t const new_ran_out = ran_out - ran_out_[w];
            found_[w] = found;
            ran_out_[w] = ran_out;

            // The spin time is a float (in microseconds); set clamps the
            // value to the range of the setting. It stays at least 2 us, so
            // that the first backoff step (1 us) is still spun: without any
            // spin there are no spins to count, and the tuner could never
            // raise the spin time again.
            float const spin_time = parameters.get<float>(w, spin_time_);
            if (new_ran_out / 4 > new_found)
            {
                parameters.set(w, spin_time_, (std::max) (spin_time / 2, 2.0f));
            }
            else if (new_found > new_ran_out)
            {
                parameters.set(w, spin_time_, (std::max) (2 * spin_time, 2.0f));
            }
        }
    }

private:
    std::chrono::milliseconds const period_;
    std::size_t spin_time_ = policies::scheduler_tuning_parameters::npos;
    std::vector<std::uint32_t> found_;
    std::vector<std::uint32_t> ran_out_;
};

int hpx_main()
{
    // bursts of work with idle periods in between, for the tuner to see
    std::vector<double> data(100000, 1.0);
    for (int i = 0; i != 10; ++i)
    {
        hpx::for_each(hpx::execution::par, data.begin(), data.end(),
            [](double& d) { d *= 1.01; });
        hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::cout << "data[0] = " << data[0] << "\n";
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // make the tuner available as "spin" before the runtime is started
    policies::register_scheduler_tuner(
        "spin", [](policies::scheduler_tuner_context const& context) {
            return std::make_unique<spin_tuner>(context);
        });

    // use the tuned scheduler with this tuner; the tuner's own entry needs
    // a '!' as it has no default
    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.scheduler=local-priority-tuned-fifo",
        "hpx.scheduler_tuner=spin", "hpx.scheduler_tuner.spin_period!=50"};
    return hpx::local::init(hpx_main, argc, argv, init_args);
}
//]
