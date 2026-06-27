//  Copyright (c) 2007-2020 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming.hpp>

#include <hpx/async_distributed/continuation.hpp>
#include <hpx/async_distributed/transfer_continuation_action.hpp>
#include <hpx/async_distributed/trigger_lco.hpp>

#include <exception>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::actions {

    continuation::continuation() = default;

    continuation::continuation(hpx::id_type const& id)
      : id_(id)
    {
        // Try to resolve the address locally ...
        if (id_ && !agas::is_local_address_cached(id_, addr_))
        {
            addr_ = naming::address();
        }
    }

    continuation::continuation(hpx::id_type&& id)
      : id_(HPX_MOVE(id))
    {
        // Try to resolve the address locally ...
        if (id_ && !agas::is_local_address_cached(id_, addr_))
        {
            addr_ = naming::address();
        }
    }

    continuation::continuation(hpx::id_type const& id, naming::address&& addr)
      : id_(id)
      , addr_(HPX_MOVE(addr))
    {
    }

    continuation::continuation(
        hpx::id_type&& id, naming::address&& addr) noexcept
      : id_(HPX_MOVE(id))
      , addr_(HPX_MOVE(addr))
    {
    }

    continuation::continuation(continuation&& o) noexcept = default;

    continuation& continuation::operator=(continuation&& o) noexcept = default;

    ///////////////////////////////////////////////////////////////////////////
    void continuation::trigger_error(std::exception_ptr const& e)
    {
        if (!id_)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "continuation::trigger_error",
                "attempt to trigger invalid LCO (the id is invalid)");
            return;
        }

        LLCO_(info).format("continuation::trigger_error({})", id_);
        set_lco_error(id_, this->get_addr(), e);
    }

    void continuation::trigger_error(std::exception_ptr&& e)    //-V659
    {
        if (!id_)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "continuation::trigger_error",
                "attempt to trigger invalid LCO (the id is invalid)");
            return;
        }

        LLCO_(info).format("continuation::trigger_error({})", id_);
        set_lco_error(id_, this->get_addr(), HPX_MOVE(e));
    }

    void continuation::serialize(
        hpx::serialization::input_archive& ar, unsigned)
    {
        // clang-format off
        ar & id_ & addr_;
        // clang-format on
    }

    void continuation::serialize(
        hpx::serialization::output_archive& ar, unsigned)
    {
        // clang-format off
        ar & id_ & addr_;
        // clang-format on
    }

    ///////////////////////////////////////////////////////////////////////////
    void typed_continuation<void, util::unused_type>::serialize(
        hpx::serialization::input_archive& ar, unsigned)
    {
        // clang-format off
        // serialize base class
        ar & hpx::serialization::base_object<continuation>(*this);
        ar & f_;
        // clang-format on
    }

    void typed_continuation<void, util::unused_type>::serialize(
        hpx::serialization::output_archive& ar, unsigned)
    {
        // clang-format off
        // serialize base class
        ar & hpx::serialization::base_object<continuation>(*this);
        ar & f_;
        // clang-format on
    }

    void typed_continuation<void, util::unused_type>::trigger() const
    {
        LLCO_(info).format(
            "typed_continuation<void>::trigger({})", this->get_id());

        if (f_.empty())
        {
            if (!this->get_id())
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "typed_continuation<void>::trigger",
                    "attempt to trigger invalid LCO (the id is invalid)");
                return;
            }
            trigger_lco_event(this->get_id(), this->get_addr());
        }
        else
        {
            f_(this->get_id());
        }
    }

    typed_continuation<void, util::unused_type>::typed_continuation(
        hpx::id_type const& id)
      : continuation(id)
    {
    }

    typed_continuation<void, util::unused_type>::typed_continuation(
        hpx::id_type&& id) noexcept
      : continuation(HPX_MOVE(id))
    {
    }

    typed_continuation<void, util::unused_type>::typed_continuation(
        hpx::id_type const& id, naming::address&& addr)
      : continuation(id, HPX_MOVE(addr))
    {
    }

    typed_continuation<void, util::unused_type>::typed_continuation(
        hpx::id_type&& id, naming::address&& addr) noexcept
      : continuation(HPX_MOVE(id), HPX_MOVE(addr))
    {
    }

    typed_continuation<void, util::unused_type>::typed_continuation(
        typed_continuation&&) noexcept = default;
    typed_continuation<void, util::unused_type>& typed_continuation<void,
        util::unused_type>::operator=(typed_continuation&&) noexcept = default;

    void typed_continuation<void, util::unused_type>::trigger_value(
        util::unused_type&&) const
    {
        trigger();
    }

    void typed_continuation<void, util::unused_type>::trigger_value(
        util::unused_type const&) const
    {
        trigger();
    }
}    // namespace hpx::actions
