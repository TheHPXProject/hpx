//  Copyright (c) 2017 Shoshana Jakobovits
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/thread_pools/scheduled_thread_pool.hpp>
#include <hpx/thread_pools/scheduled_thread_pool_impl.hpp>

///////////////////////////////////////////////////////////////////////////////
/// explicit template instantiation for the thread pools of our choice
template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_queue_scheduler<>>;

template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::static_queue_scheduler<>>;

template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::background_scheduler<>>;

template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
        hpx::threads::policies::lockfree_fifo>>;
template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
        hpx::threads::policies::lockfree_fifo_double>>;

template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::static_priority_queue_scheduler<>>;
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
        hpx::threads::policies::lockfree_lifo>>;
#endif

#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
        hpx::threads::policies::lockfree_abp_fifo>>;
template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_priority_queue_scheduler<std::mutex,
        hpx::threads::policies::lockfree_abp_lifo>>;
#endif

template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::shared_priority_queue_scheduler<>>;

#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_workrequesting_scheduler<>>;

#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_workrequesting_scheduler<std::mutex,
        hpx::threads::policies::lockfree_lifo>>;
#endif

template class HPX_CORE_EXPORT hpx::threads::detail::scheduled_thread_pool<
    hpx::threads::policies::local_workrequesting_scheduler<std::mutex,
        hpx::threads::policies::concurrentqueue_fifo>>;
#endif
