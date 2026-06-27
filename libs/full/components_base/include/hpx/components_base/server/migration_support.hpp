//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components_base/agas_interface.hpp>
#include <hpx/components_base/pinned_ptr.hpp>
#include <hpx/components_base/traits/action_decorate_function.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <type_traits>
#include <utility>

namespace hpx::components {

    /// \cond NOINTERNAL
    namespace detail {

        template <typename Mutex>
        struct migration_support_data
        {
            migration_support_data() noexcept
              : count_(1)
            {
            }

            migration_support_data(migration_support_data const&) = delete;
            migration_support_data(migration_support_data&&) = delete;
            migration_support_data& operator=(
                migration_support_data const&) = delete;
            migration_support_data& operator=(
                migration_support_data&&) = delete;

            ~migration_support_data()
            {
                // make sure object is deleted only after all pin-counts have
                // been given back
                HPX_ASSERT(pin_count_ == ~0x0u || pin_count_ == 0);
            }

            mutable Mutex mtx_;
            std::uint32_t pin_count_ = 0;

        private:
            friend void intrusive_ptr_add_ref(
                migration_support_data* p) noexcept
            {
                p->count_.increment();
            }
            friend void intrusive_ptr_release(
                migration_support_data* p) noexcept
            {
                if (0 == p->count_.decrement())
                {
                    // The thread that decrements the reference count to zero
                    // must perform an acquire to ensure that it doesn't start
                    // destructing the object until all previous writes have
                    // drained.
                    std::atomic_thread_fence(std::memory_order_acquire);

                    delete p;
                }
            }

            hpx::util::atomic_count count_;
        };
    }    // namespace detail
    /// \endcond

    /// This hook has to be inserted into the derivation chain of any component
    /// for it to support migration.
    HPX_CXX_EXPORT template <typename BaseComponent,
        typename Mutex = hpx::spinlock>
    struct migration_support : BaseComponent
    {
    private:
        using base_type = BaseComponent;
        using this_component_type = typename base_type::this_component_type;

    public:
        migration_support()
          : data_(new detail::migration_support_data<Mutex>(), false)
        {
        }

        template <typename T, typename... Ts,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<T>, migration_support>>>
        explicit migration_support(T&& t, Ts&&... ts)
          : base_type(HPX_FORWARD(T, t), HPX_FORWARD(Ts, ts)...)
          , data_(new detail::migration_support_data<Mutex>(), false)
        {
        }

        migration_support(migration_support const&) = default;
        migration_support(migration_support&&) = default;
        migration_support& operator=(migration_support const&) = default;
        migration_support& operator=(migration_support&&) = default;

        ~migration_support() = default;

        naming::gid_type get_base_gid(
            naming::gid_type const& assign_gid = naming::invalid_gid) const
        {
            return this->BaseComponent::get_base_gid_dynamic(assign_gid,
                static_cast<this_component_type const&>(*this)
                    .get_current_address(),
                [](naming::gid_type gid) -> naming::gid_type {
                    // we don't store migrating objects in the AGAS cache
                    naming::detail::set_dont_store_in_cache(gid);
                    // also mark gid as migratable
                    naming::detail::set_is_migratable(gid);
                    return gid;
                });
        }

        // This component type supports migration.
        [[nodiscard]] static constexpr bool supports_migration() noexcept
        {
            return true;
        }

        // Pinning functionality
        bool pin() noexcept
        {
            intrusive_ptr_add_ref(data_.get());    // keep alive

            std::unique_lock l(data_->mtx_);

            if (data_->pin_count_ != ~0x0u)
            {
                // there shouldn't be any pinning happening once the pin-count
                // has gone to zero and has triggered a migration
                HPX_ASSERT_LOCKED(l,
                    data_->pin_count_ != 0 ||
                        (!started_migration_ && !was_marked_for_migration_));
                ++data_->pin_count_;
                return true;
            }

            intrusive_ptr_release(data_.get());    // release keep alive

            return false;
        }

        bool unpin()
        {
            // pin() acquired an additional reference count that needs to be
            // released after unpinning.
            auto on_exit = hpx::experimental::scope_exit(
                [this] { intrusive_ptr_release(data_.get()); });

            {
                // no need to go through AGAS if the object is currently pinned
                // more than once
                std::unique_lock l(data_->mtx_);

                if (data_->pin_count_ != ~0x0u && data_->pin_count_ > 1)
                {
                    --data_->pin_count_;
                    return false;
                }

                // no need to go through AGAS either if this object is not
                // currently being migrated (unpin will be called for each
                // action that runs on this object)
                if (!was_marked_for_migration_)
                {
                    if (data_->pin_count_ != ~0x0u)
                    {
                        --data_->pin_count_;
                    }
                    return false;
                }
            }

            // make sure to always grab the AGAS lock first
            bool was_migrated = false;
            agas::mark_as_migrated(
                this->gid_,
                [this, &was_migrated]() -> std::pair<bool, hpx::future<void>> {
                    std::unique_lock l(data_->mtx_);

                    was_migrated = data_->pin_count_ == ~0x0u;

                    HPX_ASSERT_LOCKED(l, data_->pin_count_ != 0);
                    if (data_->pin_count_ != ~0x0u)
                    {
                        if (--data_->pin_count_ == 0)
                        {
                            // trigger pending migration if this was the last
                            // unpin and a migration operation is pending
                            HPX_ASSERT_LOCKED(l, trigger_migration_.valid());
                            if (was_marked_for_migration_)
                            {
                                hpx::promise<void> p;
                                std::swap(p, trigger_migration_);

                                l.unlock();

                                p.set_value();
                                return std::make_pair(
                                    true, make_ready_future());
                            }
                        }
                    }
                    return std::make_pair(false, make_ready_future());
                },
                true)
                .get();

            return was_migrated;
        }

        [[nodiscard]] std::uint32_t pin_count() const noexcept
        {
            auto const data = data_;    // keep alive
            std::unique_lock l(data->mtx_);

            return data->pin_count_;
        }
        void mark_as_migrated()
        {
            auto const data = data_;    // keep alive
            std::unique_lock l(data->mtx_);

            HPX_ASSERT_LOCKED(l, 0 == data->pin_count_);
            data->pin_count_ = ~0x0u;

            // prevent base destructor from unregistering the gid if this
            // instance was migrated
            this->gid_ = naming::invalid_gid;
        }

        hpx::future<void> mark_as_migrated(hpx::id_type const& to_migrate)
        {
            // we need to first lock the AGAS migrated objects table, only then
            // access (lock) the object
            return agas::mark_as_migrated(
                to_migrate.get_gid(),
                [this]() mutable -> std::pair<bool, hpx::future<void>> {
                    auto const data = data_;    // keep alive
                    std::unique_lock l(data->mtx_);

                    // make sure that no migration is currently in flight
                    if (was_marked_for_migration_)
                    {
                        l.unlock();
                        return std::make_pair(false,
                            hpx::make_exceptional_future<
                                void>(HPX_GET_EXCEPTION(
                                hpx::error::invalid_status,
                                "migration_support::mark_as_migrated",
                                "migration operation is already in flight")));
                    }

                    if (1 == data->pin_count_)
                    {
                        // all is well, migration can be triggered now
                        started_migration_ = true;
                        return std::make_pair(true, make_ready_future());
                    }

                    // delay migrate operation until pin count goes to zero
                    was_marked_for_migration_ = true;
                    hpx::future<void> f = trigger_migration_.get_future();

                    l.unlock();
                    return std::make_pair(true, HPX_MOVE(f));
                },
                false);
        }

        // This hook is invoked if a migration step is being cancelled
        void unmark_as_migrated(hpx::id_type const& to_migrate)
        {
            // we need to first lock the AGAS migrated objects table, only then
            // access (lock) the object
            agas::unmark_as_migrated(to_migrate.get_gid(), [this] {
                auto const data = data_;    // keep alive
                std::unique_lock l(data->mtx_);

                HPX_ASSERT_LOCKED(l, 0 == data->pin_count_);

                started_migration_ = false;
                was_marked_for_migration_ = false;
            });
        }

        // This hook is invoked on the newly created object after the migration
        // has been finished
        static constexpr void on_migrated() noexcept {}

        using decorates_action = void;

        // This is the hook implementation for decorate_action which makes
        // sure that the object becomes pinned during the execution of an
        // action.
        template <typename F>
        static threads::thread_function_type decorate_action(
            naming::address_type lva, F&& f)
        {
            // Make sure we pin the component at construction of the bound object
            // which will also unpin it once the thread runs to completion (the
            // bound object goes out of scope).
            return util::one_shot(
                hpx::bind_front(&migration_support::thread_function,
                    get_lva<this_component_type>::call(lva),
                    traits::component_decorate_function<base_type>::call(
                        lva, HPX_FORWARD(F, f)),
                    components::pinned_ptr::create<this_component_type>(lva)));
        }

        // Return whether the given object was migrated, if it was not
        // migrated, it also returns a pinned pointer.
        static std::pair<bool, components::pinned_ptr> was_object_migrated(
            hpx::naming::gid_type const& id, naming::address_type lva)
        {
            return agas::was_object_migrated(
                id, [lva]() -> components::pinned_ptr {
                    return components::pinned_ptr::create<this_component_type>(
                        lva);
                });
        }

    protected:
        // Execute the wrapped action. This function is bound in decorate_action
        // above. The bound object performs the pinning/unpinning.
        threads::thread_result_type thread_function(
            threads::thread_function_type&& f, components::pinned_ptr,
            threads::thread_restart_state state)
        {
            return f(state);
        }

    private:
        hpx::intrusive_ptr<detail::migration_support_data<Mutex>> data_;
        hpx::promise<void> trigger_migration_;
        bool started_migration_ = false;
        bool was_marked_for_migration_ = false;
    };
}    // namespace hpx::components
