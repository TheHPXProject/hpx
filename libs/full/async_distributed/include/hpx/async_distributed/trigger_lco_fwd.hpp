//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/async_distributed/trigger_lco_fwd.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/lcos_fwd.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Trigger the LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should be
    ///                triggered.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT HPX_EXPORT void trigger_lco_event(
        hpx::id_type id, naming::address&& addr, bool move_credits = true);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Trigger the LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should be
    ///                triggered.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT inline void trigger_lco_event(
        hpx::id_type const& id, bool move_credits = true)
    {
        trigger_lco_event(id, naming::address(), move_credits);
    }

    /// \brief Trigger the LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should be
    ///                  triggered.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT HPX_EXPORT void trigger_lco_event(hpx::id_type id,
        naming::address&& addr, hpx::id_type const& cont,
        bool move_credits = true);

    /// \brief Trigger the LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should be
    ///                  triggered.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT inline void trigger_lco_event(hpx::id_type const& id,
        hpx::id_type const& cont, bool move_credits = true)
    {
        trigger_lco_event(id, naming::address(), cont, move_credits);
    }

    /// \brief Set the result value for the LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should
    ///                receive the given value.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param t  [in] This is the value which should be sent to the LCO.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT template <typename Result>
    void set_lco_value(hpx::id_type id, naming::address&& addr, Result&& t,
        bool move_credits = true);

    /// \brief Set the result value for the (managed) LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should
    ///                receive the given value.
    /// \param t  [in] This is the value which should be sent to the LCO.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT template <typename Result>
        requires(!std::is_same_v<std::decay_t<Result>, naming::address>)
    void set_lco_value(
        hpx::id_type const& id, Result&& t, bool move_credits = true)
    {
        naming::address addr(nullptr,
            to_int(components::component_enum_type::base_lco_with_value));
        set_lco_value(id, HPX_MOVE(addr), HPX_FORWARD(Result, t), move_credits);
    }

    /// \brief Set the result value for the (unmanaged) LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should
    ///                receive the given value.
    /// \param t  [in] This is the value which should be sent to the LCO.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT template <typename Result>
        requires(!std::is_same_v<std::decay_t<Result>, naming::address>)
    void set_lco_value_unmanaged(
        hpx::id_type const& id, Result&& t, bool move_credits = true)
    {
        naming::address addr(nullptr,
            to_int(components::component_enum_type::
                    base_lco_with_value_unmanaged));
        set_lco_value(id, HPX_MOVE(addr), HPX_FORWARD(Result, t), move_credits);
    }

    /// \brief Set the result value for the LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should
    ///                  receive the given value.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param t    [in] This is the value which should be sent to the LCO.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT template <typename Result>
    void set_lco_value(hpx::id_type id, naming::address&& addr, Result&& t,
        hpx::id_type const& cont, bool move_credits = true);

    /// \brief Set the result value for the (managed) LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should
    ///                  receive the given value.
    /// \param t    [in] This is the value which should be sent to the LCO.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT template <typename Result>
        requires(!std::is_same_v<std::decay_t<Result>, naming::address>)
    void set_lco_value(hpx::id_type const& id, Result&& t,
        hpx::id_type const& cont, bool move_credits = true)
    {
        naming::address addr(nullptr,
            to_int(components::component_enum_type::base_lco_with_value));
        set_lco_value(
            id, HPX_MOVE(addr), HPX_FORWARD(Result, t), cont, move_credits);
    }

    /// \brief Set the result value for the (unmanaged) LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should
    ///                  receive the given value.
    /// \param t    [in] This is the value which should be sent to the LCO.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT template <typename Result>
        requires(!std::is_same_v<std::decay_t<Result>, naming::address>)
    void set_lco_value_unmanaged(hpx::id_type const& id, Result&& t,
        hpx::id_type const& cont, bool move_credits = true)
    {
        naming::address addr(nullptr,
            to_int(components::component_enum_type::
                    base_lco_with_value_unmanaged));
        set_lco_value(
            id, HPX_MOVE(addr), HPX_FORWARD(Result, t), cont, move_credits);
    }

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should
    ///                receive the error value.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param e  [in] This is the error value which should be sent to
    ///                the LCO.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT HPX_EXPORT void set_lco_error(hpx::id_type id,
        naming::address&& addr, std::exception_ptr const& e,
        bool move_credits = true);

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should
    ///                receive the error value.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param e  [in] This is the error value which should be sent to
    ///                the LCO.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT HPX_EXPORT void set_lco_error(hpx::id_type id,
        naming::address&& addr, std::exception_ptr&& e,
        bool move_credits = true);

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should
    ///                receive the error value.
    /// \param e  [in] This is the error value which should be sent to
    ///                the LCO.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT inline void set_lco_error(hpx::id_type const& id,
        std::exception_ptr const& e, bool move_credits = true)
    {
        set_lco_error(id, naming::address(), e, move_credits);
    }

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id [in] This represents the id of the LCO which should
    ///                receive the error value.
    /// \param e  [in] This is the error value which should be sent to
    ///                the LCO.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT inline void set_lco_error(hpx::id_type const& id,
        std::exception_ptr&& e, bool move_credits = true)
    {
        set_lco_error(id, naming::address(), HPX_MOVE(e), move_credits);
    }

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should
    ///                  receive the error value.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param e    [in] This is the error value which should be sent to
    ///                  the LCO.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT HPX_EXPORT void set_lco_error(hpx::id_type id,
        naming::address&& addr, std::exception_ptr const& e,
        hpx::id_type const& cont, bool move_credits = true);

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should
    ///                  receive the error value.
    /// \param addr [in] This represents the addr of the LCO which should be
    ///                triggered.
    /// \param e    [in] This is the error value which should be sent to
    ///                  the LCO.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT HPX_EXPORT void set_lco_error(hpx::id_type id,
        naming::address&& addr, std::exception_ptr&& e,
        hpx::id_type const& cont, bool move_credits = true);

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should
    ///                  receive the error value.
    /// \param e    [in] This is the error value which should be sent to
    ///                  the LCO.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT inline void set_lco_error(hpx::id_type const& id,
        std::exception_ptr const& e, hpx::id_type const& cont,
        bool move_credits = true)
    {
        set_lco_error(id, naming::address(), e, cont, move_credits);
    }

    /// \brief Set the error state for the LCO referenced by the given id
    ///
    /// \param id   [in] This represents the id of the LCO which should
    ///                  receive the error value.
    /// \param e    [in] This is the error value which should be sent to
    ///                  the LCO.
    /// \param cont [in] This represents the LCO to trigger after completion.
    /// \param move_credits [in] If this is set to \a true then it is ok to
    ///                     send all credits in \a id along with the generated
    ///                     message. The default value is \a true.
    HPX_CXX_EXPORT inline void set_lco_error(hpx::id_type const& id,
        std::exception_ptr&& e, hpx::id_type const& cont,
        bool move_credits = true)
    {
        set_lco_error(id, naming::address(), HPX_MOVE(e), cont, move_credits);
    }
}    // namespace hpx
