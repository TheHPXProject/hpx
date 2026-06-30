//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2015-2016 Thomas Heller
//  Copyright (c) 2007 Richard D Guidry Jr
//  Copyright (c) 2007 Alexandre (aka Alex) TABBAL
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/threading_base.hpp>

#include <hpx/modules/naming_base.hpp>

#include <hpx/parcelset_base/locality.hpp>
#include <hpx/parcelset_base/parcel_buffer.hpp>
#include <hpx/parcelset_base/policies/message_handler.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset::detail {

    // abstract base class for parcels
    HPX_CXX_EXPORT struct parcel_base
    {
        using split_gids_type =
            std::map<naming::gid_type const*, naming::gid_type>;

        parcel_base() = default;

        parcel_base(parcel_base const&) = delete;
        parcel_base(parcel_base&&) = delete;
        parcel_base& operator=(parcel_base const&) = delete;
        parcel_base& operator=(parcel_base&&) = delete;

        virtual ~parcel_base() = default;

        virtual bool is_valid() const = 0;
        virtual void reset() = 0;

        virtual char const* get_action_name() const = 0;
        virtual int get_component_type() const = 0;
        virtual int get_action_type() const = 0;

        virtual hpx::id_type source_id() const = 0;
        virtual void set_source_id(hpx::id_type const& source_id) = 0;

        virtual void set_destination_id(naming::gid_type&& dest) = 0;
        virtual naming::gid_type const& destination() const = 0;

        virtual naming::address const& addr() const = 0;
        virtual naming::address& addr() = 0;

        virtual std::uint32_t destination_locality_id() const = 0;
        virtual naming::gid_type const& destination_locality() const = 0;

        virtual double start_time() const = 0;
        virtual void set_start_time(double time) = 0;
        virtual double creation_time() const = 0;

        virtual threads::thread_priority get_thread_priority() const = 0;
        virtual threads::thread_stacksize get_thread_stacksize() const = 0;

        virtual std::uint32_t get_parent_locality_id() const = 0;
        virtual threads::thread_id_type get_parent_thread_id() const = 0;
        virtual std::uint64_t get_parent_thread_phase() const = 0;

        virtual serialization::binary_filter* get_serialization_filter()
            const = 0;
        virtual policies::message_handler* get_message_handler(
            locality const& loc) const = 0;

        virtual bool does_termination_detection() const = 0;

        virtual split_gids_type move_split_gids() const = 0;
        virtual void set_split_gids(split_gids_type&& split_gids) = 0;

        virtual std::size_t num_chunks() const = 0;
        virtual std::size_t& num_chunks() = 0;

        virtual std::size_t size() const = 0;
        virtual std::size_t& size() = 0;

        virtual bool schedule_action(std::size_t num_thread) = 0;

        virtual bool load_schedule(serialization::input_archive& ar,
            std::size_t num_thread, bool& deferred_schedule) = 0;

        virtual void load(serialization::input_archive& ar, unsigned n) = 0;
        virtual void save(
            serialization::output_archive& ar, unsigned n) const = 0;

#if defined(HPX_HAVE_PARCEL_PROFILING)
        virtual naming::gid_type const& parcel_id() const = 0;
        virtual naming::gid_type& parcel_id() = 0;
#endif

        HPX_SERIALIZATION_SPLIT_MEMBER()
    };
}    // namespace hpx::parcelset::detail

namespace hpx::parcelset {

    HPX_CXX_EXPORT class HPX_EXPORT parcel
    {
        using split_gids_type = detail::parcel_base::split_gids_type;

        bool is_valid() const;

    public:
        parcel();
        explicit parcel(detail::parcel_base* p);

        parcel(parcel const&);
        parcel(parcel&&) noexcept;
        parcel& operator=(parcel const&);
        parcel& operator=(parcel&&) noexcept;

        ~parcel();

        void reset() const;

        [[nodiscard]] char const* get_action_name() const;
        [[nodiscard]] int get_component_type() const;
        [[nodiscard]] int get_action_type() const;

        [[nodiscard]] hpx::id_type source_id() const;
        void set_source_id(hpx::id_type const& source_id) const;
        void set_destination_id(naming::gid_type&& dest) const;

        naming::address const& addr() const;
        naming::address& addr();

        [[nodiscard]] std::uint32_t destination_locality_id() const;

        naming::gid_type const& destination() const;
        naming::gid_type const& destination_locality() const;

        [[nodiscard]] double start_time() const;
        void set_start_time(double time) const;
        [[nodiscard]] double creation_time() const;

        [[nodiscard]] threads::thread_priority get_thread_priority() const;
        [[nodiscard]] threads::thread_stacksize get_thread_stacksize() const;

        [[nodiscard]] std::uint32_t get_parent_locality_id() const;
        [[nodiscard]] threads::thread_id_type get_parent_thread_id() const;
        [[nodiscard]] std::uint64_t get_parent_thread_phase() const;

        naming::gid_type const& parcel_id() const;
#if defined(HPX_HAVE_PARCEL_PROFILING)
        naming::gid_type& parcel_id();
#endif

        serialization::binary_filter* get_serialization_filter() const;
        policies::message_handler* get_message_handler(
            locality const& loc) const;

        [[nodiscard]] bool does_termination_detection() const;

        [[nodiscard]] split_gids_type move_split_gids() const;
        void set_split_gids(split_gids_type&& split_gids) const;

        [[nodiscard]] std::size_t num_chunks() const;
        std::size_t& num_chunks();

        [[nodiscard]] std::size_t size() const;
        std::size_t& size();

        bool schedule_action(
            std::size_t num_thread = static_cast<std::size_t>(-1)) const;

        // returns true if parcel was migrated, false if scheduled locally
        bool load_schedule(serialization::input_archive& ar,
            std::size_t num_thread, bool& deferred_schedule);

#if defined(HPX_HAVE_PARCEL_PROFILING)
        // generate unique parcel id
        static naming::gid_type generate_unique_id(std::uint32_t locality_id);
#endif

    private:
        // serialization support
        friend class hpx::serialization::access;

        void serialize(serialization::input_archive& ar, unsigned);
        void serialize(serialization::output_archive& ar, unsigned) const;

        std::shared_ptr<detail::parcel_base> data_;
    };

    HPX_CXX_EXPORT HPX_EXPORT std::ostream& operator<<(
        std::ostream& os, parcel const& p);
    HPX_CXX_EXPORT HPX_EXPORT std::string dump_parcel(parcel const& p);
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
