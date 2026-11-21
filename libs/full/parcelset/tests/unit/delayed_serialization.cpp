//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Serialization of parcel data that completes later than the archive pass, on
// one locality (the network is emulated by copying the chunks):
//
// * Sending: the data is produced only while the parcel is preprocessed
//   (parcel_await_apply), asynchronously. The preprocessing registers a
//   keep-alive buffer (preprocess_keep_alive) and the future producing its
//   contents; the parcel waits for the future, keeps the buffer until it has
//   been sent, and the real pass sends the buffer as a zero-copy chunk.
// * Receiving with zero-copy receive: the chunk is received into a staging
//   buffer after load() has returned, a step registered by load()
//   (post_receive_actions) moves it into place, and the parcel's action is
//   scheduled after that step (parcel::on_receive_complete).

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

    std::atomic<std::size_t> buffers_created(0);
    std::atomic<std::size_t> buffers_alive(0);
    std::atomic<std::size_t> steps_run(0);

    // values produced after their buffer was released (must not happen)
    std::atomic<std::size_t> produced_too_late(0);

    // where the last load() put the values (stays valid while its vector
    // lives, also when the vector itself is moved)
    std::atomic<int const*> loaded_values(nullptr);

    // The values are produced only once the test opens this gate, so the
    // preprocessing always finds them pending.
    std::unique_ptr<hpx::promise<void>> gate_promise;
    hpx::shared_future<void> gate;

    // The zero-copy threshold of all parcels and archives of this test (the
    // configured default may be larger than the large payloads): 16 ints are
    // below it, 64 Ki ints above.
    constexpr std::size_t test_threshold = 4096;

    void close_gate()
    {
        gate_promise = std::make_unique<hpx::promise<void>>();
        gate = gate_promise->get_future().share();
    }

    void open_gate()
    {
        gate_promise->set_value();
    }

    // The deferred steps copy the data into place only once the test opens
    // this one, so it can check that the action waits for them.
    std::unique_ptr<hpx::promise<void>> step_gate_promise;
    hpx::shared_future<void> step_gate;

    // The values of a delayed_data, produced asynchronously once the gate is
    // open.
    struct produced_buffer
    {
        produced_buffer(std::size_t const size, int const first)
          : values(std::make_shared<std::vector<int>>(size))
        {
            ++buffers_created;
            ++buffers_alive;
            ready = gate.then([weak = std::weak_ptr<std::vector<int>>(values),
                                  first](auto&&) {
                // the buffer must still be kept when the values arrive
                auto const target = weak.lock();
                if (!target)
                {
                    ++produced_too_late;
                    return;
                }
                for (std::size_t i = 0; i != target->size(); ++i)
                {
                    (*target)[i] = first + static_cast<int>(i);
                }
            });
        }

        produced_buffer(produced_buffer const&) = delete;
        produced_buffer(produced_buffer&&) = delete;
        produced_buffer& operator=(produced_buffer const&) = delete;
        produced_buffer& operator=(produced_buffer&&) = delete;

        ~produced_buffer()
        {
            --buffers_alive;
        }

        std::shared_ptr<std::vector<int>> values;
        hpx::future<void> ready;
    };
}    // namespace

// Sends 'size' ints first, first + 1, ... without holding them: in a parcel
// the values are produced while the parcel is preprocessed. Receives them into
// 'values'.
struct delayed_data
{
    std::size_t size = 0;
    int first = 0;
    std::vector<int> values;

    [[nodiscard]] bool values_match() const
    {
        if (values.size() != size)
        {
            return false;
        }
        for (std::size_t i = 0; i != size; ++i)
        {
            if (values[i] != first + static_cast<int>(i))
            {
                return false;
            }
        }
        return true;
    }

    void save(hpx::serialization::output_archive& ar, unsigned) const
    {
        using namespace hpx::serialization::detail;

        ar << size << first;

        std::size_t const bytes = size * sizeof(int);
        keep_alive_buffer const* buffer = nullptr;
        if (bytes >= ar.get_zero_copy_serialization_threshold())
        {
            // only archives that keep the buffers alive (parcels) have it;
            // the buffers are matched to the objects by position
            auto* keep_alive = ar.try_get_extra_data<preprocess_keep_alive>();
            if (keep_alive != nullptr && !ar.is_preprocessing())
            {
                buffer = &keep_alive->get_next(bytes);
            }
            else if (keep_alive != nullptr)
            {
                buffer = keep_alive->next(bytes);
                if (buffer == nullptr)
                {
                    // the first preprocessing round of this parcel; the
                    // parcel waits for the buffer once it keeps it
                    auto produced =
                        std::make_shared<produced_buffer>(size, first);
                    auto state = hpx::traits::future_access<
                        hpx::future<void>>::get_shared_state(produced->ready);
                    void const* data = produced->values->data();
                    buffer = &keep_alive->add(
                        keep_alive_buffer{HPX_MOVE(produced), data, bytes});
                    if (!state->is_ready())
                    {
                        hpx::lcos::detail::preprocess_future(ar, *state);
                    }
                }
            }
        }

        bool const produced = buffer != nullptr;
        ar << produced;
        if (produced)
        {
            ar.save_binary_chunk(fundamental_types::none, buffer->data, bytes);
            return;
        }

        // small, or not preprocessed: produce the values right away
        std::vector<int> values_now(size);
        for (std::size_t i = 0; i != size; ++i)
        {
            values_now[i] = first + static_cast<int>(i);
        }
        ar.save_binary(fundamental_types::none, values_now.data(), bytes);
    }

    void load(hpx::serialization::input_archive& ar, unsigned)
    {
        using namespace hpx::serialization::detail;

        bool produced = false;
        ar >> size >> first >> produced;
        values.resize(size);
        loaded_values = values.data();

        std::size_t const bytes = size * sizeof(int);
        if (!produced)
        {
            ar.load_binary(fundamental_types::none, values.data(), bytes);
            return;
        }

        bool const zero_copy_receive = !ar.disable_receive_data_chunking() &&
            ar.try_get_extra_data<allow_zero_copy_receive>() != nullptr;
        if (!zero_copy_receive)
        {
            ar.load_binary_chunk(
                fundamental_types::none, values.data(), bytes, false);
            return;
        }

        // the data arrives in a staging buffer after this function has
        // returned, a step moves it into place
        auto staging = std::make_shared<std::vector<int>>(size);
        ar.load_binary_chunk(
            fundamental_types::none, staging->data(), bytes, true);
        ar.get_extra_data<post_receive_actions>().add(
            [staging = HPX_MOVE(staging), destination = values.data()](
                post_receive_actions::done_type done) {
                hpx::post([staging, destination, done = HPX_MOVE(done),
                              step_gate = step_gate]() {
                    if (step_gate.valid())
                    {
                        step_gate.wait();
                    }
                    std::copy(staging->begin(), staging->end(), destination);
                    ++steps_run;
                    done();
                });
            });
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

///////////////////////////////////////////////////////////////////////////////
// the action of the test parcels: reports what it received
std::atomic<std::size_t> actions_run(0);
std::atomic<std::size_t> actions_passed(0);

void check_data(delayed_data const& data)
{
    if (data.values_match())
    {
        ++actions_passed;
    }
    ++actions_run;
}
HPX_PLAIN_ACTION(check_data)

// An argument that fails to serialize after the one before it has
// registered data that is still being produced (until the test opens the
// gate).
struct failing_argument
{
    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
        throw std::runtime_error("failing_argument");
    }
};

void check_failing(delayed_data const&, failing_argument const&) {}
HPX_PLAIN_ACTION(check_failing)

// An argument that serializes in the first preprocessing round and fails in
// the next one, which runs once the data of the argument before it has been
// produced (the gate is open).
struct failing_later_argument
{
    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
        if (own_gate.valid() && own_gate.is_ready())
        {
            throw std::runtime_error("failing_later_argument");
        }
    }

    // the gate of the test that created it: a later test may replace the
    // global one while this argument is still serialized
    hpx::shared_future<void> own_gate = gate;
};

void check_failing_later(delayed_data const&, failing_later_argument const&) {}
HPX_PLAIN_ACTION(check_failing_later)
// Two delayed_data serialized one after the other from the same storage, as
// temporaries a serialize() function creates on the fly: each must get its own
// buffer although both have the same address.
struct delayed_pair
{
    std::size_t size = 0;
    int first = 0;
    int second = 0;
    delayed_data received_first;
    delayed_data received_second;

    void save(hpx::serialization::output_archive& ar, unsigned) const
    {
        ar << size << first << second;

        std::optional<delayed_data> temporary;
        temporary.emplace(delayed_data{size, first, {}});
        ar << *temporary;
        temporary.emplace(delayed_data{size, second, {}});
        ar << *temporary;
    }

    void load(hpx::serialization::input_archive& ar, unsigned)
    {
        ar >> size >> first >> second >> received_first >> received_second;
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

void check_pair(delayed_pair const& data)
{
    if (data.received_first.first == data.first &&
        data.received_first.values_match() &&
        data.received_second.first == data.second &&
        data.received_second.values_match())
    {
        ++actions_passed;
    }
    ++actions_run;
}
HPX_PLAIN_ACTION(check_pair)

// An object shared with other parcels of the same message and one of its own:
// the parcels after the first one write the shared object as a back-reference.
struct shared_and_own
{
    std::shared_ptr<delayed_data> shared;
    delayed_data own;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & shared & own;
    }
};

void check_shared(shared_and_own const& data)
{
    if (data.shared && data.shared->values_match() && data.own.values_match())
    {
        ++actions_passed;
    }
    ++actions_run;
}
HPX_PLAIN_ACTION(check_shared)

// An object in a future that isn't ready when the parcel is preprocessed
// first, before another one of the same size: the buffer of the second one is
// created first, the one of the first is created only in the next round.
void check_future_first(
    hpx::future<delayed_data> pending, delayed_data const& second)
{
    delayed_data const first = pending.get();
    if (first.first == 11 && first.values_match() && second.first == 22 &&
        second.values_match())
    {
        ++actions_passed;
    }
    ++actions_run;
}
HPX_PLAIN_ACTION(check_future_first)

// The same future twice, not ready at its first occurrence when the parcel is
// preprocessed first, but at its second one (reveal makes it ready in
// between), followed by another object: the next round inserts the buffer of
// the first occurrence only, the second one and the object after it keep the
// buffers created for them.
std::unique_ptr<hpx::promise<delayed_data>> reveal_promise;

struct reveal
{
    void save(hpx::serialization::output_archive& ar, unsigned) const
    {
        if (ar.is_preprocessing() && reveal_promise)
        {
            auto const promise = HPX_MOVE(reveal_promise);
            promise->set_value(delayed_data{65536, 11, {}});
        }
    }

    void load(hpx::serialization::input_archive&, unsigned) {}

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

void check_future_twice(hpx::shared_future<delayed_data> const& first,
    reveal const&, hpx::shared_future<delayed_data> const& second,
    delayed_data const& other)
{
    if (first.get().first == 11 && first.get().values_match() &&
        second.get().first == 11 && second.get().values_match() &&
        other.first == 33 && other.values_match())
    {
        ++actions_passed;
    }
    ++actions_run;
}
HPX_PLAIN_ACTION(check_future_twice)

// An object shared by the value of a future that isn't ready when the parcel
// is preprocessed first and the argument after it, followed by another object
// of the same size: in the next round the shared object is written first in
// the future's value, and the argument after it is a back-reference.
void check_moved_first(hpx::future<std::shared_ptr<delayed_data>> pending,
    std::shared_ptr<delayed_data> const& shared, delayed_data const& other)
{
    std::shared_ptr<delayed_data> const first = pending.get();
    if (first == shared && first && first->first == 11 &&
        first->values_match() && other.first == 22 && other.values_match())
    {
        ++actions_passed;
    }
    ++actions_run;
}
HPX_PLAIN_ACTION(check_moved_first)

// A parent whose child refers back to it before an object of its own: the
// back-reference is part of a cycle, the buffer after it belongs to the child.
// (Only sent: a cycle of tracked pointers can't be loaded.)
struct cycle_node
{
    std::shared_ptr<cycle_node> other;
    delayed_data data;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & other & data;
    }
};

void check_cycle(std::shared_ptr<cycle_node> const&) {}
HPX_PLAIN_ACTION(check_cycle)

///////////////////////////////////////////////////////////////////////////////
namespace {

    using hpx::serialization::chunk_type;
    using hpx::serialization::serialization_chunk;

    std::uint32_t archive_flags(bool const zero_copy)
    {
        std::uint32_t flags =
            static_cast<std::uint32_t>(hpx::endian::native == hpx::endian::big ?
                    hpx::serialization::archive_flags::endian_big :
                    hpx::serialization::archive_flags::endian_little);
        if (!zero_copy)
        {
            flags = flags |
                hpx::serialization::archive_flags::disable_data_chunking;
        }
        return flags;
    }

    template <typename Action, typename F, typename... Ts>
    hpx::parcelset::parcel make_action_parcel(F* f, Ts&&... ts)
    {
        hpx::id_type const here = hpx::find_here();
        hpx::naming::address addr(hpx::get_locality(),
            to_int(hpx::components::component_enum_type::invalid),
            reinterpret_cast<void*>(f));

        hpx::naming::gid_type dest = here.get_gid();
        hpx::parcelset::parcel p(hpx::parcelset::detail::create_parcel::call(
            HPX_MOVE(dest), HPX_MOVE(addr), Action(), hpx::launch::async,
            HPX_FORWARD(Ts, ts)...));
        p.set_source_id(here);
        return p;
    }

    hpx::parcelset::parcel make_parcel(delayed_data data)
    {
        return make_action_parcel<check_data_action>(
            &check_data, HPX_MOVE(data));
    }

    // what parcelset::detail::parcel_await_apply hands to the parcelport
    // 'pending': the parcel waits for produced data, which stays pending
    // until the gate is opened
    hpx::parcelset::parcel preprocess_with_closed_gate(
        hpx::parcelset::parcel&& p, std::uint32_t const flags,
        bool const pending)
    {
        hpx::promise<hpx::parcelset::parcel> promise;
        hpx::future<hpx::parcelset::parcel> f = promise.get_future();
        hpx::parcelset::detail::parcel_await_apply(
            HPX_MOVE(p),
            [&promise](
                std::error_code const& ec, hpx::parcelset::parcel const&) {
                // a failing preprocessing round
                promise.set_exception(
                    std::make_exception_ptr(std::system_error(ec)));
            },
            flags,
            [&promise](hpx::parcelset::parcel&& preprocessed,
                hpx::parcelset::write_handler_type&&) {
                promise.set_value(HPX_MOVE(preprocessed));
            },
            test_threshold);

        // the parcel is handed on only after its data has been produced
        HPX_TEST_EQ(f.is_ready(), !pending);

        open_gate();
        return f.get();
    }

    hpx::parcelset::parcel preprocess(hpx::parcelset::parcel&& p,
        std::uint32_t const flags, bool const pending)
    {
        close_gate();
        return preprocess_with_closed_gate(HPX_MOVE(p), flags, pending);
    }

    // what a parcelport sends: the archive and the contents of its zero-copy
    // chunks
    struct message
    {
        std::vector<char> data;
        std::vector<serialization_chunk> chunks;
        std::vector<std::vector<char>> chunk_data;
    };

    message encode(std::vector<hpx::parcelset::parcel> const& ps,
        std::uint32_t const flags);

    message encode(hpx::parcelset::parcel const& p, std::uint32_t const flags)
    {
        return encode(std::vector<hpx::parcelset::parcel>{p}, flags);
    }

    // the parcels of one message, as encode_parcels does it: each parcel goes
    // into the same archive with its keep-alive buffers
    message encode(std::vector<hpx::parcelset::parcel> const& ps,
        std::uint32_t const flags)
    {
        using hpx::serialization::detail::preprocess_keep_alive;

        message m;
        {
            hpx::serialization::output_archive archive(
                m.data, flags, &m.chunks, nullptr, test_threshold);
            for (hpx::parcelset::parcel const& p : ps)
            {
                if (auto const& keep_alive = p.keep_alive();
                    !keep_alive.empty() ||
                    archive.try_get_extra_data<preprocess_keep_alive>())
                {
                    archive.get_extra_data<preprocess_keep_alive>().set_buffers(
                        keep_alive, archive.bytes_written());
                }
                archive << p;
            }
            archive.flush();
            m.data.resize(archive.bytes_written());
        }

        // the chunk contents are sent before the parcel goes away
        for (serialization_chunk const& c : m.chunks)
        {
            if (c.type_ == chunk_type::chunk_type_index)
            {
                m.chunk_data.emplace_back();
                continue;
            }
            auto const* data = static_cast<char const*>(c.data());
            m.chunk_data.emplace_back(data, data + c.size());
        }
        return m;
    }

    // Receives the parcels of a message without zero-copy receive and
    // schedules their actions.
    void decode_message(message& m, std::size_t const count)
    {
        std::vector<serialization_chunk> chunks;
        for (std::size_t i = 0; i != m.chunks.size(); ++i)
        {
            serialization_chunk const& c = m.chunks[i];
            if (c.type_ == chunk_type::chunk_type_index)
            {
                chunks.push_back(c);
            }
            else
            {
                chunks.push_back(hpx::serialization::create_pointer_chunk(
                    static_cast<void*>(m.chunk_data[i].data()), c.size()));
            }
        }

        hpx::serialization::input_archive archive(
            m.data, m.data.size(), &chunks);
        for (std::size_t i = 0; i != count; ++i)
        {
            hpx::parcelset::parcel p;
            bool deferred_schedule = false;
            HPX_TEST(!p.load_schedule(archive, 0, deferred_schedule));
            HPX_TEST(!deferred_schedule);
        }
    }

    // Receives the message and schedules the parcel's action once its data
    // is complete. Returns whether a step completed the data.
    bool decode(message& m, bool const zero_copy_receive)
    {
        std::size_t const steps_before = steps_run;

        // the receive buffers: zero-copy receive lets load() choose the
        // destinations, otherwise the data has arrived already
        std::vector<serialization_chunk> chunks;
        for (std::size_t i = 0; i != m.chunks.size(); ++i)
        {
            serialization_chunk const& c = m.chunks[i];
            if (c.type_ == chunk_type::chunk_type_index)
            {
                chunks.push_back(c);
            }
            else if (zero_copy_receive)
            {
                chunks.push_back(hpx::serialization::create_pointer_chunk(
                    nullptr, c.size()));
            }
            else
            {
                chunks.push_back(hpx::serialization::create_pointer_chunk(
                    static_cast<void*>(m.chunk_data[i].data()), c.size()));
            }
        }

        hpx::parcelset::parcel p;
        {
            hpx::serialization::input_archive archive(
                m.data, m.data.size(), &chunks);
            if (zero_copy_receive)
            {
                archive.get_extra_data<
                    hpx::serialization::detail::allow_zero_copy_receive>();
            }

            // what the parcelport does: without zero-copy receive this
            // schedules the action, with it the parcel takes the steps that
            // complete its data
            bool deferred_schedule = false;
            HPX_TEST(!p.load_schedule(archive, 0, deferred_schedule));
            HPX_TEST(!deferred_schedule);
        }

        if (!zero_copy_receive)
        {
            HPX_TEST_EQ(steps_run.load(), steps_before);
            return false;
        }

        // the networking layer places the chunk data now
        bool deferred = false;
        for (std::size_t i = 0; i != chunks.size(); ++i)
        {
            serialization_chunk& c = chunks[i];
            if (c.type_ == chunk_type::chunk_type_index)
            {
                continue;
            }
            HPX_TEST(c.data() != nullptr);
            if (c.data() == nullptr)
            {
                // nowhere to put the data: don't schedule the action with
                // incomplete arguments
                return false;
            }
            std::memcpy(c.data(), m.chunk_data[i].data(), c.size());
            deferred = true;
        }

        if (deferred)
        {
            // the step hasn't run yet: the destination still holds zeros
            int const* values = loaded_values;
            HPX_TEST(values != nullptr && values[0] == 0);
            HPX_TEST_EQ(steps_run.load(), steps_before);
        }

        // the steps wait for the test to open the gate
        step_gate_promise = std::make_unique<hpx::promise<void>>();
        step_gate = step_gate_promise->get_future().share();

        // the step must have finished when the action is scheduled: check
        // it right there, before the action can run
        std::size_t const steps_expected = steps_before + (deferred ? 1 : 0);
        std::atomic<std::size_t> steps_at_schedule(0);

        hpx::promise<void> scheduled;
        hpx::future<void> f = scheduled.get_future();
        p.on_receive_complete([&p, &scheduled, &steps_at_schedule]() {
            steps_at_schedule = steps_run.load();
            p.schedule_action();
            scheduled.set_value();
        });

        // nothing is scheduled while a step is pending
        if (deferred)
        {
            HPX_TEST(!f.is_ready());
            HPX_TEST_EQ(steps_run.load(), steps_before);
        }
        step_gate_promise->set_value();
        f.get();

        HPX_TEST_EQ(steps_at_schedule.load(), steps_expected);
        HPX_TEST_EQ(steps_run.load(), steps_expected);
        return deferred;
    }

    void wait_for_actions(std::size_t const count)
    {
        for (int i = 0; i != 500 && actions_run < count; ++i)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        HPX_TEST_EQ(actions_run.load(), count);
        HPX_TEST_EQ(actions_passed.load(), count);
    }

    void test_parcel(std::size_t const size, bool const zero_copy,
        bool const zero_copy_receive)
    {
        std::size_t const actions_before = actions_run;
        std::size_t const created_before = buffers_created;
        bool const large = size * sizeof(int) >= test_threshold;

        message m;
        {
            std::uint32_t const flags = archive_flags(zero_copy);
            hpx::parcelset::parcel p = preprocess(
                make_parcel(delayed_data{size, 42, {}}), flags, large);

            // one buffer, whatever the number of preprocessing rounds (its
            // future isn't ready in the first one)
            HPX_TEST_EQ(
                buffers_created - created_before, std::size_t(large ? 1 : 0));

            m = encode(p, flags);

            // the parcel keeps the buffer until it has been sent
            HPX_TEST_EQ(buffers_alive.load(), std::size_t(large ? 1 : 0));
        }
        HPX_TEST_EQ(buffers_alive.load(), std::size_t(0));

        // a large buffer goes as a zero-copy chunk if chunks are enabled
        HPX_TEST_EQ(std::count_if(m.chunks.begin(), m.chunks.end(),
                        [](serialization_chunk const& c) {
                            return c.type_ != chunk_type::chunk_type_index;
                        }),
            std::ptrdiff_t(large && zero_copy ? 1 : 0));

        bool const deferred = decode(m, zero_copy_receive);
        HPX_TEST_EQ(deferred, large && zero_copy && zero_copy_receive);

        wait_for_actions(actions_before + 1);
    }

    // Two buffers for objects serialized from the same address, both pending
    // in the first preprocessing round: each object gets its own buffer (the
    // buffers are matched by position, not by address).
    void test_same_address(bool const zero_copy)
    {
        std::size_t const actions_before = actions_run;
        std::size_t const created_before = buffers_created;
        std::size_t const size = 65536;

        message m;
        {
            std::uint32_t const flags = archive_flags(zero_copy);
            hpx::parcelset::parcel p =
                preprocess(make_action_parcel<check_pair_action>(&check_pair,
                               delayed_pair{size, 42, 4242, {}, {}}),
                    flags, true);
            HPX_TEST_EQ(buffers_created - created_before, std::size_t(2));

            m = encode(p, flags);
            HPX_TEST_EQ(buffers_alive.load(), std::size_t(2));
        }
        HPX_TEST_EQ(buffers_alive.load(), std::size_t(0));

        HPX_TEST(!decode(m, false));
        wait_for_actions(actions_before + 1);
    }

    // Parcels preprocessed separately and sent in one message, all holding
    // the same object before one of their own of the same size: each parcel
    // has a buffer for the shared object, but the later parcels write it as a
    // back-reference to the first one. Their own objects must still get the
    // buffers created for them, not the one of the shared object.
    void test_shared_in_message(bool const zero_copy)
    {
        std::size_t const actions_before = actions_run;
        std::size_t const created_before = buffers_created;
        std::size_t const size = 65536;
        std::size_t const parcels = 3;

        message m;
        {
            std::uint32_t const flags = archive_flags(zero_copy);
            auto shared =
                std::make_shared<delayed_data>(delayed_data{size, 7, {}});

            std::vector<hpx::parcelset::parcel> ps;
            for (std::size_t i = 0; i != parcels; ++i)
            {
                ps.push_back(preprocess(
                    make_action_parcel<check_shared_action>(&check_shared,
                        shared_and_own{shared,
                            delayed_data{
                                size, 100 * static_cast<int>(i + 1), {}}}),
                    flags, true));
            }
            HPX_TEST_EQ(buffers_created - created_before, 2 * parcels);

            m = encode(ps, flags);
        }
        HPX_TEST_EQ(buffers_alive.load(), std::size_t(0));

        decode_message(m, parcels);
        wait_for_actions(actions_before + parcels);
    }

    // A future argument that is ready only after the first preprocessing
    // round holds an object of the same size as the argument after it: its
    // buffer, created in the second round, goes before the one created for
    // that argument in the first round.
    void test_pending_future(bool const zero_copy)
    {
        std::size_t const actions_before = actions_run;
        std::size_t const created_before = buffers_created;
        std::size_t const size = 65536;

        message m;
        {
            std::uint32_t const flags = archive_flags(zero_copy);
            close_gate();
            hpx::future<delayed_data> pending = gate.then(
                [size](auto&&) { return delayed_data{size, 11, {}}; });
            hpx::parcelset::parcel p = preprocess_with_closed_gate(
                make_action_parcel<check_future_first_action>(
                    &check_future_first, HPX_MOVE(pending),
                    delayed_data{size, 22, {}}),
                flags, true);
            HPX_TEST_EQ(buffers_created - created_before, std::size_t(2));

            m = encode(p, flags);
        }
        HPX_TEST_EQ(buffers_alive.load(), std::size_t(0));

        HPX_TEST(!decode(m, false));
        wait_for_actions(actions_before + 1);
    }

    // A future serialized twice is pending only at its first occurrence in
    // the first preprocessing round: the second round creates a buffer for
    // that occurrence only.
    void test_future_twice(bool const zero_copy)
    {
        std::size_t const actions_before = actions_run;
        std::size_t const created_before = buffers_created;

        message m;
        {
            std::uint32_t const flags = archive_flags(zero_copy);
            close_gate();
            reveal_promise = std::make_unique<hpx::promise<delayed_data>>();
            hpx::shared_future<delayed_data> const pending =
                reveal_promise->get_future().share();
            hpx::parcelset::parcel p = preprocess_with_closed_gate(
                make_action_parcel<check_future_twice_action>(
                    &check_future_twice, pending, reveal{}, pending,
                    delayed_data{2 * 65536, 33, {}}),
                flags, true);
            HPX_TEST(!reveal_promise);

            // the second occurrence and the object after it in the first
            // round, the first occurrence in the second one
            HPX_TEST_EQ(buffers_created - created_before, std::size_t(3));
            HPX_TEST_EQ(p.keep_alive().size(), std::size_t(3));

            m = encode(p, flags);
        }
        HPX_TEST_EQ(buffers_alive.load(), std::size_t(0));

        HPX_TEST(!decode(m, false));
        wait_for_actions(actions_before + 1);
    }

    // The first occurrence of a shared object moves into the value of a
    // future that is ready only after the first preprocessing round: the
    // buffer the first round created at its former position isn't used, the
    // object after it still gets its own.
    void test_moved_into_future(bool const zero_copy)
    {
        std::size_t const actions_before = actions_run;
        std::size_t const created_before = buffers_created;
        std::size_t const size = 65536;

        message m;
        {
            std::uint32_t const flags = archive_flags(zero_copy);
            auto shared =
                std::make_shared<delayed_data>(delayed_data{size, 11, {}});
            close_gate();
            hpx::future<std::shared_ptr<delayed_data>> pending =
                gate.then([shared](auto&&) { return shared; });
            hpx::parcelset::parcel p = preprocess_with_closed_gate(
                make_action_parcel<check_moved_first_action>(&check_moved_first,
                    HPX_MOVE(pending), shared, delayed_data{size, 22, {}}),
                flags, true);
            // the shared object's buffer of the first round is dropped
            HPX_TEST_EQ(buffers_created - created_before, std::size_t(3));
            HPX_TEST_EQ(p.keep_alive().size(), std::size_t(2));

            m = encode(p, flags);
        }
        HPX_TEST_EQ(buffers_alive.load(), std::size_t(0));

        HPX_TEST(!decode(m, false));
        wait_for_actions(actions_before + 1);
    }

    // A back-reference in a cycle (the child refers to its parent, which is
    // still being serialized) leaves the buffer of the child's object that
    // follows it in place in the later preprocessing rounds: it is created
    // once and used by the real pass.
    void test_cycle()
    {
        std::size_t const created_before = buffers_created;
        std::size_t const size = 65536;

        auto parent = std::make_shared<cycle_node>();
        auto child = std::make_shared<cycle_node>();
        parent->other = child;
        child->other = parent;
        child->data = delayed_data{size, 33, {}};

        {
            std::uint32_t const flags = archive_flags(true);
            hpx::parcelset::parcel p = preprocess(
                make_action_parcel<check_cycle_action>(&check_cycle, parent),
                flags, true);
            HPX_TEST_EQ(buffers_created - created_before, std::size_t(1));
            HPX_TEST_EQ(p.keep_alive().size(), std::size_t(1));

            // the real pass finds the buffer where the child's object is
            message const m = encode(p, flags);
            HPX_TEST_EQ(std::count_if(m.chunks.begin(), m.chunks.end(),
                            [](serialization_chunk const& c) {
                                return c.type_ != chunk_type::chunk_type_index;
                            }),
                std::ptrdiff_t(1));
        }
        HPX_TEST_EQ(buffers_alive.load(), std::size_t(0));

        child->other.reset();
    }

    // without a parcel nothing is preprocessed, the values go inline
    void test_archive(std::size_t const size)
    {
        std::size_t const created_before = buffers_created;

        std::vector<char> buffer;
        std::vector<serialization_chunk> chunks;
        {
            hpx::serialization::output_archive archive(
                buffer, archive_flags(true), &chunks, nullptr, test_threshold);
            archive << delayed_data{size, 7, {}};
        }
        HPX_TEST(chunks.empty() ||
            (chunks.size() == 1 &&
                chunks[0].type_ == chunk_type::chunk_type_index));

        delayed_data result;
        {
            hpx::serialization::input_archive archive(
                buffer, buffer.size(), &chunks);
            archive >> result;
        }
        HPX_TEST(result.values_match());
        HPX_TEST_EQ(result.first, 7);
        HPX_TEST_EQ(buffers_created.load(), created_before);
    }
}    // namespace

namespace {

    // Serializing a parcel fails after an argument has registered data that
    // is still being produced: the error is passed on right away, and the
    // archive tracking that data is kept until the data is ready.
    void test_failing_serialization()
    {
        std::size_t const alive_before = buffers_alive;

        close_gate();
        bool caught = false;
        try
        {
            hpx::parcelset::detail::parcel_await_apply(
                make_action_parcel<check_failing_action>(&check_failing,
                    delayed_data{65536, 7, {}}, failing_argument{}),
                [](std::error_code const&, hpx::parcelset::parcel const&) {},
                archive_flags(true),
                [](hpx::parcelset::parcel&&,
                    hpx::parcelset::write_handler_type&&) {
                    // nothing is sent
                    HPX_TEST(false);
                },
                test_threshold);
        }
        catch (std::exception const& e)
        {
            caught = std::string(e.what()).find("failing_argument") !=
                std::string::npos;
        }

        // the error was passed on while the data was still being produced,
        // its buffer is kept until then
        HPX_TEST(caught);
        HPX_TEST(!gate.is_ready());
        HPX_TEST_EQ(buffers_alive.load(), alive_before + 1);

        // and released once the data is ready
        open_gate();
        for (auto const deadline =
                 std::chrono::steady_clock::now() + std::chrono::seconds(30);
            buffers_alive != alive_before &&
            std::chrono::steady_clock::now() < deadline;)
        {
            hpx::this_thread::yield();
        }
        HPX_TEST_EQ(buffers_alive.load(), alive_before);
        HPX_TEST_EQ(produced_too_late.load(), std::size_t(0));
    }
}    // namespace

namespace {

    // A later preprocessing round fails: it runs from the continuation of
    // the data the first round waits for, the error must reach the parcel's
    // write handler.
    void test_failing_resumed_serialization()
    {
        std::size_t const alive_before = buffers_alive;
        close_gate();

        // shared with the handler, which may run after a timeout of the test
        struct state
        {
            std::atomic<bool> handled = false;
            std::atomic<bool> failed = false;
        };
        auto const result = std::make_shared<state>();
        std::atomic<bool> const& handled = result->handled;
        std::atomic<bool> const& failed = result->failed;

        hpx::parcelset::detail::parcel_await_apply(
            make_action_parcel<check_failing_later_action>(&check_failing_later,
                delayed_data{65536, 7, {}}, failing_later_argument{}),
            [result](std::error_code const& ec, hpx::parcelset::parcel const&) {
                result->failed = static_cast<bool>(ec);
                result->handled = true;
            },
            archive_flags(true),
            [](hpx::parcelset::parcel&&, hpx::parcelset::write_handler_type&&) {
                // nothing is sent
                HPX_TEST(false);
            },
            test_threshold);

        // the first round waits for the data
        HPX_TEST(!handled);

        // the error reaches the handler, and the failed archive releases its
        // buffer (maybe only after the handler ran): the next tests expect
        // no buffers alive
        open_gate();
        for (auto const deadline =
                 std::chrono::steady_clock::now() + std::chrono::seconds(30);
            (!handled || buffers_alive != alive_before) &&
            std::chrono::steady_clock::now() < deadline;)
        {
            hpx::this_thread::yield();
        }
        HPX_TEST(handled);
        HPX_TEST(failed);
        HPX_TEST_EQ(buffers_alive.load(), alive_before);
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map&)
{
    test_failing_serialization();
    test_failing_resumed_serialization();

    // 16 ints go inline, 64 Ki ints (256 KiB) as a zero-copy chunk
    for (std::size_t const size : {std::size_t(16), std::size_t(65536)})
    {
        test_parcel(size, true, true);
        test_parcel(size, true, false);
        test_parcel(size, false, false);
        test_archive(size);
    }
    test_same_address(true);
    test_same_address(false);
    test_shared_in_message(true);
    test_shared_in_message(false);
    test_pending_future(true);
    test_pending_future(false);
    test_future_twice(true);
    test_future_twice(false);
    test_moved_into_future(true);
    test_moved_into_future(false);
    test_cycle();
    // the gate's shared state goes away while the runtime is still there
    gate = hpx::shared_future<void>();
    gate_promise.reset();
    step_gate = hpx::shared_future<void>();
    step_gate_promise.reset();

    HPX_TEST_EQ(produced_too_late.load(), std::size_t(0));
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}

#endif
