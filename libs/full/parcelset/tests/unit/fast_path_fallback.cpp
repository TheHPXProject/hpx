//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// With hpx.parcel.fast_path=1 a parcel is encoded once, without the
// preprocessing pass. Arguments that need that pass (a managed id_type, a
// future that is not ready) must stop that encode and take the usual path.
// This test sends such arguments to another locality and checks the results.
// With --fast-path it also checks that some parcels skipped the preprocessing
// pass, without it that none did.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::int32_t add(std::int32_t a, std::int32_t b)
{
    return a + b;
}
HPX_PLAIN_ACTION(add)

std::int32_t add_future(hpx::shared_future<std::int32_t> f, std::int32_t b)
{
    return f.get() + b;
}
HPX_PLAIN_ACTION(add_future)

///////////////////////////////////////////////////////////////////////////////
// counts the saves of a value on this locality: a parcel on the usual path
// saves it twice (the preprocessing pass first), on the fast path once
std::atomic<int> saves(0);
std::atomic<int> preprocessing_saves(0);

struct counted
{
    std::int32_t value = 0;

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        ++(ar.is_preprocessing() ? preprocessing_saves : saves);
        ar << value;
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        ar >> value;
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

std::int32_t add_counted(counted c, std::int32_t b)
{
    return c.value + b;
}
HPX_PLAIN_ACTION(add_counted)

///////////////////////////////////////////////////////////////////////////////
struct value_server : hpx::components::component_base<value_server>
{
    std::int32_t get_value() const
    {
        return 42;
    }

    HPX_DEFINE_COMPONENT_ACTION(value_server, get_value)
};

using server_type = hpx::components::component<value_server>;
HPX_REGISTER_COMPONENT(server_type, value_server)

using get_value_action = value_server::get_value_action;
HPX_REGISTER_ACTION_DECLARATION(get_value_action)
HPX_REGISTER_ACTION(get_value_action)

// receives a managed id_type as an argument and uses it
std::int32_t call_through(hpx::id_type const& id)
{
    return hpx::async<get_value_action>(id).get();
}
HPX_PLAIN_ACTION(call_through)

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    hpx::id_type there = hpx::find_here();
    bool const remote = hpx::get_num_localities(hpx::launch::sync) > 1;
    if (remote)
    {
        there = hpx::find_remote_localities()[0];
    }

    // one parcel at a time, so the destination is free for each of them
    if (remote)
    {
        for (std::int32_t i = 0; i != 20; ++i)
        {
            HPX_TEST_EQ(
                hpx::async<add_counted_action>(there, counted{i}, 1).get(),
                i + 1);
        }

        HPX_TEST_EQ(saves.load(), 20);
        if (vm.count("fast-path") != 0)
        {
            HPX_TEST_LT(preprocessing_saves.load(), saves.load());
        }
        else if (hpx::get_config_entry("hpx.parcel.fast_path", "0") == "0")
        {
            HPX_TEST_EQ(preprocessing_saves.load(), saves.load());
        }
    }

    // plain values: no stop
    {
        std::vector<hpx::future<std::int32_t>> results;
        for (std::int32_t i = 0; i != 100; ++i)
        {
            results.push_back(hpx::async<add_action>(there, i, 1));
        }
        for (std::int32_t i = 0; i != 100; ++i)
        {
            HPX_TEST_EQ(results[i].get(), i + 1);
        }
    }

    // a ready future: no stop
    {
        hpx::shared_future<std::int32_t> f = hpx::make_ready_future(7);
        HPX_TEST_EQ(hpx::async<add_future_action>(there, f, 1).get(), 8);
    }

    // a future that is not ready: the encode stops, the parcel waits
    {
        hpx::promise<std::int32_t> p;
        hpx::shared_future<std::int32_t> f = p.get_future();

        hpx::future<std::int32_t> result =
            hpx::async<add_future_action>(there, f, 1);

        hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        HPX_TEST(!result.is_ready());

        p.set_value(41);
        HPX_TEST_EQ(result.get(), 42);
    }

    // a managed id_type as an argument: the encode stops before the credit
    // split; 64 sends use up the credits, so later splits ask AGAS
    {
        hpx::id_type id = hpx::components::new_<value_server>(there).get();

        std::vector<hpx::future<std::int32_t>> results;
        for (int i = 0; i != 64; ++i)
        {
            results.push_back(hpx::async<call_through_action>(there, id));
        }
        for (auto& r : results)
        {
            HPX_TEST_EQ(r.get(), 42);
        }

        // the object is still reachable with the id that was split
        HPX_TEST_EQ(hpx::async<get_value_action>(id).get(), 42);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("fast-path", "expect the fast path to send some of the parcels")
        ;
    // clang-format on

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, init_args), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
#endif
