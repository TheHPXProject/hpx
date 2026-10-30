//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Unit tests for the ring-walk fallback logic used while forwarding the
// Dijkstra termination detection token (see
// hpx::components::server::detail::dijkstra_forward_token). These tests
// exercise the pure decision logic through a mocked send callback, without
// requiring AGAS, parcels, or a running distributed runtime.

#include <hpx/config.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using hpx::components::server::detail::dijkstra_forward_token;
using hpx::components::server::detail::dijkstra_should_reprobe;

namespace {

    // Records every locality id that the mocked send was invoked with, in
    // call order, and returns a caller-supplied result for each target.
    struct mock_send
    {
        explicit mock_send(std::vector<std::uint32_t> live_targets)
          : live_targets_(HPX_MOVE(live_targets))
        {
        }

        bool operator()(std::uint32_t const target_locality_id)
        {
            calls_.push_back(target_locality_id);
            for (auto const live : live_targets_)
            {
                if (live == target_locality_id)
                {
                    succeeded_calls_.push_back(target_locality_id);
                    return true;
                }
            }
            return false;
        }

        std::vector<std::uint32_t> live_targets_;
        std::vector<std::uint32_t> calls_;
        std::vector<std::uint32_t> succeeded_calls_;
    };
}    // namespace

// 1. Happy path -- immediate neighbor alive.
void test_immediate_neighbor_alive()
{
    std::uint32_t locality_id = 4;
    constexpr std::uint32_t initiating_locality_id = 0;

    mock_send send({3});
    bool const result =
        dijkstra_forward_token(locality_id, initiating_locality_id, send);

    HPX_TEST(result);
    HPX_TEST_EQ(send.calls_.size(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(send.calls_[0], static_cast<std::uint32_t>(3));
    HPX_TEST_EQ(send.succeeded_calls_.size(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(send.succeeded_calls_[0], static_cast<std::uint32_t>(3));
    HPX_TEST_EQ(locality_id, static_cast<std::uint32_t>(3));
}

// 2. One dead intermediate locality -- the walk should skip it rather than
//    looping forever or stopping early.
void test_one_dead_intermediate_locality()
{
    std::uint32_t locality_id = 4;
    constexpr std::uint32_t initiating_locality_id = 0;

    mock_send send({2});
    bool const result =
        dijkstra_forward_token(locality_id, initiating_locality_id, send);

    HPX_TEST(result);
    std::vector<std::uint32_t> const expected_calls{3, 2};
    HPX_TEST(send.calls_ == expected_calls);
    std::vector<std::uint32_t> const expected_succeeded_calls{2};
    HPX_TEST(send.succeeded_calls_ == expected_succeeded_calls);
    HPX_TEST_EQ(locality_id, static_cast<std::uint32_t>(2));
}

// 3. All intermediates dead down to the initiator -- the walk returns false
//    once it reaches the initiating locality, leaving the fallback send to the
//    initiator to the caller. Note that the initiator is the walk's own last
//    candidate here (the calls end at 0), and this mock fails it like any
//    other; in production that send is local and succeeds, which is why the
//    return value alone cannot tell the caller whether the token left this
//    locality. See test 9.
void test_all_intermediates_dead_reaches_initiator()
{
    std::uint32_t locality_id = 4;
    constexpr std::uint32_t initiating_locality_id = 0;

    mock_send send({});
    bool const result =
        dijkstra_forward_token(locality_id, initiating_locality_id, send);

    HPX_TEST(!result);
    std::vector<std::uint32_t> const expected_calls{3, 2, 1, 0};
    HPX_TEST(send.calls_ == expected_calls);
    HPX_TEST(send.succeeded_calls_.empty());
    HPX_TEST_EQ(locality_id, initiating_locality_id);

    // caller-side fallback: send directly to the initiator exactly once
    mock_send fallback_send({});
    bool const fallback_result = fallback_send(initiating_locality_id);
    HPX_TEST(!fallback_result);
    HPX_TEST_EQ(fallback_send.calls_.size(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(fallback_send.calls_[0], initiating_locality_id);
}

// 4. Fallback target itself unreachable -- the walk truthfully reports that
//    nobody along the ring accepted the token, and
//    runtime_support::dijkstra_termination then retries the direct send to the
//    initiator max_fallback_attempts times before logging that the initiator
//    will never see this token. Model that retry so the attempt count stays
//    pinned to the production one.
void test_fallback_to_initiator_also_unreachable()
{
    constexpr int max_fallback_attempts = 3;

    std::uint32_t locality_id = 4;
    constexpr std::uint32_t initiating_locality_id = 0;

    mock_send send({});
    bool const result =
        dijkstra_forward_token(locality_id, initiating_locality_id, send);
    HPX_TEST(!result);

    // the fallback send to the initiator fails on every attempt
    mock_send fallback_send({});
    bool fallback_sent = false;
    for (int attempt = 0; !fallback_sent && attempt != max_fallback_attempts;
        ++attempt)
    {
        fallback_sent = fallback_send(initiating_locality_id);
    }

    HPX_TEST(!fallback_sent);
    HPX_TEST_EQ(fallback_send.calls_.size(),
        static_cast<std::size_t>(max_fallback_attempts));
    for (std::uint32_t const call : fallback_send.calls_)
    {
        HPX_TEST_EQ(call, initiating_locality_id);
    }
    HPX_TEST(fallback_send.succeeded_calls_.empty());
}

// 5. Wrap-around when locality_id == 0 is handled by the caller before
//    invoking dijkstra_forward_token; verify the first probed target is
//    num_localities - 1.
void test_wrap_around_before_forwarding()
{
    constexpr std::uint32_t num_localities = 5;
    std::uint32_t locality_id = 0;
    if (0 == locality_id)
        locality_id = num_localities;

    constexpr std::uint32_t initiating_locality_id = 0;

    mock_send send({});
    bool const result =
        dijkstra_forward_token(locality_id, initiating_locality_id, send);

    HPX_TEST(!result);
    HPX_TEST(!send.calls_.empty());
    HPX_TEST_EQ(send.calls_[0], num_localities - 1);
    HPX_TEST(send.succeeded_calls_.empty());
}

// 6. No forwarding when already at the initiator -- guards against an
//    accidental self-send loop.
void test_no_forwarding_when_already_at_initiator()
{
    std::uint32_t locality_id = 2;
    constexpr std::uint32_t initiating_locality_id = 2;

    mock_send send({});
    bool const result =
        dijkstra_forward_token(locality_id, initiating_locality_id, send);

    HPX_TEST(!result);
    HPX_TEST(send.calls_.empty());
    HPX_TEST(send.succeeded_calls_.empty());
}

// 7. Regression for the --hpx:threads=1 termination-detection livelock
//    (dijkstra_termination_detection). The retry loop restarts the ring cursor
//    for every probe; dijkstra_forward_token consumes it (walks it backwards
//    in-place), so a probe that resumes from the leftover value walks past the
//    remaining localities and the initiator ends up handing the token to
//    itself, which can never terminate. These cases pin the two-probe contract
//    the loop must uphold, exercised through the real forwarding helper.
void test_repeated_probe_restarts_cursor()
{
    constexpr std::uint32_t initiating_locality_id = 0;
    constexpr std::uint32_t num_localities = 2;

    // The initiator's predecessor in the ring, recomputed at the start of each
    // probe (this mirrors dijkstra_termination_detection).
    auto const probe_start = [](std::uint32_t const initiating,
                                 std::uint32_t const n) {
        std::uint32_t target = initiating;
        if (0 == target)
            target = n;
        return target;
    };

    // Restarting the cursor for each of two probes reaches N-1 every time and
    // never targets the initiator.
    {
        mock_send send({num_localities - 1});    // only the neighbor is alive
        for (int probe = 0; probe != 2; ++probe)
        {
            std::uint32_t target =
                probe_start(initiating_locality_id, num_localities);
            HPX_TEST(
                dijkstra_forward_token(target, initiating_locality_id, send));
        }
        std::vector<std::uint32_t> const expected_calls = {
            num_localities - 1, num_localities - 1};
        HPX_TEST(send.calls_ == expected_calls);
        for (std::uint32_t const call : send.calls_)
        {
            HPX_TEST_NEQ(call, initiating_locality_id);
        }
    }

    // The failure mode being guarded against: reusing the cursor consumed by
    // the first probe makes the second walk fall straight through to the
    // initiator, i.e. the token is handed to the initiator itself.
    {
        mock_send send({num_localities - 1});
        std::uint32_t target =
            probe_start(initiating_locality_id, num_localities);
        HPX_TEST(dijkstra_forward_token(target, initiating_locality_id, send));
        HPX_TEST_EQ(target, num_localities - 1);    // cursor consumed

        bool const reused =
            dijkstra_forward_token(target, initiating_locality_id, send);
        HPX_TEST(!reused);
        HPX_TEST_EQ(target, initiating_locality_id);
    }
}

// 8. Regression for the unbounded retry when the token cannot be handed to any
//    locality (dijkstra_termination_detection). An undeliverable probe marks
//    the initiator black again, so before this bound the retry loop reprobed
//    forever and shutdown burned the whole job wall clock instead of failing
//    with a diagnostic. dijkstra_should_reprobe is the loop's real exit
//    condition, so these cases pin it directly.
void test_undeliverable_probes_are_bounded()
{
    constexpr std::size_t max_probes = 3;

    // A white initiator terminates regardless of the count.
    HPX_TEST(!dijkstra_should_reprobe(false, 0, max_probes));
    HPX_TEST(!dijkstra_should_reprobe(false, max_probes, max_probes));

    // A black initiator keeps probing while probes are still being delivered.
    HPX_TEST(dijkstra_should_reprobe(true, 0, max_probes));
    HPX_TEST(dijkstra_should_reprobe(true, max_probes - 1, max_probes));

    // Once the bound is reached the loop stops even though the initiator is
    // still black. This is the case that used to spin forever.
    HPX_TEST(!dijkstra_should_reprobe(true, max_probes, max_probes));

    // Rule 3 repetition stays unbounded: an undeliverable count that never
    // grows keeps reprobing however many probes have run.
    for (std::size_t probe = 0; probe != 100; ++probe)
    {
        HPX_TEST(dijkstra_should_reprobe(true, 0, max_probes));
    }
}

// 9. The loop drives that predicate off a real ring walk. The initiator is the
//    walk's own last candidate when it is locality 0, because the ring wraps
//    through it, and that send is local and always succeeds. A broken ring
//    therefore still reports a successful handoff, so the bound has to count
//    probes that reached no other locality rather than probes that failed to
//    send. The bound then only has to fire for a busy initiator, since a
//    passive one settles termination on the first probe: if nothing else is
//    reachable, nothing else can still be running. These cases pin that down.
void test_bound_reached_only_when_nothing_is_reachable()
{
    constexpr std::uint32_t initiating_locality_id = 0;
    constexpr std::uint32_t num_localities = 4;
    constexpr std::size_t max_probes = 3;

    // Whether the token comes back black is decided by the local thread
    // manager inside send_dijkstra_termination_token, not by the ring walk, so
    // it is an input here rather than something inferred from reachability.
    auto const run_probes = [&](mock_send& send, bool const initiator_busy) {
        std::size_t undeliverable = 0;
        std::size_t probes = 0;
        bool initiator_black = false;
        do
        {
            // The initiator's predecessor, recomputed per probe as the retry
            // loop does.
            std::uint32_t target = initiating_locality_id;
            if (0 == target)
                target = num_localities;

            bool reached_other_locality = false;
            bool token_sent =
                dijkstra_forward_token(target, initiating_locality_id,
                    [&](std::uint32_t const target_locality_id) {
                        bool const sent = send(target_locality_id);
                        reached_other_locality |= sent &&
                            target_locality_id != initiating_locality_id;
                        return sent;
                    });

            if (!token_sent)
            {
                token_sent = send(initiating_locality_id);
            }

            if (reached_other_locality)
            {
                undeliverable = 0;
            }
            else
            {
                ++undeliverable;
            }

            initiator_black = initiator_busy;
            ++probes;
        } while (dijkstra_should_reprobe(
            initiator_black, undeliverable, max_probes));

        return probes;
    };

    {
        mock_send send({});    // nothing reachable at all
        HPX_TEST_EQ(run_probes(send, true), max_probes);
        HPX_TEST(send.succeeded_calls_.empty());
        HPX_TEST(!send.calls_.empty());
    }

    {
        // The case that actually happens: the ring is broken, but the walk's
        // last hop and the fallback both target the initiator, whose own send
        // is local and cannot fail. Every probe reports a successful handoff
        // and must still count as undeliverable, or a busy initiator reprobes
        // forever.
        mock_send send({initiating_locality_id});
        HPX_TEST_EQ(run_probes(send, true), max_probes);
        HPX_TEST(!send.succeeded_calls_.empty());
        for (std::uint32_t const call : send.succeeded_calls_)
        {
            HPX_TEST_EQ(call, initiating_locality_id);
        }
    }

    {
        // Same broken ring, but this locality has gone passive. Nothing else
        // is reachable, so nothing else can still be running, and the white
        // token settles termination on the first probe. The bound is not
        // needed here and must not delay shutdown.
        mock_send send({initiating_locality_id});
        HPX_TEST_EQ(run_probes(send, false), static_cast<std::size_t>(1));
    }

    {
        mock_send send({num_localities - 1});    // intact ring
        HPX_TEST_EQ(run_probes(send, false), static_cast<std::size_t>(1));
        std::vector<std::uint32_t> const expected = {num_localities - 1};
        HPX_TEST(send.succeeded_calls_ == expected);
    }
}

int main()
{
    test_immediate_neighbor_alive();
    test_one_dead_intermediate_locality();
    test_all_intermediates_dead_reaches_initiator();
    test_fallback_to_initiator_also_unreachable();
    test_wrap_around_before_forwarding();
    test_no_forwarding_when_already_at_initiator();
    test_repeated_probe_restarts_cursor();
    test_undeliverable_probes_are_bounded();
    test_bound_reached_only_when_nothing_is_reachable();

    return hpx::util::report_errors();
}
