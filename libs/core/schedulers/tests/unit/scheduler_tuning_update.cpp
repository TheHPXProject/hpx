//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The local-priority-tuned-fifo scheduler calls the update of a tuner with an
// update period on a background thread of its pool while the pool runs, and
// ends that before the pool has stopped. It counts the threads it runs by
// their size. The recording tuner writes what it sees as CSV, to a stream or,
// registered as "recording", to the file given by hpx.scheduler_tuner.file.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pool_util.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace policies = hpx::threads::policies;

constexpr std::chrono::milliseconds period(5);
constexpr std::chrono::seconds timeout(10);
constexpr std::size_t num_tasks = 100;

// the number of workers of the pool, as seen by hpx_main
std::size_t num_workers = 0;

int argc_ = 0;
char** argv_ = nullptr;

// the update calls of the tuner below
std::atomic<std::size_t> updates{0};
std::atomic<bool> updated_off_background_thread{false};
std::atomic<bool> tuner_destroyed{false};

// what the tuner below records
std::ostringstream recorded;

// records with a recording_tuner and checks where it is called from
struct checking_tuner final : policies::scheduler_tuner
{
    checking_tuner()
      : recorder(recorded, period, true, "default")
    {
    }

    ~checking_tuner() override
    {
        tuner_destroyed = true;
    }

    void attach(policies::scheduler_tuning_parameters& parameters) override
    {
        recorder.attach(parameters);
    }

    [[nodiscard]] std::chrono::milliseconds update_period() const override
    {
        return recorder.update_period();
    }

    void update(policies::scheduler_tuning_counters const& counters,
        policies::scheduler_tuning_parameters& parameters) override
    {
        auto* self = hpx::threads::get_self_id_data();
        if (self == nullptr || !self->is_background())
        {
            updated_off_background_thread = true;
        }
        recorder.update(counters, parameters);
        ++updates;
    }

    policies::recording_tuner recorder;
};

// the tested scheduler is used regardless of --hpx:queuing
void use_tuned_scheduler(hpx::local::init_params& init_args)
{
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            hpx::resource::scheduling_policy::local_priority_tuned_fifo);
    };
}

policies::scheduler_tuning_counters const& get_counters()
{
    auto const* counters = hpx::threads::get_self_id_data()
                               ->get_scheduler_base()
                               ->get_tuning_counters();
    HPX_TEST(counters != nullptr);
    return *counters;
}

// the given counter of all workers and of the threads from outside the pool
std::uint64_t total(policies::scheduler_tuning_counters const& counters,
    policies::scheduler_counter const c)
{
    std::uint64_t sum =
        counters.get(policies::scheduler_tuning_counters::external, c);
    for (std::size_t w = 0; w != counters.num_workers(); ++w)
    {
        sum += counters.get(w, c);
    }
    return sum;
}

// wait for the given number of updates after the current one
void wait_for_updates(std::size_t const n)
{
    std::size_t const until = updates.load() + n;
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    while (
        updates.load() < until && std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::sleep_for(period);
    }
    HPX_TEST_LTE(until, updates.load());
}

int test_update()
{
    num_workers = hpx::get_os_thread_count();

    // the tuner runs once the workers have started
    wait_for_updates(1);

    auto const& counters = get_counters();
    std::uint64_t const run_nano =
        total(counters, policies::scheduler_counter::run_nano);
    std::uint64_t const run_small =
        total(counters, policies::scheduler_counter::run_small);

    hpx::latch done(static_cast<std::ptrdiff_t>(2 * num_tasks + 1));
    for (std::size_t i = 0; i != num_tasks; ++i)
    {
        hpx::post(hpx::threads::nano_task([&done] { done.count_down(1); }));
        hpx::post(hpx::threads::small_task([&done] { done.count_down(1); }));
    }
    done.arrive_and_wait();

    // a thread is counted when it is handed to a worker, before it runs
    HPX_TEST_LTE(run_nano + num_tasks,
        total(counters, policies::scheduler_counter::run_nano));
    HPX_TEST_LTE(run_small + num_tasks,
        total(counters, policies::scheduler_counter::run_small));
    HPX_TEST_LTE(
        num_tasks, total(counters, policies::scheduler_counter::created_nano));

    // the last record has all of the tasks
    wait_for_updates(2);

    return hpx::local::finalize();
}

// the CSV records: the header and the rows
std::vector<std::vector<std::string>> parse_csv(std::string const& text)
{
    std::vector<std::vector<std::string>> rows;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        std::vector<std::string> row;
        std::istringstream fields(line);
        std::string field;
        while (std::getline(fields, field, ','))
        {
            row.push_back(field);
        }
        // a trailing empty field (the settings of external)
        if (!line.empty() && line.back() == ',')
        {
            row.emplace_back();
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::size_t column_of(
    std::vector<std::string> const& header, std::string_view const name)
{
    for (std::size_t i = 0; i != header.size(); ++i)
    {
        if (header[i] == name)
            return i;
    }
    HPX_TEST(false);
    return 0;
}

// The rows are written one per worker and one for external for each update.
// Checks the header and the rows, returns the sum of the given column in the
// last update's rows.
std::uint64_t check_records(std::string const& text, std::size_t const workers,
    std::string_view const column)
{
    auto const rows = parse_csv(text);
    HPX_TEST_LTE(std::size_t(2), rows.size());
    if (rows.size() < 2)
        return 0;

    auto const& header = rows.front();
    HPX_TEST_LTE(std::size_t(3) + policies::scheduler_tuning_counters::size,
        header.size());
    HPX_TEST_EQ(header[0], std::string("ms"));
    HPX_TEST_EQ(header[1], std::string("pool"));
    HPX_TEST_EQ(header[2], std::string("worker"));
    HPX_TEST_EQ(header[3],
        std::string(policies::get_scheduler_counter_name(
            policies::scheduler_counter::created_unknown)));

    std::size_t const per_update = workers + 1;
    HPX_TEST_EQ((rows.size() - 1) % per_update, std::size_t(0));
    for (std::size_t r = 1; r != rows.size(); ++r)
    {
        HPX_TEST_EQ(rows[r].size(), header.size());
        HPX_TEST_EQ(rows[r][1], std::string("default"));
        std::size_t const w = (r - 1) % per_update;
        HPX_TEST_EQ(rows[r][2],
            w == workers ? std::string("external") : std::to_string(w));
    }

    std::size_t const c = column_of(header, column);
    std::uint64_t sum = 0;
    for (std::size_t r = rows.size() - per_update; r != rows.size(); ++r)
    {
        sum += std::stoull(rows[r][c]);
    }
    return sum;
}

void test_update_loop()
{
    policies::register_scheduler_tuner(
        "checking", [](policies::scheduler_tuner_context const&) {
            return std::make_unique<checking_tuner>();
        });

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=2", "hpx.scheduler_tuner=checking"};
    use_tuned_scheduler(init_args);
    HPX_TEST_EQ(hpx::local::init(test_update, argc_, argv_, init_args), 0);

    HPX_TEST(!updated_off_background_thread.load());

    // the loop has ended before the pool stopped, and the tuner is gone
    HPX_TEST(tuner_destroyed.load());
    std::size_t const final_updates = updates.load();
    std::this_thread::sleep_for(10 * period);
    HPX_TEST_EQ(updates.load(), final_updates);

    // the totals of the last update have all the tasks
    HPX_TEST_LTE(
        num_tasks, check_records(recorded.str(), num_workers, "run_nano"));
    HPX_TEST_LTE(
        num_tasks, check_records(recorded.str(), num_workers, "run_small"));
}

// the file the "recording" tuner writes
std::filesystem::path recording_file;

// the number of complete rows (ending with a line break) of the given file
// after its header
std::size_t complete_rows(std::filesystem::path const& file)
{
    std::ifstream in(file);
    std::ostringstream content;
    content << in.rdbuf();
    std::string const text = content.str();

    std::size_t lines = 0;
    for (char const c : text)
    {
        if (c == '\n')
            ++lines;
    }
    return lines == 0 ? 0 : lines - 1;
}

int test_recording_file()
{
    num_workers = hpx::get_os_thread_count();

    // let the tuner write a few rows (one per worker and one for external
    // per update): wait for them rather than for a fixed time, as the tuner
    // may be late
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    while (complete_rows(recording_file) < 2 * (num_workers + 1) &&
        std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::sleep_for(period);
    }
    return hpx::local::finalize();
}

// the registered "recording" tuner, configured with its entries
void test_recording_tuner()
{
    recording_file = std::filesystem::temp_directory_path() /
        ("scheduler_tuning_update_" +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()) +
            ".csv");
    std::filesystem::path const& file = recording_file;

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=2", "hpx.scheduler_tuner=recording",
        "hpx.scheduler_tuner.period=" + std::to_string(period.count()),
        "hpx.scheduler_tuner.file=" + file.generic_string(),
        "hpx.scheduler_tuner.columns=totals"};
    use_tuned_scheduler(init_args);
    HPX_TEST_EQ(
        hpx::local::init(test_recording_file, argc_, argv_, init_args), 0);

    std::string text;
    {
        std::ifstream in(file);
        HPX_TEST(in.is_open());
        std::ostringstream content;
        content << in.rdbuf();
        text = content.str();
    }
    std::filesystem::remove(file);

    // hpx_main itself ran
    HPX_TEST_LTE(
        std::uint64_t(1), check_records(text, num_workers, "run_unknown"));
}

// invalid entries of the "recording" tuner are reported when it is created
void test_invalid_entries()
{
    auto const create = [](std::string period, std::string file) {
        policies::scheduler_tuner_context context;
        context.pool_name = "default";
        context.lookup = [period = std::move(period), file = std::move(file)](
                             std::string_view key, std::string_view) {
            return key == "period" ? period : file;
        };

        try
        {
            [[maybe_unused]] auto tuner =
                policies::create_scheduler_tuner("recording", context);
        }
        catch (hpx::exception const& e)
        {
            return e.get_error() == hpx::error::bad_parameter;
        }
        return false;
    };

    std::string const no_file = (std::filesystem::temp_directory_path() /
        "no_such_directory" / "scheduler_tuning_update.csv")
                                    .generic_string();
    std::string const file = (std::filesystem::temp_directory_path() /
        "scheduler_tuning_update_invalid.csv")
                                 .generic_string();

    HPX_TEST(create("0", file));
    HPX_TEST(create("ten", file));
    HPX_TEST(create("10", no_file));
    std::filesystem::remove(file);
}

// A size out of range (the hint has room for more values than there are
// sizes) is counted as unknown; asserted against in debug builds.
void test_invalid_size()
{
#if !defined(HPX_DEBUG)
    constexpr auto invalid = static_cast<hpx::threads::task_size>(7);
    static_assert(
        policies::run_of(invalid) == policies::scheduler_counter::run_unknown);
    static_assert(policies::created_of(invalid) ==
        policies::scheduler_counter::created_unknown);
    static_assert(policies::stolen_of(invalid) ==
        policies::scheduler_counter::stolen_unknown);
#endif
    static_assert(policies::run_of(hpx::threads::task_size::large) ==
        policies::scheduler_counter::run_large);
}

// The counters are 32 bits wide and wrap around; the recording tuner writes
// the right difference and keeps the totals in 64 bits. Without a runtime:
// counters and settings of one worker, updated by hand.
void test_counter_wraps()
{
    policies::scheduler_tuning_counters counters(1);
    policies::scheduler_tuning_parameters parameters(1);
    parameters.add("count", 0u, 10u, true, 3u);
    parameters.add("time", 0.0f, 10.0f, true, 2.5f);

    auto const c = policies::scheduler_counter::run_nano;
    std::uint32_t const max = (std::numeric_limits<std::uint32_t>::max)();

    for (bool const totals : {false, true})
    {
        std::ostringstream out;
        policies::recording_tuner tuner(out, period, totals, "default");
        tuner.attach(parameters);

        counters.add(0, c, max);    // 2^32 - 1 so far
        tuner.update(counters, parameters);
        counters.add(0, c, 10);    // wraps around to 9
        HPX_TEST_EQ(counters.get(0, c), std::uint32_t(9));
        tuner.update(counters, parameters);

        auto const rows = parse_csv(out.str());
        HPX_TEST_EQ(rows.size(), std::size_t(5));    // header, 2 x 2 rows
        if (rows.size() != 5)
            continue;

        std::size_t const run_nano = column_of(rows[0], "run_nano");
        std::uint64_t const first = std::stoull(rows[1][run_nano]);
        std::uint64_t const second = std::stoull(rows[3][run_nano]);
        if (totals)
        {
            HPX_TEST_EQ(first, std::uint64_t(max));
            HPX_TEST_EQ(second, std::uint64_t(max) + 10);
        }
        else
        {
            HPX_TEST_EQ(first, std::uint64_t(max));
            HPX_TEST_EQ(second, std::uint64_t(10));
        }

        // the settings as their types
        HPX_TEST_EQ(rows[1][column_of(rows[0], "count")], std::string("3"));
        HPX_TEST_EQ(rows[1][column_of(rows[0], "time")], std::string("2.5"));

        // the next round starts from 0 again
        counters.add(0, c, 0u - counters.get(0, c));
    }
}

///////////////////////////////////////////////////////////////////////////////
// A pool whose tuner has an update period can be suspended, resumed and
// stopped while suspended (by the end of the runtime). The tuner's thread
// waits for its next update with a timeout, which keeps the timer's threads
// in the pool: suspending the pool ends the tuner's thread first, resuming it
// starts the thread again.
std::atomic<std::size_t> suspended_pool_updates{0};

struct counting_tuner final : policies::scheduler_tuner
{
    void attach(policies::scheduler_tuning_parameters&) override {}

    [[nodiscard]] std::chrono::milliseconds update_period() const override
    {
        return period;
    }

    void update(policies::scheduler_tuning_counters const&,
        policies::scheduler_tuning_parameters&) override
    {
        ++suspended_pool_updates;
    }
};

// wait until the tuner has run at least the given number of updates
void wait_for_suspended_pool_updates(std::size_t const n)
{
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    while (suspended_pool_updates.load() < n &&
        std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::sleep_for(period);
    }
    HPX_TEST_LTE(n, suspended_pool_updates.load());
}

int test_suspended_pool()
{
    auto& pool = hpx::resource::get_thread_pool("tuned");

    // the tuner's thread runs in its pool
    wait_for_suspended_pool_updates(2);

    // no updates while the pool is suspended
    hpx::threads::suspend_pool(pool).get();
    std::size_t const updates_when_suspended = suspended_pool_updates.load();
    hpx::this_thread::sleep_for(4 * period);
    HPX_TEST_EQ(suspended_pool_updates.load(), updates_when_suspended);

    // they go on once it is resumed
    hpx::threads::resume_pool(pool).get();
    wait_for_suspended_pool_updates(updates_when_suspended + 2);

    // resuming a pool that runs, twice at the same time, starts no second
    // tuner thread (the suspension below would then end only one of them)
    auto resumed_once = hpx::threads::resume_pool(pool);
    auto resumed_twice = hpx::threads::resume_pool(pool);
    resumed_once.get();
    resumed_twice.get();

    // stopping the runtime stops the pool while it is suspended
    hpx::threads::suspend_pool(pool).get();
    return hpx::local::finalize();
}

void test_stop_suspended_pool()
{
    policies::register_scheduler_tuner(
        "counting", [](policies::scheduler_tuner_context const&) {
            return std::make_unique<counting_tuner>();
        });

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=2", "hpx.scheduler_tuner=counting"};
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        // the default pool keeps the first processing unit, the tuned pool
        // gets the second one
        rp.create_thread_pool("tuned",
            hpx::resource::scheduling_policy::local_priority_tuned_fifo,
            policies::scheduler_mode::default_ |
                policies::scheduler_mode::enable_elasticity);

        bool added = false;
        for (hpx::resource::numa_domain const& d : rp.numa_domains())
        {
            for (hpx::resource::core const& c : d.cores())
            {
                for (hpx::resource::pu const& p : c.pus())
                {
                    if (!added && p.id() != 0)
                    {
                        rp.add_resource(p, "tuned");
                        added = true;
                    }
                }
            }
        }
    };

    // the suspension of the pool didn't end while the tuner ran
    HPX_TEST_EQ(
        hpx::local::init(test_suspended_pool, argc_, argv_, init_args), 0);
}

int main(int argc, char* argv[])
{
    argc_ = argc;
    argv_ = argv;

    test_invalid_size();
    test_counter_wraps();
    test_invalid_entries();
    test_update_loop();
    test_recording_tuner();
    test_stop_suspended_pool();

    return hpx::util::report_errors();
}
