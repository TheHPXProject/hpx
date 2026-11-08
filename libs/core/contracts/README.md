<!--
    Copyright (c) 2025 The STE||AR-Group
    Copyright (c) 2025 Alexandros Papadakis

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
-->

# HPX Contracts Module

A forward-compatible C++ contracts implementation for HPX with intelligent fallback to assertions.

## Quick Start

```cpp
#include <hpx/contracts.hpp>

// Preconditions and postconditions with return value access
int factorial(int n) HPX_PRE(n >= 0) HPX_POST(r: r > 0) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// Contract assertions (always available)
void process(std::vector<int>& data, size_t index) {
    HPX_CONTRACT_ASSERT(index < data.size());
    data[index] *= 2;
}
```

## Build Configuration

Contracts are disabled by default (`HPX_WITH_CONTRACTS=OFF`):

- With `HPX_WITH_CONTRACTS=OFF`, `HPX_PRE` and `HPX_POST` are no-ops and
  `HPX_CONTRACT_ASSERT` falls back to `HPX_ASSERT`, even if the compiler
  supports native C++26 contracts.
- With `HPX_WITH_CONTRACTS=ON` and native C++26 contracts available, the macros
  map to the standard contract syntax.
- With `HPX_WITH_CONTRACTS=ON` and no native contracts, `HPX_PRE` and
  `HPX_POST` are no-ops and `HPX_CONTRACT_ASSERT` is checked according to
  `HPX_WITH_CONTRACTS_MODE`.

```bash
# Enable contracts
cmake -DHPX_WITH_CONTRACTS=ON -DHPX_WITH_CXX_STANDARD=26

# Enable contract-enhanced assertions (optional)
cmake -DHPX_WITH_CONTRACTS=ON -DHPX_WITH_ASSERTS_AS_CONTRACT_ASSERTS=ON

# Select the evaluation mode used without native contracts
cmake -DHPX_WITH_CONTRACTS=ON -DHPX_WITH_CONTRACTS_MODE=ENFORCE|QUICK_ENFORCE|OBSERVE|IGNORE

# Contracts disabled (HPX_CONTRACT_ASSERT works as HPX_ASSERT)
cmake -DHPX_WITH_CONTRACTS=OFF # default
```

## Advanced Features

### Contract-Enhanced Assertions
When `HPX_WITH_ASSERTS_AS_CONTRACT_ASSERTS=ON`, regular `HPX_ASSERT` calls are automatically upgraded to use contract assertions:

```cpp
void process_data(std::vector<int>& data, size_t index) {
    HPX_ASSERT(index < data.size());  // Becomes HPX_CONTRACT_ASSERT() -> adapts to current mode
    data[index] *= 2;
}
```

**Important**: Enhanced assertions only provide benefits when native C++26 contracts are supported by the compiler. Without native contract support, `HPX_ASSERT` → `HPX_CONTRACT_ASSERT` → `HPX_ASSERT` (no enhancement). CMake will warn you if you enable this option without native contract support.

The implementation works by overriding the `HPX_ASSERT` macro in `contracts.hpp` to use `HPX_CONTRACT_ASSERT`, which automatically adapts to the current contract mode (native C++26 contracts when available, or assertion fallback otherwise).

## API Reference

- **`HPX_PRE(condition)`**: Precondition contracts
- **`HPX_POST(condition)`**: Postcondition contracts
- **`HPX_CONTRACT_ASSERT(condition)`**: Contract assertions (always available)

## Documentation

See [Module Documentation](docs/index.rst) for comprehensive usage guide, API reference, and implementation details.

## Features

- ✅ Automatic C++26 native contract detection
- ✅ Graceful fallback: HPX_PRE/HPX_POST become no-ops, HPX_CONTRACT_ASSERT maps to HPX_ASSERT
- ✅ Zero overhead when disabled
- ✅ Forward-compatible API
- ✅ Comprehensive test suite with automatic mode detection