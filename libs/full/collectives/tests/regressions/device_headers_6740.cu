//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for GitHub issue #6740: a CUDA translation unit must be able
// to name HPX types and functions from the collectives, component-factory,
// coalescing-registration, colocated-async, colocated-async-callback,
// colocated-post, colocated-post-callback, base-LCO and distributed-channel
// APIs in host member functions of a class that also owns a __global__ kernel.
// Compile-only smoke test; a successful build is the pass.

#include <hpx/collectives.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/lcos_distributed.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/runtime_configuration.hpp>

__global__ void k() {}

// 1. Collectives shape - spans the headers whose whole-file guards were
//    removed: create_communicator, barrier, broadcast, all_reduce, all_gather,
//    scatter.
struct collectives_shape
{
    void host_side()
    {
        hpx::collectives::communicator c;
        (void) c;

        using hpx::collectives::all_gather;
        using hpx::collectives::all_reduce;
        using hpx::collectives::barrier;
        using hpx::collectives::broadcast_to;
        using hpx::collectives::scatter_to;
    }
    void launch()
    {
        k<<<1, 1>>>();
    }
};

// 2. component_factory_base shape - small guard used to hide the forward
//    declaration of the struct on the device pass.
struct component_factory_base_shape
{
    void host_side()
    {
        hpx::components::component_factory_base* p = nullptr;
        (void) p;
    }
    void launch()
    {
        k<<<1, 1>>>();
    }
};

// 3. coalescing_message_handler_registration shape - the compound guard on
//    the header dropped its HPX_COMPUTE_DEVICE_CODE clause. The names below
//    only exist when parcel coalescing and networking are on, so wrap the
//    reference in the same conditions the header uses.
#if defined(HPX_HAVE_PARCEL_COALESCING) && defined(HPX_HAVE_NETWORKING)
struct coalescing_registration_shape
{
    void host_side()
    {
        hpx::parcelset::register_coalescing_for_action<int>* p = nullptr;
        (void) p;
    }
    void launch()
    {
        k<<<1, 1>>>();
    }
};
#endif

// The four <hpx/async_colocated/...> headers expose only function templates,
// so there is no non-template surface a host member could name. The four
// includes above are the check - guard removal is proven by those headers
// parsing on device.

// 4. base_lco_with_value shape - the narrow guard around the set_value_action
//    alias was removed, since base_lco::set_event_action now exists as an
//    empty struct on device.
struct base_lco_with_value_shape
{
    void host_side()
    {
        hpx::lcos::base_lco_with_value<int, int>* p = nullptr;
        (void) p;
    }
    void launch()
    {
        k<<<1, 1>>>();
    }
};

// 5. distributed channel shape - the whole-file guard on channel.hpp was
//    removed. The class template bodies reference server-side action
//    typedefs (get_generation_action, set_generation_action, close_action)
//    that now exist as empty structs on the device pass.
struct channel_shape
{
    void host_side()
    {
        hpx::lcos::channel<int>* p = nullptr;
        (void) p;
    }
    void launch()
    {
        k<<<1, 1>>>();
    }
};

int main()
{
    return 0;
}
