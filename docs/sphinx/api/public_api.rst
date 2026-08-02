..
    Copyright (C) 2021 Dimitra Karatza
    Copyright (C) 2020-2022 ETH Zurich

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _public_api:

==========
Public API
==========

Our API is semantically conforming; hence, the reader is highly encouraged to refer to the
corresponding facility in the `C++ Standard <https://en.cppreference.com/w/cpp/header>`_ if
needed. All names below are also available in the top-level ``hpx`` namespace unless
otherwise noted. The names in ``hpx`` should be preferred. The names in
sub-namespaces will eventually be removed.

.. _public_api_header_hpx_algorithm:

``hpx/algorithm.hpp``
=====================

The header :hpx-header:`libs/full/include/include,hpx/algorithm.hpp` corresponds to the
C++ standard library header :cppreference-header:`algorithm`. See :ref:`parallel_algorithms` for
more information about the parallel algorithms.

Classes
-------

.. table:: Classes of header ``hpx/algorithm.hpp``

   =========================================  ==============
   Class                                      C++ standard
   =========================================  ==============
   :cpp:class:`hpx::experimental::reduction`  |cpp19_n4808|_
   :cpp:class:`hpx::experimental::induction`  |cpp19_n4808|_
   =========================================  ==============

Functions
---------

.. table:: `hpx` functions of header ``hpx/algorithm.hpp``

   =================================================  ==========================================================
   `hpx` function                                     C++ standard
   =================================================  ==========================================================
   :cpp:func:`hpx::adjacent_find`                     :cppreference-generic:`algorithm,adjacent_find`
   :cpp:func:`hpx::all_of`                            :cppreference-generic:`algorithm,all_any_none_of,all_of`
   :cpp:func:`hpx::any_of`                            :cppreference-generic:`algorithm,all_any_none_of,any_of`
   :cpp:func:`hpx::copy`                              :cppreference-generic:`algorithm,copy`
   :cpp:func:`hpx::copy_if`                           :cppreference-generic:`algorithm,copy,copy_if`
   :cpp:func:`hpx::copy_n`                            :cppreference-generic:`algorithm,copy_n`
   :cpp:func:`hpx::count`                             :cppreference-generic:`algorithm,count`
   :cpp:func:`hpx::count_if`                          :cppreference-generic:`algorithm,count,count_if`
   :cpp:func:`hpx::ends_with`                         :cppreference-generic:`algorithm/ranges,ends_with`
   :cpp:func:`hpx::equal`                             :cppreference-generic:`algorithm,equal`
   :cpp:func:`hpx::fill`                              :cppreference-generic:`algorithm,fill`
   :cpp:func:`hpx::fill_n`                            :cppreference-generic:`algorithm,fill_n`
   :cpp:func:`hpx::find`                              :cppreference-generic:`algorithm,find`
   :cpp:func:`hpx::find_end`                          :cppreference-generic:`algorithm,find_end`
   :cpp:func:`hpx::find_first_of`                     :cppreference-generic:`algorithm,find_first_of`
   :cpp:func:`hpx::find_if`                           :cppreference-generic:`algorithm,find,find_if`
   :cpp:func:`hpx::find_if_not`                       :cppreference-generic:`algorithm,find,find_if_not`
   :cpp:func:`hpx::for_each`                          :cppreference-generic:`algorithm,for_each`
   :cpp:func:`hpx::for_each_n`                        :cppreference-generic:`algorithm,for_each_n`
   :cpp:func:`hpx::generate`                          :cppreference-generic:`algorithm,generate`
   :cpp:func:`hpx::generate_n`                        :cppreference-generic:`algorithm,generate_n`
   :cpp:func:`hpx::includes`                          :cppreference-generic:`algorithm,includes`
   :cpp:func:`hpx::inplace_merge`                     :cppreference-generic:`algorithm,inplace_merge`
   :cpp:func:`hpx::is_heap`                           :cppreference-generic:`algorithm,is_heap`
   :cpp:func:`hpx::is_heap_until`                     :cppreference-generic:`algorithm,is_heap_until`
   :cpp:func:`hpx::is_partitioned`                    :cppreference-generic:`algorithm,is_partitioned`
   :cpp:func:`hpx::is_sorted`                         :cppreference-generic:`algorithm,is_sorted`
   :cpp:func:`hpx::is_sorted_until`                   :cppreference-generic:`algorithm,is_sorted_until`
   :cpp:func:`hpx::lexicographical_compare`           :cppreference-generic:`algorithm,lexicographical_compare`
   :cpp:func:`hpx::make_heap`                         :cppreference-generic:`algorithm,make_heap`
   :cpp:func:`hpx::max_element`                       :cppreference-generic:`algorithm,max_element`
   :cpp:func:`hpx::merge`                             :cppreference-generic:`algorithm,merge`
   :cpp:func:`hpx::min_element`                       :cppreference-generic:`algorithm,min_element`
   :cpp:func:`hpx::minmax_element`                    :cppreference-generic:`algorithm,minmax_element`
   :cpp:func:`hpx::mismatch`                          :cppreference-generic:`algorithm,mismatch`
   :cpp:func:`hpx::move`                              :cppreference-generic:`algorithm,move`
   :cpp:func:`hpx::none_of`                           :cppreference-generic:`algorithm,all_any_none_of,none_of`
   :cpp:func:`hpx::nth_element`                       :cppreference-generic:`algorithm,nth_element`
   :cpp:func:`hpx::partial_sort`                      :cppreference-generic:`algorithm,partial_sort`
   :cpp:func:`hpx::partial_sort_copy`                 :cppreference-generic:`algorithm,partial_sort_copy`
   :cpp:func:`hpx::partition`                         :cppreference-generic:`algorithm,partition`
   :cpp:func:`hpx::partition_copy`                    :cppreference-generic:`algorithm,partition_copy`
   :cpp:func:`hpx::experimental::reduce_by_key`       `reduce_by_key <https://thrust.github.io/doc/group__reductions_gad5623f203f9b3fdcab72481c3913f0e0.html>`_
   :cpp:func:`hpx::remove`                            :cppreference-generic:`algorithm,remove`
   :cpp:func:`hpx::remove_copy`                       :cppreference-generic:`algorithm,remove_copy`
   :cpp:func:`hpx::remove_copy_if`                    :cppreference-generic:`algorithm,remove_copy,remove_copy_if`
   :cpp:func:`hpx::remove_if`                         :cppreference-generic:`algorithm,remove,remove_if`
   :cpp:func:`hpx::replace`                           :cppreference-generic:`algorithm,replace`
   :cpp:func:`hpx::replace_copy`                      :cppreference-generic:`algorithm,replace_copy`
   :cpp:func:`hpx::replace_copy_if`                   :cppreference-generic:`algorithm,replace_copy,replace_copy_if`
   :cpp:func:`hpx::replace_if`                        :cppreference-generic:`algorithm,replace,replace_if`
   :cpp:func:`hpx::reverse`                           :cppreference-generic:`algorithm,reverse`
   :cpp:func:`hpx::reverse_copy`                      :cppreference-generic:`algorithm,reverse_copy`
   :cpp:func:`hpx::rotate`                            :cppreference-generic:`algorithm,rotate`
   :cpp:func:`hpx::rotate_copy`                       :cppreference-generic:`algorithm,rotate_copy`
   :cpp:func:`hpx::search`                            :cppreference-generic:`algorithm,search`
   :cpp:func:`hpx::search_n`                          :cppreference-generic:`algorithm,search_n`
   :cpp:func:`hpx::set_difference`                    :cppreference-generic:`algorithm,set_difference`
   :cpp:func:`hpx::set_intersection`                  :cppreference-generic:`algorithm,set_intersection`
   :cpp:func:`hpx::set_symmetric_difference`          :cppreference-generic:`algorithm,set_symmetric_difference`
   :cpp:func:`hpx::set_union`                         :cppreference-generic:`algorithm,set_union`
   :cpp:func:`hpx::shift_left`                        :cppreference-generic:`algorithm,shift,shift_left`
   :cpp:func:`hpx::shift_right`                       :cppreference-generic:`algorithm,shift,shift_right`
   :cpp:func:`hpx::sort`                              :cppreference-generic:`algorithm,sort`
   :cpp:func:`hpx::experimental::sort_by_key`         `sort_by_key <https://thrust.github.io/doc/group__sorting_gabe038d6107f7c824cf74120500ef45ea.html>`_
   :cpp:func:`hpx::stable_partition`                  :cppreference-generic:`algorithm,stable_partition`
   :cpp:func:`hpx::stable_sort`                       :cppreference-generic:`algorithm,stable_sort`
   :cpp:func:`hpx::starts_with`                       :cppreference-generic:`algorithm/ranges,starts_with`
   :cpp:func:`hpx::swap_ranges`                       :cppreference-generic:`algorithm,swap_ranges`
   :cpp:func:`hpx::transform`                         :cppreference-generic:`algorithm,transform`
   :cpp:func:`hpx::unique`                            :cppreference-generic:`algorithm,unique`
   :cpp:func:`hpx::unique_copy`                       :cppreference-generic:`algorithm,unique_copy`
   :cpp:func:`hpx::experimental::for_loop`            |cpp19_n4808|_
   :cpp:func:`hpx::experimental::for_loop_strided`    |cpp19_n4808|_
   :cpp:func:`hpx::experimental::for_loop_n`          |cpp19_n4808|_
   :cpp:func:`hpx::experimental::for_loop_n_strided`  |cpp19_n4808|_
   =================================================  ==========================================================

.. table:: `hpx::ranges` functions of header ``hpx/algorithm.hpp``

   =======================================================  =================================================================
   `hpx::ranges` function                                   C++ standard
   =======================================================  =================================================================
   :cpp:func:`hpx::ranges::adjacent_find`                   :cppreference-generic:`algorithm/ranges,adjacent_find`
   :cpp:func:`hpx::ranges::all_of`                          :cppreference-generic:`algorithm/ranges,all_any_none_of,all_of`
   :cpp:func:`hpx::ranges::any_of`                          :cppreference-generic:`algorithm/ranges,all_any_none_of,any_of`
   :cpp:func:`hpx::ranges::copy`                            :cppreference-generic:`algorithm/ranges,copy`
   :cpp:func:`hpx::ranges::copy_if`                         :cppreference-generic:`algorithm/ranges,copy,copy_if`
   :cpp:func:`hpx::ranges::copy_n`                          :cppreference-generic:`algorithm/ranges,copy_n`
   :cpp:func:`hpx::ranges::count`                           :cppreference-generic:`algorithm/ranges,count`
   :cpp:func:`hpx::ranges::count_if`                        :cppreference-generic:`algorithm/ranges,count,count_if`
   :cpp:func:`hpx::ranges::ends_with`                       :cppreference-generic:`algorithm/ranges,ends_with`
   :cpp:func:`hpx::ranges::equal`                           :cppreference-generic:`algorithm/ranges,equal`
   :cpp:func:`hpx::ranges::fill`                            :cppreference-generic:`algorithm/ranges,fill`
   :cpp:func:`hpx::ranges::fill_n`                          :cppreference-generic:`algorithm/ranges,fill_n`
   :cpp:func:`hpx::ranges::find`                            :cppreference-generic:`algorithm/ranges,find`
   :cpp:func:`hpx::ranges::find_end`                        :cppreference-generic:`algorithm/ranges,find_end`
   :cpp:func:`hpx::ranges::find_first_of`                   :cppreference-generic:`algorithm/ranges,find_first_of`
   :cpp:func:`hpx::ranges::find_if`                         :cppreference-generic:`algorithm/ranges,find,find_if`
   :cpp:func:`hpx::ranges::find_if_not`                     :cppreference-generic:`algorithm/ranges,find,find_if_not`
   :cpp:func:`hpx::ranges::for_each`                        :cppreference-generic:`algorithm/ranges,for_each`
   :cpp:func:`hpx::ranges::for_each_n`                      :cppreference-generic:`algorithm/ranges,for_each_n`
   :cpp:func:`hpx::ranges::generate`                        :cppreference-generic:`algorithm/ranges,generate`
   :cpp:func:`hpx::ranges::generate_n`                      :cppreference-generic:`algorithm/ranges,generate_n`
   :cpp:func:`hpx::ranges::includes`                        :cppreference-generic:`algorithm/ranges,includes`
   :cpp:func:`hpx::ranges::inplace_merge`                   :cppreference-generic:`algorithm/ranges,inplace_merge`
   :cpp:func:`hpx::ranges::is_heap`                         :cppreference-generic:`algorithm/ranges,is_heap`
   :cpp:func:`hpx::ranges::is_heap_until`                   :cppreference-generic:`algorithm/ranges,is_heap_until`
   :cpp:func:`hpx::ranges::is_partitioned`                  :cppreference-generic:`algorithm/ranges,is_partitioned`
   :cpp:func:`hpx::ranges::is_sorted`                       :cppreference-generic:`algorithm/ranges,is_sorted`
   :cpp:func:`hpx::ranges::is_sorted_until`                 :cppreference-generic:`algorithm/ranges,is_sorted_until`
   :cpp:func:`hpx::ranges::make_heap`                       :cppreference-generic:`algorithm/ranges,make_heap`
   :cpp:func:`hpx::ranges::max_element`                     :cppreference-generic:`algorithm/ranges,max_element`
   :cpp:func:`hpx::ranges::merge`                           :cppreference-generic:`algorithm/ranges,merge`
   :cpp:func:`hpx::ranges::min_element`                     :cppreference-generic:`algorithm/ranges,min_element`
   :cpp:func:`hpx::ranges::minmax_element`                  :cppreference-generic:`algorithm/ranges,minmax_element`
   :cpp:func:`hpx::ranges::mismatch`                        :cppreference-generic:`algorithm/ranges,mismatch`
   :cpp:func:`hpx::ranges::move`                            :cppreference-generic:`algorithm/ranges,move`
   :cpp:func:`hpx::ranges::none_of`                         :cppreference-generic:`algorithm/ranges,all_any_none_of,none_of`
   :cpp:func:`hpx::ranges::nth_element`                     :cppreference-generic:`algorithm/ranges,nth_element`
   :cpp:func:`hpx::ranges::partial_sort`                    :cppreference-generic:`algorithm/ranges,partial_sort`
   :cpp:func:`hpx::ranges::partial_sort_copy`               :cppreference-generic:`algorithm/ranges,partial_sort_copy`
   :cpp:func:`hpx::ranges::partition`                       :cppreference-generic:`algorithm/ranges,partition`
   :cpp:func:`hpx::ranges::partition_copy`                  :cppreference-generic:`algorithm/ranges,partition_copy`
   :cpp:func:`hpx::ranges::set_difference`                  :cppreference-generic:`algorithm/ranges,set_difference`
   :cpp:func:`hpx::ranges::set_intersection`                :cppreference-generic:`algorithm/ranges,set_intersection`
   :cpp:func:`hpx::ranges::set_symmetric_difference`        :cppreference-generic:`algorithm/ranges,set_symmetric_difference`
   :cpp:func:`hpx::ranges::set_union`                       :cppreference-generic:`algorithm/ranges,set_union`
   :cpp:func:`hpx::ranges::shift_left`                      |p2440|_
   :cpp:func:`hpx::ranges::shift_right`                     |p2440|_
   :cpp:func:`hpx::ranges::sort`                            :cppreference-generic:`algorithm/ranges,sort`
   :cpp:func:`hpx::ranges::stable_partition`                :cppreference-generic:`algorithm/ranges,stable_partition`
   :cpp:func:`hpx::ranges::stable_sort`                     :cppreference-generic:`algorithm/ranges,stable_sort`
   :cpp:func:`hpx::ranges::starts_with`                     :cppreference-generic:`algorithm/ranges,starts_with`
   :cpp:func:`hpx::ranges::swap_ranges`                     :cppreference-generic:`algorithm/ranges,swap_ranges`
   :cpp:func:`hpx::ranges::transform`                       :cppreference-generic:`algorithm/ranges,transform`
   :cpp:func:`hpx::ranges::unique`                          :cppreference-generic:`algorithm/ranges,unique`
   :cpp:func:`hpx::ranges::unique_copy`                     :cppreference-generic:`algorithm/ranges,unique_copy`
   :cpp:func:`hpx::ranges::experimental::for_loop`          |cpp19_n4808|_
   :cpp:func:`hpx::ranges::experimental::for_loop_strided`  |cpp19_n4808|_
   =======================================================  =================================================================

.. _public_api_header_hpx_any:

``hpx/any.hpp``
===============

The header :hpx-header:`libs/core/include_local/include,hpx/any.hpp` corresponds to the C++
standard library header :cppreference-header:`any`.

:cpp:type:`hpx::any` is compatible with ``std::any``.

Classes
-------

.. table:: Classes of header ``hpx/any.hpp``

   ==================================  ================================================
   Class                               C++ standard
   ==================================  ================================================
   :cpp:type:`hpx::any`                :cppreference-generic:`utility,any`
   :cpp:type:`hpx::any_nonser`
   :cpp:type:`hpx::bad_any_cast`       :cppreference-generic:`utility/any,bad_any_cast`
   :cpp:type:`hpx::unique_any_nonser`
   ==================================  ================================================

Functions
---------

.. table:: Functions of header ``hpx/any.hpp``

   =======================================  ================================================
   Function                                 C++ standard
   =======================================  ================================================
   :cpp:func:`hpx::any_cast`                :cppreference-generic:`utility/any,any_cast`
   :cpp:func:`hpx::make_any`                :cppreference-generic:`utility/any,make_any`
   :cpp:func:`hpx::make_any_nonser`
   :cpp:func:`hpx::make_unique_any_nonser`
   =======================================  ================================================

.. _public_api_header_hpx_assert:

``hpx/assert.hpp``
==================

The header :hpx-header:`libs/core/assertion/include,hpx/assert.hpp` corresponds to the C++ standard
library header :cppreference-header:`cassert`.

:c:macro:`HPX_ASSERT` is the |hpx| equivalent to ``assert`` in ``cassert``.
:c:macro:`HPX_ASSERT` can also be used in CUDA device code.

Macros
------

.. table:: Macros of header ``hpx/assert.hpp``

   +--------------------------+
   | Macro                    |
   +==========================+
   | :c:macro:`HPX_ASSERT`    |
   +--------------------------+
   | :c:macro:`HPX_ASSERT_MSG`|
   +--------------------------+

.. _public_api_header_hpx_barrier:

``hpx/barrier.hpp``
===================

The header :hpx-header:`libs/full/include/include,hpx/barrier.hpp` corresponds to the
C++ standard library header :cppreference-header:`barrier` and contains a distributed barrier implementation. This
functionality is also exposed through the ``hpx::distributed`` namespace. The name in
``hpx::distributed`` should be preferred.

Classes
-------

.. table:: Classes of header ``hpx/barrier.hpp``

   +--------------------------+----------------------------------------+
   | Class                    | C++ standard                           |
   +==========================+========================================+
   | :cpp:class:`hpx::barrier`| :cppreference-generic:`thread,barrier` |
   +--------------------------+----------------------------------------+

.. table:: Distributed implementation of classes of header ``hpx/barrier.hpp``

   +----------------------------------------+
   | Class                                  |
   +========================================+
   | :cpp:class:`hpx::distributed::barrier` |
   +----------------------------------------+

.. _public_api_header_hpx_channel:

``hpx/channel.hpp``
===================

The header :hpx-header:`libs/full/include/include,hpx/channel.hpp` contains a local and a
distributed channel implementation. This  functionality is also exposed through the ``hpx::distributed``
namespace. The name in ``hpx::distributed`` should be preferred.

Classes
-------

.. table:: Classes of header ``hpx/channel.hpp``

   +--------------------------+
   | Class                    |
   +==========================+
   | :cpp:class:`hpx::channel`|
   +--------------------------+

.. table:: Distributed implementation of classes of header ``hpx/channel.hpp``

   +----------------------------------------+
   | Class                                  |
   +========================================+
   | :cpp:class:`hpx::distributed::channel` |
   +----------------------------------------+

.. _public_api_header_hpx_chrono:

``hpx/chrono.hpp``
==================

The header :hpx-header:`libs/core/include_local/include,hpx/chrono.hpp` corresponds to the
C++ standard library header :cppreference-header:`chrono`. The following replacements and
extensions are provided compared to :cppreference-header:`chrono`.

Classes
-------

.. table:: Classes of header ``hpx/chrono.hpp``

   ===============================================  ====================================================
   Class                                            C++ standard
   ===============================================  ====================================================
   :cpp:class:`hpx::chrono::high_resolution_clock`  :cppreference-generic:`chrono,high_resolution_clock`
   :cpp:class:`hpx::chrono::high_resolution_timer`
   :cpp:class:`hpx::chrono::steady_time_point`      :cppreference-generic:`chrono,time_point`
   ===============================================  ====================================================

.. _public_api_header_hpx_condition_variable:

``hpx/condition_variable.hpp``
==============================

The header :hpx-header:`libs/core/include_local/include,hpx/condition_variable.hpp` corresponds to the C++
standard library header :cppreference-header:`condition_variable`.

Classes
-------

.. table:: Classes of header ``hpx/condition_variable.hpp``

   ========================================  =====================================================
   Class                                     C++ standard
   ========================================  =====================================================
   :cpp:class:`hpx::condition_variable`      :cppreference-generic:`thread,condition_variable`
   :cpp:class:`hpx::condition_variable_any`  :cppreference-generic:`thread,condition_variable_any`
   :cpp:class:`hpx::cv_status`               :cppreference-generic:`thread,cv_status`
   ========================================  =====================================================

.. _public_api_header_hpx_exception:

``hpx/exception.hpp``
=====================

The header :hpx-header:`libs/core/include_local/include,hpx/exception.hpp` corresponds to
the C++ standard library header :cppreference-header:`exception`. :cpp:class:`hpx::exception`
extends ``std::exception`` and is the base class for all exceptions thrown in |hpx|.
:c:macro:`HPX_THROW_EXCEPTION` can be used to throw |hpx| exceptions with file and line information
attached to the exception.

Macros
------

- :c:macro:`HPX_THROW_EXCEPTION`

Classes
-------

.. table:: Classes of header ``hpx/exception.hpp``

   +----------------------------+----------------------------------------+
   | Class                      | C++ standard                           |
   +============================+========================================+
   | :cpp:class:`hpx::exception`| :cppreference-generic:`error,exception`|
   +----------------------------+----------------------------------------+

.. _public_api_header_hpx_execution:

``hpx/execution.hpp``
=====================

The header :hpx-header:`libs/core/include_local/include,hpx/execution.hpp` corresponds to the
C++ standard library header :cppreference-header:`execution`. See :ref:`parallel`,
:ref:`parallel_algorithms` and :ref:`executor_parameters` for more information about execution
policies and executor parameters.

.. note::

   These names are only available in the ``hpx::execution`` namespace, not in
   the top-level ``hpx`` namespace.

Constants
---------

.. table:: Constants of header ``hpx/execution.hpp``

   ====================================  ======================================================
   Constant                              C++ standard
   ====================================  ======================================================
   :cpp:var:`hpx::execution::seq`        :cppreference-generic:`algorithm,execution_policy_tag`
   :cpp:var:`hpx::execution::par`        :cppreference-generic:`algorithm,execution_policy_tag`
   :cpp:var:`hpx::execution::par_unseq`  :cppreference-generic:`algorithm,execution_policy_tag`
   :cpp:var:`hpx::execution::task`
   ====================================  ======================================================

Classes
-------

.. table:: Classes of header ``hpx/execution.hpp``

   =====================================================================  ========================================================
   Class                                                                  C++ standard
   =====================================================================  ========================================================
   :cpp:class:`hpx::execution::sequenced_policy`                          :cppreference-generic:`algorithm,execution_policy_tag_t`
   :cpp:class:`hpx::execution::parallel_policy`                           :cppreference-generic:`algorithm,execution_policy_tag_t`
   :cpp:class:`hpx::execution::parallel_unsequenced_policy`               :cppreference-generic:`algorithm,execution_policy_tag_t`
   :cpp:class:`hpx::execution::sequenced_task_policy`
   :cpp:class:`hpx::execution::parallel_task_policy`
   :cpp:class:`hpx::execution::experimental::auto_chunk_size`
   :cpp:class:`hpx::execution::experimental::dynamic_chunk_size`
   :cpp:class:`hpx::execution::experimental::guided_chunk_size`
   :cpp:class:`hpx::execution::experimental::persistent_auto_chunk_size`
   :cpp:class:`hpx::execution::experimental::static_chunk_size`
   :cpp:class:`hpx::execution::experimental::num_cores`
   =====================================================================  ========================================================

.. _public_api_header_hpx_functional:

``hpx/functional.hpp``
======================

The header :hpx-header:`libs/core/include_local/include,hpx/functional.hpp` corresponds to the
C++ standard library header :cppreference-header:`functional`. :cpp:class:`hpx::function` is a more
efficient and serializable replacement for ``std::function``.

Constants
---------

The following constants correspond to the C++ standard :cppreference-generic:`utility/functional,placeholders`

.. table:: Constants of header ``hpx/functional.hpp``

   +---------------------------------+
   | Constant                        |
   +=================================+
   | :cpp:var:`hpx::placeholders::_1`|
   +---------------------------------+
   | :cpp:var:`hpx::placeholders::_2`|
   +---------------------------------+
   | ...                             |
   +---------------------------------+
   | :cpp:var:`hpx::placeholders::_9`|
   +---------------------------------+


Classes
-------

.. table:: Classes of header ``hpx/functional.hpp``

   =============================================  =============================================================
   Class                                          C++ standard
   =============================================  =============================================================
   :cpp:class:`hpx::function`                     :cppreference-generic:`utility/functional,function`
   :cpp:class:`hpx::function_ref`                 |p0792|_
   :cpp:class:`hpx::move_only_function`           :cppreference-generic:`utility/functional,move_only_function`
   :cpp:struct:`hpx::is_bind_expression`          :cppreference-generic:`utility/functional,is_bind_expression`
   :cpp:struct:`hpx::is_placeholder`              :cppreference-generic:`utility/functional,is_placeholder`
   :cpp:struct:`hpx::scoped_annotation`
   =============================================  =============================================================

Functions
---------

.. table:: Functions of header ``hpx/functional.hpp``

   ========================================  =====================================================
   Function                                  C++ standard
   ========================================  =====================================================
   :cpp:func:`hpx::annotated_function`
   :cpp:func:`hpx::bind`                     :cppreference-generic:`utility/functional,bind`
   :cpp:func:`hpx::bind_back`                :cppreference-generic:`utility/functional,bind_front`
   :cpp:func:`hpx::bind_front`               :cppreference-generic:`utility/functional,bind_front`
   :cpp:func:`hpx::invoke`                   :cppreference-generic:`utility/functional,invoke`
   :cpp:func:`hpx::invoke_fused`             :cppreference-generic:`utility,apply`
   :cpp:func:`hpx::invoke_fused_r`
   :cpp:func:`hpx::mem_fn`                   :cppreference-generic:`utility/functional,mem_fn`
   ========================================  =====================================================

.. _public_api_header_hpx_future:

``hpx/future.hpp``
==================

The header :hpx-header:`libs/full/include/include,hpx/future.hpp` corresponds to the
C++ standard library header :cppreference-header:`future`. See :ref:`extend_futures` for more
information about extensions to futures compared to the C++ standard library.

This header file also contains overloads of :cpp:func:`hpx::async`,
:cpp:func:`hpx::post`, :cpp:func:`hpx::sync`, and :cpp:func:`hpx::dataflow` that can be used with
actions. See :ref:`action_invocation` for more information about invoking actions.

Classes
-------

.. table:: Classes of header ``hpx/future.hpp``

   ===============================  ============================================
   Class                            C++ standard
   ===============================  ============================================
   :cpp:class:`hpx::future`         :cppreference-generic:`thread,future`
   :cpp:class:`hpx::shared_future`  :cppreference-generic:`thread,shared_future`
   :cpp:class:`hpx::promise`        :cppreference-generic:`thread,promise`
   :cpp:class:`hpx::launch`         :cppreference-generic:`thread,launch`
   :cpp:class:`hpx::packaged_task`  :cppreference-generic:`thread,packaged_task`
   ===============================  ============================================

.. note::

   All names except :cpp:class:`hpx::promise` are also available in
   the top-level ``hpx`` namespace. ``hpx::promise`` refers to
   :cpp:class:`hpx::distributed::promise`, a distributed variant of
   :cpp:class:`hpx::promise`, but will eventually refer to
   :cpp:class:`hpx::promise` after a deprecation period.

.. table:: Distributed implementation of classes of header ``hpx/future.hpp``

   +---------------------------------------+
   | Class                                 |
   +=======================================+
   | :cpp:class:`hpx::distributed::promise`|
   +---------------------------------------+

Functions
---------

.. table:: Functions of header ``hpx/future.hpp``

   ========================================  =====================================
   Function                                  C++ standard
   ========================================  =====================================
   :cpp:func:`hpx::async`                    :cppreference-generic:`thread,async`
   :cpp:func:`hpx::post`
   :cpp:func:`hpx::sync`
   :cpp:func:`hpx::dataflow`
   :cpp:func:`hpx::make_future`
   :cpp:func:`hpx::make_shared_future`
   :cpp:func:`hpx::make_ready_future`        |p0159|_
   :cpp:func:`hpx::make_ready_future_alloc`
   :cpp:func:`hpx::make_ready_future_at`
   :cpp:func:`hpx::make_ready_future_after`
   :cpp:func:`hpx::make_exceptional_future`  |p0159|_
   :cpp:func:`hpx::when_all`                 |p0159|_
   :cpp:func:`hpx::when_any`                 |p0159|_
   :cpp:func:`hpx::when_some`
   :cpp:func:`hpx::when_each`
   :cpp:func:`hpx::wait_all`
   :cpp:func:`hpx::wait_all_nothrow`
   :cpp:func:`hpx::wait_all_n`
   :cpp:func:`hpx::wait_all_n_nothrow`
   :cpp:func:`hpx::wait_all_for`
   :cpp:func:`hpx::wait_all_for_nothrow`
   :cpp:func:`hpx::wait_all_for_n`
   :cpp:func:`hpx::wait_all_for_n_nothrow`
   :cpp:func:`hpx::wait_any`
   :cpp:func:`hpx::wait_any_n`
   :cpp:func:`hpx::wait_any_nothrow`
   :cpp:func:`hpx::wait_any_n_nothrow`
   :cpp:func:`hpx::wait_some`
   :cpp:func:`hpx::wait_some_n`
   :cpp:func:`hpx::wait_some_nothrow`
   :cpp:func:`hpx::wait_some_n_nothrow`
   :cpp:func:`hpx::wait_each`
   :cpp:func:`hpx::wait_each_n`
   :cpp:func:`hpx::wait_each_nothrow`
   :cpp:func:`hpx::wait_each_n_nothrow`
   ========================================  =====================================

.. _public_api_header_hpx_init:

``hpx/init.hpp``
================

The header :hpx-header:`libs/full/init_runtime/include,hpx/init.hpp` contains functionality for
starting, stopping, suspending, and resuming the |hpx| runtime. This is the main way to explicitly
start the |hpx| runtime. See :ref:`starting_hpx` for more details on starting the |hpx| runtime.

Classes
-------

.. table:: Classes of header ``hpx/init.hpp``

   +------------------------------+
   | Class                        |
   +==============================+
   | :cpp:class:`hpx::init_params`|
   +------------------------------+
   | :cpp:enum:`hpx::runtime_mode`|
   +------------------------------+


Functions
---------

.. table:: Functions of header ``hpx/init.hpp``

   +------------------------------+
   | Function                     |
   +==============================+
   | :cpp:func:`hpx::init`        |
   +------------------------------+
   | :cpp:func:`hpx::start`       |
   +------------------------------+
   | :cpp:func:`hpx::finalize`    |
   +------------------------------+
   | :cpp:func:`hpx::disconnect`  |
   +------------------------------+
   | :cpp:func:`hpx::suspend`     |
   +------------------------------+
   | :cpp:func:`hpx::resume`      |
   +------------------------------+

.. _public_api_header_hpx_latch:

``hpx/latch.hpp``
=================

The header :hpx-header:`libs/full/include/include,hpx/latch.hpp` corresponds to the C++
standard library header :cppreference-header:`latch`. It contains a local and a distributed latch
implementation. This functionality is also exposed through the ``hpx::distributed`` namespace.
The name in ``hpx::distributed`` should be preferred.

Classes
-------

.. table:: Classes of header ``hpx/latch.hpp``

   +----------------------------+----------------------------------------+
   | Class                      | C++ standard                           |
   +============================+========================================+
   | :cpp:class:`hpx::latch`    |  :cppreference-generic:`thread,latch`  |
   +----------------------------+----------------------------------------+

.. table:: Distributed implementation of classes of header ``hpx/latch.hpp``

   +--------------------------------------+
   | Class                                |
   +======================================+
   | :cpp:class:`hpx::distributed::latch` |
   +--------------------------------------+

.. _public_api_header_hpx_mutex:

``hpx/mutex.hpp``
=================

The header :hpx-header:`libs/core/include_local/include,hpx/mutex.hpp` corresponds to the
C++ standard library header :cppreference-header:`mutex`.

Classes
-------

.. table:: Classes of header ``hpx/mutex.hpp``

   =================================  ==============================================
   Class                              C++ standard
   =================================  ==============================================
   :cpp:class:`hpx::mutex`            :cppreference-generic:`thread,mutex`
   :cpp:class:`hpx::no_mutex`
   :cpp:class:`hpx::once_flag`        :cppreference-generic:`thread,once_flag`
   :cpp:class:`hpx::recursive_mutex`  :cppreference-generic:`thread,recursive_mutex`
   :cpp:class:`hpx::spinlock`
   :cpp:class:`hpx::timed_mutex`      :cppreference-generic:`thread,timed_mutex`
   :cpp:class:`hpx::unlock_guard`
   =================================  ==============================================

Functions
---------

.. table:: Functions of header ``hpx/mutex.hpp``

   +----------------------------+------------------------------------------+
   | Class                      | C++ standard                             |
   +============================+==========================================+
   | :cpp:func:`hpx::call_once` | :cppreference-generic:`thread,call_once` |
   +----------------------------+------------------------------------------+

.. _public_api_header_hpx_memory:

``hpx/memory.hpp``
==================

The header :hpx-header:`libs/core/include_local/include,hpx/memory.hpp` corresponds to the
C++ standard library header :cppreference-header:`memory`. It contains parallel versions of the
copy, fill, move, and construct helper functions in :cppreference-header:`memory`. See
:ref:`parallel_algorithms` for more information about the parallel algorithms.

Functions
---------

.. table:: `hpx` functions of header ``hpx/memory.hpp``

   ================================================== ================================================================
   `hpx` function                                     C++ standard
   ================================================== ================================================================
   :cpp:func:`hpx::uninitialized_copy`                :cppreference-generic:`memory,uninitialized_copy`
   :cpp:func:`hpx::uninitialized_copy_n`              :cppreference-generic:`memory,uninitialized_copy_n`
   :cpp:func:`hpx::uninitialized_default_construct`   :cppreference-generic:`memory,uninitialized_default_construct`
   :cpp:func:`hpx::uninitialized_default_construct_n` :cppreference-generic:`memory,uninitialized_default_construct_n`
   :cpp:func:`hpx::uninitialized_fill`                :cppreference-generic:`memory,uninitialized_fill`
   :cpp:func:`hpx::uninitialized_fill_n`              :cppreference-generic:`memory,uninitialized_fill_n`
   :cpp:func:`hpx::uninitialized_move`                :cppreference-generic:`memory,uninitialized_move`
   :cpp:func:`hpx::uninitialized_move_n`              :cppreference-generic:`memory,uninitialized_move_n`
   :cpp:func:`hpx::uninitialized_value_construct`     :cppreference-generic:`memory,uninitialized_value_construct`
   :cpp:func:`hpx::uninitialized_value_construct_n`   :cppreference-generic:`memory,uninitialized_value_construct_n`
   ================================================== ================================================================

.. table:: `hpx::ranges` functions of header ``hpx/memory.hpp``

   ========================================================== =======================================================================
   `hpx::ranges` function                                     C++ standard
   ========================================================== =======================================================================
   :cpp:func:`hpx::ranges::uninitialized_copy`                :cppreference-generic:`memory/ranges,uninitialized_copy`
   :cpp:func:`hpx::ranges::uninitialized_copy_n`              :cppreference-generic:`memory/ranges,uninitialized_copy_n`
   :cpp:func:`hpx::ranges::uninitialized_default_construct`   :cppreference-generic:`memory/ranges,uninitialized_default_construct`
   :cpp:func:`hpx::ranges::uninitialized_default_construct_n` :cppreference-generic:`memory/ranges,uninitialized_default_construct_n`
   :cpp:func:`hpx::ranges::uninitialized_fill`                :cppreference-generic:`memory/ranges,uninitialized_fill`
   :cpp:func:`hpx::ranges::uninitialized_fill_n`              :cppreference-generic:`memory/ranges,uninitialized_fill_n`
   :cpp:func:`hpx::ranges::uninitialized_move`                :cppreference-generic:`memory/ranges,uninitialized_move`
   :cpp:func:`hpx::ranges::uninitialized_move_n`              :cppreference-generic:`memory/ranges,uninitialized_move_n`
   :cpp:func:`hpx::ranges::uninitialized_value_construct`     :cppreference-generic:`memory/ranges,uninitialized_value_construct`
   :cpp:func:`hpx::ranges::uninitialized_value_construct_n`   :cppreference-generic:`memory/ranges,uninitialized_value_construct_n`
   ========================================================== =======================================================================

.. _public_api_header_hpx_numeric:

``hpx/numeric.hpp``
===================

The header :hpx-header:`libs/core/include_local/include,hpx/numeric.hpp` corresponds to the
C++ standard library header :cppreference-header:`numeric`. See :ref:`parallel_algorithms` for more
information about the parallel algorithms.

Functions
---------

.. table:: `hpx` functions of header ``hpx/numeric.hpp``

   ========================================= ==========================================================
   `hpx` function                                     C++ standard
   ========================================= ==========================================================
   :cpp:func:`hpx::adjacent_difference`      :cppreference-generic:`algorithm,adjacent_difference`
   :cpp:func:`hpx::exclusive_scan`           :cppreference-generic:`algorithm,exclusive_scan`
   :cpp:func:`hpx::inclusive_scan`           :cppreference-generic:`algorithm,inclusive_scan`
   :cpp:func:`hpx::reduce`                   :cppreference-generic:`algorithm,reduce`
   :cpp:func:`hpx::transform_exclusive_scan` :cppreference-generic:`algorithm,transform_exclusive_scan`
   :cpp:func:`hpx::transform_inclusive_scan` :cppreference-generic:`algorithm,transform_inclusive_scan`
   :cpp:func:`hpx::transform_reduce`         :cppreference-generic:`algorithm,transform_reduce`
   ========================================= ==========================================================

.. table:: `hpx::ranges` functions of header ``hpx/numeric.hpp``

   +--------------------------------------------------+
   | `hpx::ranges` function                           |
   +==================================================+
   | :cpp:func:`hpx::ranges::adjacent_difference`     |
   +--------------------------------------------------+
   | :cpp:func:`hpx::ranges::exclusive_scan`          |
   +--------------------------------------------------+
   | :cpp:func:`hpx::ranges::inclusive_scan`          |
   +--------------------------------------------------+
   | :cpp:func:`hpx::ranges::reduce`                  |
   +--------------------------------------------------+
   | :cpp:func:`hpx::ranges::transform_exclusive_scan`|
   +--------------------------------------------------+
   | :cpp:func:`hpx::ranges::transform_inclusive_scan`|
   +--------------------------------------------------+
   | :cpp:func:`hpx::ranges::transform_reduce`        |
   +--------------------------------------------------+

.. _public_api_header_hpx_optional:

``hpx/optional.hpp``
====================

The header :hpx-header:`libs/core/include_local/include,hpx/optional.hpp` corresponds to the
C++ standard library header :cppreference-header:`optional`. :cpp:type:`hpx::optional` is compatible
with ``std::optional``.

Constants
---------

- :cpp:var:`hpx::nullopt`

Classes
-------

.. table:: Classes of header ``hpx/optional.hpp``

   =====================================  ============================================================
   Class                                  C++ standard
   =====================================  ============================================================
   :cpp:class:`hpx::optional`             :cppreference-generic:`utility,optional`
   :cpp:class:`hpx::nullopt_t`            :cppreference-generic:`utility,nullopt_t`
   :cpp:class:`hpx::bad_optional_access`  :cppreference-generic:`utility/optional,bad_optional_access`
   =====================================  ============================================================

.. _public_api_header_hpx_runtime:

``hpx/runtime.hpp``
===================

The header :hpx-header:`libs/full/include/include,hpx/runtime.hpp` contains functions for accessing
local and distributed runtime information.

Typedefs
--------

.. table:: Typedefs of header ``hpx/runtime.hpp``

   +-----------------------------------------+
   | Typedef                                 |
   +=========================================+
   | :cpp:type:`hpx::startup_function_type`  |
   +-----------------------------------------+
   | :cpp:type:`hpx::shutdown_function_type` |
   +-----------------------------------------+

Functions
---------

.. table:: Functions of header ``hpx/runtime.hpp``

   +------------------------------------------------------------+
   | Function                                                   |
   +============================================================+
   | :cpp:func:`hpx::find_root_locality`                        |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::find_all_localities`                       |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::find_remote_localities`                    |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::find_locality`                             |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::get_colocation_id`                         |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::get_locality_id`                           |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::get_num_worker_threads`                    |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::get_worker_thread_num`                     |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::get_thread_name`                           |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::register_pre_startup_function`             |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::register_startup_function`                 |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::register_pre_shutdown_function`            |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::register_shutdown_function`                |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::get_num_localities`                        |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::get_locality_name`                         |
   +------------------------------------------------------------+
   | :cpp:func:`hpx::local::termination_detection`              |
   +------------------------------------------------------------+

.. _public_api_header_hpx_scope:

``hpx/experimental/scope.hpp``
==============================

The header :hpx-header:`libs/core/include_local/include,hpx/experimental/scope.hpp` corresponds to the
C++ standard library header :cppreference-header:`experimental/scope`.

Classes
-------

.. table:: Classes of header ``hpx/scope.hpp``

   ==============================================  ==================================================
   Class                                           C++ standard
   ==============================================  ==================================================
   :cpp:class:`hpx::experimental::scope_exit`      :cppreference-generic:`experimental,scope_exit`
   :cpp:class:`hpx::experimental::scope_fail`      :cppreference-generic:`experimental,scope_fail`
   :cpp:class:`hpx::experimental::scope_success`   :cppreference-generic:`experimental,scope_success`
   ==============================================  ==================================================

.. _public_api_header_hpx_semaphore:

``hpx/semaphore.hpp``
=====================

The header :hpx-header:`libs/core/include_local/include,hpx/semaphore.hpp` corresponds to the
C++ standard library header :cppreference-header:`semaphore`.

Classes
-------

.. table:: Classes of header ``hpx/semaphore.hpp``

   ==========================================  =================================================
   Class                                       C++ standard
   ==========================================  =================================================
   :cpp:class:`hpx::binary_semaphore`          :cppreference-generic:`thread,counting_semaphore`
   :cpp:class:`hpx::counting_semaphore`        :cppreference-generic:`thread,counting_semaphore`
   ==========================================  =================================================

.. _public_api_header_hpx_shared_mutex:

``hpx/shared_mutex.hpp``
========================

The header :hpx-header:`libs/core/include_local/include,hpx/shared_mutex.hpp` corresponds to the
C++ standard library header :cppreference-header:`shared_mutex`.

Classes
-------

.. table:: Classes of header ``hpx/shared_mutex.hpp``

   +--------------------------------+---------------------------------------------+
   | Class                          | C++ standard                                |
   +================================+=============================================+
   | :cpp:class:`hpx::shared_mutex` | :cppreference-generic:`thread,shared_mutex` |
   +--------------------------------+---------------------------------------------+

.. _public_api_header_hpx_source_location:

``hpx/source_location.hpp``
===========================

The header :hpx-header:`libs/core/include_local/include,hpx/source_location.hpp` corresponds to the
C++ standard library header :cppreference-header:`source_location`.

Classes
-------

.. table:: Classes of header ``hpx/system_error.hpp``

   +-----------------------------------+-------------------------------------------------+
   | Class                             | C++ standard                                    |
   +===================================+=================================================+
   | :cpp:class:`hpx::source_location` | :cppreference-generic:`utility,source_location` |
   +-----------------------------------+-------------------------------------------------+

.. _public_api_header_hpx_stop_token:

``hpx/stop_token.hpp``
======================

The header :hpx-header:`libs/core/include_local/include,hpx/stop_token.hpp` corresponds to the
C++ standard library header :cppreference-header:`stop_token`.

Constants
---------

.. table:: Constants of header ``hpx/stop_token.hpp``

   +-----------------------------+--------------------------------------------------------+
   | Constant                    | C++ standard                                           |
   +=============================+========================================================+
   | :cpp:var:`hpx::nostopstate` | :cppreference-generic:`thread/stop_source,nostopstate` |
   +-----------------------------+--------------------------------------------------------+

Classes
-------

.. table:: Classes of header ``hpx/stop_token.hpp``

   ================================  ========================================================
   Class                             C++ standard
   ================================  ========================================================
   :cpp:class:`hpx::stop_callback`   :cppreference-generic:`thread,stop_callback`
   :cpp:class:`hpx::stop_source`     :cppreference-generic:`thread,stop_source`
   :cpp:class:`hpx::stop_token`      :cppreference-generic:`thread,stop_token`
   :cpp:struct:`hpx::nostopstate_t`  :cppreference-generic:`thread/stop_source,nostopstate_t`
   ================================  ========================================================

.. _public_api_header_hpx_system_error:

``hpx/system_error.hpp``
========================

The header :hpx-header:`libs/core/include_local/include,hpx/system_error.hpp` corresponds to the
C++ standard library header :cppreference-header:`system_error`.

Classes
-------

.. table:: Classes of header ``hpx/system_error.hpp``

   +------------------------------+------------------------------------------+
   | Class                        | C++ standard                             |
   +==============================+==========================================+
   | :cpp:class:`hpx::error_code` | :cppreference-generic:`error,error_code` |
   +------------------------------+------------------------------------------+

.. _public_api_header_hpx_task_block:

``hpx/task_block.hpp``
======================

The header :hpx-header:`libs/core/include_local/include,hpx/task_block.hpp` corresponds to the
``task_block`` feature in |cpp17_n4755|_. See :ref:`using_task_block` for more details on using task
blocks.

Classes
-------

.. table:: Classes of header ``hpx/task_block.hpp``

   +---------------------------------------------------------+
   | Class                                                   |
   +=========================================================+
   | :cpp:class:`hpx::experimental::task_canceled_exception` |
   +---------------------------------------------------------+
   | :cpp:class:`hpx::experimental::task_block`              |
   +---------------------------------------------------------+

Functions
---------

.. table:: Functions of header ``hpx/task_block.hpp``

   +-----------------------------------------------------------------+
   | Function                                                        |
   +=================================================================+
   | :cpp:func:`hpx::experimental::define_task_block`                |
   +-----------------------------------------------------------------+
   | :cpp:func:`hpx::experimental::define_task_block_restore_thread` |
   +-----------------------------------------------------------------+

.. _public_api_header_hpx_task_group:

``hpx/experimental/task_group.hpp``
===================================

The header :hpx-header:`libs/core/include_local/include,hpx/experimental/task_group.hpp`
corresponds to the ``task_group`` feature in |oneTBB|_.

Classes
-------

.. table:: Classes of header ``hpx/experimental/task_group.hpp``

   +---------------------------------------------------------+
   | Class                                                   |
   +=========================================================+
   | :cpp:class:`hpx::experimental::task_group`              |
   +---------------------------------------------------------+

.. _public_api_header_hpx_thread:

``hpx/thread.hpp``
==================

The header :hpx-header:`libs/core/include_local/include,hpx/thread.hpp` corresponds to the
C++ standard library header :cppreference-header:`thread`. The functionality in this header is
equivalent to the standard library thread functionality, with the exception that the |hpx|
equivalents are implemented on top of lightweight threads and the |hpx| runtime.

Classes
-------

.. table:: Classes of header ``hpx/thread.hpp``

   =========================  ======================================
   Class                      C++ standard
   =========================  ======================================
   :cpp:class:`hpx::thread`   :cppreference-generic:`thread,thread`
   :cpp:class:`hpx::jthread`  :cppreference-generic:`thread,jthread`
   =========================  ======================================

Functions
---------

.. table:: Functions of header ``hpx/thread.hpp``

   =========================================  ==========================================
   Function                                     C++ standard
   =========================================  ==========================================
   :cpp:func:`hpx::this_thread::yield`        :cppreference-generic:`thread,yield`
   :cpp:func:`hpx::this_thread::get_id`       :cppreference-generic:`thread,get_id`
   :cpp:func:`hpx::this_thread::sleep_for`    :cppreference-generic:`thread,sleep_for`
   :cpp:func:`hpx::this_thread::sleep_until`  :cppreference-generic:`thread,sleep_until`
   =========================================  ==========================================

.. _public_api_header_hpx_tuple:

``hpx/tuple.hpp``
=================

The header :hpx-header:`libs/core/include_local/include,hpx/tuple.hpp` corresponds to the
C++ standard library header :cppreference-header:`tuple`. :cpp:class:`hpx::tuple` can be used in
CUDA device code, unlike ``std::tuple``.

Constants
---------

.. table:: Constants of header ``hpx/tuple.hpp``

   +------------------------+----------------------------------------------+
   | Constant               | C++ standard                                 |
   +========================+==============================================+
   | :cpp:var:`hpx::ignore` | :cppreference-generic:`utility/tuple,ignore` |
   +------------------------+----------------------------------------------+

Classes
-------

.. table:: Classes of header ``hpx/tuple.hpp``

   ================================  ===================================================
   Class                             C++ standard
   ================================  ===================================================
   :cpp:struct:`hpx::tuple`          :cppreference-generic:`utility,tuple`
   :cpp:struct:`hpx::tuple_size`     :cppreference-generic:`utility,tuple_size`
   :cpp:struct:`hpx::tuple_element`  :cppreference-generic:`utility,tuple_element`
   ================================  ===================================================

Functions
---------

.. table:: Functions of header ``hpx/tuple.hpp``

   =================================  ======================================================
   Function                           C++ standard
   =================================  ======================================================
   :cpp:func:`hpx::make_tuple`        :cppreference-generic:`utility/tuple,tuple_element`
   :cpp:func:`hpx::tie`               :cppreference-generic:`utility/tuple,tie`
   :cpp:func:`hpx::forward_as_tuple`  :cppreference-generic:`utility/tuple,forward_as_tuple`
   :cpp:func:`hpx::tuple_cat`         :cppreference-generic:`utility/tuple,tuple_cat`
   :cpp:func:`hpx::get`               :cppreference-generic:`utility/tuple,get`
   =================================  ======================================================

.. _public_api_header_hpx_type_traits:

``hpx/type_traits.hpp``
=======================

The header :hpx-header:`libs/core/include_local/include,hpx/type_traits.hpp` corresponds to the
C++ standard library header :cppreference-header:`type_traits`.

Classes
-------

.. table:: Classes of header ``hpx/type_traits.hpp``

   =================================  ==========================================
   Class                              C++ standard
   =================================  ==========================================
   :cpp:struct:`hpx::is_invocable`    :cppreference-generic:`types,is_invocable`
   :cpp:struct:`hpx::is_invocable_r`  :cppreference-generic:`types,is_invocable`
   =================================  ==========================================

.. _public_api_header_hpx_unwrap:

``hpx/unwrap.hpp``
==================

The header :hpx-header:`libs/core/include_local/include,hpx/unwrap.hpp` contains utilities for
unwrapping futures.

Classes
-------

.. table:: Classes of header ``hpx/unwrap.hpp``

   +-------------------------------------------+
   | Class                                     |
   +===========================================+
   | :cpp:struct:`hpx::functional::unwrap`     |
   +-------------------------------------------+
   | :cpp:struct:`hpx::functional::unwrap_n`   |
   +-------------------------------------------+
   | :cpp:struct:`hpx::functional::unwrap_all` |
   +-------------------------------------------+

Functions
---------

.. table:: Functions of header ``hpx/unwrap.hpp``

   +----------------------------------+
   | Function                         |
   +==================================+
   | :cpp:func:`hpx::unwrap`          |
   +----------------------------------+
   | :cpp:func:`hpx::unwrap_n`        |
   +----------------------------------+
   | :cpp:func:`hpx::unwrap_all`      |
   +----------------------------------+
   | :cpp:func:`hpx::unwrapping`      |
   +----------------------------------+
   | :cpp:func:`hpx::unwrapping_n`    |
   +----------------------------------+
   | :cpp:func:`hpx::unwrapping_all`  |
   +----------------------------------+

.. _public_api_header_hpx_version:

``hpx/version.hpp``
===================

The header :hpx-header:`libs/core/version/include,hpx/version.hpp` provides version information
about |hpx|.

Macros
------

.. table:: Macros of header ``hpx/version.hpp``

   +----------------------------------+
   | Macro                            |
   +==================================+
   | :c:macro:`HPX_VERSION_MAJOR`     |
   +----------------------------------+
   | :c:macro:`HPX_VERSION_MINOR`     |
   +----------------------------------+
   | :c:macro:`HPX_VERSION_SUBMINOR`  |
   +----------------------------------+
   | :c:macro:`HPX_VERSION_FULL`      |
   +----------------------------------+
   | :c:macro:`HPX_VERSION_DATE`      |
   +----------------------------------+
   | :c:macro:`HPX_VERSION_TAG`       |
   +----------------------------------+
   | :c:macro:`HPX_AGAS_VERSION`      |
   +----------------------------------+

Functions
---------

.. table:: Functions of header ``hpx/version.hpp``

   +-----------------------------------------+
   | Function                                |
   +=========================================+
   | :cpp:func:`hpx::major_version`          |
   +-----------------------------------------+
   | :cpp:func:`hpx::minor_version`          |
   +-----------------------------------------+
   | :cpp:func:`hpx::subminor_version`       |
   +-----------------------------------------+
   | :cpp:func:`hpx::full_version`           |
   +-----------------------------------------+
   | :cpp:func:`hpx::full_version_as_string` |
   +-----------------------------------------+
   | :cpp:func:`hpx::tag`                    |
   +-----------------------------------------+
   | :cpp:func:`hpx::agas_version`           |
   +-----------------------------------------+
   | :cpp:func:`hpx::build_type`             |
   +-----------------------------------------+
   | :cpp:func:`hpx::build_date_time`        |
   +-----------------------------------------+

.. _public_api_header_hpx_wrap_main:

``hpx/wrap_main.hpp``
=====================

The header :hpx-header:`wrap/include,hpx/wrap_main.hpp` does not provide any direct functionality
but is used for implicitly using ``main`` as the runtime entry point. See :ref:`minimal` for more
details on implicitly starting the |hpx| runtime.
