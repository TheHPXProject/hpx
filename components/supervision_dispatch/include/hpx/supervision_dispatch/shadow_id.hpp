//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/supervision_dispatch/shadow_id.hpp
/// \page hpx::supervision::shadow_id, hpx::supervision::invalid_shadow_id, hpx::supervision::joined_peer
/// \headerfile hpx/supervision_dispatch.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/supervision_dispatch/export_definitions.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>

namespace hpx::supervision {

    /// \brief A distinct, non-dereferenceable identifier for a peer's local
    ///        supervision "shadow" state.
    ///
    /// A \c shadow_id wraps the opaque, unmanaged \c hpx::id_type minted by
    /// \c make_shadow_target() to key a peer's fencing/lifecycle state inside
    /// this locality's own \c supervision_manager. It is intentionally *not*
    /// implicitly convertible to or from \c hpx::id_type: a shadow id must
    /// never be resolved by AGAS, passed to \c hpx::colocated(), or used as the
    /// destination of an action dispatch (e.g. \c hpx::sync(act, id, ...)) -
    /// doing so is undefined behavior at the id level and will throw at
    /// invocation time for plain actions, since a shadow id does not satisfy \c
    /// naming::is_locality().
    ///
    /// Functions that operate on generic, id-type-agnostic supervision
    /// primitives (\c query_state(), \c publish_event(), \c check_admission(),
    /// \c await_terminal()) continue to take plain \c hpx::id_type, since
    /// those primitives are also used with real, remotely-addressable ids. Call
    /// \c get() to unwrap a \c shadow_id only at the boundary where it
    /// is passed into one of those primitives as an opaque lookup key.
    ///
    /// \see make_shadow_target()
    /// \see dispatch_work()
    class shadow_id
    {
    public:
        /// \brief Constructs an invalid (empty) shadow id.
        ///
        /// Equivalent to \c invalid_shadow_id. Useful as a default/sentinel
        /// value before a real shadow has been established (e.g. prior to a
        /// successful \c registry::join()).
        shadow_id() = default;

        /// \brief Wraps an existing \c hpx::id_type as a shadow id.
        ///
        /// \param id The underlying, opaque local identifier to wrap. Callers
        ///           are responsible for ensuring \p id was produced by
        ///           \c make_shadow_target() (or an equivalent shadow-minting
        ///           facility) and is never used as a real, colocatable
        ///           destination id.
        ///
        /// This constructor is deliberately \c explicit to prevent an
        /// \c hpx::id_type from being implicitly substituted wherever a
        /// \c shadow_id is expected (and vice versa via \c get()).
        explicit shadow_id(hpx::id_type id) noexcept
          : id_(HPX_MOVE(id))
        {
        }

        /// \brief Checks whether this shadow id holds a valid underlying id.
        ///
        /// \return \c true if the wrapped \c hpx::id_type is valid (non-empty);
        ///         \c false otherwise, e.g. for a default-constructed or
        ///         \c invalid_shadow_id instance.
        explicit operator bool() const noexcept
        {
            return static_cast<bool>(id_);
        }

        /// \brief Compares two shadow ids for equality.
        ///
        /// Two shadow ids compare equal if and only if their underlying
        /// wrapped \c hpx::id_type values compare equal.
        friend bool operator==(
            shadow_id const& lhs, shadow_id const& rhs) noexcept
        {
            return lhs.id_ == rhs.id_;
        }

        /// \brief Compares two shadow ids for inequality.
        friend bool operator!=(
            shadow_id const& lhs, shadow_id const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        /// \brief Establishes a strict weak ordering between two shadow ids.
        ///
        /// Provided so \c shadow_id can be used as an ordered associative
        /// container key (e.g. in a \c std::map). The ordering is derived
        /// from the underlying \c hpx::id_type ordering and carries no
        /// semantic meaning beyond that.
        friend bool operator<(
            shadow_id const& lhs, shadow_id const& rhs) noexcept
        {
            return lhs.id_ < rhs.id_;
        }

        /// \brief Returns \c true if \p lhs orders after \p rhs.
        friend bool operator>(
            shadow_id const& lhs, shadow_id const& rhs) noexcept
        {
            return rhs.id_ < lhs.id_;
        }

        /// \brief Returns \c true if \p lhs does not order after \p rhs.
        friend bool operator<=(
            shadow_id const& lhs, shadow_id const& rhs) noexcept
        {
            return !(rhs.id_ < lhs.id_);
        }

        /// \brief Returns \c true if \p lhs does not order before \p rhs.
        friend bool operator>=(
            shadow_id const& lhs, shadow_id const& rhs) noexcept
        {
            return !(lhs.id_ < rhs.id_);
        }

        /// \brief Returns the underlying, opaque \c hpx::id_type.
        ///
        /// \warning The returned id must never be dereferenced, resolved via
        ///          AGAS, passed to \c hpx::colocated(), or used as an action
        ///          dispatch destination. It is intended solely as a lookup key
        ///          into a local \c supervision_manager's fencing state (e.g.
        ///          via \c query_state(), \c publish_event(),
        ///          \c check_admission(), \c await_terminal()).
        ///
        /// \return A const reference to the wrapped \c hpx::id_type.
        [[nodiscard]] constexpr hpx::id_type const& get() const noexcept
        {
            return id_;
        }

    private:
        friend class hpx::serialization::access;

        /// \brief Serializes/deserializes this shadow id.
        ///
        /// \param ar The archive to read from or write to.
        template <typename Archive>
        void serialize(Archive& ar, unsigned int)
        {
            ar & id_;
        }

        hpx::id_type id_;
    };

    /// \brief A well-known, invalid \c shadow_id sentinel value.
    ///
    /// Equivalent to a default-constructed \c shadow_id. Use this to signal "no
    /// shadow" (e.g. when a peer has not yet joined or has been fully evicted)
    /// without relying on default construction at each use site.
    inline shadow_id const invalid_shadow_id = shadow_id();

    HPX_SUPERVISION_DISPATCH_EXPORT std::ostream& operator<<(
        std::ostream& strm, shadow_id const& id);

    /// \brief Pairs the two identifiers a caller obtains from a successful
    ///        \c registry::join() and needs to perform a fenced dispatch.
    ///
    /// \c dispatch_work() requires two distinct identifiers that must never
    /// be confused with one another:
    /// - a \c shadow_id used purely as an opaque, local fencing/admission
    ///   lookup key (never dereferenced, resolved, or dispatched to), and
    /// - a real, colocatable \c hpx::id_type identifying the locality the
    ///   wrapped action must actually execute on.
    ///
    /// \c joined_peer bundles both values together so callers can carry a
    /// single object returned from \c registry::join() instead of two loose
    /// parameters, without introducing any dependency on the registry itself
    /// into \c dispatch_work.hpp - the struct is a plain value type composed
    /// solely from \c shadow_id and \c hpx::id_type.
    ///
    /// \see shadow_id
    /// \see dispatch_work()
    /// \see registry::join()
    struct joined_peer
    {
        /// \brief The opaque, local fencing key for this peer relationship.
        ///
        /// Produced by \c make_shadow_target() and returned by
        /// \c registry::join(). Used only as the \c fencing_key argument to
        /// \c check_admission()-style calls inside \c dispatch_work(); must
        /// never be passed to \c hpx::colocated() or used as an action
        /// dispatch destination.
        shadow_id shadow;

        /// \brief The real, colocatable destination for the wrapped action.
        ///
        /// Typically the peer's locality id (i.e. \c peer_locality, the
        /// value originally passed into \c registry::join()). Forwarded to
        /// \c hpx::colocated() and used as the destination of
        /// \c hpx::sync(act, target, ts...) inside \c dispatch_work().
        hpx::id_type target;

        /// \brief The epoch at which this peer was joined.
        ///
        /// Recorded by \c registry::join() and used, instead of \c shadow,
        /// to identify which registry entry a later \c registry::leave()
        /// call refers to - so that a racing re-join of the same peer
        /// sentinel (which mints a fresh \c shadow and \c join_epoch of its
        /// own) cannot be mistaken for the join this \c joined_peer was
        /// returned from.
        std::uint64_t join_epoch = 0;
    };

    inline bool operator==(
        joined_peer const& lhs, joined_peer const& rhs) noexcept
    {
        return lhs.shadow == rhs.shadow && lhs.target == rhs.target &&
            lhs.join_epoch == rhs.join_epoch;
    }
    inline bool operator!=(
        joined_peer const& lhs, joined_peer const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    HPX_SUPERVISION_DISPATCH_EXPORT std::ostream& operator<<(
        std::ostream& strm, joined_peer const& peer);
}    // namespace hpx::supervision

namespace std {

    template <>
    struct hash<hpx::supervision::shadow_id>
    {
        std::size_t operator()(
            hpx::supervision::shadow_id const& id) const noexcept
        {
            return std::hash<hpx::id_type>()(id.get());
        }
    };
}    // namespace std
