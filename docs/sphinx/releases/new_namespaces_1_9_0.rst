..
    Copyright (C) 2023 Dimitra Karatza

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _new_namespaces_1_9_0:

==============================
|hpx| V1.9.0 Namespace changes
==============================

The latest release includes amongst others changes in the namespaces so that |hpx|
facilities correspond to the C++ Standard Library. The old namespaces are
deprecated. Below is a comprehensive list of the namespace changes.

.. table:: Namespace changes in V1.9.0

   ===========================================================  ==============================================================
   Old namespace                                                New namespace
   ===========================================================  ==============================================================
   :hpx:func:`hpx::util::mem_fn`                                :hpx:func:`hpx::mem_fn`
   :hpx:func:`hpx::util::invoke`                                :hpx:func:`hpx::invoke`
   :hpx:func:`hpx::util::invoke_r`                              :hpx:func:`hpx::invoke_r`
   :hpx:func:`hpx::util::invoke_fused`                          :hpx:func:`hpx::invoke_fused`
   :hpx:func:`hpx::util::invoke_fused_r`                        :hpx:func:`hpx::invoke_fused_r`
   :hpx:class:`hpx::util::unlock_guard`                         :hpx:class:`hpx::unlock_guard`
   :hpx:func:`hpx::parallel::v1::reduce_by_key`                 :hpx:func:`hpx::experimental::reduce_by_key`
   :hpx:func:`hpx::parallel::v1::sort_by_key`                   :hpx:func:`hpx::experimental::sort_by_key`
   :hpx:class:`hpx::parallel::task_canceled_exception`          :hpx:class:`hpx::experimental::task_canceled_exception`
   :hpx:class:`hpx::parallel::task_block`                       :hpx:class:`hpx::experimental::task_block`
   :hpx:func:`hpx::parallel::define_task_block`                 :hpx:func:`hpx::experimental::define_task_block`                |
   :hpx:func:`hpx::parallel::define_task_block_restore_thread`  :hpx:func:`hpx::experimental::define_task_block_restore_thread`
   :hpx:class:`hpx::execution::experimental::task_group`        :hpx:class:`hpx::experimental::task_group`
   ===========================================================  ==============================================================
