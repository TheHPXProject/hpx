//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct partition_hooks_parameters
{
    explicit partition_hooks_parameters() = default;

    template <typename Executor>
    friend void tag_override_invoke(
        hpx::execution::experimental::collect_execution_parameters_t,
        partition_hooks_parameters& self, Executor&&, std::size_t const,
        std::size_t const, std::size_t const num_chunks,
        std::size_t const) noexcept
    {
        self.num_chunks_ = num_chunks;
        self.values_.resize(num_chunks);
        self.seen_.resize(num_chunks);
    }

    template <typename Executor>
    friend void tag_override_invoke(
        hpx::execution::experimental::mark_partition_t,
        partition_hooks_parameters& self, Executor&&, std::size_t partition,
        auto state, std::size_t a, std::size_t b, auto) noexcept
    {
        HPX_TEST_LT(partition, self.values_.size());
        HPX_TEST_EQ(std::uint8_t(1), static_cast<std::uint8_t>(state));
        self.values_[partition] = {a, b};
        self.seen_[partition] = 1;
    }

    std::size_t count_seen() const
    {
        std::size_t c = 0;
        for (auto v : seen_)
            c += (v != 0);
        return c;
    }

    std::vector<std::pair<std::size_t, std::size_t>> values_;
    std::vector<unsigned char> seen_;
    std::size_t num_chunks_ = 0;
};

namespace hpx::execution::experimental {
    template <>
    struct is_executor_parameters<partition_hooks_parameters> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

void test_mark_partition_sync()
{
    std::vector<std::uint64_t> left(200000);
    std::vector<std::uint64_t> right(200000);
    std::vector<std::uint64_t> out(left.size() + right.size());

    std::iota(left.begin(), left.end(), std::uint64_t(0));
    std::iota(right.begin(), right.end(), std::uint64_t(left.size()));

    partition_hooks_parameters params;

    auto policy = hpx::execution::par.with(std::ref(params));
    hpx::merge(policy, left.begin(), left.end(), right.begin(), right.end(),
        out.begin());

    HPX_TEST(std::is_sorted(out.begin(), out.end()));
    HPX_TEST_LT(std::size_t(0), params.num_chunks_);
    HPX_TEST_EQ(params.count_seen(), params.num_chunks_);
}

void test_mark_partition_async()
{
    std::vector<std::uint64_t> left(200000);
    std::vector<std::uint64_t> right(200000);
    std::vector<std::uint64_t> out(left.size() + right.size());

    std::iota(left.begin(), left.end(), std::uint64_t(0));
    std::iota(right.begin(), right.end(), std::uint64_t(left.size()));

    partition_hooks_parameters params;

    auto policy =
        hpx::execution::par(hpx::execution::task).with(std::ref(params));
    auto f = hpx::merge(policy, left.begin(), left.end(), right.begin(),
        right.end(), out.begin());
    auto result_iter = f.get();
    HPX_UNUSED(result_iter);

    HPX_TEST(std::is_sorted(out.begin(), out.end()));
    HPX_TEST_LT(std::size_t(0), params.num_chunks_);
    HPX_TEST_EQ(params.count_seen(), params.num_chunks_);
}

int hpx_main()
{
    test_mark_partition_sync();
    test_mark_partition_async();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
