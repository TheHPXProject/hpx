//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The steps de-serialization defers until zero-copy chunk data has arrived
// (post_receive_actions), as the parcels de-serialized one after the other
// from an archive take them: a parcel completes once its own steps and those
// of the parcels before it have finished, since objects shared through
// pointers are loaded only by the first parcel referring to them.

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using hpx::serialization::detail::post_receive_actions;
using hpx::serialization::detail::receive_completion;

namespace {

    // a step that finishes when the test says so
    struct controlled_step
    {
        std::shared_ptr<post_receive_actions::done_type> done =
            std::make_shared<post_receive_actions::done_type>();

        post_receive_actions::action_type action() const
        {
            return [done = done](post_receive_actions::done_type d) {
                *done = HPX_MOVE(d);
            };
        }

        void finish() const
        {
            HPX_TEST(static_cast<bool>(*done));
            (*done)();
        }
    };

    // what the parcel does with its steps (see parcel::on_receive_complete)
    struct parcel
    {
        explicit parcel(post_receive_actions& actions)
          : steps(actions.take_parcel_steps())
        {
        }

        void start()
        {
            if (!steps.completion)
            {
                HPX_TEST(steps.actions.empty());
                complete = true;
                return;
            }
            steps.completion->when_done([this]() { complete = true; });
            for (auto& action : steps.actions)
            {
                action([c = steps.completion]() { c->step_done(); });
            }
        }

        post_receive_actions::parcel_steps steps;
        bool complete = false;
    };

    void test_no_steps()
    {
        post_receive_actions actions;

        parcel p1(actions);
        parcel p2(actions);
        HPX_TEST(!p1.steps.completion);
        HPX_TEST(!p2.steps.completion);

        p1.start();
        p2.start();
        HPX_TEST(p1.complete);
        HPX_TEST(p2.complete);
    }

    void test_shared_steps()
    {
        post_receive_actions actions;

        // the first parcel loads an object and defers completing it
        controlled_step s1;
        actions.add(s1.action());
        parcel p1(actions);

        // the second refers to the same object, it isn't loaded again
        parcel p2(actions);

        // the third loads another object, completed by a step of its own
        controlled_step s3;
        actions.add(s3.action());
        parcel p3(actions);

        HPX_TEST_EQ(p1.steps.actions.size(), std::size_t(1));
        HPX_TEST(p2.steps.actions.empty());
        HPX_TEST_EQ(p3.steps.actions.size(), std::size_t(1));
        HPX_TEST(p2.steps.completion == p1.steps.completion);
        HPX_TEST(p3.steps.completion != p1.steps.completion);

        p1.start();
        p2.start();
        p3.start();
        HPX_TEST(!p1.complete);
        HPX_TEST(!p2.complete);
        HPX_TEST(!p3.complete);

        // the third parcel's own step: it still waits for the first one
        s3.finish();
        HPX_TEST(!p1.complete);
        HPX_TEST(!p2.complete);
        HPX_TEST(!p3.complete);

        s1.finish();
        HPX_TEST(p1.complete);
        HPX_TEST(p2.complete);
        HPX_TEST(p3.complete);

        // waiting for completed steps calls right away
        bool called = false;
        p3.steps.completion->when_done([&called]() { called = true; });
        HPX_TEST(called);
    }

    void test_steps_finished_in_order()
    {
        post_receive_actions actions;

        controlled_step s1;
        actions.add(s1.action());
        parcel p1(actions);

        controlled_step s2;
        actions.add(s2.action());
        parcel p2(actions);

        p1.start();
        p2.start();

        s1.finish();
        HPX_TEST(p1.complete);
        HPX_TEST(!p2.complete);

        s2.finish();
        HPX_TEST(p2.complete);
    }
}    // namespace

int main()
{
    test_no_steps();
    test_shared_steps();
    test_steps_finished_in_order();

    return hpx::util::report_errors();
}
