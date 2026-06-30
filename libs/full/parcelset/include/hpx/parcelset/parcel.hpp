//  Copyright (c) 2021-2026 Hartmut Kaiser
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

#include <hpx/modules/actions.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset::detail {

    HPX_CXX_EXPORT struct HPX_EXPORT parcel_data
    {
    public:
        parcel_data();
        parcel_data(naming::gid_type&& dest, naming::address&& addr,
            bool has_continuation);

        parcel_data(parcel_data&& rhs) noexcept;
        parcel_data& operator=(parcel_data&& rhs) noexcept;

        void serialize(serialization::input_archive& ar, unsigned);
        void serialize(serialization::output_archive& ar, unsigned) const;

        naming::gid_type source_id_;
        naming::gid_type dest_;
        naming::address addr_;

#if defined(HPX_HAVE_PARCEL_PROFILING)
        naming::gid_type parcel_id_;
        double start_time_ = 0.0;
        double creation_time_ = 0.0;
#endif

        bool has_continuation_ = false;
    };

    // actual parcel implementation
    HPX_CXX_EXPORT class HPX_EXPORT parcel final : public parcel_base
    {
    private:
        parcel(parcel const&) = delete;
        parcel(parcel&&) = delete;

        parcel& operator=(parcel const&) = delete;
        parcel& operator=(parcel&&) = delete;

        friend class hpx::parcelset::parcel;

        bool is_valid() const override;

    public:
        parcel();
        ~parcel() override;

    private:
        parcel(naming::gid_type&& dest, naming::address&& addr,
            std::unique_ptr<actions::base_action> act);

        friend struct create_parcel;

    public:
        void reset() override;

        char const* get_action_name() const override;
        int get_component_type() const override;
        int get_action_type() const override;

        hpx::id_type source_id() const override;
        void set_source_id(hpx::id_type const& source_id) override;

        void set_destination_id(naming::gid_type&& dest) override;
        naming::gid_type const& destination() const override;

        naming::address const& addr() const override;
        naming::address& addr() override;

        [[nodiscard]] std::uint32_t destination_locality_id() const override;
        [[nodiscard]] naming::gid_type const& destination_locality()
            const override;

        [[nodiscard]] double start_time() const override;
        void set_start_time(double time) override;
        double creation_time() const override;

        [[nodiscard]] threads::thread_priority get_thread_priority()
            const override;
        [[nodiscard]] threads::thread_stacksize get_thread_stacksize()
            const override;

        [[nodiscard]] std::uint32_t get_parent_locality_id() const override;
        [[nodiscard]] threads::thread_id_type get_parent_thread_id()
            const override;
        [[nodiscard]] std::uint64_t get_parent_thread_phase() const override;

#if defined(HPX_HAVE_NETWORKING)
        serialization::binary_filter* get_serialization_filter() const override;
        policies::message_handler* get_message_handler(
            locality const& loc) const override;
#endif

        [[nodiscard]] bool does_termination_detection() const override;

        [[nodiscard]] split_gids_type move_split_gids() const override;
        void set_split_gids(split_gids_type&& split_gids) override;

        [[nodiscard]] std::size_t num_chunks() const override;
        std::size_t& num_chunks() override;

        [[nodiscard]] std::size_t size() const override;
        std::size_t& size() override;

        bool schedule_action(std::size_t num_thread) override;

        // returns true if parcel was migrated, false if scheduled locally
        bool load_schedule(serialization::input_archive& ar,
            std::size_t num_thread, bool& deferred_schedule) override;

#if defined(HPX_HAVE_PARCEL_PROFILING)
        // generate unique parcel id
        naming::gid_type const& parcel_id() const override;
        naming::gid_type& parcel_id() override;
#endif

    private:
        // serialization support
        friend class hpx::serialization::access;
        void load_data(serialization::input_archive& ar);
        void save_data(serialization::output_archive& ar) const;

        void load(serialization::input_archive& ar, unsigned) override;
        void save(serialization::output_archive& ar, unsigned) const override;

        std::pair<naming::address_type, naming::component_type> determine_lva()
            const;

        // actual parcel data
        detail::parcel_data data_;
        std::unique_ptr<actions::base_action> action_;

        // data needed for parcel encoding
        mutable split_gids_type split_gids_;
        std::size_t size_ = 0;
        std::size_t num_chunks_ = 0;
    };

    HPX_CXX_EXPORT HPX_EXPORT std::ostream& operator<<(
        std::ostream& os, parcel const& p);
}    // namespace hpx::parcelset::detail

#include <hpx/config/warnings_suffix.hpp>

HPX_IS_BITWISE_SERIALIZABLE(hpx::parcelset::detail::parcel_data)

#endif
