//  Copyright (c) 2007-2008 Chirag Dekate
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file unlock_guard.hpp
/// \page hpx::unlock_guard
/// \headerfile hpx/mutex.hpp

#pragma once

#include <hpx/config.hpp>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // These are a helper structures to make sure a lock gets unlocked and
    // locked (relocked and unlocked) again in a scope.

    HPX_CXX_CORE_EXPORT template <typename Lock>
    class relock_guard;

    /// The class \c unlock_guard is a mutex wrapper that provides a convenient
    /// mechanism for releasing a mutex for the duration of a scoped block.
    /// \details \c unlock_guard performs the opposite functionality of
    /// \c lock_guard.
    ///
    /// When a \c lock_guard object is created, it attempts to take ownership of
    /// the mutex it is given. When control leaves the scope in which the \c
    /// lock_guard object was created, the \c lock_guard is destructed and the
    /// mutex is released. Accordingly, when an \c unlock_guard object is
    /// created, it attempts to release the ownership of the mutex it is given.
    /// So, when control leaves the scope in which the \c unlock_guard object
    /// was created, the \c unlock_guard is destructed and the mutex is owned
    /// again. In this way, the mutex is unlocked in the constructor and locked
    /// in the destructor, so that one can have an unlocked section within a
    /// locked one.
    ///
    /// \tparam Lock The type of the lockable object managed by the
    ///              \c unlock_guard.
    ///
    /// \note \c unlock_guard is non-copyable and non-movable.
    HPX_CXX_CORE_EXPORT template <typename Lock>
    class unlock_guard
    {
    public:
        explicit unlock_guard(Lock& l) noexcept
          : l_(l)
        {
            l_.unlock();
        }

        unlock_guard(unlock_guard const&) = delete;
        unlock_guard(unlock_guard&&) = delete;
        unlock_guard& operator=(unlock_guard const&) = delete;
        unlock_guard& operator=(unlock_guard&&) = delete;

        ~unlock_guard()
        {
            l_.lock();
        }

    private:
        friend class relock_guard<Lock>;
        Lock& l_;
    };

    /// The class \c relock_guard is a mutex wrapper that provides a convenient
    /// mechanism for reacquiring a lock that was released by an \c
    /// unlock_guard, for the duration of a scoped block.
    /// \details \c relock_guard performs the opposite functionality of
    /// \c unlock_guard within an already-unlocked section.
    ///
    /// When a \c relock_guard object is created, it takes ownership of the
    /// underlying \c Lock associated with the given \c unlock_guard by locking
    /// it. When control leaves the scope in which the \c relock_guard object
    /// was created, the \c relock_guard is destructed and the underlying \c
    /// Lock is released again (unlocked), restoring the unlocked state managed
    /// by the enclosing \c unlock_guard. In this way, \c relock_guard allows a
    /// temporarily locked section to be nested within an unlocked section
    /// created by \c unlock_guard.
    ///
    /// \tparam Lock The type of the lockable object managed by the associated
    ///              \c unlock_guard.
    ///
    /// \note \c relock_guard is non-copyable and non-movable.
    HPX_CXX_CORE_EXPORT template <typename Lock>
    class relock_guard
    {
    public:
        explicit relock_guard(unlock_guard<Lock>& ul) noexcept
          : ul_(ul)
        {
            ul_.l_.lock();
        }

        relock_guard(relock_guard const&) = delete;
        relock_guard(relock_guard&&) = delete;
        relock_guard& operator=(relock_guard const&) = delete;
        relock_guard& operator=(relock_guard&&) = delete;

        ~relock_guard()
        {
            ul_.l_.unlock();
        }

    private:
        unlock_guard<Lock>& ul_;
    };
}    // namespace hpx

namespace hpx::util {

    template <typename Lock>
    using unlock_guard HPX_DEPRECATED_V(1, 9,
        "hpx::util::unlock_guard is deprecated, use hpx::unlock_guard "
        "instead") = hpx::unlock_guard<Lock>;
}    // namespace hpx::util
