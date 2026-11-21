//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The GPU device data in a parcel is received on (receive_device in the input
// archive), without GPUs: types whose load() records the receive device stand
// in for device data.
//
// * Arguments: the GPU of the target of the destination component, from its
//   get_target() member function or from a specialization of
//   hpx::traits::component_target; -1 for components without a target. Set
//   for every parcel, several parcels may be decoded from one archive.
// * Results: the GPU hpx::detail::result_device() names where the call was
//   made (the default target's, with CUDA), only for result types with
//   device data (hpx::traits::has_device_data).

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Records the receive device of the archive it was loaded from, -2 if it
// wasn't loaded (the call was local).
template <bool DeviceData>
struct probe
{
    int device = -2;

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        ar << device;
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        int sent = 0;
        ar >> sent;

        auto const* receive_device = ar.template try_get_extra_data<
            hpx::serialization::detail::receive_device>();
        device = receive_device != nullptr ? receive_device->device : -1;
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

using plain_probe = probe<false>;
using device_probe = probe<true>;

template <>
struct hpx::traits::has_device_data<device_probe> : std::true_type
{
};

static_assert(hpx::traits::has_device_data_v<device_probe const&>);
static_assert(!hpx::traits::has_device_data_v<plain_probe>);

///////////////////////////////////////////////////////////////////////////////
// A target type the way hpx::cuda::experimental::target is one: its
// native_handle().get_device() is the GPU.
struct test_target
{
    struct native_handle_type
    {
        int device;

        int get_device() const noexcept
        {
            return device;
        }
    };

    native_handle_type const& native_handle() const noexcept
    {
        return handle;
    }

    native_handle_type handle;
};

// a target that stands for no GPU
struct host_only_target
{
};

///////////////////////////////////////////////////////////////////////////////
// The components report the receive device of their argument.

// a target from get_target()
struct targeted : hpx::components::component_base<targeted>
{
    targeted() = default;
    explicit targeted(int device)
      : target_{{device}}
    {
    }

    test_target const& get_target() const noexcept
    {
        return target_;
    }

    int arrived(plain_probe const& p)
    {
        return p.device;
    }
    plain_probe plain_result()
    {
        return plain_probe{};
    }
    device_probe device_result()
    {
        return device_probe{};
    }

    HPX_DEFINE_COMPONENT_ACTION(targeted, arrived)
    HPX_DEFINE_COMPONENT_ACTION(targeted, plain_result)
    HPX_DEFINE_COMPONENT_ACTION(targeted, device_result)

    test_target target_{{-1}};
};

// a target from a specialization of component_target
struct specialized : hpx::components::component_base<specialized>
{
    specialized() = default;
    explicit specialized(int device)
      : gpu(device)
    {
    }

    int arrived(plain_probe const& p)
    {
        return p.device;
    }

    HPX_DEFINE_COMPONENT_ACTION(specialized, arrived)

    int gpu = -1;
};

template <>
struct hpx::traits::component_target<specialized>
{
    static constexpr bool has_target = true;

    static test_target call(specialized const& c) noexcept
    {
        return test_target{{c.gpu}};
    }
};

// no target
struct untargeted : hpx::components::component_base<untargeted>
{
    int arrived(plain_probe const& p)
    {
        return p.device;
    }

    HPX_DEFINE_COMPONENT_ACTION(untargeted, arrived)
};

// a target standing for no GPU
struct host_targeted : hpx::components::component_base<host_targeted>
{
    host_only_target get_target() const noexcept
    {
        return {};
    }

    int arrived(plain_probe const& p)
    {
        return p.device;
    }

    HPX_DEFINE_COMPONENT_ACTION(host_targeted, arrived)
};

static_assert(hpx::traits::component_target<targeted>::has_target);
static_assert(hpx::traits::component_target<specialized>::has_target);
static_assert(!hpx::traits::component_target<untargeted>::has_target);
static_assert(hpx::traits::component_target<host_targeted>::has_target);

using targeted_type = hpx::components::component<targeted>;
HPX_REGISTER_COMPONENT(targeted_type, targeted)
HPX_REGISTER_ACTION(targeted::arrived_action, targeted_arrived_action)
HPX_REGISTER_ACTION(targeted::plain_result_action, targeted_plain_result_action)
HPX_REGISTER_ACTION(
    targeted::device_result_action, targeted_device_result_action)

using specialized_type = hpx::components::component<specialized>;
HPX_REGISTER_COMPONENT(specialized_type, specialized)
HPX_REGISTER_ACTION(specialized::arrived_action, specialized_arrived_action)

using untargeted_type = hpx::components::component<untargeted>;
HPX_REGISTER_COMPONENT(untargeted_type, untargeted)
HPX_REGISTER_ACTION(untargeted::arrived_action, untargeted_arrived_action)

using host_targeted_type = hpx::components::component<host_targeted>;
HPX_REGISTER_COMPONENT(host_targeted_type, host_targeted)
HPX_REGISTER_ACTION(host_targeted::arrived_action, host_targeted_arrived_action)

///////////////////////////////////////////////////////////////////////////////
// the GPU results are received on, on the calling locality
constexpr int result_gpu = 3;

int test_result_device() noexcept
{
    return result_gpu;
}

void test_arguments(hpx::id_type const& where)
{
    // components with targets of different GPUs on the same locality, the
    // parcels to them interleaved (several may arrive in one message)
    constexpr int gpus = 4;
    std::vector<hpx::id_type> targets;
    for (int k = 0; k != gpus; ++k)
    {
        targets.push_back(hpx::new_<targeted>(where, k).get());
    }
    hpx::id_type const special = hpx::new_<specialized>(where, 6).get();
    hpx::id_type const none = hpx::new_<untargeted>(where).get();
    hpx::id_type const host = hpx::new_<host_targeted>(where).get();

    std::vector<hpx::future<int>> arrived;
    std::vector<int> expected;
    for (int round = 0; round != 25; ++round)
    {
        for (int k = 0; k != gpus; ++k)
        {
            arrived.push_back(hpx::async<targeted::arrived_action>(
                targets[k], plain_probe{}));
            expected.push_back(k);
        }
        arrived.push_back(
            hpx::async<specialized::arrived_action>(special, plain_probe{}));
        expected.push_back(6);
        arrived.push_back(
            hpx::async<untargeted::arrived_action>(none, plain_probe{}));
        expected.push_back(-1);
        arrived.push_back(
            hpx::async<host_targeted::arrived_action>(host, plain_probe{}));
        expected.push_back(-1);
    }

    for (std::size_t i = 0; i != arrived.size(); ++i)
    {
        HPX_TEST_EQ(arrived[i].get(), expected[i]);
    }
}

void test_results(hpx::id_type const& where)
{
    // the target of the component doesn't matter for results
    hpx::id_type const target = hpx::new_<targeted>(where, 1).get();

    HPX_TEST_EQ(hpx::detail::result_device(), result_gpu);
    HPX_TEST_EQ(hpx::async<targeted::device_result_action>(target).get().device,
        result_gpu);

    // results without device data aren't received on a chosen GPU
    HPX_TEST_EQ(
        hpx::async<targeted::plain_result_action>(target).get().device, -1);
}

// the GPU of a target
void test_target_device()
{
    HPX_TEST_EQ(
        hpx::traits::target_device<test_target>::call(test_target{{5}}), 5);
    HPX_TEST_EQ(
        hpx::traits::target_device<host_only_target>::call(host_only_target{}),
        -1);
}

int hpx_main()
{
    test_target_device();

    std::vector<hpx::id_type> const remotes = hpx::find_remote_localities();
    HPX_TEST(!remotes.empty());
    if (!remotes.empty())
    {
        hpx::detail::set_result_device_function(&test_result_device);

        test_arguments(remotes[0]);
        test_results(remotes[0]);
    }
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
#endif
