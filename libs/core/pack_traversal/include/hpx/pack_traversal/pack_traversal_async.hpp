//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/pack_traversal/detail/pack_traversal_async_impl.hpp>

#include <utility>

namespace hpx::util {

    /// A tag which is passed to the `operator()` of the visitor if an element
    /// is visited synchronously.
    HPX_CXX_CORE_EXPORT using detail::async_traverse_visit_tag;

    /// A tag which is passed to the `operator()` of the visitor if an element
    /// is visited after the traversal was detached.
    HPX_CXX_CORE_EXPORT using detail::async_traverse_detach_tag;

    /// A tag which is passed to the `operator()` of the visitor if the
    /// asynchronous pack traversal was finished.
    HPX_CXX_CORE_EXPORT using detail::async_traverse_complete_tag;

    /// A tag to identify that a mapper shall be constructed in-place from the
    /// first argument passed.
    HPX_CXX_CORE_EXPORT using detail::async_traverse_in_place_tag;

    /// Traverses the pack with the given visitor in an asynchronous way.
    ///
    /// This function works in the same way as `traverse_pack`, however, we are
    /// able to suspend and continue the traversal at later time. Thus, we
    /// require a visitor callable object which provides three `operator()`
    /// overloads as depicted by the code sample below:
    ///
    ///    ```cpp
    ///    struct my_async_visitor {
    ///        // The synchronous overload is called for each object. It may
    ///        // return false to suspend the current control. In that case the
    ///        // overload below is called.
    ///        template <typename T>
    ///        bool operator()(async_traverse_visit_tag, T&& element)
    ///        {
    ///            return true;
    ///        }
    ///
    ///        // The asynchronous overload is called when the synchronous
    ///        // overload returned false. In addition to the current visited
    ///        // element, the overload is called with a continuation callable
    ///        // object which resumes the traversal when called later.
    ///        template <typename T, typename N>
    ///        void operator()(async_traverse_detach_tag, T&& element, N&& next)
    ///        {
    ///        }
    ///
    ///        // The overload is called when the traversal was finished. The
    ///        // whole pack which was traversed asynchronously is passed.
    ///        template <typename T>
    ///        void operator()(async_traverse_complete_tag, T&& pack)
    ///        {
    ///        }
    ///    };
    ///    ```
    ///
    /// \param   visitor A visitor object which provides the three `operator()`
    ///                  overloads that were described above. Additionally, the
    ///                  visitor must be compatible for referencing it from a
    ///                  `hpx::intrusive_ptr`. The visitor should have a virtual
    ///                  destructor!
    ///
    /// \param   pack    The arbitrary parameter pack which is traversed
    ///                  asynchronously. Nested objects inside containers and
    ///                  tuple like types are traversed recursively.
    ///
    /// \returns         A hpx::intrusive_ptr that references an instance of
    ///                  the given visitor object.
    ///
    /// See `traverse_pack` for a detailed description about the traversal
    /// behavior and capabilities.
    ///
    HPX_CXX_CORE_EXPORT template <typename Visitor, typename... T>
    auto traverse_pack_async(Visitor&& visitor, T&&... pack)
        -> decltype(detail::apply_pack_transform_async(
            HPX_FORWARD(Visitor, visitor), HPX_FORWARD(T, pack)...))
    {
        return detail::apply_pack_transform_async(
            HPX_FORWARD(Visitor, visitor), HPX_FORWARD(T, pack)...);
    }

    /// Traverses the pack with the given visitor in an asynchronous way.
    ///
    /// This function works in the same way as `traverse_pack`, however, we are
    /// able to suspend and continue the traversal at later time. Thus, we
    /// require a visitor callable object which provides three `operator()`
    /// overloads as depicted by the code sample below:
    ///
    /// ```cpp
    /// struct my_async_visitor {
    ///     // The synchronous overload is called for each object. It may
    ///     // return false to suspend the current control. In that case the
    ///     // overload below is called.
    ///     template <typename T>
    ///     bool operator()(async_traverse_visit_tag, T&& element)
    ///     {
    ///         return true;
    ///     }
    ///
    ///     // The asynchronous overload is called when the synchronous
    ///     // overload returned false. In addition to the current visited
    ///     // element, this overload receives a continuation callable object
    ///     // which resumes the traversal when called later. The continuation
    ///     // may be stored and called later or dropped completely to abort
    ///     // the traversal early.
    ///     template <typename T, typename N>
    ///     void operator()(async_traverse_detach_tag, T&& element, N&& next)
    ///     {
    ///     }
    ///
    ///     // This overload is called when the traversal is finished. The
    ///     // complete pack that was traversed asynchronously is passed.
    ///     template <typename T>
    ///     void operator()(async_traverse_complete_tag, T&& pack)
    ///     {
    ///     }
    /// };
    /// ```
    ///
    /// \param   visitor A visitor object which provides the three `operator()`
    ///                  overloads that were described above. Additionally, the
    ///                  visitor must be compatible for referencing it from a
    ///                  `hpx::intrusive_ptr`. The visitor should have a virtual
    ///                  destructor!
    ///
    /// \param   pack    The arbitrary parameter pack which is traversed
    ///                  asynchronously. Nested objects inside containers and
    ///                  tuple like types are traversed recursively.
    /// \param  alloc    Allocator instance to use to create the traversal
    ///                  frame.
    ///
    /// \returns         A hpx::intrusive_ptr that references an instance of
    ///                  the given visitor object.
    ///
    /// See `traverse_pack` for a detailed description about the traversal
    /// behavior and capabilities.
    ///
    HPX_CXX_CORE_EXPORT template <typename Allocator, typename Visitor,
        typename... T>
    auto traverse_pack_async_allocator(
        Allocator const& alloc, Visitor&& visitor, T&&... pack)
        -> decltype(detail::apply_pack_transform_async_allocator(
            alloc, HPX_FORWARD(Visitor, visitor), HPX_FORWARD(T, pack)...))
    {
        return detail::apply_pack_transform_async_allocator(
            alloc, HPX_FORWARD(Visitor, visitor), HPX_FORWARD(T, pack)...);
    }
}    // namespace hpx::util
