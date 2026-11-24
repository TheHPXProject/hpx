//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Exercise the AGAS address cache (gva cache): updating a cached range,
// rejecting colliding ranges, and removing entries. The ids used here belong
// to a locality that does not exist, as AGAS caches only ids managed by other
// localities.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/agas_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>

using hpx::agas::gva;
using hpx::naming::gid_type;

constexpr gva::component_type component_type = 1;

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    hpx::agas::addressing_service& agas = hpx::naming::get_agas_client();

    std::uint32_t const remote = hpx::get_locality_id() + 1;
    gid_type const prefix = hpx::naming::get_gid_from_locality_id(remote);
    gid_type const base =
        hpx::naming::replace_locality_id(gid_type(0, 0x1000), remote);

    std::uint64_t const entries = agas.get_cache_entries(false);

    // cache a range of four ids
    gva const first(prefix, component_type, 4, std::uint64_t(0x10000));
    agas.update_cache_entry(base, first);
    HPX_TEST_EQ(agas.get_cache_entries(false), entries + 1);

    gva g;
    gid_type idbase;
    HPX_TEST(agas.get_cache_entry(base + gid_type(0, 2), g, idbase));
    HPX_TEST(g == first);
    HPX_TEST_EQ(idbase, base);

    // updating the same range replaces the cached address
    gva const second(prefix, component_type, 4, std::uint64_t(0x20000));
    agas.update_cache_entry(base, second);
    HPX_TEST_EQ(agas.get_cache_entries(false), entries + 1);
    HPX_TEST(agas.get_cache_entry(base, g, idbase));
    HPX_TEST(g == second);

    // a single id inside the cached range collides with it and is rejected
    gva const inside(prefix, component_type, 1, std::uint64_t(0x30000));
    agas.update_cache_entry(base + gid_type(0, 2), inside);
    HPX_TEST_EQ(agas.get_cache_entries(false), entries + 1);
    HPX_TEST(agas.get_cache_entry(base + gid_type(0, 2), g, idbase));
    HPX_TEST(g == second);
    HPX_TEST_EQ(idbase, base);

    // a range with the same base but a different size collides as well
    gva const shorter(prefix, component_type, 2, std::uint64_t(0x40000));
    agas.update_cache_entry(base, shorter);
    HPX_TEST_EQ(agas.get_cache_entries(false), entries + 1);
    HPX_TEST(agas.get_cache_entry(base, g, idbase));
    HPX_TEST(g == second);

    // removing the range frees its cache entry
    agas.remove_cache_entry(base);
    HPX_TEST_EQ(agas.get_cache_entries(false), entries);
    HPX_TEST(!agas.get_cache_entry(base, g, idbase));

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#endif
