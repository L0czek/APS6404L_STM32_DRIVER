# APS6404L PSRAM Driver

QSPI driver for the AP Memory APS6404L 8MB PSRAM for STM32 microcontrollers.

[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C++17-blue.svg)](https://en.cppreference.com/w/cpp/17)

## Features

- 8MB capacity with Quad-SPI interface
- Fast read/write with configurable dummy cycles
- DMA support for high-bandwidth transfers
- Wrap burst mode for contiguous data
- Hardware reset functionality
- DMA-aligned buffer management

## Documentation

- [Library Documentation](library/README.md) - Integration and API reference
- [Header File](Core/Inc/aps6404l.hpp) - Complete API reference

## Directory Structure

```
APS6404L_STM32_DRIVER/
├── library/              # CMake submodule integration
│   ├── CMakeLists.txt
│   └── README.md
├── Core/
│   ├── Inc/             # Public headers
│   │   └── aps6404l.hpp
│   └── Src/             # Implementation
│       ├── aps6404l.cpp
│       └── aps6404l_expected.hpp
├── examples/            # Example code
├── tests/               # Unit tests
└── README.md
```

## Quick Start

```cpp
#include "aps6404l.hpp"

psram::PSRAMDriver psram(&hqspi1);
psram.psram_reset();
psram.psram_enter_quad_mode();
psram.psram_write(0x000000, data, size);
```

## Building

```bash
mkdir build && cd build
cmake ..
make
make test
```

## License

MIT License
