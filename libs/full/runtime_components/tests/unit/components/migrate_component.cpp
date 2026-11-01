//  Copyright (c) 2014-2025 Hartmut Kaiser
//  Copyright (c)      2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/serialization.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

constexpr std::size_t N = 50;

///////////////////////////////////////////////////////////////////////////////
struct dummy_server : hpx::components::component_base<dummy_server>
{
    dummy_server() = default;

    [[nodiscard]] hpx::id_type call() const
    {
        return hpx::find_here();
    }

    HPX_DEFINE_COMPONENT_ACTION(dummy_server, call)
};

using dummy_server_type = hpx::components::component<dummy_server>;
HPX_REGISTER_COMPONENT(dummy_server_type, dummy_server)

using dummy_action = dummy_server::call_action;
HPX_REGISTER_ACTION(dummy_action)

struct dummy_client : hpx::components::client_base<dummy_client, dummy_server>
{
    using base_type = hpx::components::client_base<dummy_client, dummy_server>;

    dummy_client() = default;

    explicit dummy_client(hpx::id_type const& id)
      : base_type(id)
    {
    }
    dummy_client(hpx::future<hpx::id_type>&& id)
      : base_type(std::move(id))
    {
    }

    [[nodiscard]] hpx::id_type call() const
    {
        return hpx::async<dummy_action>(this->get_id()).get();
    }
};

///////////////////////////////////////////////////////////////////////////////
struct test_server
  : hpx::components::migration_support<
        hpx::components::component_base<test_server>>
{
    using base_type = hpx::components::migration_support<
        hpx::components::component_base<test_server>>;

    explicit test_server(int data = 0)
      : data_(data)
    {
    }
    ~test_server() = default;

    [[nodiscard]] hpx::id_type call() const
    {
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
        return hpx::find_here();
    }

    void busy_work() const
    {
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
        hpx::this_thread::sleep_for(std::chrono::seconds(1));
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
    }

    [[nodiscard]] hpx::future<void> lazy_busy_work() const
    {
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));

        auto f = hpx::make_ready_future_after(std::chrono::seconds(1));

        return f.then([this](hpx::future<void>&& f) -> void {
            HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
            f.get();
            HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
        });
    }

    [[nodiscard]] int get_data() const
    {
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
        return data_;
    }

    [[nodiscard]] std::uintptr_t get_lva() const
    {
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
        return reinterpret_cast<std::uintptr_t>(this);
    }

    [[nodiscard]] hpx::future<int> lazy_get_data() const
    {
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));

        auto f = hpx::make_ready_future(data_);

        return f.then([this](hpx::future<int>&& f) -> int {
            HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
            auto const result = f.get();
            HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
            return result;
        });
    }

    [[nodiscard]] dummy_client lazy_get_client(hpx::id_type const& there) const
    {
        HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));

        auto f = dummy_client(hpx::new_<dummy_server>(there));

        return f.then([this](dummy_client&& f) -> hpx::id_type {
            HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
            auto result = f.get();
            HPX_TEST_NEQ(pin_count(), static_cast<std::uint32_t>(0));
            return result;
        });
    }

    // Components that should be migrated using hpx::migrate<> need to be
    // Serializable and CopyConstructable. Components can be MoveConstructable
    // in which case the serialized data is moved into the component's
    // constructor.
    test_server(test_server const& rhs)
      : base_type(rhs)
      , data_(rhs.data_)
    {
    }

    test_server(test_server&& rhs) noexcept
      : base_type(static_cast<base_type&&>(rhs))
      , data_(rhs.data_)
    {
        rhs.data_ = 0;
    }

    test_server& operator=(test_server const& rhs)
    {
        data_ = rhs.data_;
        return *this;
    }
    test_server& operator=(test_server&& rhs) noexcept
    {
        base_type::operator=(static_cast<base_type&&>(rhs));
        data_ = rhs.data_;
        rhs.data_ = 0;
        return *this;
    }

    HPX_DEFINE_COMPONENT_ACTION(test_server, call, call_action)
    HPX_DEFINE_COMPONENT_ACTION(test_server, busy_work, busy_work_action)
    HPX_DEFINE_COMPONENT_ACTION(
        test_server, lazy_busy_work, lazy_busy_work_action)

    HPX_DEFINE_COMPONENT_ACTION(test_server, get_data, get_data_action)
    HPX_DEFINE_COMPONENT_ACTION(test_server, get_lva, get_lva_action)
    HPX_DEFINE_COMPONENT_ACTION(
        test_server, lazy_get_data, lazy_get_data_action)
    HPX_DEFINE_COMPONENT_ACTION(
        test_server, lazy_get_client, lazy_get_client_action)

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        // clang-format off
        ar & data_;
        // clang-format on
    }

private:
    int data_;
};

using server_type = hpx::components::component<test_server>;
HPX_REGISTER_COMPONENT(server_type, test_server)

using call_action = test_server::call_action;
HPX_REGISTER_ACTION_DECLARATION(call_action)
HPX_REGISTER_ACTION(call_action)

using busy_work_action = test_server::busy_work_action;
HPX_REGISTER_ACTION_DECLARATION(busy_work_action)
HPX_REGISTER_ACTION(busy_work_action)

using lazy_busy_work_action = test_server::lazy_busy_work_action;
HPX_REGISTER_ACTION_DECLARATION(lazy_busy_work_action)
HPX_REGISTER_ACTION(lazy_busy_work_action)

using get_data_action = test_server::get_data_action;
HPX_REGISTER_ACTION_DECLARATION(get_data_action)
HPX_REGISTER_ACTION(get_data_action)

using get_lva_action = test_server::get_lva_action;
HPX_REGISTER_ACTION_DECLARATION(get_lva_action)
HPX_REGISTER_ACTION(get_lva_action)

using lazy_get_data_action = test_server::lazy_get_data_action;
HPX_REGISTER_ACTION_DECLARATION(lazy_get_data_action)
HPX_REGISTER_ACTION(lazy_get_data_action)

using lazy_get_client_action = test_server::lazy_get_client_action;
HPX_REGISTER_ACTION_DECLARATION(lazy_get_client_action)
HPX_REGISTER_ACTION(lazy_get_client_action)

struct test_client : hpx::components::client_base<test_client, test_server>
{
    using base_type = hpx::components::client_base<test_client, test_server>;

    test_client() = default;
    test_client(hpx::shared_future<hpx::id_type> const& id)
      : base_type(id)
    {
    }
    explicit test_client(hpx::id_type&& id)
      : base_type(std::move(id))
    {
    }

    [[nodiscard]] hpx::id_type call() const
    {
        return call_action()(this->get_id());
    }

    [[nodiscard]] hpx::future<void> busy_work() const
    {
        return hpx::async<busy_work_action>(this->get_id());
    }

    [[nodiscard]] hpx::future<void> lazy_busy_work() const
    {
        return hpx::async<lazy_busy_work_action>(this->get_id());
    }

    [[nodiscard]] int get_data() const
    {
        return get_data_action()(this->get_id());
    }

    [[nodiscard]] std::uintptr_t get_lva() const
    {
        return get_lva_action()(this->get_id());
    }

    [[nodiscard]] int lazy_get_data() const
    {
        return lazy_get_data_action()(this->get_id()).get();
    }

    [[nodiscard]] dummy_client lazy_get_client(hpx::id_type const& there) const
    {
        return lazy_get_client_action()(this->get_id(), there);
    }
};

///////////////////////////////////////////////////////////////////////////////
void test_last_unpin_keeps_migration_state_alive()
{
    auto component = std::make_unique<test_server>(42);
    hpx::id_type const id = component->get_unmanaged_id();

    // Two pins make migration wait for the last unpin.
    HPX_TEST(component->pin());
    HPX_TEST(component->pin());
    hpx::future<void> migration_ready = component->mark_as_migrated(id);
    HPX_TEST(!migration_ready.is_ready());
    component->unpin();

    bool destroyed = false;
    hpx::future<void> completion =
        migration_ready.then(hpx::launch::sync, [&](hpx::future<void>&& ready) {
            ready.get();
            component->mark_as_migrated();
            component.reset();
            destroyed = true;
        });

    // Completing migration can destroy the old instance before unpin returns.
    // Its remaining cleanup must use independently retained state.
    component->unpin();
    completion.get();
    HPX_TEST(destroyed);
    HPX_TEST(!component);

    // This test controls the component lifetime without sending it elsewhere.
    hpx::agas::unmark_as_migrated(id.get_gid(), [] {});
    hpx::agas::unbind(hpx::launch::sync, id.get_gid(), 1);
}

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_component_rejects_stale_lva(
    hpx::id_type const& source, hpx::id_type const& target)
{
    try
    {
        HPX_TEST_EQ(source, hpx::find_here());

        test_client const component = hpx::new_<test_client>(source, 42);

        // This action records the original LVA while the component is pinned.
        std::uintptr_t const old_lva = component.get_lva();

        test_client const remote(hpx::components::migrate(component, target));
        HPX_TEST_EQ(remote.call(), target);

        {
            auto const moved_away_state =
                hpx::traits::action_was_object_migrated<get_data_action>::call(
                    component.get_id().get_gid(),
                    reinterpret_cast<hpx::naming::address_type>(old_lva));

            HPX_TEST(moved_away_state.first);
            HPX_TEST(!moved_away_state.second);

            bool called = false;
            auto const legacy_state = hpx::agas::was_object_migrated(
                component.get_id().get_gid(), [&called] {
                    called = true;
                    return hpx::components::pinned_ptr();
                });
            HPX_TEST(legacy_state.first);
            HPX_TEST(!called);
        }

        // The configured component allocator does not guarantee reuse of the
        // original LVA. Keep another same-type object alive and use its LVA as
        // a deterministic stand-in for a stale LVA reused by another object.
        test_client const occupier = hpx::new_<test_client>(source, 84);
        std::uintptr_t const stale_lva = occupier.get_lva();

        test_client const returned(hpx::components::migrate(remote, source));
        HPX_TEST_EQ(returned.call(), source);
        std::uintptr_t const current_lva = returned.get_lva();

        HPX_TEST_NEQ(old_lva, static_cast<std::uintptr_t>(0));
        HPX_TEST_NEQ(stale_lva, static_cast<std::uintptr_t>(0));
        HPX_TEST_NEQ(current_lva, stale_lva);

        {
            auto const current_state =
                hpx::traits::action_was_object_migrated<get_data_action>::call(
                    component.get_id().get_gid(),
                    reinterpret_cast<hpx::naming::address_type>(current_lva));

            HPX_TEST(!current_state.first);
            HPX_TEST(current_state.second);
        }

        auto const migration_state =
            hpx::traits::action_was_object_migrated<get_data_action>::call(
                component.get_id().get_gid(),
                reinterpret_cast<hpx::naming::address_type>(stale_lva));

        HPX_TEST(migration_state.first);
        HPX_TEST(!migration_state.second);
        HPX_TEST_EQ(returned.get_data(), 42);
        HPX_TEST_EQ(occupier.get_data(), 84);

        bool called = false;
        auto const legacy_state = hpx::agas::was_object_migrated(
            component.get_id().get_gid(), [&called] {
                called = true;
                return hpx::components::pinned_ptr();
            });
        HPX_TEST(!legacy_state.first);
        HPX_TEST(called);
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_component(
    hpx::id_type const& source, hpx::id_type const& target)
{
    // create component on given locality
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    try
    {
        // migrate t1 to the target
        test_client const t2(hpx::components::migrate(t1, target));

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.get_data(), 42);
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_lazy_component(
    hpx::id_type const& source, hpx::id_type const& target)
{
    // create component on given locality
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_data(), 42);

    try
    {
        // migrate t1 to the target
        test_client const t2(hpx::components::migrate(t1, target));

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.lazy_get_data(), 42);
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_lazy_component_client(
    hpx::id_type const& source, hpx::id_type const& target)
{
    // create component on given locality
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_client(target).call(), target);

    try
    {
        // migrate t1 to the target
        test_client const t2(hpx::components::migrate(t1, target));

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.lazy_get_client(source).call(), source);
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_busy_component(
    hpx::id_type const& source, hpx::id_type const& target)
{
    // create component on given locality
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    // add some concurrent busy work
    hpx::future<void> const busy_work = t1.busy_work();

    try
    {
        // migrate t1 to the target
        test_client const t2(hpx::components::migrate(t1, target));

        HPX_TEST_EQ(t1.get_data(), 42);

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.get_data(), 42);
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    // the busy work should be finished by now, wait anyways
    busy_work.wait();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool test_migrate_lazy_busy_component(
    hpx::id_type const& source, hpx::id_type const& target)
{
    // create component on given locality
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_data(), 42);

    // add some concurrent busy work
    hpx::future<void> const lazy_busy_work = t1.lazy_busy_work();

    try
    {
        // migrate t1 to the target
        test_client const t2(hpx::components::migrate(t1, target));

        HPX_TEST_EQ(t1.lazy_get_data(), 42);

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.lazy_get_data(), 42);
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    // the busy work should be finished by now, wait anyways
    lazy_busy_work.wait();

    return true;
}

bool test_migrate_lazy_busy_component_client(
    hpx::id_type const& source, hpx::id_type const& target)
{
    // create component on given locality
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_client(target).call(), target);

    // add some concurrent busy work
    hpx::future<void> const lazy_busy_work = t1.lazy_busy_work();

    try
    {
        // migrate t1 to the target
        test_client const t2(hpx::components::migrate(t1, target));

        HPX_TEST_EQ(t1.lazy_get_client(target).call(), target);

        // wait for migration to be done
        HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

        // the migrated object should have the same id as before
        HPX_TEST_EQ(t1.get_id(), t2.get_id());

        // the migrated object should live on the target now
        HPX_TEST_EQ(t2.call(), target);
        HPX_TEST_EQ(t2.lazy_get_client(source).call(), source);
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    // the busy work should be finished by now, wait anyways
    lazy_busy_work.wait();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool test_migrate_component2(hpx::id_type source, hpx::id_type target)
{
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    try
    {
        // migrate an object back and forth between 2 localities a couple of
        // times
        for (std::size_t i = 0; i < N; ++i)
        {
            // migrate t1 to the target (loc2)
            test_client const t2(hpx::components::migrate(t1, target));

            HPX_TEST_EQ(t1.get_data(), 42);

            // wait for migration to be done
            HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

            // the migrated object should have the same id as before
            HPX_TEST_EQ(t1.get_id(), t2.get_id());

            // the migrated object should live on the target now
            HPX_TEST_EQ(t2.call(), target);
            HPX_TEST_EQ(t2.get_data(), 42);

            hpx::cout << "." << std::flush;

            std::swap(source, target);
        }

        hpx::cout << std::endl;
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_lazy_component2(hpx::id_type source, hpx::id_type target)
{
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_data(), 42);

    try
    {
        // migrate an object back and forth between 2 localities a couple of
        // times
        for (std::size_t i = 0; i < N; ++i)
        {
            // migrate t1 to the target (loc2)
            test_client const t2(hpx::components::migrate(t1, target));

            HPX_TEST_EQ(t1.lazy_get_data(), 42);

            // wait for migration to be done
            HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

            // the migrated object should have the same id as before
            HPX_TEST_EQ(t1.get_id(), t2.get_id());

            // the migrated object should live on the target now
            HPX_TEST_EQ(t2.call(), target);
            HPX_TEST_EQ(t2.lazy_get_data(), 42);

            hpx::cout << "." << std::flush;

            std::swap(source, target);
        }

        hpx::cout << std::endl;
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_lazy_component_client2(
    hpx::id_type source, hpx::id_type target)
{
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.lazy_get_client(target).call(), target);

    try
    {
        // migrate an object back and forth between 2 localities a couple of
        // times
        for (std::size_t i = 0; i < N; ++i)
        {
            // migrate t1 to the target (loc2)
            test_client const t2(hpx::components::migrate(t1, target));

            HPX_TEST_EQ(t1.lazy_get_client(target).call(), target);

            // wait for migration to be done
            HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

            // the migrated object should have the same id as before
            HPX_TEST_EQ(t1.get_id(), t2.get_id());

            // the migrated object should live on the target now
            HPX_TEST_EQ(t2.call(), target);
            HPX_TEST_EQ(t2.lazy_get_data(), 42);
            HPX_TEST_EQ(t2.lazy_get_client(source).call(), source);

            hpx::cout << "." << std::flush;

            std::swap(source, target);
        }

        hpx::cout << std::endl;
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    return true;
}

bool test_migrate_busy_component2(hpx::id_type source, hpx::id_type target)
{
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    // First, spawn a thread (locally) that will migrate the object between
    // source and target a few times
    hpx::future<void> migrate_future =
        hpx::async([source, target, t1]() mutable {
            for (std::size_t i = 0; i < N; ++i)
            {
                // migrate t1 to the target (loc2)
                test_client const t2(hpx::components::migrate(t1, target));

                HPX_TEST_EQ(t1.get_data(), 42);

                // wait for migration to be done
                HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

                // the migrated object should have the same id as before
                HPX_TEST_EQ(t1.get_id(), t2.get_id());

                // the migrated object should live on the target now
                HPX_TEST_EQ(t2.call(), target);
                HPX_TEST_EQ(t2.get_data(), 42);

                hpx::cout << "." << std::flush;

                std::swap(source, target);
            }

            hpx::cout << std::endl;
        });

    // Second, we generate tons of work that should automatically follow
    // the migration.
    hpx::future<void> create_work = hpx::async([t1]() {
        for (std::size_t i = 0; i < 2 * N; ++i)
        {
            hpx::cout << hpx::naming::get_locality_id_from_id(t1.call())
                      << std::flush;
            HPX_TEST_EQ(t1.get_data(), 42);
        }
    });

    hpx::wait_all(migrate_future, create_work);

    // rethrow exceptions
    try
    {
        migrate_future.get();
        create_work.get();
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    hpx::cout << std::endl;

    return true;
}

bool test_migrate_lazy_busy_component2(hpx::id_type source, hpx::id_type target)
{
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    // First, spawn a thread (locally) that will migrate the object between
    // source and target a few times
    hpx::future<void> migrate_future =
        hpx::async([source, target, t1]() mutable {
            for (std::size_t i = 0; i < N; ++i)
            {
                // migrate t1 to the target (loc2)
                test_client const t2(hpx::components::migrate(t1, target));

                HPX_TEST_EQ(t1.lazy_get_data(), 42);

                // wait for migration to be done
                HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

                // the migrated object should have the same id as before
                HPX_TEST_EQ(t1.get_id(), t2.get_id());

                // the migrated object should live on the target now
                HPX_TEST_EQ(t2.call(), target);
                HPX_TEST_EQ(t2.lazy_get_data(), 42);

                hpx::cout << "." << std::flush;

                std::swap(source, target);
            }

            hpx::cout << std::endl;
        });

    // Second, we generate tons of work that should automatically follow
    // the migration.
    hpx::future<void> create_work = hpx::async([t1]() {
        for (std::size_t i = 0; i < 2 * N; ++i)
        {
            hpx::cout << hpx::naming::get_locality_id_from_id(t1.call())
                      << std::flush;
            HPX_TEST_EQ(t1.lazy_get_data(), 42);
        }
    });

    hpx::wait_all(migrate_future, create_work);

    // rethrow exceptions
    try
    {
        migrate_future.get();
        create_work.get();
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    hpx::cout << std::endl;

    return true;
}

bool test_migrate_lazy_busy_component_client2(
    hpx::id_type source, hpx::id_type target)
{
    test_client const t1 = hpx::new_<test_client>(source, 42);
    HPX_TEST_NEQ(hpx::invalid_id, t1.get_id());

    // the new object should live on the source locality
    HPX_TEST_EQ(t1.call(), source);
    HPX_TEST_EQ(t1.get_data(), 42);

    // First, spawn a thread (locally) that will migrate the object between
    // source and target a few times
    hpx::future<void> migrate_future =
        hpx::async([source, target, t1]() mutable {
            for (std::size_t i = 0; i < N; ++i)
            {
                // migrate t1 to the target (loc2)
                test_client const t2(hpx::components::migrate(t1, target));

                HPX_TEST_EQ(t1.lazy_get_client(target).call(), target);

                // wait for migration to be done
                HPX_TEST_NEQ(hpx::invalid_id, t2.get_id());

                // the migrated object should have the same id as before
                HPX_TEST_EQ(t1.get_id(), t2.get_id());

                // the migrated object should live on the target now
                HPX_TEST_EQ(t2.call(), target);
                HPX_TEST_EQ(t2.lazy_get_client(source).call(), source);

                hpx::cout << "." << std::flush;

                std::swap(source, target);
            }

            hpx::cout << std::endl;
        });

    // Second, we generate tons of work that should automatically follow
    // the migration.
    hpx::future<void> create_work = hpx::async([t1, target]() {
        for (std::size_t i = 0; i < 2 * N; ++i)
        {
            hpx::cout << hpx::naming::get_locality_id_from_id(t1.call())
                      << std::flush;
            HPX_TEST_EQ(t1.lazy_get_client(target).call(), target);
        }
    });

    hpx::wait_all(migrate_future, create_work);

    // rethrow exceptions
    try
    {
        migrate_future.get();
        create_work.get();
    }
    catch (hpx::exception const& e)
    {
        hpx::cout << hpx::get_error_what(e) << std::endl;
        return false;
    }

    hpx::cout << std::endl;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_last_unpin_keeps_migration_state_alive();

    std::vector<hpx::id_type> const localities = hpx::find_all_localities();

    auto const remote = std::find_if(localities.begin(), localities.end(),
        [](hpx::id_type const& id) { return id != hpx::find_here(); });
    if (remote != localities.end())
    {
        HPX_TEST(test_migrate_component_rejects_stale_lva(
            hpx::find_here(), *remote));
    }

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_component(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_component_client: ->" << id
                  << std::endl;
        HPX_TEST(test_migrate_lazy_component_client(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_component_client: <-" << id
                  << std::endl;
        HPX_TEST(test_migrate_lazy_component_client(id, hpx::find_here()));
    }

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_busy_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_busy_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_busy_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_busy_component(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_busy_component: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_busy_component: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_busy_component_client: ->" << id
                  << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component_client(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_busy_component_client: <-" << id
                  << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component_client(id, hpx::find_here()));
    }

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_component2(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_component2(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_component_client2: ->" << id
                  << std::endl;
        HPX_TEST(test_migrate_lazy_component_client2(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_component_client2: <-" << id
                  << std::endl;
        HPX_TEST(test_migrate_lazy_component_client2(id, hpx::find_here()));
    }

    for (hpx::id_type const& id : localities)
    {
        hpx::cout << "test_migrate_busy_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_busy_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_busy_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_busy_component2(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_busy_component2: ->" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component2(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_busy_component2: <-" << id << std::endl;
        HPX_TEST(test_migrate_lazy_busy_component2(id, hpx::find_here()));

        hpx::cout << "test_migrate_lazy_busy_component_client2: ->" << id
                  << std::endl;
        HPX_TEST(
            test_migrate_lazy_busy_component_client2(hpx::find_here(), id));
        hpx::cout << "test_migrate_lazy_busy_component_client2: <-" << id
                  << std::endl;
        HPX_TEST(
            test_migrate_lazy_busy_component_client2(id, hpx::find_here()));
    }

    return hpx::util::report_errors();
}
#endif
