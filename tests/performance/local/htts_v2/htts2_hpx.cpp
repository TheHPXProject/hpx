//  Copyright (c) 2011-2014 Bryce Adelstein-Lelbach
//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013-2014 Thomas Heller
//  Copyright (c) 2013-2014 Patricia Grubel
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/barrier.hpp>
#include <hpx/format.hpp>
#include <hpx/functional.hpp>
#include <hpx/init.hpp>
#include <hpx/thread.hpp>

#include "htts2.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

template <typename BaseClock = std::chrono::steady_clock>
struct hpx_driver : htts2::driver
{
    hpx_driver(int argc, char** argv)
      : htts2::driver(argc, argv, true)
    //      , count_(0)
    {
    }

    void run()
    {
        std::vector<std::string> const cfg = {
            "hpx.os_threads=" + std::to_string(osthreads_),
            "hpx.run_hpx_main!=0", "hpx.commandline.allow_unknown!=1"};

        hpx::function<int(hpx::program_options::variables_map & vm)> f;
        hpx::program_options::options_description desc;

        hpx::local::init_params init_args;
        init_args.cfg = cfg;
        init_args.desc_cmdline = desc;

        using hpx::placeholders::_1;

        hpx::local::init(
            std::function<int(hpx::program_options::variables_map&)>(
                hpx::bind(&hpx_driver::run_impl, std::ref(*this), _1)),
            argc_, argv_, init_args);
    }

private:
    int run_impl(hpx::program_options::variables_map&)
    {
        // Cold run
        //kernel();

        // Hot run
        results_type results = kernel();
        print_results(results);

        return hpx::local::finalize();
    }

    hpx::threads::thread_result_type payload_thread_function(
        hpx::threads::thread_restart_state =
            hpx::threads::thread_restart_state::signaled)
    {
        htts2::payload<BaseClock>(this->payload_duration_ /* = p */);
        //++count_;
        return hpx::threads::thread_result_type(
            hpx::threads::thread_schedule_state::terminated,
            hpx::threads::invalid_thread_id);
    }

    void stage_tasks(std::uint64_t target_osthread)
    {
        std::uint64_t const this_osthread = hpx::get_worker_thread_num();

        // This branch is very rarely taken (I've measured); this only occurs
        // if we are unlucky enough to be stolen from our intended queue.
        if (this_osthread != target_osthread)
        {
            // Reschedule in an attempt to correct.
            hpx::threads::thread_init_data data(
                hpx::threads::make_thread_function_nullary(
                    hpx::bind(&hpx_driver::stage_tasks, std::ref(*this),
                        target_osthread)),
                nullptr    // No HPX-thread name.
                ,
                hpx::threads::thread_priority::normal
                // Place in the target OS-thread's queue.
                ,
                hpx::threads::thread_schedule_hint(
                    static_cast<std::int16_t>(target_osthread)));
            hpx::threads::register_work(data);
        }

        for (std::uint64_t i = 0; i < this->tasks_; ++i)
        {
            using hpx::placeholders::_1;
            hpx::threads::thread_init_data data(
                hpx::bind(
                    &hpx_driver::payload_thread_function, std::ref(*this), _1),
                nullptr    // No HPX-thread name.
                ,
                hpx::threads::thread_priority::normal
                // Place in the target OS-thread's queue.
                ,
                hpx::threads::thread_schedule_hint(
                    static_cast<std::int16_t>(target_osthread)));
            hpx::threads::register_work(data);
        }
    }

    void wait_for_tasks(std::shared_ptr<hpx::barrier<>> finished)
    {
        std::uint64_t const pending_count =
            get_thread_count(hpx::threads::thread_priority::normal,
                hpx::threads::thread_schedule_state::pending);

        if (pending_count == 0)
        {
            std::uint64_t const all_count =
                get_thread_count(hpx::threads::thread_priority::normal);

            if (all_count != 1)
            {
                hpx::threads::thread_init_data data(
                    hpx::threads::make_thread_function_nullary(
                        hpx::bind(&hpx_driver::wait_for_tasks, std::ref(*this),
                            std::ref(finished))),
                    nullptr, hpx::threads::thread_priority::low);
                register_work(data);
                return;
            }
        }

        finished->arrive_and_wait();
    }

    typedef double results_type;

    results_type kernel()
    {
        ///////////////////////////////////////////////////////////////////////

        //count_ = 0;

        results_type results;

        std::uint64_t const this_osthread = hpx::get_worker_thread_num();

        htts2::timer<BaseClock> t;

        ///////////////////////////////////////////////////////////////////////
        // Warmup Phase
        for (std::uint64_t i = 0; i < this->osthreads_; ++i)
        {
            if (this_osthread == i)
                continue;

            hpx::threads::thread_init_data data(
                hpx::threads::make_thread_function_nullary(
                    hpx::bind(&hpx_driver::stage_tasks, std::ref(*this), i)),
                nullptr,    // No HPX-thread name.
                // Place in the target OS-thread's queue.
                hpx::threads::thread_priority::normal,
                hpx::threads::thread_schedule_hint(
                    static_cast<std::int16_t>(i)));
            hpx::threads::register_work(data);
        }

        stage_tasks(this_osthread);

        ///////////////////////////////////////////////////////////////////////
        // Compute + Cooldown Phase

        // The use of an atomic and live waiting here does not add any noticeable
        // overhead, as compared to the more complicated continuation-style
        // detection method that checks the threadmanager internal counters
        // (I've measured). Using this technique is preferable as it is more
        // comparable to the other implementations.
        //        do {
        //            hpx::this_thread::suspend();
        //        } while (count_ < (this->tasks_ * this->osthreads_));

        // Schedule a low-priority thread; when it is executed, it checks to
        // make sure all the tasks (which are normal priority) have been
        // executed, and then it
        std::shared_ptr<hpx::barrier<>> finished =
            std::make_shared<hpx::barrier<>>(2);

        hpx::threads::thread_init_data data(
            hpx::threads::make_thread_function_nullary(hpx::bind(
                &hpx_driver::wait_for_tasks, std::ref(*this), finished)),
            nullptr, hpx::threads::thread_priority::low);
        register_work(data);

        finished->arrive_and_wait();

        // w_M [nanoseconds]
        results = static_cast<double>(t.elapsed());

        return results;

        ///////////////////////////////////////////////////////////////////////
    }

    void print_results(results_type results) const
    {
        if (this->io_ == htts2::csv_with_headers)
            std::cout
                << "OS-threads (Independent Variable),"
                << "Tasks per OS-thread (Control Variable) [tasks/OS-threads],"
                << "Payload Duration (Control Variable) [nanoseconds],"
                << "Total Walltime [nanoseconds]"
                << "\n";

        hpx::util::format_to(std::cout, "{},{},{},{:.14g}\n", this->osthreads_,
            this->tasks_, this->payload_duration_, results);
    }

    //    std::atomic<std::uint64_t> count_;
};

int main(int argc, char** argv)
{
    hpx_driver<> d(argc, argv);

    d.run();

    return 0;
}
