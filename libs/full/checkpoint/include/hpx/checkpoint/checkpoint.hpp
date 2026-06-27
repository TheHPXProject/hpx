// Copyright (c) 2018 Adrian Serio
// Copyright (c) 2018-2026 Hartmut Kaiser
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// This header defines the save_checkpoint and restore_checkpoint functions.
/// These functions are designed to help HPX application developer's checkpoint
/// their applications. Save_checkpoint serializes one or more objects and saves
/// them as a byte stream. Restore_checkpoint converts the byte stream back into
/// instances of the objects.

/// \file hpx/checkpoint/checkpoint.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/checkpoint_base.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    // Forward declarations
    HPX_CXX_EXPORT class checkpoint;

    ///////////////////////////////////////////////////////////////////////////
    /// Operator<< Overload
    ///
    /// \param ost           Output stream to write to.
    ///
    /// \param ckp           Checkpoint to copy from.
    ///
    /// This overload is the main way to write data from a checkpoint to an
    /// object such as a file. Inside the function, the size of the checkpoint
    /// will be written to the stream before the checkpoint's data. The
    /// operator>> overload uses this to read the correct number of bytes. Be
    /// mindful of this additional write and read when you use different
    /// facilities to write out or read in data to a checkpoint!
    ///
    /// \returns Operator<< returns the ostream object.
    ///
    HPX_CXX_EXPORT HPX_EXPORT std::ostream& operator<<(
        std::ostream& ost, checkpoint const& ckp);

    ///////////////////////////////////////////////////////////////////////////
    /// Operator>> Overload
    ///
    /// \param ist           Input stream to write from.
    ///
    /// \param ckp           Checkpoint to write to.
    ///
    /// This overload is the main way to read in data from an object such as a
    /// file to a checkpoint. It is important to note that inside the function,
    /// the first variable to be read is the size of the checkpoint. This size
    /// variable is written to the stream before the checkpoint's data in the
    /// operator<< overload. Be mindful of this additional read and write when
    /// you use different facilities to read in or write out data from a
    /// checkpoint!
    ///
    /// \returns Operator>> returns the ostream object.
    ///
    HPX_CXX_EXPORT HPX_EXPORT std::istream& operator>>(
        std::istream& ist, checkpoint& ckp);

    namespace detail {

        struct save_funct_obj;
        struct prepare_checkpoint;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Checkpoint Object
    ///
    /// Checkpoint is the container object which is produced by save_checkpoint
    /// and is consumed by a restore_checkpoint. A checkpoint may be moved into
    /// the save_checkpoint object to write the byte stream to the pre-created
    /// checkpoint object.
    ///
    /// Checkpoints are able to store all containers which are serializable
    /// including components.
    HPX_CXX_EXPORT class checkpoint
    {
    private:
        std::vector<char> data_;

        friend HPX_EXPORT std::ostream& operator<<(
            std::ostream& ost, checkpoint const& ckp);
        friend HPX_EXPORT std::istream& operator>>(
            std::istream& ist, checkpoint& ckp);

        // Serialization Definition
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& arch, unsigned int const /* version */)
        {
            // clang-format off
            arch & data_;
            // clang-format on
        }

        friend struct detail::save_funct_obj;
        friend struct detail::prepare_checkpoint;

        template <typename T, typename... Ts>
        friend void restore_checkpoint(checkpoint const& c, T& t, Ts&... ts);

    public:
        checkpoint() = default;
        ~checkpoint() = default;

        checkpoint(checkpoint const& c) = default;
        checkpoint(checkpoint&& c) noexcept = default;

        // Other Constructors
        explicit checkpoint(std::vector<char> const& vec)
          : data_(vec)
        {
        }

        explicit checkpoint(std::vector<char>&& vec) noexcept
          : data_(HPX_MOVE(vec))
        {
        }

        // Overloads
        checkpoint& operator=(checkpoint const& c) = default;
        checkpoint& operator=(checkpoint&& c) noexcept = default;

        friend bool operator==(
            checkpoint const& lhs, checkpoint const& rhs) noexcept
        {
            return lhs.data_ == rhs.data_;
        }
        friend bool operator!=(
            checkpoint const& lhs, checkpoint const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        // Iterators
        //  expose iterators to access data held by checkpoint
        using const_iterator = std::vector<char>::const_iterator;

        const_iterator begin() const noexcept
        {
            return data_.begin();
        }
        const_iterator end() const noexcept
        {
            return data_.end();
        }

        // Functions
        std::size_t size() const noexcept
        {
            return data_.size();
        }

        char* data() noexcept
        {
            return data_.data();
        }
        char const* data() const noexcept
        {
            return data_.data();
        }
    };

    // Function objects for save_checkpoint
    namespace detail {

        // Properly handle non clients
        template <typename T>
            requires(!hpx::traits::is_client_v<std::decay_t<T>>)
        constexpr T&& prepare_client(T&& t) noexcept
        {
            return HPX_FORWARD(T, t);
        }

        // Properly handle Clients to components
        template <typename Client, typename Server, typename Data>
        hpx::future<std::shared_ptr<typename hpx::components::client_base<
            Client, Server, Data>::server_component_type>>
        prepare_client(
            hpx::components::client_base<Client, Server, Data> const& c)
        {
            // Use shared pointer to serialize server
            using server_type = hpx::components::client_base<Client, Server,
                Data>::server_component_type;
            return hpx::get_ptr<server_type>(c.get_id());
        }

        struct save_funct_obj
        {
            template <typename... Ts>
            checkpoint operator()(checkpoint&& c, Ts&&... ts) const
            {
                hpx::util::save_checkpoint_data(
                    c.data_, HPX_FORWARD(Ts, ts)...);
                return HPX_MOVE(c);
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Save_checkpoint
    ///
    /// \tparam T            Containers passed to save_checkpoint to be
    ///                      serialized and placed into a checkpoint object.
    ///
    /// \tparam Ts           More containers passed to save_checkpoint
    ///                      to be serialized and placed into a checkpoint
    ///                      object.
    ///
    /// \tparam U            This parameter is used to make sure that T
    ///                      is not a launch policy or a checkpoint. This forces
    ///                      the compiler to choose the correct overload.
    ///
    /// \param t             A container to restore.
    ///
    /// \param ts            Other containers to restore Containers
    ///                      must be in the same order that they were inserted
    ///                      into the checkpoint.
    ///
    /// Save_checkpoint takes any number of objects which a user may wish to
    /// store and returns a future to a checkpoint object. This function can
    /// also store a component either by passing a shared_ptr to the component
    /// or by passing a component's client instance to save_checkpoint.
    /// Additionally, the function can take a policy as a first object which
    /// changes its behavior depending on the policy passed to it. Most notably,
    /// if a sync policy is used save_checkpoint will simply return a checkpoint
    /// object.
    ///
    /// \returns Save_checkpoint returns a future to a checkpoint with one
    ///          exception: if you pass hpx::launch::sync as the first argument.
    ///          In this case save_checkpoint will simply return a checkpoint.
    HPX_CXX_EXPORT template <typename T, typename... Ts>
        requires(!hpx::traits::is_launch_policy_v<T> &&
            !std::is_same_v<std::decay_t<T>, checkpoint>)
    hpx::future<checkpoint> save_checkpoint(T&& t, Ts&&... ts)
    {
        return hpx::dataflow(detail::save_funct_obj{}, checkpoint{},
            detail::prepare_client(HPX_FORWARD(T, t)),
            detail::prepare_client(HPX_FORWARD(Ts, ts))...);
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> save_checkpoint()
    {
        return hpx::make_ready_future(checkpoint{});
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// Save_checkpoint - Take a pre-initialized checkpoint
    ///
    /// \tparam T            Containers passed to save_checkpoint to be
    ///                      serialized and placed into a checkpoint object.
    ///
    /// \tparam Ts           More containers passed to save_checkpoint
    ///                      to be serialized and placed into a checkpoint
    ///                      object.
    ///
    /// \param c             Takes a pre-initialized checkpoint to copy
    ///                      data into.
    ///
    /// \param t             A container to restore.
    ///
    /// \param ts            Other containers to restore Containers
    ///                      must be in the same order that they were inserted
    ///                      into the checkpoint.
    ///
    /// Save_checkpoint takes any number of objects which a user may wish to
    /// store and returns a future to a checkpoint object. This function can
    /// also store a component either by passing a shared_ptr to the component
    /// or by passing a component's client instance to save_checkpoint.
    /// Additionally, the function can take a policy as a first object which
    /// changes its behavior depending on the policy passed to it. Most notably,
    /// if a sync policy is used save_checkpoint will simply return a checkpoint
    /// object.
    ///
    /// \returns Save_checkpoint returns a future to a checkpoint with one
    ///          exception: if you pass hpx::launch::sync as the first argument.
    ///          In this case save_checkpoint will simply return a checkpoint.
    ///
    HPX_CXX_EXPORT template <typename T, typename... Ts>
    hpx::future<checkpoint> save_checkpoint(checkpoint&& c, T&& t, Ts&&... ts)
    {
        return hpx::dataflow(detail::save_funct_obj{}, HPX_MOVE(c),
            detail::prepare_client(HPX_FORWARD(T, t)),
            detail::prepare_client(HPX_FORWARD(Ts, ts))...);
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> save_checkpoint(
        checkpoint&& c)
    {
        return hpx::make_ready_future(HPX_MOVE(c));
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// Save_checkpoint - Policy overload
    ///
    /// \tparam T            Containers passed to save_checkpoint to be
    ///                      serialized and placed into a checkpoint object.
    ///
    /// \tparam Ts           More containers passed to save_checkpoint
    ///                      to be serialized and placed into a
    ///                      checkpoint object.
    ///
    /// \param p             Takes an HPX launch policy. Allows the user
    ///                      to change the way the function is launched
    ///                      i.e. async, sync, etc.
    ///
    /// \param t             A container to restore.
    ///
    /// \param ts            Other containers to restore Containers
    ///                      must be in the same order that they were
    ///                      inserted into the checkpoint.
    ///
    /// Save_checkpoint takes any number of objects which a user may wish
    /// to store and returns a future to a checkpoint object.
    /// This function can also store a component either by passing a
    /// shared_ptr to the component or by passing a component's client
    /// instance to save_checkpoint.
    /// Additionally, the function can take a policy as a first object which
    /// changes its behavior depending on the policy passed to it. Most
    /// notably, if a sync policy is used save_checkpoint will simply return a
    /// checkpoint object.
    ///
    /// \returns Save_checkpoint returns a future to a checkpoint with one
    ///          exception: if you pass hpx::launch::sync as the first
    ///          argument. In this case save_checkpoint will simply return
    ///          a checkpoint.
    ///
    HPX_CXX_EXPORT template <typename T, typename... Ts>
        requires(!std::is_same_v<std::decay_t<T>, checkpoint>)
    hpx::future<checkpoint> save_checkpoint(hpx::launch p, T&& t, Ts&&... ts)
    {
        return hpx::dataflow(p, detail::save_funct_obj{}, checkpoint{},
            detail::prepare_client(HPX_FORWARD(T, t)),
            detail::prepare_client(HPX_FORWARD(Ts, ts))...);
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> save_checkpoint(hpx::launch)
    {
        return hpx::make_ready_future(checkpoint{});
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// Save_checkpoint - Policy overload & pre-initialized checkpoint
    ///
    /// \tparam T            Containers passed to save_checkpoint to be
    ///                      serialized and placed into a checkpoint object.
    ///
    /// \tparam Ts           More containers passed to save_checkpoint
    ///                      to be serialized and placed into a checkpoint
    ///                      object.
    ///
    /// \param p             Takes an HPX launch policy. Allows the user
    ///                      to change the way the function is launched i.e.
    ///                      async, sync, etc.
    ///
    /// \param c             Takes a pre-initialized checkpoint to copy
    ///                      data into.
    ///
    /// \param t             A container to restore.
    ///
    /// \param ts            Other containers to restore Containers
    ///                      must be in the same order that they were inserted
    ///                      into the checkpoint.
    ///
    /// Save_checkpoint takes any number of objects which a user may wish to
    /// store and returns a future to a checkpoint object. This function can
    /// also store a component either by passing a shared_ptr to the component
    /// or by passing a component's client instance to save_checkpoint.
    /// Additionally, the function can take a policy as a first object which
    /// changes its behavior depending on the policy passed to it. Most notably,
    /// if a sync policy is used save_checkpoint will simply return a checkpoint
    /// object.
    ///
    /// \returns Save_checkpoint returns a future to a checkpoint with one
    ///          exception: if you pass hpx::launch::sync as the first argument.
    ///          In this case save_checkpoint will simply return a checkpoint.
    ///
    HPX_CXX_EXPORT template <typename T, typename... Ts>
    hpx::future<checkpoint> save_checkpoint(
        hpx::launch p, checkpoint&& c, T&& t, Ts&&... ts)
    {
        return hpx::dataflow(p, detail::save_funct_obj{}, HPX_MOVE(c),
            detail::prepare_client(HPX_FORWARD(T, t)),
            detail::prepare_client(HPX_FORWARD(Ts, ts))...);
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> save_checkpoint(
        hpx::launch, checkpoint&& c)
    {
        return hpx::make_ready_future(HPX_MOVE(c));
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// Save_checkpoint - Sync_policy overload
    ///
    /// \tparam T            Containers passed to save_checkpoint to be
    ///                      serialized and placed into a checkpoint object.
    ///
    /// \tparam Ts           More containers passed to save_checkpoint
    ///                      to be serialized and placed into a
    ///                      checkpoint object.
    ///
    /// \tparam U            This parameter is used to make sure that T
    ///                      is not a checkpoint. This forces the compiler
    ///                      to choose the correct overload.
    ///
    /// \param sync_p        hpx::launch::sync_policy
    ///
    /// \param t             A container to restore.
    ///
    /// \param ts            Other containers to restore Containers
    ///                      must be in the same order that they were
    ///                      inserted into the checkpoint.
    ///
    /// Save_checkpoint takes any number of objects which a user may wish
    /// to store and returns a future to a checkpoint object.
    /// This function can also store a component either by passing a
    /// shared_ptr to the component or by passing a component's client
    /// instance to save_checkpoint.
    /// Additionally, the function can take a policy as a first object which
    /// changes its behavior depending on the policy passed to it. Most
    /// notably, if a sync policy is used save_checkpoint will simply return a
    /// checkpoint object.
    ///
    /// \returns Save_checkpoint which is passed hpx::launch::sync_policy
    ///          will return a checkpoint which contains the serialized
    ///          values checkpoint.
    ///
    HPX_CXX_EXPORT template <typename T, typename... Ts>
        requires(!std::is_same_v<std::decay_t<T>, checkpoint>)
    checkpoint save_checkpoint(
        hpx::launch::sync_policy sync_p, T&& t, Ts&&... ts)
    {
        hpx::future<checkpoint> f_chk =
            hpx::dataflow(sync_p, detail::save_funct_obj{}, checkpoint{},
                detail::prepare_client(HPX_FORWARD(T, t)),
                detail::prepare_client(HPX_FORWARD(Ts, ts))...);
        return f_chk.get();
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline checkpoint save_checkpoint(
        hpx::launch::sync_policy) noexcept
    {
        return checkpoint{};
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// Save_checkpoint - Sync_policy overload & pre-init. checkpoint
    ///
    /// \tparam T            Containers passed to save_checkpoint to be
    ///                      serialized and placed into a checkpoint object.
    ///
    /// \tparam Ts           More containers passed to save_checkpoint
    ///                      to be serialized and placed into a checkpoint
    ///                      object.
    ///
    /// \param sync_p        hpx::launch::sync_policy
    ///
    /// \param c             Takes a pre-initialized checkpoint to copy
    ///                      data into.
    ///
    /// \param t             A container to restore.
    ///
    /// \param ts            Other containers to restore Containers
    ///                      must be in the same order that they were inserted
    ///                      into the checkpoint.
    ///
    /// Save_checkpoint takes any number of objects which a user may wish to
    /// store and returns a future to a checkpoint object. This function can
    /// also store a component either by passing a shared_ptr to the component
    /// or by passing a component's client instance to save_checkpoint.
    /// Additionally, the function can take a policy as a first object which
    /// changes its behavior depending on the policy passed to it. Most notably,
    /// if a sync policy is used save_checkpoint will simply return a checkpoint
    /// object.
    ///
    /// \returns Save_checkpoint which is passed hpx::launch::sync_policy will
    ///          return a checkpoint which contains the serialized values
    ///          checkpoint.
    ///
    HPX_CXX_EXPORT template <typename T, typename... Ts>
    checkpoint save_checkpoint(
        hpx::launch::sync_policy sync_p, checkpoint&& c, T&& t, Ts&&... ts)
    {
        hpx::future<checkpoint> f_chk =
            hpx::dataflow(sync_p, detail::save_funct_obj{}, HPX_MOVE(c),
                detail::prepare_client(HPX_FORWARD(T, t)),
                detail::prepare_client(HPX_FORWARD(Ts, ts))...);
        return f_chk.get();
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline checkpoint save_checkpoint(
        hpx::launch::sync_policy, checkpoint&& c) noexcept
    {
        return HPX_MOVE(c);
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        struct prepare_checkpoint
        {
            template <typename... Ts>
            checkpoint operator()(checkpoint&& c, Ts const&... ts) const
            {
                std::size_t size = hpx::util::prepare_checkpoint_data(ts...);
                c.data_.resize(size);
                return HPX_MOVE(c);
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// prepare_checkpoint
    ///
    /// prepare_checkpoint takes the containers which have to be filled from the
    /// byte stream by a subsequent restore_checkpoint invocation.
    /// prepare_checkpoint will calculate the necessary buffer size and will
    /// return an appropriately sized checkpoint object.
    ///
    /// \tparam T           A container to restore.
    /// \tparam Ts          Other containers to restore. Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \param t            A container to restore.
    /// \param ts           Other containers to restore Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \returns prepare_checkpoint returns a properly resized checkpoint object
    ///          that can be used for a subsequent restore_checkpoint operation.
    HPX_CXX_EXPORT template <typename T, typename... Ts>
        requires(!hpx::traits::is_launch_policy_v<T> &&
            !std::is_same_v<std::decay_t<T>, checkpoint>)
    hpx::future<checkpoint> prepare_checkpoint(T const& t, Ts const&... ts)
    {
        return hpx::dataflow(detail::prepare_checkpoint{}, checkpoint{},
            detail::prepare_client(t), detail::prepare_client(ts)...);
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> prepare_checkpoint()
    {
        return hpx::make_ready_future(checkpoint{});
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// prepare_checkpoint
    ///
    /// prepare_checkpoint takes the containers which have to be filled from the
    /// byte stream by a subsequent restore_checkpoint invocation.
    /// prepare_checkpoint will calculate the necessary buffer size and will
    /// return an appropriately sized checkpoint object.
    ///
    /// \tparam T           A container to restore.
    /// \tparam Ts          Other containers to restore. Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \param c            Takes a pre-initialized checkpoint to prepare
    ///
    /// \param t            A container to restore.
    /// \param ts           Other containers to restore Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \returns prepare_checkpoint returns a properly resized checkpoint object
    ///          that can be used for a subsequent restore_checkpoint operation.
    HPX_CXX_EXPORT template <typename T, typename... Ts>
    hpx::future<checkpoint> prepare_checkpoint(
        checkpoint&& c, T const& t, Ts const&... ts)
    {
        return hpx::dataflow(detail::prepare_checkpoint{}, HPX_MOVE(c),
            detail::prepare_client(t), detail::prepare_client(ts)...);
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> prepare_checkpoint(
        checkpoint&& c)
    {
        return hpx::make_ready_future(HPX_MOVE(c));
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// prepare_checkpoint
    ///
    /// prepare_checkpoint takes the containers which have to be filled from the
    /// byte stream by a subsequent restore_checkpoint invocation.
    /// prepare_checkpoint will calculate the necessary buffer size and will
    /// return an appropriately sized checkpoint object.
    ///
    /// \tparam T           A container to restore.
    /// \tparam Ts          Other containers to restore. Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \param p             Takes an HPX launch policy. Allows the user
    ///                      to change the way the function is launched i.e.
    ///                      async, sync, etc.
    ///
    /// \param t            A container to restore.
    /// \param ts           Other containers to restore Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \returns prepare_checkpoint returns a properly resized checkpoint object
    ///          that can be used for a subsequent restore_checkpoint operation.
    HPX_CXX_EXPORT template <typename T, typename... Ts>
        requires(!std::is_same_v<T, checkpoint>)
    hpx::future<checkpoint> prepare_checkpoint(
        hpx::launch p, T const& t, Ts const&... ts)
    {
        return hpx::dataflow(p, detail::prepare_checkpoint{}, checkpoint{},
            detail::prepare_client(t), detail::prepare_client(ts)...);
    }

    /// \cond NOINTERNAL
    HPX_CXX_EXPORT template <typename T, typename... Ts>
        requires(!std::is_same_v<T, checkpoint>)
    checkpoint prepare_checkpoint(
        hpx::launch::sync_policy sync_p, T const& t, Ts const&... ts)
    {
        return hpx::dataflow(sync_p, detail::prepare_checkpoint{}, checkpoint{},
            detail::prepare_client(t), detail::prepare_client(ts)...)
            .get();
    }

    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> prepare_checkpoint(
        hpx::launch)
    {
        return hpx::make_ready_future(checkpoint{});
    }

    HPX_CXX_EXPORT inline checkpoint prepare_checkpoint(
        hpx::launch::sync_policy) noexcept
    {
        return checkpoint{};
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// prepare_checkpoint
    ///
    /// prepare_checkpoint takes the containers which have to be filled from the
    /// byte stream by a subsequent restore_checkpoint invocation.
    /// prepare_checkpoint will calculate the necessary buffer size and will
    /// return an appropriately sized checkpoint object.
    ///
    /// \tparam T           A container to restore.
    /// \tparam Ts          Other containers to restore. Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \param p             Takes an HPX launch policy. Allows the user
    ///                      to change the way the function is launched i.e.
    ///                      async, sync, etc.
    ///
    /// \param c            Takes a pre-initialized checkpoint to prepare
    ///
    /// \param t            A container to restore.
    /// \param ts           Other containers to restore Containers
    ///                     must be in the same order that they were inserted
    ///                     into the checkpoint.
    ///
    /// \returns prepare_checkpoint returns a properly resized checkpoint object
    ///          that can be used for a subsequent restore_checkpoint operation.
    HPX_CXX_EXPORT template <typename T, typename... Ts>
    hpx::future<checkpoint> prepare_checkpoint(
        hpx::launch p, checkpoint&& c, T const& t, Ts const&... ts)
    {
        return hpx::dataflow(p, detail::prepare_checkpoint{}, HPX_MOVE(c),
            detail::prepare_client(t), detail::prepare_client(ts)...);
    }

    /// \cond NOINTERNAL
    HPX_CXX_EXPORT template <typename T, typename... Ts>
    checkpoint prepare_checkpoint(hpx::launch::sync_policy sync_p,
        checkpoint&& c, T const& t, Ts const&... ts)
    {
        return hpx::dataflow(sync_p, detail::prepare_checkpoint{}, HPX_MOVE(c),
            detail::prepare_client(t), detail::prepare_client(ts)...)
            .get();
    }

    // Same as above, just nullary
    HPX_CXX_EXPORT inline hpx::future<checkpoint> prepare_checkpoint(
        hpx::launch, checkpoint&& c)
    {
        return hpx::make_ready_future(HPX_MOVE(c));
    }

    HPX_CXX_EXPORT inline checkpoint prepare_checkpoint(
        hpx::launch::sync_policy, checkpoint&& c) noexcept
    {
        return HPX_MOVE(c);
    }
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Properly handle non client/server restoration
        struct restore_impl
        {
            template <typename T>
                requires(!hpx::traits::is_client_v<T>)
            void operator()(hpx::serialization::input_archive& ar, T& t) const
            {
                ar >> t;
            }

            // Properly handle client/server restoration
            template <typename Client, typename Server, typename Data>
            void operator()(hpx::serialization::input_archive& ar,
                hpx::components::client_base<Client, Server, Data>& c) const
            {
                // Revive server
                using server_component_type =
                    hpx::components::client_base<Client, Server,
                        Data>::server_component_type;

                hpx::future<std::shared_ptr<server_component_type>>
                    f_server_ptr;
                ar >> f_server_ptr;
                c = hpx::new_<Client>(
                    naming::get_id_from_locality_id(agas::get_locality_id()),
                    HPX_MOVE(*(f_server_ptr.get())));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Restore_checkpoint
    ///
    /// Restore_checkpoint takes a checkpoint object as a first argument and
    /// the containers which will be filled from the byte stream (in the same
    /// order as they were placed in save_checkpoint). Restore_checkpoint can
    /// resurrect a stored component in two ways: by passing in an instance of
    /// a component's shared_ptr or by passing in an
    /// instance of the component's client.
    ///
    /// \tparam T           A container to restore.
    ///
    /// \tparam Ts          Other containers to restore. Containers
    ///                     must be in the same order that they were
    ///                     inserted into the checkpoint.
    ///
    /// \param c            The checkpoint to restore.
    ///
    /// \param t            A container to restore.
    ///
    /// \param ts           Other containers to restore Containers
    ///                     must be in the same order that they were
    ///                     inserted into the checkpoint.
    ///
    /// \returns Restore_checkpoint returns void.
    HPX_CXX_EXPORT template <typename T, typename... Ts>
    void restore_checkpoint(checkpoint const& c, T& t, Ts&... ts)
    {
        hpx::util::restore_checkpoint_data_func(
            c.data_, detail::restore_impl{}, t, ts...);
    }

    /// \cond NOINTERNAL
    // Same as above, just nullary
    HPX_CXX_EXPORT constexpr void restore_checkpoint(checkpoint const&) noexcept
    {
    }
    /// \endcond
}    // namespace hpx::util
