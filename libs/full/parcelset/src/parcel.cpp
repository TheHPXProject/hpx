//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/tracing.hpp>

#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelset/parcel.hpp>
#include <hpx/parcelset/parcelhandler.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset::detail {

    parcel_data::parcel_data()
      : source_id_(naming::invalid_gid)
      , dest_(naming::invalid_gid)
#if defined(HPX_HAVE_PARCEL_PROFILING)
      , start_time_(0)
      , creation_time_(chrono::high_resolution_timer::now())

#endif
    {
    }

    parcel_data::parcel_data(
        naming::gid_type&& dest, naming::address&& addr, bool has_continuation)
      : source_id_(naming::invalid_gid)
      , dest_(HPX_MOVE(dest))
      , addr_(HPX_MOVE(addr))
#if defined(HPX_HAVE_PARCEL_PROFILING)
      , start_time_(0)
      , creation_time_(chrono::high_resolution_timer::now())
#endif
      , has_continuation_(has_continuation)
    {
    }

    parcel_data::parcel_data(parcel_data&& rhs) noexcept
      : source_id_(HPX_MOVE(rhs.source_id_))
      , dest_(HPX_MOVE(rhs.dest_))
      , addr_(HPX_MOVE(rhs.addr_))
#if defined(HPX_HAVE_PARCEL_PROFILING)
      , parcel_id_(HPX_MOVE(rhs.parcel_id_))
      , start_time_(rhs.start_time_)
      , creation_time_(rhs.creation_time_)
#endif
      , has_continuation_(rhs.has_continuation_)
    {
        rhs.source_id_ = naming::invalid_gid;
        rhs.dest_ = naming::invalid_gid;
        rhs.addr_ = naming::address();

#if defined(HPX_HAVE_PARCEL_PROFILING)
        rhs.parcel_id_ = naming::invalid_gid;
        rhs.start_time_ = 0;
        rhs.creation_time_ = 0;
#endif
    }

    parcel_data& parcel_data::operator=(parcel_data&& rhs) noexcept
    {
        source_id_ = HPX_MOVE(rhs.source_id_);
        dest_ = HPX_MOVE(rhs.dest_);
        addr_ = HPX_MOVE(rhs.addr_);
#if defined(HPX_HAVE_PARCEL_PROFILING)
        parcel_id_ = HPX_MOVE(rhs.parcel_id_);
        start_time_ = rhs.start_time_;
        creation_time_ = rhs.creation_time_;
#endif
        has_continuation_ = rhs.has_continuation_;

        rhs.source_id_ = naming::invalid_gid;
        rhs.dest_ = naming::invalid_gid;
        rhs.addr_ = naming::address();
#if defined(HPX_HAVE_PARCEL_PROFILING)
        rhs.parcel_id_ = naming::invalid_gid;
        rhs.start_time_ = 0;
        rhs.creation_time_ = 0;
#endif
        return *this;
    }

    void parcel_data::serialize(serialization::input_archive& ar, unsigned)
    {
        ar >> source_id_;
        ar >> dest_;
        ar >> addr_;

#if defined(HPX_HAVE_PARCEL_PROFILING)
        ar >> parcel_id_;
        ar >> start_time_;
        ar >> creation_time_;
#endif

        ar >> has_continuation_;
    }

    void parcel_data::serialize(
        serialization::output_archive& ar, unsigned) const
    {
        ar << source_id_;
        ar << dest_;
        ar << addr_;

#if defined(HPX_HAVE_PARCEL_PROFILING)
        ar << parcel_id_;
        ar << start_time_;
        ar << creation_time_;
#endif

        ar << has_continuation_;
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_DEBUG)
    bool parcel::is_valid() const
    {
        // empty parcels are always valid
#if defined(HPX_HAVE_PARCEL_PROFILING)
        if (0.0 == data_.creation_time_)    //-V550
            return true;
#endif

        // verify target destination
        if (data_.dest_ && data_.addr_.locality_)
        {
            // if we have a destination we need an action as well
            if (!action_)
            {
                return false;
            }
        }

        // verify that the action targets the correct type
        if (action_ &&
            data_.addr_.type_ !=
                to_int(hpx::components::component_enum_type::invalid))
        {
            int const type = action_->get_component_type();
            if (!components::types_are_compatible(type, data_.addr_.type_))
            {
                return false;
            }
        }

        return true;
    }
#else
    // Only used in debug mode.
    bool parcel::is_valid() const
    {
        return true;
    }
#endif

    parcel::parcel() = default;

    parcel::~parcel() = default;

    parcel::parcel(naming::gid_type&& dest, naming::address&& addr,
        std::unique_ptr<actions::base_action> act)
      : data_(HPX_MOVE(dest), HPX_MOVE(addr), act->has_continuation())
      , action_(HPX_MOVE(act))
    {
    }

    void parcel::reset()
    {
        data_ = detail::parcel_data();
        action_.reset();

        split_gids_.clear();
        size_ = 0;
        num_chunks_ = 0;
    }

    char const* parcel::get_action_name() const
    {
        return action_->get_action_name();
    }

    int parcel::get_component_type() const
    {
        return action_->get_component_type();
    }

    int parcel::get_action_type() const
    {
        return static_cast<int>(action_->get_action_type());
    }

    hpx::id_type parcel::source_id() const
    {
        return {data_.source_id_, hpx::id_type::management_type::unmanaged};
    }

    void parcel::set_source_id(hpx::id_type const& source_id)
    {
        if (source_id != hpx::invalid_id)
        {
            data_.source_id_ = source_id.get_gid();
        }
    }

    void parcel::set_destination_id(naming::gid_type&& dest)
    {
        data_.dest_ = dest;
        HPX_ASSERT(is_valid());
    }

    naming::gid_type const& parcel::destination() const
    {
        HPX_ASSERT(is_valid());
        return data_.dest_;
    }

    naming::address const& parcel::addr() const
    {
        return data_.addr_;
    }

    naming::address& parcel::addr()
    {
        return data_.addr_;
    }

    std::uint32_t parcel::destination_locality_id() const
    {
        return naming::get_locality_id_from_gid(destination_locality());
    }

    naming::gid_type const& parcel::destination_locality() const
    {
        return addr().locality_;
    }

    double parcel::start_time() const
    {
#if defined(HPX_HAVE_PARCEL_PROFILING)
        return data_.start_time_;
#else
        return 0.0;
#endif
    }

    void parcel::set_start_time(double time)
    {
#if defined(HPX_HAVE_PARCEL_PROFILING)
        data_.start_time_ = time;
#else
        HPX_UNUSED(time);
#endif
    }

    double parcel::creation_time() const
    {
#if defined(HPX_HAVE_PARCEL_PROFILING)
        return data_.creation_time_;
#else
        return 0.0;
#endif
    }

    threads::thread_priority parcel::get_thread_priority() const
    {
        return action_->get_thread_priority();
    }

    threads::thread_stacksize parcel::get_thread_stacksize() const
    {
        return action_->get_thread_stacksize();
    }

    std::uint32_t parcel::get_parent_locality_id() const
    {
        return action_->get_parent_locality_id();
    }

    threads::thread_id_type parcel::get_parent_thread_id() const
    {
        return action_->get_parent_thread_id();
    }

    std::uint64_t parcel::get_parent_thread_phase() const
    {
        return action_->get_parent_thread_phase();
    }

#if defined(HPX_HAVE_PARCEL_PROFILING)
    naming::gid_type const& parcel::parcel_id() const
    {
        return data_.parcel_id_;
    }

    naming::gid_type& parcel::parcel_id()
    {
        return data_.parcel_id_;
    }
#endif

#if defined(HPX_HAVE_NETWORKING)
    serialization::binary_filter* parcel::get_serialization_filter() const
    {
        hpx::optional<parcelset::parcel> const p =
            action_->get_embedded_parcel();
        if (!p)
        {
            return action_->get_serialization_filter();
        }
        return p->get_serialization_filter();
    }

    policies::message_handler* parcel::get_message_handler(
        locality const& loc) const
    {
        hpx::optional<parcelset::parcel> const p =
            action_->get_embedded_parcel();
        if (!p)
        {
            return action_->get_message_handler(loc);
        }
        return p->get_message_handler(loc);
    }
#endif

    bool parcel::does_termination_detection() const
    {
        return action_ ? action_->does_termination_detection() : false;
    }

    parcel::split_gids_type parcel::move_split_gids() const
    {
        split_gids_type gids;
        std::swap(gids, split_gids_);
        return gids;
    }

    void parcel::set_split_gids(parcel::split_gids_type&& split_gids)
    {
        HPX_ASSERT(split_gids_.empty());
        split_gids_ = HPX_MOVE(split_gids);
    }

    std::size_t parcel::num_chunks() const
    {
        return num_chunks_;
    }

    std::size_t& parcel::num_chunks()
    {
        return num_chunks_;
    }

    std::size_t parcel::size() const
    {
        return size_;
    }

    std::size_t& parcel::size()
    {
        return size_;
    }

    std::pair<naming::address_type, naming::component_type>
    parcel::determine_lva() const
    {
        int comptype = action_->get_component_type();

        // decode the local virtual address of the parcel
        naming::address::address_type lva = data_.addr_.address_;

        // by convention, a zero address references either the local
        // runtime support component or one of the AGAS components
        if (nullptr == lva)
        {
            switch (static_cast<components::component_enum_type>(comptype))
            {
            case components::component_enum_type::runtime_support:
                lva = agas::get_runtime_support_lva();
                break;

            case components::component_enum_type::agas_primary_namespace:
                lva = agas::get_primary_ns_lva();
                break;

            case components::component_enum_type::agas_symbol_namespace:
                lva = agas::get_symbol_ns_lva();
                break;

            case components::component_enum_type::plain_function:
                break;

            default:
                HPX_ASSERT(false);
            }
        }

        // make sure the component_type of the action matches the
        // component type in the destination address
        if (HPX_UNLIKELY(
                !components::types_are_compatible(data_.addr_.type_, comptype)))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_component_type,
                "parcel::determine_lva",
                " types are not compatible: destination_type({}) "
                "action_type({}) parcel ({})",
                data_.addr_.type_, comptype, *this);
        }

        return std::make_pair(lva, comptype);
    }

    bool parcel::load_schedule(serialization::input_archive& ar,
        std::size_t num_thread, bool& deferred_schedule)
    {
        load_data(ar);

        // make sure this parcel destination matches the proper locality
        HPX_ASSERT(destination_locality() == data_.addr_.locality_);

        std::pair<naming::address_type, naming::component_type> const p =
            determine_lva();

        // make sure the target has not been migrated away
        auto const r = action_->was_object_migrated(data_.dest_, p.first);
        if (r.first)
        {
            // If the object was migrated, just load the action and return.
            action_->load(ar);
            return true;
        }

        // schedule later if this is de-serialized with zero-copy semantics
        if (ar.try_get_extra_data<
                serialization::detail::allow_zero_copy_receive>() != nullptr)
        {
            action_->load(ar);
            return false;
        }

        // continuation support, this is handled in the transfer action
        action_->load_schedule(ar, HPX_MOVE(data_.dest_), p.first, p.second,
            num_thread, deferred_schedule);

        HPX_TRACING_MARK_EVENT("recv_parcel");

#if defined(HPX_HAVE_PARCEL_PROFILING)
        hpx::tracing::recv_parcel(data_.parcel_id_.get_lsb(), size_,
            naming::get_locality_id_from_gid(data_.source_id_),
            reinterpret_cast<std::uint64_t>(
                action_->get_parent_thread_id().get()));
#endif

        return false;
    }

    bool parcel::schedule_action(std::size_t num_thread)
    {
        // make sure this parcel destination matches the proper locality
        HPX_ASSERT(destination_locality() == data_.addr_.locality_);

        std::pair<naming::address_type, naming::component_type> const p =
            determine_lva();

        // make sure the target has not been migrated away
        auto const r = action_->was_object_migrated(data_.dest_, p.first);
        if (r.first)
        {
            // If the object was migrated, just route.
            return true;
        }

        // dispatch action, register work item either with or without
        // continuation support, this is handled in the transfer action
        action_->schedule_thread(
            HPX_MOVE(data_.dest_), p.first, p.second, num_thread);
        return false;
    }

    void parcel::load_data(serialization::input_archive& ar)
    {
        using hpx::actions::detail::action_registry;
        ar >> data_;

        std::uint32_t id;
        ar >> id;

#if !defined(HPX_DEBUG)
        action_.reset(action_registry::create(id, data_.has_continuation_));
#else
        std::string name;
        ar >> name;
        action_.reset(
            action_registry::create(id, data_.has_continuation_, &name));
#endif
    }

    void parcel::load(serialization::input_archive& ar, unsigned)
    {
        load_data(ar);
        action_->load(ar);
    }

    void parcel::save_data(serialization::output_archive& ar) const
    {
        using hpx::serialization::access;
        ar << data_;

        std::uint32_t const id = action_->get_action_id();
        ar << id;

#if defined(HPX_DEBUG)
        std::string const name(action_->get_action_name());
        ar << name;
#endif
    }

    void parcel::save(serialization::output_archive& ar, unsigned) const
    {
        save_data(ar);
        action_->save(ar);
    }

    std::ostream& operator<<(std::ostream& os, parcel const& p)
    {
        return hpx::util::format_to(
            os, "({}:{}:{})", p.destination(), p.addr(), p.get_action_name());
    }
}    // namespace hpx::parcelset::detail

#endif
