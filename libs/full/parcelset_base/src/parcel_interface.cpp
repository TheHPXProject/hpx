//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/format.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>

#include <hpx/parcelset_base/locality.hpp>
#include <hpx/parcelset_base/locality_interface.hpp>
#include <hpx/parcelset_base/parcel_interface.hpp>
#include <hpx/parcelset_base/parcelset_base_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace hpx::parcelset {

    parcel empty_parcel;

    bool parcel::is_valid() const
    {
        return data_ && data_->is_valid();
    }

    parcel::parcel() = default;

    parcel::parcel(detail::parcel_base* p)
      : data_(p)
    {
    }

    parcel::~parcel() = default;

    parcel::parcel(parcel const&) = default;
    parcel::parcel(parcel&&) noexcept = default;
    parcel& parcel::operator=(parcel const&) = default;
    parcel& parcel::operator=(parcel&&) noexcept = default;

    void parcel::reset() const
    {
        data_->reset();
    }

    char const* parcel::get_action_name() const
    {
        return data_->get_action_name();
    }

    int parcel::get_component_type() const
    {
        return data_->get_component_type();
    }

    int parcel::get_action_type() const
    {
        return data_->get_action_type();
    }

    hpx::id_type parcel::source_id() const
    {
        return data_->source_id();
    }

    void parcel::set_source_id(hpx::id_type const& source_id) const
    {
        data_->set_source_id(source_id);
    }

    void parcel::set_destination_id(naming::gid_type&& dest) const
    {
        data_->set_destination_id(HPX_MOVE(dest));
    }

    naming::address const& parcel::addr() const
    {
        return data_->addr();
    }

    naming::address& parcel::addr()
    {
        return data_->addr();
    }

    std::uint32_t parcel::destination_locality_id() const
    {
        return data_->destination_locality_id();
    }

    naming::gid_type const& parcel::destination() const
    {
        return data_->destination();
    }

    naming::gid_type const& parcel::destination_locality() const
    {
        return data_->destination_locality();
    }

    double parcel::start_time() const
    {
        return data_->start_time();
    }

    void parcel::set_start_time(double const time) const
    {
        data_->set_start_time(time);
    }

    double parcel::creation_time() const
    {
        return data_->creation_time();
    }

    threads::thread_priority parcel::get_thread_priority() const
    {
        return data_->get_thread_priority();
    }

    threads::thread_stacksize parcel::get_thread_stacksize() const
    {
        return data_->get_thread_stacksize();
    }

    std::uint32_t parcel::get_parent_locality_id() const
    {
        return data_->get_parent_locality_id();
    }

    threads::thread_id_type parcel::get_parent_thread_id() const
    {
        return data_->get_parent_thread_id();
    }

    std::uint64_t parcel::get_parent_thread_phase() const
    {
        return data_->get_parent_thread_phase();
    }

#if defined(HPX_HAVE_NETWORKING)
    serialization::binary_filter* parcel::get_serialization_filter() const
    {
        return data_ ? data_->get_serialization_filter() : nullptr;
    }

    policies::message_handler* parcel::get_message_handler(
        locality const& loc) const
    {
        return data_ ? data_->get_message_handler(loc) : nullptr;
    }
#endif

    bool parcel::does_termination_detection() const
    {
        return data_ ? data_->does_termination_detection() : false;
    }

    parcel::split_gids_type parcel::move_split_gids() const
    {
        return data_->move_split_gids();
    }

    void parcel::set_split_gids(split_gids_type&& split_gids) const
    {
        data_->set_split_gids(HPX_MOVE(split_gids));
    }

    std::size_t parcel::num_chunks() const
    {
        return data_->num_chunks();
    }

    std::size_t& parcel::num_chunks()
    {
        return data_->num_chunks();
    }

    std::size_t parcel::size() const
    {
        return data_->size();
    }

    std::size_t& parcel::size()
    {
        return data_->size();
    }

    bool parcel::schedule_action(std::size_t num_thread) const
    {
        return data_->schedule_action(num_thread);
    }

    // returns true if parcel was migrated, false if scheduled locally
    bool parcel::load_schedule(serialization::input_archive& ar,
        std::size_t const num_thread, bool& deferred_schedule)
    {
        *this = parcelset::create_parcel();
        return data_->load_schedule(ar, num_thread, deferred_schedule);
    }

#if defined(HPX_HAVE_PARCEL_PROFILING)
    naming::gid_type const& parcel::parcel_id() const
    {
        return data_->parcel_id();
    }

    naming::gid_type& parcel::parcel_id()
    {
        return data_->parcel_id();
    }

    // generate unique parcel id
    naming::gid_type parcel::generate_unique_id(std::uint32_t locality_id)
    {
        static hpx::util::atomic_count id(0);

        naming::gid_type result = naming::get_gid_from_locality_id(locality_id);
        result.set_lsb(++id);
        return result;
    }
#else
    naming::gid_type const& parcel::parcel_id() const
    {
        return naming::invalid_gid;
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    void parcel::serialize(serialization::input_archive& ar, unsigned)
    {
        *this = parcelset::create_parcel();
        data_->load(ar, 0);
    }

    void parcel::serialize(serialization::output_archive& ar, unsigned) const
    {
        data_->save(ar, 0);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(std::ostream& os, parcel const& p)
    {
        return hpx::util::format_to(
            os, "({}:{}:{})", p.destination(), p.addr(), p.get_action_name());
    }

    std::string dump_parcel(parcel const& p)
    {
        return hpx::util::to_string(p);
    }
}    // namespace hpx::parcelset

#endif
