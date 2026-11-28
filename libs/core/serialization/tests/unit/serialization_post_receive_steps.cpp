//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The steps de-serialization defers until zero-copy chunk data has arrived
// (post_receive_actions), as the parcels de-serialized one after the other
// from an archive take them: a parcel completes once its own steps have
// finished, and if it uses objects that parcels before it have loaded (shared
// through pointers, loaded only by the first parcel referring to them), once
// the steps of those parcels have finished as well.

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/serialization/shared_ptr.hpp>

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
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

        void fail() const
        {
            HPX_TEST(static_cast<bool>(*done));
            (*done)(std::make_exception_ptr(std::runtime_error("step")));
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
            steps.completion->when_done([this]() {
                complete = true;
                failed = static_cast<bool>(steps.completion->error());
            });
            for (auto& action : steps.actions)
            {
                action([c = steps.completion](std::exception_ptr error) {
                    c->step_done(HPX_MOVE(error));
                });
            }
        }

        post_receive_actions::parcel_steps steps;
        bool complete = false;
        bool failed = false;
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
        actions.object_loaded(10);
        actions.add(s1.action());
        parcel p1(actions);

        // the second refers to the same object, it isn't loaded again
        actions.back_reference(10);
        parcel p2(actions);

        // the third loads another object, completed by a step of its own
        controlled_step s3;
        actions.object_loaded(20);
        actions.add(s3.action());
        parcel p3(actions);

        // the fourth refers to the third one's object
        actions.back_reference(20);
        parcel p4(actions);

        HPX_TEST_EQ(p1.steps.actions.size(), std::size_t(1));
        HPX_TEST(p2.steps.actions.empty());
        HPX_TEST_EQ(p3.steps.actions.size(), std::size_t(1));
        HPX_TEST(p4.steps.actions.empty());
        HPX_TEST(p2.steps.completion == p1.steps.completion);
        HPX_TEST(p3.steps.completion != p1.steps.completion);

        p1.start();
        p2.start();
        p3.start();
        p4.start();
        HPX_TEST(!p1.complete);
        HPX_TEST(!p2.complete);
        HPX_TEST(!p3.complete);
        HPX_TEST(!p4.complete);

        // the third parcel doesn't use the first one's object; the fourth
        // waits for all steps before it
        s3.finish();
        HPX_TEST(!p1.complete);
        HPX_TEST(!p2.complete);
        HPX_TEST(p3.complete);
        HPX_TEST(!p4.complete);

        s1.finish();
        HPX_TEST(p1.complete);
        HPX_TEST(p2.complete);
        HPX_TEST(p4.complete);

        // waiting for completed steps calls right away
        bool called = false;
        p4.steps.completion->when_done([&called]() { called = true; });
        HPX_TEST(called);
    }

    // Parcels that don't use each other's objects complete independently:
    // a later one must not wait for transfers that may depend on it.
    void test_independent_parcels()
    {
        post_receive_actions actions;

        controlled_step s1;
        actions.object_loaded(10);
        actions.add(s1.action());
        parcel p1(actions);

        // no steps, no shared objects: complete right away
        parcel p2(actions);
        HPX_TEST(!p2.steps.completion);

        // an object of its own, referred to twice in the same parcel
        controlled_step s3;
        actions.object_loaded(30);
        actions.back_reference(30);
        actions.add(s3.action());
        parcel p3(actions);

        p1.start();
        p2.start();
        p3.start();
        HPX_TEST(p2.complete);

        s3.finish();
        HPX_TEST(p3.complete);
        HPX_TEST(!p1.complete);

        s1.finish();
        HPX_TEST(p1.complete);
    }

    // A failed step completes its parcel with the error, and the parcels
    // after it that use the same objects as well.
    void test_failed_step()
    {
        post_receive_actions actions;
        controlled_step s1;
        actions.object_loaded(10);
        actions.add(s1.action());
        parcel p1(actions);

        controlled_step s2;
        actions.back_reference(10);
        actions.add(s2.action());
        parcel p2(actions);

        controlled_step s3;
        actions.add(s3.action());
        parcel p3(actions);

        p1.start();
        p2.start();
        p3.start();
        s2.finish();
        s3.finish();
        HPX_TEST(!p2.complete);
        HPX_TEST(p3.complete);
        HPX_TEST(!p3.failed);

        s1.fail();
        HPX_TEST(p1.complete);
        HPX_TEST(p1.failed);
        HPX_TEST(p2.complete);
        HPX_TEST(p2.failed);
    }

    // a new message: the objects of the last one don't count
    void test_reset()
    {
        post_receive_actions actions;
        controlled_step s1;
        actions.object_loaded(10);
        actions.add(s1.action());
        parcel p1(actions);

        actions.reset();
        actions.back_reference(10);
        parcel p2(actions);
        HPX_TEST(!p2.steps.completion);

        p1.start();
        p2.start();
        HPX_TEST(p2.complete);
        HPX_TEST(!p1.complete);
        s1.finish();
        HPX_TEST(p1.complete);
    }

    // All waiters run when the last step finishes, also if one throws; the
    // exception is passed on afterwards.
    void test_throwing_waiter()
    {
        auto completion = receive_completion::create(nullptr, 1);
        bool second_called = false;
        completion->when_done([]() { throw std::runtime_error("waiter"); });
        completion->when_done([&second_called]() { second_called = true; });

        bool caught = false;
        try
        {
            completion->step_done();
        }
        catch (std::runtime_error const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
        HPX_TEST(second_called);
        HPX_TEST(!completion->error());
    }

    // transfers (e.g. receiving device memory over a device transport) run
    // before the other steps, which may use their data (e.g. copy it from a
    // staging buffer)
    void test_transfers_first()
    {
        post_receive_actions actions;

        controlled_step copy;
        bool copy_started = false;
        actions.add([&copy_started, action = copy.action()](
                        post_receive_actions::done_type d) {
            copy_started = true;
            action(HPX_MOVE(d));
        });

        controlled_step transfer1;
        controlled_step transfer2;
        actions.add_transfer(transfer1.action());
        actions.add_transfer(transfer2.action());
        HPX_TEST(!actions.empty());

        parcel p(actions);
        HPX_TEST(actions.empty());
        p.start();
        HPX_TEST(!copy_started);

        transfer2.finish();
        HPX_TEST(!copy_started);
        transfer1.finish();
        HPX_TEST(copy_started);
        HPX_TEST(!p.complete);

        copy.finish();
        HPX_TEST(p.complete);
    }

    // a parcel with transfers only
    void test_transfers_only()
    {
        post_receive_actions actions;

        controlled_step transfer;
        actions.add_transfer(transfer.action());

        parcel p(actions);
        p.start();
        HPX_TEST(!p.complete);
        transfer.finish();
        HPX_TEST(p.complete);
    }

    // a failed transfer skips the other steps and fails the parcel
    void test_failed_transfer()
    {
        post_receive_actions actions;

        controlled_step copy;
        bool copy_started = false;
        actions.add([&copy_started, action = copy.action()](
                        post_receive_actions::done_type d) {
            copy_started = true;
            action(HPX_MOVE(d));
        });

        controlled_step transfer1;
        controlled_step transfer2;
        actions.add_transfer(transfer1.action());
        actions.add_transfer(transfer2.action());

        parcel p(actions);
        p.start();

        transfer1.fail();
        HPX_TEST(!p.complete);
        transfer2.finish();
        HPX_TEST(!copy_started);
        HPX_TEST(p.complete);
        HPX_TEST(p.failed);
    }
}    // namespace

namespace {

    // An object loaded through a tracked pointer by a load() that registers
    // the parcel's first step only after it: a later parcel referring to the
    // object still waits for that step.
    struct child
    {
        int value = 0;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & value;
        }
    };

    std::shared_ptr<controlled_step> parent_step;

    struct parent
    {
        std::shared_ptr<child> c;

        template <typename Archive>
        void save(Archive& ar, unsigned) const
        {
            ar << c;
        }

        template <typename Archive>
        void load(Archive& ar, unsigned)
        {
            // the child is loaded before the archive has any step
            ar >> c;
            ar.template get_extra_data<post_receive_actions>().add(
                parent_step->action());
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()
    };

    void test_tracked_object_before_first_step()
    {
        auto const shared = std::make_shared<child>(child{42});
        std::vector<char> buffer;
        {
            hpx::serialization::output_archive oar(buffer);
            parent const p{shared};
            oar << p;
            oar << shared;    // a back-reference to the parent's child
        }

        parent_step = std::make_shared<controlled_step>();
        hpx::serialization::input_archive iar(buffer, buffer.size());

        // the first parcel: the parent with its child
        parent p;
        iar >> p;
        auto* actions = iar.try_get_extra_data<post_receive_actions>();
        HPX_TEST(actions != nullptr);
        if (actions == nullptr)
        {
            return;
        }
        auto const first = actions->take_parcel_steps();
        HPX_TEST_EQ(first.actions.size(), std::size_t(1));

        // the second parcel refers to the child: it waits for the first
        // parcel's step
        std::shared_ptr<child> c;
        iar >> c;
        HPX_TEST(c == p.c);
        auto const second = actions->take_parcel_steps();
        HPX_TEST(second.actions.empty());
        HPX_TEST(second.completion != nullptr);
        HPX_TEST(second.completion == first.completion);
        parent_step.reset();
    }
}    // namespace

int main()
{
    test_no_steps();
    test_shared_steps();
    test_independent_parcels();
    test_failed_step();
    test_reset();
    test_throwing_waiter();
    test_transfers_first();
    test_transfers_only();
    test_failed_transfer();
    test_tracked_object_before_first_step();

    return hpx::util::report_errors();
}
