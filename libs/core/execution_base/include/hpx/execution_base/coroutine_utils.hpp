//  Copyright (c) 2022 Shreyas Atre
//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX20_COROUTINES)
#include <hpx/execution_base/traits/coroutine_traits.hpp>

namespace hpx::execution::experimental {

    // execution::as_awaitable [execution.coro_utils.as_awaitable]
    // as_awaitable is used to transform an object into one that is
    // awaitable within a particular coroutine.
    //
    // as_awaitable is a customization point object.
    // For some subexpressions e and p where p is an lvalue,
    // E names the type decltype((e)) and P names the type decltype((p)),
    // as_awaitable(e, p) is expression-equivalent to the following:
    //
    //   1. tag_invoke(as_awaitable, e, p)
    //      if that expression is well-formed.
    //      -- Mandates: is-awaitable<A> is true,
    //       where A is the type of the tag_invoke expression above.
    //   2. Otherwise, e if is-awaitable<E> is true.
    //   3. Otherwise, sender-awaitable{e, p} if awaitable-sender<E, P>
    //      is true.
    //   4. Otherwise, e.
}    // namespace hpx::execution::experimental

#endif    // HPX_HAVE_CXX20_COROUTINES
