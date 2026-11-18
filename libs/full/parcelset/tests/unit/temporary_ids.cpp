//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Managed ids that a serialize() function creates on the fly, sent in a parcel
// on one locality (the network is emulated): every pass over the parcel, each
// preprocessing round and the real one, creates new id_type objects, possibly
// at the addresses of earlier ones. Preprocessing splits a credit for each id
// once; the real pass must send a credit of the id's gid, and every credit
// must go back to AGAS exactly once, so that the component the ids refer to
// is destroyed once all of them are gone.
//
// The parcels also hold a future of an id ahead of the temporary ids: when it
// is pending in the first preprocessing round, the next round finds an id
// there that the first one didn't see. Parcels that hold the same id, or the
// same shared_ptr to an id, and are sent in one message write it once, the
// later ones as a back-reference: their credits for it are returned, and the
// ids after it still get theirs.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// the component the ids refer to
std::atomic<std::size_t> servers_alive(0);

struct server : hpx::components::component_base<server>
{
    server()
    {
        ++servers_alive;
    }

    server(server const&) = delete;
    server(server&&) = delete;
    server& operator=(server const&) = delete;
    server& operator=(server&&) = delete;

    ~server()
    {
        --servers_alive;
    }
};

using server_type = hpx::components::component<server>;
HPX_REGISTER_COMPONENT(server_type, temporary_ids_server)

// the id all temporary ids are split from, and another component's id that
// several parcels of one message hold
hpx::id_type base;
hpx::id_type other;

///////////////////////////////////////////////////////////////////////////////
// Sends 'shared' and 'boxed' (if set), then 'count' ids of 'base' that it
// creates while it is serialized, each with a credit of its own, and receives
// them into 'ids'.
struct temporary_ids
{
    hpx::id_type shared;
    std::shared_ptr<hpx::id_type> boxed;
    std::size_t count = 0;
    std::vector<hpx::id_type> ids;

    void save(hpx::serialization::output_archive& ar, unsigned) const
    {
        ar << shared << boxed << count;
        for (std::size_t i = 0; i != count; ++i)
        {
            // a new id_type_impl, which goes away once it is written (the
            // credit of base is replenished when needed)
            hpx::id_type const id(hpx::naming::detail::split_gid_if_needed(
                                      hpx::launch::sync, *base.impl()),
                hpx::id_type::management_type::managed);
            ar << id;
        }
    }

    void load(hpx::serialization::input_archive& ar, unsigned)
    {
        ar >> shared >> boxed >> count;
        ids.resize(count);
        for (auto& id : ids)
        {
            ar >> id;
        }
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
};

///////////////////////////////////////////////////////////////////////////////
// the action of the test parcels: checks the ids it received
std::atomic<std::size_t> actions_run(0);
std::atomic<std::size_t> actions_passed(0);

bool refers_to(hpx::id_type const& id, hpx::id_type const& target)
{
    return id &&
        hpx::naming::detail::get_stripped_gid(id.get_gid()) ==
        hpx::naming::detail::get_stripped_gid(target.get_gid());
}

void check_ids(
    hpx::shared_future<hpx::id_type> const& f, temporary_ids const& data)
{
    bool passed = data.ids.size() == data.count && refers_to(f.get(), other);
    if (data.shared)
    {
        passed = passed && refers_to(data.shared, other);
    }
    if (data.boxed)
    {
        passed = passed && refers_to(*data.boxed, other);
    }
    for (hpx::id_type const& id : data.ids)
    {
        passed = passed &&
            id.get_management_type() ==
                hpx::id_type::management_type::managed &&
            hpx::naming::detail::get_stripped_gid(id.get_gid()) ==
                hpx::naming::detail::get_stripped_gid(base.get_gid());
    }
    if (passed)
    {
        ++actions_passed;
    }
    ++actions_run;
}
HPX_PLAIN_ACTION(check_ids)

///////////////////////////////////////////////////////////////////////////////
namespace {

    std::uint32_t archive_flags()
    {
        std::uint32_t flags =
            static_cast<std::uint32_t>(hpx::endian::native == hpx::endian::big ?
                    hpx::serialization::archive_flags::endian_big :
                    hpx::serialization::archive_flags::endian_little);
        return flags | hpx::serialization::archive_flags::disable_data_chunking;
    }

    hpx::parcelset::parcel make_parcel(
        hpx::shared_future<hpx::id_type> f, temporary_ids data)
    {
        hpx::id_type const here = hpx::find_here();
        hpx::naming::address addr(hpx::get_locality(),
            to_int(hpx::components::component_enum_type::invalid),
            reinterpret_cast<void*>(&check_ids));

        hpx::naming::gid_type dest = here.get_gid();
        hpx::parcelset::parcel p(hpx::parcelset::detail::create_parcel::call(
            HPX_MOVE(dest), HPX_MOVE(addr), check_ids_action(),
            hpx::launch::async, HPX_MOVE(f), HPX_MOVE(data)));
        p.set_source_id(here);
        return p;
    }

    // What parcelset::detail::parcel_await_apply hands to the parcelport.
    // With 'pending', the future in the parcel becomes ready only after the
    // first preprocessing round, which makes a second round that finds the
    // id in the future ahead of the ids the first round has seen.
    hpx::parcelset::parcel preprocess(temporary_ids data, bool const pending)
    {
        hpx::promise<hpx::id_type> value;
        hpx::shared_future<hpx::id_type> f = value.get_future().share();
        if (!pending)
        {
            value.set_value(other);
        }

        hpx::promise<hpx::parcelset::parcel> promise;
        hpx::future<hpx::parcelset::parcel> preprocessed = promise.get_future();
        hpx::parcelset::detail::parcel_await_apply(
            make_parcel(f, HPX_MOVE(data)),
            [](std::error_code const&, hpx::parcelset::parcel const&) {},
            archive_flags(),
            [&promise](hpx::parcelset::parcel&& p,
                hpx::parcelset::write_handler_type&&) {
                promise.set_value(HPX_MOVE(p));
            },
            HPX_ZERO_COPY_SERIALIZATION_THRESHOLD);

        // the parcel is handed on only after the future is ready
        HPX_TEST_EQ(preprocessed.is_ready(), !pending);
        if (pending)
        {
            value.set_value(other);
        }
        return preprocessed.get();
    }

    // What a parcelport does with the parcels of one message, the steps of
    // encode_parcels: each parcel goes into the same archive with its split
    // credits.
    std::vector<char> encode(std::vector<hpx::parcelset::parcel>& ps)
    {
        using namespace hpx::serialization::detail;

        std::vector<char> data;
        hpx::serialization::output_archive archive(data, archive_flags());
        for (auto& p : ps)
        {
            auto split_gids = p.move_split_gids();
            if (!split_gids.empty() ||
                archive.try_get_extra_data<preprocess_gid_types>())
            {
                archive.get_extra_data<preprocess_gid_types>().set_split_gids(
                    HPX_MOVE(split_gids));
            }

            archive << p;
        }
        archive.flush();
        data.resize(archive.bytes_written());
        return data;
    }

    // receives the parcels of a message and schedules their actions
    void decode(std::vector<char> const& data, std::size_t const count)
    {
        hpx::serialization::input_archive archive(data, data.size());
        for (std::size_t i = 0; i != count; ++i)
        {
            hpx::parcelset::parcel p;
            bool deferred_schedule = false;
            HPX_TEST(!p.load_schedule(archive, 0, deferred_schedule));
            HPX_TEST(!deferred_schedule);
        }
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

    void test_parcel(std::size_t const count, bool const pending)
    {
        std::size_t const actions_before = actions_run;
        {
            std::vector<hpx::parcelset::parcel> ps;
            ps.push_back(preprocess(temporary_ids{{}, {}, count, {}}, pending));
            decode(encode(ps), 1);
        }
        wait_for_actions(actions_before + 1);
    }

    // Parcels preprocessed separately and sent in one message, all holding
    // the same id and the same shared_ptr to an id before their temporary
    // ones: each parcel has split credits for them, but the later parcels
    // write them as back-references to the first one (the id in the
    // shared_ptr isn't visited at all). Their temporary ids must still get
    // the credits split for them.
    void test_message(std::size_t const count, std::size_t const parcels)
    {
        std::size_t const actions_before = actions_run;
        {
            auto const boxed = std::make_shared<hpx::id_type>(other);

            std::vector<hpx::parcelset::parcel> ps;
            for (std::size_t i = 0; i != parcels; ++i)
            {
                ps.push_back(preprocess(
                    temporary_ids{other, boxed, count, {}}, i % 2 != 0));
            }
            decode(encode(ps), parcels);
        }
        wait_for_actions(actions_before + parcels);
    }

    // The real pass of one parcel (as encode_parcels does it) with the given
    // split credits, writing 'write' and reading it back into 'read'.
    template <typename Write, typename Read>
    void real_pass(
        hpx::serialization::detail::preprocess_gid_types::split_gids_type&&
            split_gids,
        Write&& write, Read&& read)
    {
        using hpx::serialization::detail::preprocess_gid_types;

        std::vector<char> data;
        {
            hpx::serialization::output_archive archive(data, archive_flags());
            archive.get_extra_data<preprocess_gid_types>().set_split_gids(
                HPX_MOVE(split_gids));
            write(archive);
            archive.flush();
            data.resize(archive.bytes_written());
        }

        hpx::serialization::input_archive archive(data, data.size());
        read(archive);
    }

    // A split credit of the id 'id', as preprocessing makes it.
    hpx::naming::gid_type split_credit(hpx::id_type& id)
    {
        return hpx::naming::detail::split_gid_if_needed(
            hpx::launch::sync, *id.impl());
    }

    hpx::naming::gid_type stripped(hpx::id_type const& id)
    {
        return hpx::naming::detail::get_stripped_gid(id.get_gid());
    }

    // A parcel writes an id twice and then another one, with the credits
    // split in another order: each id gets a credit of its gid (the second
    // write of the first id is a back-reference and needs none).
    void test_any_order()
    {
        hpx::id_type x = base;
        hpx::id_type y = other;

        hpx::serialization::detail::preprocess_gid_types::split_gids_type
            split_gids;
        split_gids.emplace_back(stripped(y), split_credit(y));
        split_gids.emplace_back(stripped(x), split_credit(x));

        hpx::id_type rx1, rx2, ry;
        bool thrown = false;
        try
        {
            real_pass(
                HPX_MOVE(split_gids),
                [&](hpx::serialization::output_archive& ar) {
                    ar << x << x << y;
                },
                [&](hpx::serialization::input_archive& ar) {
                    ar >> rx1 >> rx2 >> ry;
                });
        }
        catch (hpx::exception const&)
        {
            thrown = true;
        }

        HPX_TEST(!thrown);
        HPX_TEST(refers_to(rx1, base));
        HPX_TEST(rx2 && rx2.impl() == rx1.impl());
        HPX_TEST(refers_to(ry, other));
    }

    // The credit recorded for the gid of the managed id belongs to another
    // gid: the real pass throws instead of sending the other gid.
    void test_credit_of_other_id()
    {
        hpx::id_type x = base;
        hpx::id_type y = other;

        hpx::serialization::detail::preprocess_gid_types::split_gids_type
            split_gids;
        split_gids.emplace_back(stripped(x), split_credit(y));

        hpx::id_type rx;
        hpx::error error = hpx::error::success;
        try
        {
            real_pass(
                HPX_MOVE(split_gids),
                [&](hpx::serialization::output_archive& ar) { ar << x; },
                [&](hpx::serialization::input_archive& ar) { ar >> rx; });
        }
        catch (hpx::exception const& e)
        {
            error = e.get_error();
        }

        HPX_TEST(error == hpx::error::serialization_error);
        HPX_TEST(!rx);
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map&)
{
    base = hpx::new_<server>(hpx::find_here()).get();
    other = hpx::new_<server>(hpx::find_here()).get();
    HPX_TEST(
        base.get_management_type() == hpx::id_type::management_type::managed);
    HPX_TEST_EQ(servers_alive.load(), std::size_t(2));

    for (std::size_t const count : {std::size_t(1), std::size_t(4)})
    {
        // one preprocessing round, and two
        test_parcel(count, false);
        test_parcel(count, true);

        // several parcels in one message
        test_message(count, 2);
        test_message(count, 3);
    }

    // the real pass of a parcel with crafted split credits
    test_any_order();
    test_credit_of_other_id();

    // every credit came back: the components go away with their last id
    base = hpx::invalid_id;
    other = hpx::invalid_id;
    for (int i = 0; i != 500 && servers_alive != 0; ++i)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    HPX_TEST_EQ(servers_alive.load(), std::size_t(0));

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}

#endif
