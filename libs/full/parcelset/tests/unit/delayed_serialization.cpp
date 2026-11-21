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
#include <memory>
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
            // only archives that keep the buffers alive (parcels) have it
            auto* keep_alive = ar.try_get_extra_data<preprocess_keep_alive>();
            if (keep_alive != nullptr)
            {
                buffer = keep_alive->find(this);
            }
            if (buffer == nullptr && keep_alive != nullptr &&
                ar.is_preprocessing())
            {
                // the first preprocessing round of this parcel; the parcel
                // waits for the buffer once it keeps it
                auto produced = std::make_shared<produced_buffer>(size, first);
                auto state = hpx::traits::future_access<
                    hpx::future<void>>::get_shared_state(produced->ready);
                void const* data = produced->values->data();
                keep_alive->add(
                    this, keep_alive_buffer{HPX_MOVE(produced), data, bytes});
                if (!state->is_ready())
                {
                    hpx::lcos::detail::preprocess_future(ar, *state);
                }
                buffer = keep_alive->find(this);
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
    hpx::parcelset::parcel preprocess(hpx::parcelset::parcel&& p,
        std::uint32_t const flags, bool const pending)
    {
        close_gate();

        hpx::promise<hpx::parcelset::parcel> promise;
        hpx::future<hpx::parcelset::parcel> f = promise.get_future();
        hpx::parcelset::detail::parcel_await_apply(
            HPX_MOVE(p),
            [](std::error_code const&, hpx::parcelset::parcel const&) {}, flags,
            [&promise](hpx::parcelset::parcel&& preprocessed,
                hpx::parcelset::write_handler_type&&) {
                promise.set_value(HPX_MOVE(preprocessed));
            });

        // the parcel is handed on only after its data has been produced
        HPX_TEST_EQ(f.is_ready(), !pending);

        open_gate();
        return f.get();
    }

    // what a parcelport sends: the archive and the contents of its zero-copy
    // chunks
    struct message
    {
        std::vector<char> data;
        std::vector<serialization_chunk> chunks;
        std::vector<std::vector<char>> chunk_data;
    };

    message encode(hpx::parcelset::parcel const& p, std::uint32_t const flags)
    {
        message m;
        {
            hpx::serialization::output_archive archive(
                m.data, flags, &m.chunks);
            if (auto const& keep_alive = p.keep_alive(); !keep_alive.empty())
            {
                archive
                    .get_extra_data<
                        hpx::serialization::detail::preprocess_keep_alive>()
                    .add_buffers(keep_alive);
            }
            archive << p;
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

        hpx::promise<void> scheduled;
        hpx::future<void> f = scheduled.get_future();
        p.on_receive_complete([&p, &scheduled]() {
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

        HPX_TEST_EQ(steps_run.load(), steps_before + (deferred ? 1 : 0));
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
        bool const large =
            size * sizeof(int) >= HPX_ZERO_COPY_SERIALIZATION_THRESHOLD;

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

    // without a parcel nothing is preprocessed, the values go inline
    void test_archive(std::size_t const size)
    {
        std::size_t const created_before = buffers_created;

        std::vector<char> buffer;
        std::vector<serialization_chunk> chunks;
        {
            hpx::serialization::output_archive archive(
                buffer, archive_flags(true), &chunks);
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
    // is still being produced: the error is passed on only once that data
    // is ready, the archive tracking it must not go away before.
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
                });
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
        for (int i = 0; i != 100000 && buffers_alive != alive_before; ++i)
        {
            hpx::this_thread::yield();
        }
        HPX_TEST_EQ(buffers_alive.load(), alive_before);
        HPX_TEST_EQ(produced_too_late.load(), std::size_t(0));
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map&)
{
    test_failing_serialization();

    // 16 ints go inline, 64 Ki ints (256 KiB) as a zero-copy chunk
    for (std::size_t const size : {std::size_t(16), std::size_t(65536)})
    {
        test_parcel(size, true, true);
        test_parcel(size, true, false);
        test_parcel(size, false, false);
        test_archive(size);
    }
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
