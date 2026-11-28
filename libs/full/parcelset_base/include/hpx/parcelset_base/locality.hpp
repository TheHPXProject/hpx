//  Copyright (c)      2014 Thomas Heller
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2007 Richard D. Guidry Jr.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/parcelset_base/parcelset_base_fwd.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    /// Where a destination is seen from the sender: on the sender's node or
    /// on another one (see locality_capabilities::node). Some settings of
    /// the parcel layer have a value per class, for example
    /// hpx.parcel.<parcelport>.same_node.conduit_host_chunks.
    HPX_CXX_EXPORT enum class destination_class : std::uint8_t {
        same_node,
        other_nodes
    };

    /// The identity of the node this process runs on: the same for all
    /// processes on one node (also of different builds), different for
    /// different nodes; a hash of the host name and, where there is one, the
    /// kernel's boot id. Never 0 (0 means unknown in
    /// locality_capabilities::node).
    HPX_CXX_EXPORT HPX_EXPORT std::uint64_t this_node_identity();

    /// What the parcelport behind an endpoint can do, besides being reached.
    /// The parcel handler sets it for the endpoints of its locality; it
    /// travels with them to the other localities (registration, AGAS), also
    /// to localities connecting later, so a sender knows it for every
    /// destination.
    HPX_CXX_EXPORT struct locality_capabilities
    {
        /// The parcelport receives the zero-copy chunks of a parcel after
        /// decoding it, into the memory of the de-serialized objects.
        static constexpr std::uint32_t zero_copy_receive = 0x1;

        /// The locality receives host chunks through its device conduit
        /// ('conduit') when they come through this parcelport from a sender
        /// on another node.
        static constexpr std::uint32_t conduit_host_chunks = 0x2;

        /// The same for a sender on the locality's node.
        static constexpr std::uint32_t conduit_host_chunks_same_node = 0x4;

        std::uint32_t flags = 0;

        /// The name of the locality's device conduit, empty if it has none.
        std::string conduit;

        /// The node the locality runs on (this_node_identity), 0 if unknown.
        std::uint64_t node = 0;

        /// The class of this destination for a sender on 'sender_node':
        /// other_nodes if either node is unknown.
        [[nodiscard]] destination_class class_for(
            std::uint64_t const sender_node) const noexcept
        {
            return node != 0 && node == sender_node ?
                destination_class::same_node :
                destination_class::other_nodes;
        }

        /// The flag saying that host chunks of a sender of the given class
        /// go through the conduit.
        [[nodiscard]] static constexpr std::uint32_t conduit_host_chunks_for(
            destination_class const c) noexcept
        {
            return c == destination_class::same_node ?
                conduit_host_chunks_same_node :
                conduit_host_chunks;
        }

        [[nodiscard]] bool has(std::uint32_t const f) const noexcept
        {
            return (flags & f) == f;
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & flags & conduit & node;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT class locality
    {
        template <typename Impl>
        class impl;

        class impl_base
        {
        public:
            virtual ~impl_base() = default;

            virtual bool equal(impl_base const& rhs) const = 0;
            virtual bool less_than(impl_base const& rhs) const = 0;
            virtual bool valid() const = 0;
            virtual char const* type() const = 0;
            virtual std::ostream& print(std::ostream& os) const = 0;
            virtual void save(serialization::output_archive& ar) const = 0;
            virtual void load(serialization::input_archive& ar) = 0;
            virtual impl_base* clone() const = 0;
            virtual impl_base* move() = 0;

            template <typename Impl>
            Impl& get()
            {
                HPX_ASSERT(Impl::type() == type());
                return static_cast<impl<Impl>*>(this)->impl_;
            }

            template <typename Impl>
            Impl const& get() const
            {
                HPX_ASSERT(Impl::type() == type());
                return static_cast<impl<Impl> const*>(this)->impl_;
            }
        };

    public:
        locality() = default;

        template <typename Impl,
            typename Enable1 =
                std::enable_if_t<!std::is_same_v<locality, std::decay_t<Impl>>>,
            typename Enable2 = std::enable_if_t<!traits::is_iterator_v<Impl>>>
        explicit locality(Impl&& i)
          : impl_(new impl<std::decay_t<Impl>>(HPX_FORWARD(Impl, i)))
        {
        }

        locality(locality const& other);
        locality(locality&& other) noexcept;

        locality& operator=(locality const& other);
        locality& operator=(locality&& other) noexcept;

        ///////////////////////////////////////////////////////////////////////
        explicit operator bool() const noexcept
        {
            return impl_ ? impl_->valid() : false;
        }

        char const* type() const
        {
            return impl_ ? impl_->type() : "";
        }

        template <typename Impl>
        Impl& get()
        {
            HPX_ASSERT(impl_);
            return impl_->get<Impl>();
        }

        template <typename Impl>
        Impl const& get() const
        {
            HPX_ASSERT(impl_);
            return impl_->get<Impl>();
        }

        /// What the parcelport behind this endpoint can do (not part of the
        /// endpoint's identity: comparisons ignore it).
        [[nodiscard]] locality_capabilities const& capabilities() const noexcept
        {
            return capabilities_;
        }

        void set_capabilities(locality_capabilities caps)
        {
            capabilities_ = HPX_MOVE(caps);
        }

    private:
        friend HPX_EXPORT bool operator==(
            locality const& lhs, locality const& rhs);
        friend HPX_EXPORT bool operator!=(
            locality const& lhs, locality const& rhs);
        friend HPX_EXPORT bool operator<(
            locality const& lhs, locality const& rhs);
        friend HPX_EXPORT bool operator>(
            locality const& lhs, locality const& rhs);

        friend HPX_EXPORT std::ostream& operator<<(
            std::ostream& os, locality const& l);

        // serialization support
        friend class hpx::serialization::access;

        void save(
            serialization::output_archive& ar, unsigned int version) const;

        void load(serialization::input_archive& ar, unsigned int version);

        HPX_SERIALIZATION_SPLIT_MEMBER()

        std::unique_ptr<impl_base> impl_;
        locality_capabilities capabilities_;

        template <typename Impl>
        class impl : public impl_base
        {
        public:
            explicit impl(Impl&& i) noexcept
              : impl_(HPX_MOVE(i))
            {
            }
            explicit impl(Impl const& i)
              : impl_(i)
            {
            }

            bool equal(impl_base const& rhs) const override
            {
                if (type() != rhs.type())
                    return false;
                return impl_ == rhs.get<Impl>();
            }

            bool less_than(impl_base const& rhs) const override
            {
                return type() < rhs.type() ||
                    (type() == rhs.type() && impl_ < rhs.get<Impl>());
            }

            bool valid() const override
            {
                return !!impl_;
            }

            char const* type() const override
            {
                return Impl::type();
            }

            std::ostream& print(std::ostream& os) const override
            {
                os << impl_;
                return os;
            }

            void save(serialization::output_archive& ar) const override
            {
                impl_.save(ar);
            }

            void load(serialization::input_archive& ar) override
            {
                impl_.load(ar);
            }

            impl_base* clone() const override
            {
                return new impl<Impl>(impl_);
            }

            impl_base* move() override
            {
                return new impl<Impl>(HPX_MOVE(impl_));
            }

            Impl impl_;
        };
    };

    HPX_CXX_EXPORT using endpoints_type = std::map<std::string, locality>;

    HPX_CXX_EXPORT HPX_EXPORT std::ostream& operator<<(
        std::ostream& os, endpoints_type const& endpoints);
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>
