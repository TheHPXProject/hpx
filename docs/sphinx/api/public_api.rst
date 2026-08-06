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

   ================================================  ==========================================================
   `hpx` function                                    C++ standard
   ================================================  ==========================================================
   :hpx-api:`hpx::adjacent_find`                     :cppreference-generic:`algorithm,adjacent_find`
   :hpx-api:`hpx::all_of`                            :cppreference-generic:`algorithm,all_any_none_of,all_of`
   :hpx-api:`hpx::any_of`                            :cppreference-generic:`algorithm,all_any_none_of,any_of`
   :hpx-api:`hpx::copy`                              :cppreference-generic:`algorithm,copy`
   :hpx-api:`hpx::copy_if`                           :cppreference-generic:`algorithm,copy,copy_if`
   :hpx-api:`hpx::copy_n`                            :cppreference-generic:`algorithm,copy_n`
   :hpx-api:`hpx::count`                             :cppreference-generic:`algorithm,count`
   :hpx-api:`hpx::count_if`                          :cppreference-generic:`algorithm,count,count_if`
   :hpx-api:`hpx::ends_with`                         :cppreference-generic:`algorithm/ranges,ends_with`
   :hpx-api:`hpx::equal`                             :cppreference-generic:`algorithm,equal`
   :hpx-api:`hpx::fill`                              :cppreference-generic:`algorithm,fill`
   :hpx-api:`hpx::fill_n`                            :cppreference-generic:`algorithm,fill_n`
   :hpx-api:`hpx::find`                              :cppreference-generic:`algorithm,find`
   :hpx-api:`hpx::find_end`                          :cppreference-generic:`algorithm,find_end`
   :hpx-api:`hpx::find_first_of`                     :cppreference-generic:`algorithm,find_first_of`
   :hpx-api:`hpx::find_if`                           :cppreference-generic:`algorithm,find,find_if`
   :hpx-api:`hpx::find_if_not`                       :cppreference-generic:`algorithm,find,find_if_not`
   :hpx-api:`hpx::for_each`                          :cppreference-generic:`algorithm,for_each`
   :hpx-api:`hpx::for_each_n`                        :cppreference-generic:`algorithm,for_each_n`
   :hpx-api:`hpx::generate`                          :cppreference-generic:`algorithm,generate`
   :hpx-api:`hpx::generate_n`                        :cppreference-generic:`algorithm,generate_n`
   :hpx-api:`hpx::includes`                          :cppreference-generic:`algorithm,includes`
   :hpx-api:`hpx::inplace_merge`                     :cppreference-generic:`algorithm,inplace_merge`
   :hpx-api:`hpx::is_heap`                           :cppreference-generic:`algorithm,is_heap`
   :hpx-api:`hpx::is_heap_until`                     :cppreference-generic:`algorithm,is_heap_until`
   :hpx-api:`hpx::is_partitioned`                    :cppreference-generic:`algorithm,is_partitioned`
   :hpx-api:`hpx::is_sorted`                         :cppreference-generic:`algorithm,is_sorted`
   :hpx-api:`hpx::is_sorted_until`                   :cppreference-generic:`algorithm,is_sorted_until`
   :hpx-api:`hpx::lexicographical_compare`           :cppreference-generic:`algorithm,lexicographical_compare`
   :hpx-api:`hpx::make_heap`                         :cppreference-generic:`algorithm,make_heap`
   :hpx-api:`hpx::max_element`                       :cppreference-generic:`algorithm,max_element`
   :hpx-api:`hpx::merge`                             :cppreference-generic:`algorithm,merge`
   :hpx-api:`hpx::min_element`                       :cppreference-generic:`algorithm,min_element`
   :hpx-api:`hpx::minmax_element`                    :cppreference-generic:`algorithm,minmax_element`
   :hpx-api:`hpx::mismatch`                          :cppreference-generic:`algorithm,mismatch`
   :hpx-api:`hpx::move`                              :cppreference-generic:`algorithm,move`
   :hpx-api:`hpx::none_of`                           :cppreference-generic:`algorithm,all_any_none_of,none_of`
   :hpx-api:`hpx::nth_element`                       :cppreference-generic:`algorithm,nth_element`
   :hpx-api:`hpx::partial_sort`                      :cppreference-generic:`algorithm,partial_sort`
   :hpx-api:`hpx::partial_sort_copy`                 :cppreference-generic:`algorithm,partial_sort_copy`
   :hpx-api:`hpx::partition`                         :cppreference-generic:`algorithm,partition`
   :hpx-api:`hpx::partition_copy`                    :cppreference-generic:`algorithm,partition_copy`
   :hpx-api:`hpx::experimental::reduce_by_key`       `reduce_by_key <https://thrust.github.io/doc/group__reductions_gad5623f203f9b3fdcab72481c3913f0e0.html>`_
   :hpx-api:`hpx::remove`                            :cppreference-generic:`algorithm,remove`
   :hpx-api:`hpx::remove_copy`                       :cppreference-generic:`algorithm,remove_copy`
   :hpx-api:`hpx::remove_copy_if`                    :cppreference-generic:`algorithm,remove_copy,remove_copy_if`
   :hpx-api:`hpx::remove_if`                         :cppreference-generic:`algorithm,remove,remove_if`
   :hpx-api:`hpx::replace`                           :cppreference-generic:`algorithm,replace`
   :hpx-api:`hpx::replace_copy`                      :cppreference-generic:`algorithm,replace_copy`
   :hpx-api:`hpx::replace_copy_if`                   :cppreference-generic:`algorithm,replace_copy,replace_copy_if`
   :hpx-api:`hpx::replace_if`                        :cppreference-generic:`algorithm,replace,replace_if`
   :hpx-api:`hpx::reverse`                           :cppreference-generic:`algorithm,reverse`
   :hpx-api:`hpx::reverse_copy`                      :cppreference-generic:`algorithm,reverse_copy`
   :hpx-api:`hpx::rotate`                            :cppreference-generic:`algorithm,rotate`
   :hpx-api:`hpx::rotate_copy`                       :cppreference-generic:`algorithm,rotate_copy`
   :hpx-api:`hpx::search`                            :cppreference-generic:`algorithm,search`
   :hpx-api:`hpx::search_n`                          :cppreference-generic:`algorithm,search_n`
   :hpx-api:`hpx::set_difference`                    :cppreference-generic:`algorithm,set_difference`
   :hpx-api:`hpx::set_intersection`                  :cppreference-generic:`algorithm,set_intersection`
   :hpx-api:`hpx::set_symmetric_difference`          :cppreference-generic:`algorithm,set_symmetric_difference`
   :hpx-api:`hpx::set_union`                         :cppreference-generic:`algorithm,set_union`
   :hpx-api:`hpx::shift_left`                        :cppreference-generic:`algorithm,shift,shift_left`
   :hpx-api:`hpx::shift_right`                       :cppreference-generic:`algorithm,shift,shift_right`
   :hpx-api:`hpx::sort`                              :cppreference-generic:`algorithm,sort`
   :hpx-api:`hpx::experimental::sort_by_key`         `sort_by_key <https://thrust.github.io/doc/group__sorting_gabe038d6107f7c824cf74120500ef45ea.html>`_
   :hpx-api:`hpx::stable_partition`                  :cppreference-generic:`algorithm,stable_partition`
   :hpx-api:`hpx::stable_sort`                       :cppreference-generic:`algorithm,stable_sort`
   :hpx-api:`hpx::starts_with`                       :cppreference-generic:`algorithm/ranges,starts_with`
   :hpx-api:`hpx::swap_ranges`                       :cppreference-generic:`algorithm,swap_ranges`
   :hpx-api:`hpx::transform`                         :cppreference-generic:`algorithm,transform`
   :hpx-api:`hpx::unique`                            :cppreference-generic:`algorithm,unique`
   :hpx-api:`hpx::unique_copy`                       :cppreference-generic:`algorithm,unique_copy`
   :hpx-api:`hpx::experimental::for_loop`            |cpp19_n4808|_
   :hpx-api:`hpx::experimental::for_loop_strided`    |cpp19_n4808|_
   :hpx-api:`hpx::experimental::for_loop_n`          |cpp19_n4808|_
   :hpx-api:`hpx::experimental::for_loop_n_strided`  |cpp19_n4808|_
   ================================================  ==========================================================

.. table:: `hpx::ranges` functions of header ``hpx/algorithm.hpp``

   ======================================================  =================================================================
   `hpx::ranges` function                                  C++ standard
   ======================================================  =================================================================
   :hpx-api:`hpx::ranges::adjacent_find`                   :cppreference-generic:`algorithm/ranges,adjacent_find`
   :hpx-api:`hpx::ranges::all_of`                          :cppreference-generic:`algorithm/ranges,all_any_none_of,all_of`
   :hpx-api:`hpx::ranges::any_of`                          :cppreference-generic:`algorithm/ranges,all_any_none_of,any_of`
   :hpx-api:`hpx::ranges::copy`                            :cppreference-generic:`algorithm/ranges,copy`
   :hpx-api:`hpx::ranges::copy_if`                         :cppreference-generic:`algorithm/ranges,copy,copy_if`
   :hpx-api:`hpx::ranges::copy_n`                          :cppreference-generic:`algorithm/ranges,copy_n`
   :hpx-api:`hpx::ranges::count`                           :cppreference-generic:`algorithm/ranges,count`
   :hpx-api:`hpx::ranges::count_if`                        :cppreference-generic:`algorithm/ranges,count,count_if`
   :hpx-api:`hpx::ranges::ends_with`                       :cppreference-generic:`algorithm/ranges,ends_with`
   :hpx-api:`hpx::ranges::equal`                           :cppreference-generic:`algorithm/ranges,equal`
   :hpx-api:`hpx::ranges::fill`                            :cppreference-generic:`algorithm/ranges,fill`
   :hpx-api:`hpx::ranges::fill_n`                          :cppreference-generic:`algorithm/ranges,fill_n`
   :hpx-api:`hpx::ranges::find`                            :cppreference-generic:`algorithm/ranges,find`
   :hpx-api:`hpx::ranges::find_end`                        :cppreference-generic:`algorithm/ranges,find_end`
   :hpx-api:`hpx::ranges::find_first_of`                   :cppreference-generic:`algorithm/ranges,find_first_of`
   :hpx-api:`hpx::ranges::find_if`                         :cppreference-generic:`algorithm/ranges,find,find_if`
   :hpx-api:`hpx::ranges::find_if_not`                     :cppreference-generic:`algorithm/ranges,find,find_if_not`
   :hpx-api:`hpx::ranges::for_each`                        :cppreference-generic:`algorithm/ranges,for_each`
   :hpx-api:`hpx::ranges::for_each_n`                      :cppreference-generic:`algorithm/ranges,for_each_n`
   :hpx-api:`hpx::ranges::generate`                        :cppreference-generic:`algorithm/ranges,generate`
   :hpx-api:`hpx::ranges::generate_n`                      :cppreference-generic:`algorithm/ranges,generate_n`
   :hpx-api:`hpx::ranges::includes`                        :cppreference-generic:`algorithm/ranges,includes`
   :hpx-api:`hpx::ranges::inplace_merge`                   :cppreference-generic:`algorithm/ranges,inplace_merge`
   :hpx-api:`hpx::ranges::is_heap`                         :cppreference-generic:`algorithm/ranges,is_heap`
   :hpx-api:`hpx::ranges::is_heap_until`                   :cppreference-generic:`algorithm/ranges,is_heap_until`
   :hpx-api:`hpx::ranges::is_partitioned`                  :cppreference-generic:`algorithm/ranges,is_partitioned`
   :hpx-api:`hpx::ranges::is_sorted`                       :cppreference-generic:`algorithm/ranges,is_sorted`
   :hpx-api:`hpx::ranges::is_sorted_until`                 :cppreference-generic:`algorithm/ranges,is_sorted_until`
   :hpx-api:`hpx::ranges::make_heap`                       :cppreference-generic:`algorithm/ranges,make_heap`
   :hpx-api:`hpx::ranges::max_element`                     :cppreference-generic:`algorithm/ranges,max_element`
   :hpx-api:`hpx::ranges::merge`                           :cppreference-generic:`algorithm/ranges,merge`
   :hpx-api:`hpx::ranges::min_element`                     :cppreference-generic:`algorithm/ranges,min_element`
   :hpx-api:`hpx::ranges::minmax_element`                  :cppreference-generic:`algorithm/ranges,minmax_element`
   :hpx-api:`hpx::ranges::mismatch`                        :cppreference-generic:`algorithm/ranges,mismatch`
   :hpx-api:`hpx::ranges::move`                            :cppreference-generic:`algorithm/ranges,move`
   :hpx-api:`hpx::ranges::none_of`                         :cppreference-generic:`algorithm/ranges,all_any_none_of,none_of`
   :hpx-api:`hpx::ranges::nth_element`                     :cppreference-generic:`algorithm/ranges,nth_element`
   :hpx-api:`hpx::ranges::partial_sort`                    :cppreference-generic:`algorithm/ranges,partial_sort`
   :hpx-api:`hpx::ranges::partial_sort_copy`               :cppreference-generic:`algorithm/ranges,partial_sort_copy`
   :hpx-api:`hpx::ranges::partition`                       :cppreference-generic:`algorithm/ranges,partition`
   :hpx-api:`hpx::ranges::partition_copy`                  :cppreference-generic:`algorithm/ranges,partition_copy`
   :hpx-api:`hpx::ranges::set_difference`                  :cppreference-generic:`algorithm/ranges,set_difference`
   :hpx-api:`hpx::ranges::set_intersection`                :cppreference-generic:`algorithm/ranges,set_intersection`
   :hpx-api:`hpx::ranges::set_symmetric_difference`        :cppreference-generic:`algorithm/ranges,set_symmetric_difference`
   :hpx-api:`hpx::ranges::set_union`                       :cppreference-generic:`algorithm/ranges,set_union`
   :hpx-api:`hpx::ranges::shift_left`                      |p2440|_
   :hpx-api:`hpx::ranges::shift_right`                     |p2440|_
   :hpx-api:`hpx::ranges::sort`                            :cppreference-generic:`algorithm/ranges,sort`
   :hpx-api:`hpx::ranges::stable_partition`                :cppreference-generic:`algorithm/ranges,stable_partition`
   :hpx-api:`hpx::ranges::stable_sort`                     :cppreference-generic:`algorithm/ranges,stable_sort`
   :hpx-api:`hpx::ranges::starts_with`                     :cppreference-generic:`algorithm/ranges,starts_with`
   :hpx-api:`hpx::ranges::swap_ranges`                     :cppreference-generic:`algorithm/ranges,swap_ranges`
   :hpx-api:`hpx::ranges::transform`                       :cppreference-generic:`algorithm/ranges,transform`
   :hpx-api:`hpx::ranges::unique`                          :cppreference-generic:`algorithm/ranges,unique`
   :hpx-api:`hpx::ranges::unique_copy`                     :cppreference-generic:`algorithm/ranges,unique_copy`
   :hpx-api:`hpx::ranges::experimental::for_loop`          |cpp19_n4808|_
   :hpx-api:`hpx::ranges::experimental::for_loop_strided`  |cpp19_n4808|_
   ======================================================  =================================================================

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

   ======================================  ================================================
   Function                                C++ standard
   ======================================  ================================================
   :hpx-api:`hpx::any_cast`                :cppreference-generic:`utility/any,any_cast`
   :hpx-api:`hpx::make_any`                :cppreference-generic:`utility/any,make_any`
   :hpx-api:`hpx::make_any_nonser`
   :hpx-api:`hpx::make_unique_any_nonser`
   ======================================  ================================================

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

   =======================================  =====================================================
   Function                                  C++ standard
   ======================================  =====================================================
   :hpx-api:`hpx::annotated_function`
   :hpx-api:`hpx::bind`                     :cppreference-generic:`utility/functional,bind`
   :hpx-api:`hpx::bind_back`                :cppreference-generic:`utility/functional,bind_front`
   :hpx-api:`hpx::bind_front`               :cppreference-generic:`utility/functional,bind_front`
   :hpx-api:`hpx::invoke`                   :cppreference-generic:`utility/functional,invoke`
   :hpx-api:`hpx::invoke_fused`             :cppreference-generic:`utility,apply`
   :hpx-api:`hpx::invoke_fused_r`
   :hpx-api:`hpx::mem_fn`                   :cppreference-generic:`utility/functional,mem_fn`
   =======================================  =====================================================

.. _public_api_header_hpx_future:

``hpx/future.hpp``
==================

The header :hpx-header:`libs/full/include/include,hpx/future.hpp` corresponds to the
C++ standard library header :cppreference-header:`future`. See :ref:`extend_futures` for more
information about extensions to futures compared to the C++ standard library.

This header file also contains overloads of :hpx-api:`hpx::async`,
:hpx-api:`hpx::post`, :hpx-api:`hpx::sync`, and :hpx-api:`hpx::dataflow` that can be used with
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

   =======================================  =====================================
   Function                                 C++ standard
   =======================================  =====================================
   :hpx-api:`hpx::async`                    :cppreference-generic:`thread,async`
   :hpx-api:`hpx::post`
   :hpx-api:`hpx::sync`
   :hpx-api:`hpx::dataflow`
   :hpx-api:`hpx::make_future`
   :hpx-api:`hpx::make_shared_future`
   :hpx-api:`hpx::make_ready_future`        |p0159|_
   :hpx-api:`hpx::make_ready_future_alloc`
   :hpx-api:`hpx::make_ready_future_at`
   :hpx-api:`hpx::make_ready_future_after`
   :hpx-api:`hpx::make_exceptional_future`  |p0159|_
   :hpx-api:`hpx::when_all`                 |p0159|_
   :hpx-api:`hpx::when_any`                 |p0159|_
   :hpx-api:`hpx::when_some`
   :hpx-api:`hpx::when_each`
   :hpx-api:`hpx::wait_all`
   :hpx-api:`hpx::wait_all_nothrow`
   :hpx-api:`hpx::wait_all_n`
   :hpx-api:`hpx::wait_all_n_nothrow`
   :hpx-api:`hpx::wait_all_for`
   :hpx-api:`hpx::wait_all_for_nothrow`
   :hpx-api:`hpx::wait_all_for_n`
   :hpx-api:`hpx::wait_all_for_n_nothrow`
   :hpx-api:`hpx::wait_any`
   :hpx-api:`hpx::wait_any_n`
   :hpx-api:`hpx::wait_any_nothrow`
   :hpx-api:`hpx::wait_any_n_nothrow`
   :hpx-api:`hpx::wait_some`
   :hpx-api:`hpx::wait_some_n`
   :hpx-api:`hpx::wait_some_nothrow`
   :hpx-api:`hpx::wait_some_n_nothrow`
   :hpx-api:`hpx::wait_each`
   :hpx-api:`hpx::wait_each_n`
   :hpx-api:`hpx::wait_each_nothrow`
   :hpx-api:`hpx::wait_each_n_nothrow`
   =======================================  =====================================

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
   | :hpx-api:`hpx::init`         |
   +------------------------------+
   | :hpx-api:`hpx::start`        |
   +------------------------------+
   | :hpx-api:`hpx::finalize`     |
   +------------------------------+
   | :hpx-api:`hpx::disconnect`   |
   +------------------------------+
   | :hpx-api:`hpx::suspend`      |
   +------------------------------+
   | :hpx-api:`hpx::resume`       |
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
   | :hpx-api:`hpx::call_once`  | :cppreference-generic:`thread,call_once` |
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

   ================================================= ================================================================
   `hpx` function                                    C++ standard
   ================================================= ================================================================
   :hpx-api:`hpx::uninitialized_copy`                :cppreference-generic:`memory,uninitialized_copy`
   :hpx-api:`hpx::uninitialized_copy_n`              :cppreference-generic:`memory,uninitialized_copy_n`
   :hpx-api:`hpx::uninitialized_default_construct`   :cppreference-generic:`memory,uninitialized_default_construct`
   :hpx-api:`hpx::uninitialized_default_construct_n` :cppreference-generic:`memory,uninitialized_default_construct_n`
   :hpx-api:`hpx::uninitialized_fill`                :cppreference-generic:`memory,uninitialized_fill`
   :hpx-api:`hpx::uninitialized_fill_n`              :cppreference-generic:`memory,uninitialized_fill_n`
   :hpx-api:`hpx::uninitialized_move`                :cppreference-generic:`memory,uninitialized_move`
   :hpx-api:`hpx::uninitialized_move_n`              :cppreference-generic:`memory,uninitialized_move_n`
   :hpx-api:`hpx::uninitialized_value_construct`     :cppreference-generic:`memory,uninitialized_value_construct`
   :hpx-api:`hpx::uninitialized_value_construct_n`   :cppreference-generic:`memory,uninitialized_value_construct_n`
   ================================================ ================================================================

.. table:: `hpx::ranges` functions of header ``hpx/memory.hpp``

   ========================================================= =======================================================================
   `hpx::ranges` function                                    C++ standard
   ========================================================= =======================================================================
   :hpx-api:`hpx::ranges::uninitialized_copy`                :cppreference-generic:`memory/ranges,uninitialized_copy`
   :hpx-api:`hpx::ranges::uninitialized_copy_n`              :cppreference-generic:`memory/ranges,uninitialized_copy_n`
   :hpx-api:`hpx::ranges::uninitialized_default_construct`   :cppreference-generic:`memory/ranges,uninitialized_default_construct`
   :hpx-api:`hpx::ranges::uninitialized_default_construct_n` :cppreference-generic:`memory/ranges,uninitialized_default_construct_n`
   :hpx-api:`hpx::ranges::uninitialized_fill`                :cppreference-generic:`memory/ranges,uninitialized_fill`
   :hpx-api:`hpx::ranges::uninitialized_fill_n`              :cppreference-generic:`memory/ranges,uninitialized_fill_n`
   :hpx-api:`hpx::ranges::uninitialized_move`                :cppreference-generic:`memory/ranges,uninitialized_move`
   :hpx-api:`hpx::ranges::uninitialized_move_n`              :cppreference-generic:`memory/ranges,uninitialized_move_n`
   :hpx-api:`hpx::ranges::uninitialized_value_construct`     :cppreference-generic:`memory/ranges,uninitialized_value_construct`
   :hpx-api:`hpx::ranges::uninitialized_value_construct_n`   :cppreference-generic:`memory/ranges,uninitialized_value_construct_n`
   ========================================================= =======================================================================

.. _public_api_header_hpx_numeric:

``hpx/numeric.hpp``
===================

The header :hpx-header:`libs/core/include_local/include,hpx/numeric.hpp` corresponds to the
C++ standard library header :cppreference-header:`numeric`. See :ref:`parallel_algorithms` for more
information about the parallel algorithms.

Functions
---------

.. table:: `hpx` functions of header ``hpx/numeric.hpp``

   ======================================== ==========================================================
   `hpx` function                                    C++ standard
   ======================================== ==========================================================
   :hpx-api:`hpx::adjacent_difference`      :cppreference-generic:`algorithm,adjacent_difference`
   :hpx-api:`hpx::exclusive_scan`           :cppreference-generic:`algorithm,exclusive_scan`
   :hpx-api:`hpx::inclusive_scan`           :cppreference-generic:`algorithm,inclusive_scan`
   :hpx-api:`hpx::reduce`                   :cppreference-generic:`algorithm,reduce`
   :hpx-api:`hpx::transform_exclusive_scan` :cppreference-generic:`algorithm,transform_exclusive_scan`
   :hpx-api:`hpx::transform_inclusive_scan` :cppreference-generic:`algorithm,transform_inclusive_scan`
   :hpx-api:`hpx::transform_reduce`         :cppreference-generic:`algorithm,transform_reduce`
   ======================================== ==========================================================

.. table:: `hpx::ranges` functions of header ``hpx/numeric.hpp``

   +--------------------------------------------------+
   | `hpx::ranges` function                           |
   +==================================================+
   | :hpx-api:`hpx::ranges::adjacent_difference`      |
   +--------------------------------------------------+
   | :hpx-api:`hpx::ranges::exclusive_scan`           |
   +--------------------------------------------------+
   | :hpx-api:`hpx::ranges::inclusive_scan`           |
   +--------------------------------------------------+
   | :hpx-api:`hpx::ranges::reduce`                   |
   +--------------------------------------------------+
   | :hpx-api:`hpx::ranges::transform_exclusive_scan` |
   +--------------------------------------------------+
   | :hpx-api:`hpx::ranges::transform_inclusive_scan` |
   +--------------------------------------------------+
   | :hpx-api:`hpx::ranges::transform_reduce`         |
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
   | :hpx-api:`hpx::find_root_locality`                         |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::find_all_localities`                        |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::find_remote_localities`                     |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::find_locality`                              |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::get_colocation_id`                          |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::get_locality_id`                            |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::get_num_worker_threads`                     |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::get_worker_thread_num`                      |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::get_thread_name`                            |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::register_pre_startup_function`              |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::register_startup_function`                  |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::register_pre_shutdown_function`             |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::register_shutdown_function`                 |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::get_num_localities`                         |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::get_locality_name`                          |
   +------------------------------------------------------------+
   | :hpx-api:`hpx::local::termination_detection`               |
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
   | :hpx-api:`hpx::experimental::define_task_block`                 |
   +-----------------------------------------------------------------+
   | :hpx-api:`hpx::experimental::define_task_block_restore_thread`  |
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

   ========================================  ==========================================
   Function                                    C++ standard
   ========================================  ==========================================
   :hpx-api:`hpx::this_thread::yield`        :cppreference-generic:`thread,yield`
   :hpx-api:`hpx::this_thread::get_id`       :cppreference-generic:`thread,get_id`
   :hpx-api:`hpx::this_thread::sleep_for`    :cppreference-generic:`thread,sleep_for`
   :hpx-api:`hpx::this_thread::sleep_until`  :cppreference-generic:`thread,sleep_until`
   ========================================  ==========================================

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

   ================================  ======================================================
   Function                          C++ standard
   ================================  ======================================================
   :hpx-api:`hpx::make_tuple`        :cppreference-generic:`utility/tuple,tuple_element`
   :hpx-api:`hpx::tie`               :cppreference-generic:`utility/tuple,tie`
   :hpx-api:`hpx::forward_as_tuple`  :cppreference-generic:`utility/tuple,forward_as_tuple`
   :hpx-api:`hpx::tuple_cat`         :cppreference-generic:`utility/tuple,tuple_cat`
   :hpx-api:`hpx::get`               :cppreference-generic:`utility/tuple,get`
   ================================  ======================================================

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
   | :hpx-api:`hpx::unwrap`           |
   +----------------------------------+
   | :hpx-api:`hpx::unwrap_n`         |
   +----------------------------------+
   | :hpx-api:`hpx::unwrap_all`       |
   +----------------------------------+
   | :hpx-api:`hpx::unwrapping`       |
   +----------------------------------+
   | :hpx-api:`hpx::unwrapping_n`     |
   +----------------------------------+
   | :hpx-api:`hpx::unwrapping_all`   |
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
   | :hpx-api:`hpx::major_version`           |
   +-----------------------------------------+
   | :hpx-api:`hpx::minor_version`           |
   +-----------------------------------------+
   | :hpx-api:`hpx::subminor_version`        |
   +-----------------------------------------+
   | :hpx-api:`hpx::full_version`            |
   +-----------------------------------------+
   | :hpx-api:`hpx::full_version_as_string`  |
   +-----------------------------------------+
   | :hpx-api:`hpx::tag`                     |
   +-----------------------------------------+
   | :hpx-api:`hpx::agas_version`            |
   +-----------------------------------------+
   | :hpx-api:`hpx::build_type`              |
   +-----------------------------------------+
   | :hpx-api:`hpx::build_date_time`         |
   +-----------------------------------------+

.. _public_api_header_hpx_wrap_main:

``hpx/wrap_main.hpp``
=====================

The header :hpx-header:`wrap/include,hpx/wrap_main.hpp` does not provide any direct functionality
but is used for implicitly using ``main`` as the runtime entry point. See :ref:`minimal` for more
details on implicitly starting the |hpx| runtime.
