//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks that the thread created for an action has the size given to the
// action with HPX_ACTION_HAS_TASK_SIZE, or the size of a function given one
// with hpx::threads::with_task_size and bound to the action.
// HPX_ACTION_HAS_TASK_SIZE overrides the size of such a function. A scheduler
// that counts the sizes it is asked to create threads with sees it.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/functional.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

using hpx::threads::task_size;

namespace {

    std::array<std::atomic<int>, 4> created_with{};

    using base_scheduler =
        hpx::threads::policies::local_priority_queue_scheduler<>;

    struct counting_scheduler : base_scheduler
    {
        using base_scheduler::base_scheduler;

        void create_thread(hpx::threads::thread_init_data& data,
            hpx::threads::thread_id_ref_type* id, hpx::error_code& ec) override
        {
            ++created_with[static_cast<std::size_t>(
                data.schedulehint.task_size())];
            base_scheduler::create_thread(data, id, ec);
        }
    };

    // Runs f and checks how many threads it created with each known size.
    // Threads created by the runtime itself have no size, so they don't count.
    template <typename F>
    void check_sizes(F&& f, int small_, int medium, int large)
    {
        auto const count = [](task_size s) {
            return created_with[static_cast<std::size_t>(s)].load();
        };
        int const small_before = count(task_size::small_);
        int const medium_before = count(task_size::medium);
        int const large_before = count(task_size::large);

        HPX_FORWARD(F, f)();

        HPX_TEST_EQ(count(task_size::small_) - small_before, small_);
        HPX_TEST_EQ(count(task_size::medium) - medium_before, medium);
        HPX_TEST_EQ(count(task_size::large) - large_before, large);
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
// a function without a size
int plain()
{
    return 42;
}
HPX_PLAIN_ACTION(plain, plain_action)

// an action given a size
int medium_function()
{
    return 42;
}
HPX_PLAIN_ACTION(medium_function, medium_action)
HPX_ACTION_HAS_TASK_SIZE(medium_action, task_size::medium)

// a function given a size with with_task_size, bound to an action
int sized()
{
    return 42;
}

struct sized_action
  : hpx::actions::make_action_t<decltype(hpx::small_task(&sized)),
        hpx::small_task(&sized), sized_action>
{
};
HPX_REGISTER_ACTION_DECLARATION(sized_action, sized_action)
HPX_REGISTER_ACTION(sized_action, sized_action)

// an action given a size, which overrides the one of its function
struct overriding_action
  : hpx::actions::make_action_t<decltype(hpx::small_task(&sized)),
        hpx::small_task(&sized), overriding_action>
{
};
HPX_REGISTER_ACTION_DECLARATION(overriding_action, overriding_action)
HPX_REGISTER_ACTION(overriding_action, overriding_action)
HPX_ACTION_HAS_TASK_SIZE(overriding_action, task_size::large)

// a direct action doesn't create a thread
HPX_PLAIN_DIRECT_ACTION(medium_function, medium_direct_action)
HPX_ACTION_HAS_TASK_SIZE(medium_direct_action, task_size::medium)

///////////////////////////////////////////////////////////////////////////////
// a component action given a size
struct server : hpx::components::component_base<server>
{
    int get() const
    {
        return 42;
    }

    HPX_DEFINE_COMPONENT_ACTION(server, get, get_action)
};

using server_type = hpx::components::component<server>;
HPX_REGISTER_COMPONENT(server_type, server)

HPX_REGISTER_ACTION_DECLARATION(server::get_action, server_get_action)
HPX_REGISTER_ACTION(server::get_action, server_get_action)
HPX_ACTION_HAS_TASK_SIZE(server::get_action, task_size::large)

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_CXX26_REFLECTION)
// a reflection based action given a size
int reflected()
{
    return 42;
}
using reflected_action = hpx::actions::reflect_action<^^reflected>;
HPX_ACTION_HAS_TASK_SIZE(reflected_action, task_size::medium)
#endif

///////////////////////////////////////////////////////////////////////////////
void test_plain_actions()
{
    hpx::id_type const here = hpx::find_here();

    check_sizes([&] { HPX_TEST_EQ(hpx::async<plain_action>(here).get(), 42); },
        0, 0, 0);
    check_sizes([&] { HPX_TEST_EQ(hpx::async<medium_action>(here).get(), 42); },
        0, 1, 0);
    check_sizes(
        [&] { HPX_TEST_EQ(hpx::async<overriding_action>(here).get(), 42); }, 0,
        0, 1);
    check_sizes([&] { HPX_TEST_EQ(hpx::async<sized_action>(here).get(), 42); },
        1, 0, 0);
    check_sizes(
        [&] { HPX_TEST_EQ(hpx::async<medium_direct_action>(here).get(), 42); },
        0, 0, 0);

    // hpx::post creates the thread the same way
    check_sizes([&] { hpx::post<medium_action>(here); }, 0, 1, 0);

    static_assert(
        hpx::traits::action_task_size_v<plain_action> == task_size::unknown);
    static_assert(
        hpx::traits::action_task_size_v<medium_action> == task_size::medium);
    static_assert(
        hpx::traits::action_task_size_v<overriding_action> == task_size::large);
    static_assert(
        hpx::traits::action_task_size_v<sized_action> == task_size::small_);
}

void test_component_actions()
{
    hpx::id_type const id = hpx::new_<server>(hpx::find_here()).get();

    check_sizes(
        [&] { HPX_TEST_EQ(hpx::async<server::get_action>(id).get(), 42); }, 0,
        0, 1);
}

void test_reflected_actions()
{
#if defined(HPX_HAVE_CXX26_REFLECTION)
    hpx::id_type const here = hpx::find_here();

    check_sizes(
        [&] { HPX_TEST_EQ(hpx::async<reflected_action>(here).get(), 42); }, 0,
        1, 0);
#endif
}

int hpx_main(int, char*[])
{
    test_plain_actions();
    test_component_actions();
    test_reflected_actions();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::init_params init_args;
    init_args.rp_callback = [](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            [](hpx::threads::thread_pool_init_parameters thread_pool_init,
                hpx::threads::policies::thread_queue_init_parameters
                    thread_queue_init)
                -> std::unique_ptr<hpx::threads::thread_pool_base> {
                counting_scheduler::init_parameter_type init(
                    thread_pool_init.num_threads_,
                    thread_pool_init.affinity_data_, std::size_t(-1),
                    thread_queue_init);
                auto scheduler = std::make_unique<counting_scheduler>(init);

                return std::make_unique<hpx::threads::detail::
                        scheduled_thread_pool<counting_scheduler>>(
                    std::move(scheduler), thread_pool_init);
            });
    };

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
#endif
