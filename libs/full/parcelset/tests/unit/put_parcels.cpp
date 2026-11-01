//  Copyright (c) 2016-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/experimental/task_group.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
constexpr std::size_t vsize_default = 1024;
constexpr std::size_t numparcels_default = 10;

///////////////////////////////////////////////////////////////////////////////
template <typename Action, typename T>
hpx::parcelset::parcel generate_parcel(
    hpx::id_type const& dest_id, hpx::id_type const& cont, T&& data)
{
    hpx::naming::address addr;
    hpx::naming::gid_type dest = dest_id.get_gid();
    hpx::parcelset::parcel p(hpx::parcelset::detail::create_parcel::call(
        std::move(dest), std::move(addr),
        hpx::actions::typed_continuation<hpx::id_type>(cont), Action(),
        hpx::launch::async, std::forward<T>(data)));

    p.set_source_id(hpx::find_here());
    p.size() = 4096;
    return p;
}

///////////////////////////////////////////////////////////////////////////////
hpx::id_type test1(std::vector<double> const&)
{
    return hpx::find_here();
}
HPX_PLAIN_ACTION(test1)

void test_plain_argument(hpx::id_type const& id)
{
    std::vector<double> data(vsize_default);
    std::generate(data.begin(), data.end(), std::rand);

    std::vector<hpx::future<hpx::id_type>> results;
    results.reserve(numparcels_default);

    // create parcels
    std::vector<hpx::parcelset::parcel> parcels;
    for (std::size_t i = 0; i != numparcels_default; ++i)
    {
        hpx::distributed::promise<hpx::id_type> p;
        auto f = p.get_future();
        parcels.push_back(generate_parcel<test1_action>(id, p.get_id(), data));
        results.push_back(std::move(f));
    }

    // send parcels
    hpx::get_runtime_distributed().get_parcel_handler().put_parcels(
        std::move(parcels));

    // verify all messages got actually sent to the correct locality
    hpx::wait_all(results);

    for (hpx::future<hpx::id_type>& f : results)
    {
        HPX_TEST_EQ(f.get(), id);
    }
}

///////////////////////////////////////////////////////////////////////////////
hpx::id_type test2(hpx::future<double> const&)
{
    return hpx::find_here();
}
HPX_PLAIN_ACTION(test2)

void test_future_argument(hpx::id_type const& id)
{
    std::vector<hpx::promise<double>> args;
    args.reserve(numparcels_default);

    std::vector<hpx::future<hpx::id_type>> results;
    results.reserve(numparcels_default);

    // create parcels
    std::vector<hpx::parcelset::parcel> parcels;
    for (std::size_t i = 0; i != numparcels_default; ++i)
    {
        hpx::promise<double> p_arg;
        hpx::distributed::promise<hpx::id_type> p_cont;
        auto f_cont = p_cont.get_future();

        parcels.push_back(generate_parcel<test2_action>(
            id, p_cont.get_id(), p_arg.get_future()));

        args.push_back(std::move(p_arg));
        results.push_back(std::move(f_cont));
    }

    // send parcels
    hpx::get_runtime_distributed().get_parcel_handler().put_parcels(
        std::move(parcels));

    // now make the futures ready
    for (hpx::promise<double>& arg : args)
    {
        arg.set_value(42.0);
    }

    // verify all messages got actually sent to the correct locality
    hpx::wait_all(results);

    for (hpx::future<hpx::id_type>& f : results)
    {
        HPX_TEST_EQ(f.get(), id);
    }
}

void test_mixed_arguments(hpx::id_type const& id)
{
    std::vector<double> data(vsize_default);
    std::generate(data.begin(), data.end(), std::rand);

    std::vector<hpx::promise<double>> args;
    args.reserve(numparcels_default);

    std::vector<hpx::future<hpx::id_type>> results;
    results.reserve(numparcels_default);

    // create parcels
    std::vector<hpx::parcelset::parcel> parcels;
    for (std::size_t i = 0; i != numparcels_default; ++i)
    {
        hpx::distributed::promise<hpx::id_type> p_cont;
        auto f_cont = p_cont.get_future();

        if (std::rand() % 2)
        {
            parcels.push_back(
                generate_parcel<test1_action>(id, p_cont.get_id(), data));
        }
        else
        {
            hpx::promise<double> p_arg;

            parcels.push_back(generate_parcel<test2_action>(
                id, p_cont.get_id(), p_arg.get_future()));

            args.push_back(std::move(p_arg));
        }

        results.push_back(std::move(f_cont));
    }

    // send parcels
    hpx::get_runtime_distributed().get_parcel_handler().put_parcels(
        std::move(parcels));

    // now make the futures ready
    for (hpx::promise<double>& arg : args)
    {
        arg.set_value(42.0);
    }

    // verify all messages got actually sent to the correct locality
    hpx::wait_all(results);

    for (hpx::future<hpx::id_type>& f : results)
    {
        HPX_TEST_EQ(f.get(), id);
    }
}

///////////////////////////////////////////////////////////////////////////////
hpx::id_type test3(std::shared_ptr<hpx::experimental::task_group> const& tg)
{
    tg->wait();
    return hpx::find_here();
}
HPX_PLAIN_ACTION(test3)

void wait_a_while()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void test_task_group_argument(hpx::id_type const& id)
{
    std::vector<std::shared_ptr<hpx::experimental::task_group>> args;
    args.reserve(numparcels_default);

    std::vector<hpx::future<hpx::id_type>> results;
    results.reserve(numparcels_default);

    // create parcels
    std::vector<hpx::parcelset::parcel> parcels;
    for (std::size_t i = 0; i != numparcels_default; ++i)
    {
        auto tg = std::make_shared<hpx::experimental::task_group>();
        tg->run(wait_a_while);
        tg->run(wait_a_while);
        tg->run(wait_a_while);

        hpx::distributed::promise<hpx::id_type> p_cont;
        auto f_cont = p_cont.get_future();

        parcels.push_back(
            generate_parcel<test3_action>(id, p_cont.get_id(), tg));

        args.push_back(std::move(tg));
        results.push_back(std::move(f_cont));
    }

    // send parcels
    hpx::get_runtime_distributed().get_parcel_handler().put_parcels(
        std::move(parcels));

    // now wait for task groups
    for (auto const& arg : args)
    {
        arg->wait();
    }

    // verify all messages got actually sent to the correct locality
    hpx::wait_all(results);

    for (hpx::future<hpx::id_type>& f : results)
    {
        HPX_TEST_EQ(f.get(), id);
    }
}

///////////////////////////////////////////////////////////////////////////////
hpx::naming::gid_type missing_locality_gid()
{
    return hpx::naming::get_gid_from_locality_id(
        hpx::get_num_localities(hpx::launch::sync));
}

hpx::parcelset::parcel generate_missing_locality_parcel()
{
    std::vector<double> data(1, 1.0);
    hpx::naming::address addr;
    hpx::parcelset::parcel p(hpx::parcelset::detail::create_parcel::call(
        missing_locality_gid(), std::move(addr), test1_action(), data));

    p.set_source_id(hpx::find_here());
    p.size() = 4096;
    return p;
}

void test_put_parcel_unknown_locality_synchronous()
{
    hpx::execution::parallel_executor exec(
        hpx::threads::thread_stacksize::large);

    hpx::async(exec, [] {
        HPX_TEST(hpx::this_thread::has_sufficient_stack_space());

        bool called = false;
        auto handler = [&](std::error_code const&,
                           hpx::parcelset::parcel const&) { called = true; };

        bool threw = false;
        try
        {
            hpx::get_runtime_distributed().get_parcel_handler().put_parcel(
                generate_missing_locality_parcel(), handler);
        }
        catch (hpx::exception const& e)
        {
            threw = true;
            HPX_TEST_EQ(static_cast<int>(e.get_error()),
                static_cast<int>(hpx::error::bad_parameter));
        }
        HPX_TEST(threw);
        HPX_TEST(!called);
    }).get();
}

void test_put_parcel_unknown_locality_rescheduled()
{
    hpx::promise<void> done;
    std::error_code got;
    auto handler = [&](std::error_code const& ec,
                       hpx::parcelset::parcel const&) {
        got = ec;
        done.set_value();
    };

    std::thread os_thread([parcel = generate_missing_locality_parcel(),
                              handler = std::move(handler)]() mutable {
        hpx::get_runtime_distributed().get_parcel_handler().put_parcel(
            std::move(parcel), std::move(handler));
    });
    os_thread.join();

    hpx::future<void> f = done.get_future();
    HPX_TEST(f.wait_for(std::chrono::seconds(10)) == hpx::future_status::ready);
    if (f.is_ready())
    {
        f.get();
        HPX_TEST_EQ(got.value(), static_cast<int>(hpx::error::bad_parameter));
    }
}

void test_put_parcels_unknown_locality_rescheduled()
{
    hpx::promise<void> done;
    std::error_code got;

    std::vector<hpx::parcelset::parcel> parcels;
    parcels.push_back(generate_missing_locality_parcel());

    using handler_type = hpx::parcelset::parcelhandler::write_handler_type;
    std::vector<handler_type> handlers;
    handlers.emplace_back(
        [&](std::error_code const& ec, hpx::parcelset::parcel const&) {
            got = ec;
            done.set_value();
        });

    std::thread os_thread([parcels = std::move(parcels),
                              handlers = std::move(handlers)]() mutable {
        hpx::get_runtime_distributed().get_parcel_handler().put_parcels(
            std::move(parcels), std::move(handlers));
    });
    os_thread.join();

    hpx::future<void> f = done.get_future();
    HPX_TEST(f.wait_for(std::chrono::seconds(10)) == hpx::future_status::ready);
    if (f.is_ready())
    {
        f.get();
        HPX_TEST_EQ(got.value(), static_cast<int>(hpx::error::bad_parameter));
    }
}

void test_put_parcel_rescheduled_reaches_destination()
{
    hpx::id_type const id = hpx::find_here();
    std::vector<double> data(8, 1.0);

    hpx::distributed::promise<hpx::id_type> cont;
    hpx::future<hpx::id_type> result = cont.get_future();
    hpx::parcelset::parcel parcel =
        generate_parcel<test1_action>(id, cont.get_id(), data);

    hpx::promise<void> sent;
    std::error_code got;
    auto handler = [&](std::error_code const& ec,
                       hpx::parcelset::parcel const&) {
        got = ec;
        sent.set_value();
    };

    std::thread os_thread(
        [parcel = std::move(parcel), handler = std::move(handler)]() mutable {
            hpx::get_runtime_distributed().get_parcel_handler().put_parcel(
                std::move(parcel), std::move(handler));
        });
    os_thread.join();

    hpx::future<void> sent_f = sent.get_future();
    HPX_TEST(
        sent_f.wait_for(std::chrono::seconds(10)) == hpx::future_status::ready);
    if (sent_f.is_ready())
    {
        sent_f.get();
        HPX_TEST(!got);
    }

    HPX_TEST(
        result.wait_for(std::chrono::seconds(10)) == hpx::future_status::ready);
    if (result.is_ready())
    {
        HPX_TEST_EQ(result.get(), id);
    }
}

void test_put_parcels_rescheduled_reaches_destination()
{
    hpx::id_type const id = hpx::find_here();
    std::vector<double> data(8, 1.0);

    hpx::distributed::promise<hpx::id_type> cont;
    hpx::future<hpx::id_type> result = cont.get_future();

    std::vector<hpx::parcelset::parcel> parcels;
    parcels.push_back(generate_parcel<test1_action>(id, cont.get_id(), data));

    hpx::promise<void> sent;
    std::error_code got;
    using handler_type = hpx::parcelset::parcelhandler::write_handler_type;
    std::vector<handler_type> handlers;
    handlers.emplace_back(
        [&](std::error_code const& ec, hpx::parcelset::parcel const&) {
            got = ec;
            sent.set_value();
        });

    std::thread os_thread([parcels = std::move(parcels),
                              handlers = std::move(handlers)]() mutable {
        hpx::get_runtime_distributed().get_parcel_handler().put_parcels(
            std::move(parcels), std::move(handlers));
    });
    os_thread.join();

    hpx::future<void> sent_f = sent.get_future();
    HPX_TEST(
        sent_f.wait_for(std::chrono::seconds(10)) == hpx::future_status::ready);
    if (sent_f.is_ready())
    {
        sent_f.get();
        HPX_TEST(!got);
    }

    HPX_TEST(
        result.wait_for(std::chrono::seconds(10)) == hpx::future_status::ready);
    if (result.is_ready())
    {
        HPX_TEST_EQ(result.get(), id);
    }
}

///////////////////////////////////////////////////////////////////////////////
void print_counters(char const* name)
{
    using namespace hpx::performance_counters;

    hpx::error_code ec;
    for (performance_counter const& c : discover_counters(name, ec))
    {
        counter_value value = c.get_counter_value(hpx::launch::sync, ec);
        if (!ec)
        {
            std::cout << "counter: " << c.get_name(hpx::launch::sync)
                      << ", value: " << value.get_value<double>() << std::endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    auto seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test_put_parcel_unknown_locality_synchronous();
    test_put_parcel_unknown_locality_rescheduled();
    test_put_parcels_unknown_locality_rescheduled();
    test_put_parcel_rescheduled_reaches_destination();
    test_put_parcels_rescheduled_reaches_destination();

    for (hpx::id_type const& id : hpx::find_remote_localities())
    {
        test_plain_argument(id);
        test_future_argument(id);
        test_mixed_arguments(id);
        test_task_group_argument(id);
    }

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_COUNTERS)
    if (hpx::is_networking_enabled())
    {
        // compare number of parcels with number of messages generated
        print_counters("/parcels/count/*/sent");
        print_counters("/messages/count/*/sent");
    }
#endif

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("seed,s", value<unsigned int>(),
         "the random number generator seed to use for this run")
        ;
    // clang-format on

    // explicitly disable message handlers (parcel coalescing)
    std::vector<std::string> const cfg = {
#if defined(HPX_HAVE_NETWORKING)
        "hpx.parcel.message_handlers=0"
#endif
    };

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
