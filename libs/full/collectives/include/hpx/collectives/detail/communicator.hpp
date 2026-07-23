//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/assert.hpp>
#include <hpx/collectives/argument_types.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <exception>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::traits {

    // This type can be specialized for a particular collective operation
    template <typename Communicator, typename Operation>
    struct communication_operation;

    namespace communication {

        // Retrieve name of the current communicator
        template <typename Operation>
        struct communicator_data
        {
            static constexpr char const* name() noexcept
            {
                return "<unknown>";
            }
        };
    }    // namespace communication
}    // namespace hpx::traits

namespace hpx::collectives::detail {

    ///////////////////////////////////////////////////////////////////////////
    class communicator_server
      : public hpx::components::component_base<communicator_server>
    {
        using mutex_type = hpx::spinlock;

    public:
        HPX_EXPORT communicator_server() noexcept;

        HPX_EXPORT explicit communicator_server(
            std::size_t num_sites, std::string basename);

        communicator_server(communicator_server const&) = delete;
        communicator_server(communicator_server&&) = delete;
        communicator_server& operator=(communicator_server const&) = delete;
        communicator_server& operator=(communicator_server&&) = delete;

        HPX_EXPORT ~communicator_server();

    private:
        template <typename Operation>
        struct logging_helper
        {
#if defined(HPX_HAVE_LOGGING)
            logging_helper(
                std::size_t which, std::size_t generation, char const* op)
              : which_(which)
              , generation_(generation)
              , op_(op)
            {
                LHPX_(info, " [COL] ")
                    .format("{}(>>> {}): which({}), generation({})", op,
                        traits::communication::communicator_data<
                            Operation>::name(),
                        which, generation);
            }

            ~logging_helper()
            {
                LHPX_(info, " [COL] ")
                    .format("{}(<<< {}): which({}), generation({})", op_,
                        traits::communication::communicator_data<
                            Operation>::name(),
                        which_, generation_);
            }

            std::size_t which_;
            std::size_t generation_;
            char const* op_;
#else
            constexpr logging_helper(
                std::size_t, std::size_t, char const*) noexcept
            {
            }

            ~logging_helper() = default;
#endif

            logging_helper(logging_helper const&) = delete;
            logging_helper(logging_helper&&) = delete;
            logging_helper& operator=(logging_helper const&) = delete;
            logging_helper& operator=(logging_helper&&) = delete;
        };

    public:
        ///////////////////////////////////////////////////////////////////////
        // generic get action, dispatches to proper operation
        template <typename Operation, typename Result, typename... Args>
        Result get_result(
            std::size_t which, std::size_t generation, Args... args)
        {
            using collective_operation =
                traits::communication_operation<communicator_server, Operation>;

            [[maybe_unused]] logging_helper<Operation> log(
                which, generation, "get");

            return collective_operation::template get<Result>(
                *this, which, generation, HPX_MOVE(args)...);
        }

        template <typename Operation, typename Result, typename... Args>
        struct communication_get_action
          : hpx::actions::action<Result (communicator_server::*)(
                                     std::size_t, std::size_t, Args...),
                &communicator_server::get_result<Operation, Result, Args...>,
                communication_get_action<Operation, Result, Args...>>
        {
        };

        template <typename Operation, typename Result, typename... Args>
        struct communication_get_direct_action
          : hpx::actions::direct_action<Result (communicator_server::*)(
                                            std::size_t, std::size_t, Args...),
                &communicator_server::get_result<Operation, Result, Args...>,
                communication_get_direct_action<Operation, Result, Args...>>
        {
        };

        template <typename Operation, typename Result, typename... Args>
        Result set_result(
            std::size_t which, std::size_t generation, Args... args)
        {
            using collective_operation =
                traits::communication_operation<communicator_server, Operation>;

            [[maybe_unused]] logging_helper<Operation> log(
                which, generation, "set");

            return collective_operation::template set<Result>(
                *this, which, generation, HPX_MOVE(args)...);
        }

        template <typename Operation, typename Result, typename... Args>
        struct communication_set_action
          : hpx::actions::action<Result (communicator_server::*)(
                                     std::size_t, std::size_t, Args...),
                &communicator_server::set_result<Operation, Result, Args...>,
                communication_set_action<Operation, Result, Args...>>
        {
        };

        template <typename Operation, typename Result, typename... Args>
        struct communication_set_direct_action
          : hpx::actions::direct_action<Result (communicator_server::*)(
                                            std::size_t, std::size_t, Args...),
                &communicator_server::set_result<Operation, Result, Args...>,
                communication_set_direct_action<Operation, Result, Args...>>
        {
        };

    private:
        [[nodiscard]] constexpr std::size_t get_num_sites(
            std::size_t num_values) const noexcept
        {
            return num_values == static_cast<std::size_t>(-1) ? num_sites_ :
                                                                num_values;
        }

        // re-initialize data
        template <typename T>
        void reinitialize_data(std::size_t num_values)
        {
            if (needs_initialization_)
            {
                needs_initialization_ = false;
                data_available_ = false;
                operation_error_ = nullptr;

                if constexpr (!std::is_void_v<T>)
                {
                    auto const new_size = get_num_sites(num_values);
                    auto const* data = hpx::any_cast<std::vector<T>>(&data_);
                    if (data == nullptr || data->size() < new_size)
                    {
                        data_ = std::vector<T>(new_size);
                    }
                }
            }
        }

        template <typename T>
        std::vector<T>& access_data(
            std::size_t num_values = static_cast<std::size_t>(-1))
        {
            reinitialize_data<T>(num_values);
            return hpx::any_cast<std::vector<T>&>(data_);
        }

        template <typename Lock>
        void invalidate_data(Lock& l) noexcept
        {
            HPX_ASSERT_OWNS_LOCK(l);
            if (!needs_initialization_)
            {
                needs_initialization_ = true;
                data_available_ = false;
                operation_error_ = nullptr;
                on_ready_count_ = 0;
                current_operation_ = nullptr;
            }
        }

        template <typename F, typename Lock>
        auto get_future_and_synchronize(std::size_t generation, F&& f, Lock& l)
        {
            HPX_ASSERT_OWNS_LOCK(l);

            // Wait for the requested generation to be processed.
            gate_.synchronize(generation == generation_arg{} ?
                    gate_.generation(l) :
                    generation,
                l);

            // Get future from gate only after synchronization as otherwise we
            // may get a future returned that does not belong to the requested
            // generation.
            auto sf = gate_.get_shared_future(l);

            traits::detail::get_shared_state(sf)->reserve_callbacks(num_sites_);

            return sf.then(hpx::launch::sync, HPX_FORWARD(F, f));
        }

        template <typename Lock>
        bool set_operation_and_check_sequencing(Lock& l, char const* operation,
            std::size_t which, std::size_t generation)
        {
            if (current_operation_ == nullptr)
            {
                if (on_ready_count_ != 0)
                {
                    l.unlock();
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "communicator::handle_data",
                        "communicator: {}: sequencing error, on_ready callback "
                        "was already invoked before the start of the "
                        "collective operation {}, which {}, generation {}.",
                        basename_, operation, which, generation);
                }

                if (generation == generation_arg{} ||
                    generation == gate_.generation(l))
                {
                    current_operation_ = operation;
                }

                return true;
            }

            return false;
        }

        // Step will be invoked under lock for each site that checks in (either
        // set or get).
        //
        // Finalizer will be invoked under lock after all sites have checked in.
        // num_generations is how many internal generations this operation
        // consumes on this communicator's gate (default generation_mode::
        // single_step). A hierarchical collective that touches a communicator
        // only once per user call but has to stay in lock-step with collectives
        // that touch it twice passes generation_mode::double_step, advancing the
        // gate by two in a single step so the skipped generation is consumed
        // here instead of through a second round-trip. It comes before
        // num_values so the common callers (which never override num_values) can
        // leave that argument off.
        template <typename Data, typename Step, typename Finalizer>
        auto handle_data(char const* operation, std::size_t which,
            std::size_t generation, [[maybe_unused]] Step&& step,
            Finalizer&& finalizer,
            generation_mode num_generations = generation_mode::single_step,
            std::size_t num_values = static_cast<std::size_t>(-1))
        {
            if (which >= num_sites_)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "communicator::handle_data",
                    "site index must be smaller than the number of "
                    "participating sites");
            }

            auto on_ready = [this, operation, which, generation, num_values,
                                finalizer = HPX_FORWARD(Finalizer, finalizer)](
                                shared_future<void>&& f) mutable {
                // This callback will be invoked once for each participating
                // site after all sites have checked in.

                // On exit, keep track of number of invocations of this
                // callback.
                auto on_exit = hpx::experimental::scope_exit(
                    [this] { ++on_ready_count_; });

                f.get();    // propagate any exceptions

                // It does not matter whether the lock will be acquired here. It
                // either is still being held by the surrounding logic or is
                // re-acquired here (if `on_ready` happens to run on a new
                // thread asynchronously).
                std::unique_lock l(mtx_, std::try_to_lock);
                //[[maybe_unused]] util::ignore_while_checking il(&l);

                // Verify that there is no overlap between different types of
                // operations on the same communicator.
                if (current_operation_ == nullptr ||
                    current_operation_ != operation)
                {
                    l.unlock();
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "communicator::handle_data::on_ready",
                        "communicator {}: sequencing error, operation type "
                        "mismatch: invoked for {}, ongoing operation {}, which "
                        "{}, generation {}.",
                        basename_, operation,
                        current_operation_ ? current_operation_ : "unknown",
                        which, generation);
                }

                // Verify that the number of invocations of this callback is in
                // the expected range.
                if (on_ready_count_ >= num_sites_)
                {
                    l.unlock();
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "communicator::handle_data::on_ready",
                        "communicator {}: sequencing error, an excessive "
                        "number of on_ready callbacks have been invoked before "
                        "the end of the collective operation {}, which {}, "
                        "generation {}. Expected count {}, received count {}.",
                        basename_, operation, which, generation, num_sites_,
                        on_ready_count_);
                }

                // A step or finalizer that threw for an earlier site has
                // typically consumed (moved from) state the reduction relies
                // on, or has left the collected data incomplete. Re-invoking
                // the finalizer here would operate on that state, letting
                // different sites observe different outcomes for the same
                // collective. Instead, rethrow the cached first failure for
                // every site of this operation, including sites that pass no
                // finalizer.
                if (operation_error_)
                {
                    std::rethrow_exception(operation_error_);
                }

                if constexpr (!std::is_same_v<std::nullptr_t,
                                  std::decay_t<Finalizer>>)
                {
                    try
                    {
                        if constexpr (std::is_void_v<Data>)
                        {
                            return HPX_FORWARD(Finalizer, finalizer)(
                                data_available_, which);
                        }
                        else
                        {
                            return HPX_FORWARD(Finalizer, finalizer)(
                                access_data<Data>(num_values), data_available_,
                                which);
                        }
                    }
                    catch (...)
                    {
                        operation_error_ = std::current_exception();
                        throw;
                    }
                }
                else
                {
                    HPX_UNUSED(this);
                    HPX_UNUSED(num_values);
                    HPX_UNUSED(finalizer);
                }
            };

            std::unique_lock l(mtx_);
            [[maybe_unused]] util::ignore_all_while_checking il;

            // An explicit generation number is usable only while the gate
            // position remains a pure function of the numbers supplied so far.
            // An auto (default) generation advances the gate without telling
            // the caller by how much, leaving no reliable way to choose the
            // next explicit number: too small throws, too large waits forever.
            // Latch the first auto use and reject the transition up front; the
            // reverse order stays valid because an auto generation always
            // synchronizes on the gate's current position.
            if (generation == generation_arg{})
            {
                auto_generation_used_ = true;
            }
            else if (auto_generation_used_)
            {
                l.unlock();
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "communicator::handle_data",
                    "communicator {}: an explicit generation number cannot "
                    "follow auto-generation operations on the same "
                    "communicator: operation {}, which {}, generation {}.",
                    basename_, operation, which, generation);
            }

            // Verify that there is no overlap between different types of
            // operations on the same communicator.
            set_operation_and_check_sequencing(l, operation, which, generation);

            auto f =
                get_future_and_synchronize(generation, HPX_MOVE(on_ready), l);

            // We may have just finished a different operation, thus we have to
            // possibly reset the operation type stored in this communicator.
            if (current_operation_ != operation &&
                !set_operation_and_check_sequencing(
                    l, operation, which, generation))
            {
                l.unlock();
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "communicator::handle_data",
                    "communicator {}: sequencing error, operation type "
                    "mismatch: invoked for {}, ongoing operation {}, which {}, "
                    "generation {}.",
                    basename_, operation, current_operation_, which,
                    generation);
            }

            if constexpr (std::is_void_v<Data>)
            {
                reinitialize_data<void>(num_values);
            }

            if constexpr (!std::is_same_v<std::nullptr_t, std::decay_t<Step>>)
            {
                // A throwing step (a throwing move assignment of the payload,
                // bad_alloc while allocating the data vector) must not leave
                // the gate waiting for a segment that would never be set.
                // Cache the first failure and let the collective complete;
                // every site then observes the cached exception from its
                // on_ready callback.
                try
                {
                    if constexpr (std::is_void_v<Data>)
                    {
                        HPX_FORWARD(Step, step)(which);
                    }
                    else
                    {
                        HPX_FORWARD(Step, step)(
                            access_data<Data>(num_values), which);
                    }
                }
                catch (...)
                {
                    if (!operation_error_)
                    {
                        operation_error_ = std::current_exception();
                    }
                }
            }

            // Make sure next generation is enabled only after previous
            // generation has finished executing.
            gate_.set(which, l,
                [this, operation, which, generation, num_generations](
                    auto& l, auto& gate, error_code& ec) {
                    // This callback is invoked synchronously once for each
                    // collective operation after all data has been received and
                    // all (shared) futures were triggered.

                    HPX_ASSERT_OWNS_LOCK(l);

                    // Verify that all `on_ready` callbacks have finished
                    // executing at this point.
                    if (on_ready_count_ != num_sites_)
                    {
                        l.unlock();
                        HPX_THROWS_IF(ec, hpx::error::invalid_status,
                            "communicator::handle_data",
                            "sequencing error, not all on_ready callbacks have "
                            "been invoked at the end of the collective {} "
                            "operation. Expected count {}, received count {}, "
                            "which {}, generation {}.",
                            operation, num_sites_, on_ready_count_, which,
                            generation);
                        return;
                    }

                    // Reset communicator state before proceeding to the next
                    // generation.
                    invalidate_data(l);

                    // Release threads possibly waiting for the next generation
                    // to be handled. When this operation consumes more than one
                    // generation, advance the gate past the skipped ones in a
                    // single step (the gate only requires that the next value
                    // is not smaller than the current one). An auto generation
                    // (the default sentinel) always advances by one; the assert
                    // enforces that a multi-generation step is only requested
                    // with an explicit generation. Note that the step reduces to
                    // generation when num_generations == single_step.
                    HPX_ASSERT(
                        num_generations == generation_mode::single_step ||
                        generation != generation_arg{});
                    std::size_t const next_gen =
                        generation == generation_arg{} ? generation :
                                                         generation +
                            static_cast<std::size_t>(num_generations) - 1;
                    gate.next_generation(l, next_gen, ec);
                });

            return f;
        }

        template <typename Communicator, typename Operation>
        friend struct hpx::traits::communication_operation;

        hpx::unique_any_nonser data_;
        hpx::lcos::local::and_gate gate_;
        std::size_t const num_sites_;
        std::size_t on_ready_count_ = 0;
        char const* current_operation_ = nullptr;
        // Owned copy for diagnostics only. Hierarchical tree construction
        // derives communicator basenames from stack std::strings that are
        // destroyed when recursively_fill_communicators returns, while this
        // component outlives that frame. A borrowed pointer would therefore
        // dangle on later exception paths. The constructor takes the owned
        // name by value and moves it into place.
        std::string basename_;
        mutex_type mtx_;
        // First exception thrown by a step or finalizer of the current
        // operation; rethrown for every site instead of (re-)invoking the
        // finalizer on moved-from or incomplete data. Reset together with
        // data_available_.
        std::exception_ptr operation_error_;
        bool needs_initialization_ = true;
        bool data_available_ = false;
        bool auto_generation_used_ = false;
    };
}    // namespace hpx::collectives::detail

#endif    // COMPUTE_HOST_CODE
