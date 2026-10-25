//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace {

    using hpx::threads::policies::numa_sensitive_mode;

    std::vector<std::size_t> get_domains(
        numa_sensitive_mode mode, bool is_boundary_worker)
    {
        constexpr std::array<std::array<std::size_t, 4>, 4> distances = {{
            {{0, 30, 10, 10}},
            {{30, 0, 20, 20}},
            {{10, 20, 0, 40}},
            {{10, 20, 40, 0}},
        }};

        return hpx::threads::policies::detail::get_numa_stealing_domains(mode,
            0, distances.size(), is_boundary_worker,
            [&distances](std::size_t from, std::size_t to) {
                return distances[from][to];
            });
    }

    void test_no_numa_sensitivity()
    {
        std::vector<std::size_t> const domains =
            get_domains(numa_sensitive_mode::none, false);

        HPX_TEST_EQ(domains.size(), std::size_t(3));
        HPX_TEST_EQ(domains[0], std::size_t(2));
        HPX_TEST_EQ(domains[1], std::size_t(3));
        HPX_TEST_EQ(domains[2], std::size_t(1));
    }

    void test_boundary_numa_sensitivity()
    {
        HPX_TEST(get_domains(numa_sensitive_mode::boundary, false).empty());

        std::vector<std::size_t> const domains =
            get_domains(numa_sensitive_mode::boundary, true);
        HPX_TEST_EQ(domains.size(), std::size_t(2));
        HPX_TEST_EQ(domains[0], std::size_t(2));
        HPX_TEST_EQ(domains[1], std::size_t(3));
    }

    void test_strict_numa_sensitivity()
    {
        HPX_TEST(get_domains(numa_sensitive_mode::strict, false).empty());
        HPX_TEST(get_domains(numa_sensitive_mode::strict, true).empty());
    }
}    // namespace

int main()
{
    test_no_numa_sensitivity();
    test_boundary_numa_sensitivity();
    test_strict_numa_sensitivity();

    return hpx::util::report_errors();
}
