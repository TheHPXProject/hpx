//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Segmented merge regressions for exceptions thrown by comparators and
// projections, locally and on remote localities, and for bad_alloc.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>

#include "partitioned_vector_merge_regressions.hpp"

using namespace merge_regression;

namespace {

    bool contains_only_expected_errors(std::exception_ptr const& error)
    {
        try
        {
            std::rethrow_exception(error);
        }
        catch (hpx::exception_list const& errors)
        {
            if (errors.size() == 0)
            {
                return false;
            }
            for (auto const& nested : errors)
            {
                if (!contains_only_expected_errors(nested))
                {
                    return false;
                }
            }
            return true;
        }
        catch (std::runtime_error const& error)
        {
            return std::string(error.what())
                       .find("segmented-merge-regression-error") !=
                std::string::npos;
        }
        catch (...)
        {
            return false;
        }
    }

    template <typename Policy, typename Start>
    void expect_failure(Policy const&, Start&& start, bool allocation_failure,
        bool check_exception_contents)
    {
        using policy_type = std::decay_t<Policy>;

        static constexpr bool is_sequenced =
            hpx::is_sequenced_execution_policy<policy_type>::value;
        bool returned_from_call = false;
        bool caught = false;
        try
        {
            auto result = start();
            returned_from_call = true;
            if constexpr (hpx::is_async_execution_policy_v<
                              std::decay_t<Policy>>)
            {
                HPX_TEST(result.valid());
                result.get();
            }
            else
            {
                (void) result;
            }
        }
        catch (std::bad_alloc const&)
        {
            caught = true;
            HPX_TEST(allocation_failure);
        }
        catch (hpx::exception_list const& errors)
        {
            caught = true;
            HPX_TEST(!allocation_failure);

            if constexpr (is_sequenced)
            {
                HPX_TEST_EQ(errors.size(), std::size_t{1});
            }
            else
            {
                HPX_TEST(errors.size() != 0);
            }

            if (check_exception_contents)
            {
                HPX_TEST(
                    contains_only_expected_errors(std::current_exception()));
            }
        }
        catch (std::exception const& error)
        {
            caught = true;
            std::fprintf(stderr,
                "[regression] unexpected direct exception: %s\n", error.what());
            HPX_TEST(false);
        }
        catch (...)
        {
            caught = true;
            HPX_TEST(false);
        }
        HPX_TEST(caught);
        if constexpr (hpx::is_async_execution_policy_v<std::decay_t<Policy>>)
        {
            // A user-function failure belongs in the future, not in the
            // initial seq(task)/par(task) call. No timing/!is_ready assumption.
            HPX_TEST(returned_from_call);
        }
    }

    struct exception_fixture
    {
        std::vector<hpx::id_type> localities = hpx::find_all_localities();
        hpx::id_type remote_locality;
        std::vector<int> input1;
        std::vector<int> input2;
        hpx::partitioned_vector<int> source1;
        hpx::partitioned_vector<int> source2;
        hpx::partitioned_vector<int> destination;

        exception_fixture()
        {
            for (auto const& locality : localities)
            {
                if (locality != hpx::find_here())
                {
                    remote_locality = locality;
                }
            }
            for (int i = 0; i != 16; ++i)
            {
                input1.push_back(2 * i);
                input2.push_back(2 * i + 1);
            }
            source1 =
                make_vector(input1, {8, 8}, {localities[0], localities[1]});
            source2 =
                make_vector(input2, {8, 8}, {localities[1], localities[0]});
            destination = make_vector(std::vector<int>(32, -1), {8, 8, 8, 8},
                {remote_locality, remote_locality, remote_locality,
                    remote_locality});
            HPX_TEST(hpx::find_here() != remote_locality);
        }

        template <typename Policy>
        void run(Policy policy, bool remote, bool projection,
            bool allocation_failure, int threshold)
        {
            std::fprintf(stderr,
                "[regression] exception-site=%s functor=%s "
                "kind=%s threshold=%d\n",
                remote ? "destination" : "coordinator",
                projection ? "projection" : "comparator",
                allocation_failure ? "bad_alloc" : "runtime_error", threshold);
            std::fflush(stderr);

            regression_throw_point point;
            point.locality = remote ? remote_locality : hpx::find_here();
            point.allocation_failure = allocation_failure;
            point.threshold = threshold;
            if (projection)
            {
                expect_failure(
                    policy,
                    [&] {
                        return hpx::ranges::merge(policy, source1.cbegin(),
                            source1.cend(), source2.cbegin(), source2.cend(),
                            destination.begin(), regression_int_less{},
                            regression_throwing_projection{point},
                            regression_throwing_projection{point});
                    },
                    allocation_failure, !remote);
            }
            else
            {
                expect_failure(
                    policy,
                    [&] {
                        return hpx::merge(policy, source1.cbegin(),
                            source1.cend(), source2.cbegin(), source2.cend(),
                            destination.begin(),
                            regression_throwing_compare{point});
                    },
                    allocation_failure, !remote);
            }
            // Output after failure is unspecified: do not require rollback.
            // Each task future is consumed before these containers are reused.
        }

        void check_inputs() const
        {
            check_values(source1, input1);
            check_values(source2, input2);
        }
    };

    void test_diagonal_comparator_exceptions()
    {
        exception_fixture fixture;
        for_each_policy([&](auto policy) {
            fixture.run(policy, false, false, false, 0);
            // k=8 succeeds; k=16 fails. Earlier chunks may already be launched.
            fixture.run(policy, false, false, false, 12);
        });
        fixture.check_inputs();
    }

    void test_diagonal_projection_exceptions()
    {
        exception_fixture fixture;
        for_each_policy(
            [&](auto policy) { fixture.run(policy, false, true, false, 12); });
        fixture.check_inputs();
    }

    void test_remote_functor_exceptions()
    {
        exception_fixture fixture;
        for_each_policy([&](auto policy) {
            // Caller-side comparisons succeed. Fail in a remote output chunk.
            fixture.run(policy, true, false, false, 12);
            fixture.run(policy, true, true, false, 12);
        });
        fixture.check_inputs();
    }

    void test_bad_alloc_propagation()
    {
        exception_fixture fixture;
        for_each_policy([&](auto policy) {
            for (bool remote : {false, true})
            {
                fixture.run(policy, remote, false, true, 12);
                fixture.run(policy, remote, true, true, 12);
            }
        });
        fixture.check_inputs();
    }
}    // namespace

int main()
{
    if (!has_three_localities())
    {
        return hpx::util::report_errors();
    }

    run_test("test_diagonal_comparator_exceptions",
        test_diagonal_comparator_exceptions);
    run_test("test_diagonal_projection_exceptions",
        test_diagonal_projection_exceptions);
    run_test("test_remote_functor_exceptions", test_remote_functor_exceptions);
    run_test("test_bad_alloc_propagation", test_bad_alloc_propagation);

    return hpx::util::report_errors();
}

#endif
