..
    Copyright (c) 2025 The STE||AR-Group
    Copyright (c) 2025 Alexandros Papadakis

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_contracts:

=========
contracts
=========

The contracts module provides C++ contracts support for HPX with intelligent 
fallback to assertions when native contracts are not available. This module 
implements a forward-compatible API that works across different C++ standards 
and compiler capabilities.

The module provides three primary macros: :c:macro:`HPX_PRE`, 
:c:macro:`HPX_POST`, and :c:macro:`HPX_CONTRACT_ASSERT`. These macros 
automatically adapt their behavior based on compiler capabilities:

* When ``HPX_WITH_CONTRACTS=OFF`` (the default), preconditions and
  postconditions become no-ops and contract assertions fall back to
  :c:macro:`HPX_ASSERT`, independently of whether the compiler supports
  native C++26 contracts
* When ``HPX_WITH_CONTRACTS=ON`` and C++26 native contracts are available
  (``__cpp_contracts`` defined), they map to standard contract syntax
* When ``HPX_WITH_CONTRACTS=ON`` and native contracts are not available,
  preconditions and postconditions become no-ops while contract assertions
  are checked according to ``HPX_WITH_CONTRACTS_MODE``

Configuration
=============

Enable contracts in CMake::

    cmake -DHPX_WITH_CONTRACTS=ON -DCMAKE_CXX_STANDARD=26

Enable contract-enhanced assertions (optional, native contracts only)::

    cmake -DHPX_WITH_CONTRACTS=ON -DHPX_CONTRACTS_WITH_ASSERTS_AS_CONTRACT_ASSERTS=ON

Select the evaluation mode used when native contracts are not available::

    cmake -DHPX_WITH_CONTRACTS=ON -DHPX_WITH_CONTRACTS_MODE=ENFORCE|QUICK_ENFORCE|OBSERVE|IGNORE

Contract assertions work even when contracts are disabled::

    cmake -DHPX_WITH_CONTRACTS=OFF  # HPX_CONTRACT_ASSERT maps to HPX_ASSERT

Advanced Features
=================

Contract-Enhanced Assertions
----------------------------

When ``HPX_CONTRACTS_WITH_ASSERTS_AS_CONTRACT_ASSERTS=ON`` is enabled, regular 
:c:macro:`HPX_ASSERT` calls are automatically upgraded to use contract 
assertions in C++26 mode::

    void example_function(int value)
    {
        HPX_ASSERT(value > 0);  // Becomes contract_assert(value > 0) in C++26 mode
        // ... rest of function
    }

.. warning::
   Enhanced assertions only provide benefits when native C++26 contracts are 
   supported by the compiler. Without native contract support, 
   ``HPX_ASSERT`` -> ``HPX_CONTRACT_ASSERT`` -> ``HPX_ASSERT`` (no enhancement).
   CMake will issue a warning if you enable this option without native contract support.

This provides enhanced contract semantics throughout your existing codebase 
without requiring changes to assertion code. The transformation occurs in the
contracts module (``contracts.hpp``) where the ``HPX_ASSERT`` macro is 
overridden to use ``HPX_CONTRACT_ASSERT`` when ``HPX_WITH_ASSERTS_AS_CONTRACT_ASSERTS=ON``:

* ``HPX_WITH_CONTRACTS=ON`` and native contracts available - Contracts are enabled
* ``HPX_CONTRACTS_WITH_ASSERTS_AS_CONTRACT_ASSERTS=ON`` - Assertion enhancement is enabled  

The implementation works by redefining ``HPX_ASSERT`` in ``contracts.hpp`` to 
use ``HPX_CONTRACT_ASSERT``, which automatically adapts to the current contract 
mode (native C++26 contracts or assertion fallback). This ensures all existing 
``HPX_ASSERT`` calls throughout the HPX codebase automatically gain contract 
semantics when available.

Usage Examples
==============

Preconditions and postconditions using declaration syntax (C++26)::

    int divide(int a, int b) HPX_PRE(b != 0)
    {
        return a / b;
    }
    
    int factorial(int n) HPX_PRE(n >= 0) HPX_POST(r; r > 0)
    {
        return n <= 1 ? 1 : n * factorial(n - 1);
    }

Contract assertions (available in all modes)::

    void process_array(std::vector<int>& arr, size_t index)
    {
        HPX_CONTRACT_ASSERT(index < arr.size());
        arr[index] *= 2;
    }

Design Philosophy
=================

**HPX_CONTRACT_ASSERT**: Enhanced assertion mechanism
    Available even when ``HPX_WITH_CONTRACTS=OFF`` because it provides value
    as an enhanced assertion. Maps to :c:macro:`HPX_ASSERT` when contracts are
    disabled (or ``HPX_WITH_CONTRACTS_MODE=IGNORE``).

**HPX_PRE/HPX_POST**: True contract syntax
    Represent language-level contract semantics. When contracts are enabled but 
    native C++26 contracts are not available, these become no-ops to maintain 
    forward compatibility. When ``HPX_WITH_CONTRACTS=OFF``, they are also no-ops.
    This prepares for C++26 migration where they will be attached to function 
    declarations rather than used in function bodies.

Migration Strategy
==================

The module is designed for smooth migration to C++26 native contracts:

Current (transition mode)::

    int func(int x)
    {
        HPX_PRE(x > 0);    // No-op in fallback mode, active in native mode
        return x;
    }

Target (C++26 native)::

    int func(int x) HPX_PRE(x > 0)
    {
        return x;
    }

Note: In fallback mode, ``HPX_PRE`` and ``HPX_POST`` become no-ops to maintain 
forward compatibility and avoid performance overhead. Use ``HPX_CONTRACT_ASSERT`` 
when you need runtime validation in all modes.

Testing
=======

The module includes comprehensive testing with automatic compiler capability 
detection. Tests are organized into three categories:

* **Declaration tests**: Test C++26 native contract syntax when ``__cpp_contracts`` is available
* **Fallback tests**: Test assertion fallback behavior when contracts are not natively supported  
* **Disabled tests**: Test no-op behavior when contracts are disabled

The test suite automatically detects compiler capabilities at configure time 
and builds only the appropriate tests for the current configuration.

See the :ref:`API reference <modules_contracts_api>` of the module for more details.
