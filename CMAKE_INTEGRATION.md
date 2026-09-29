# CMake Integration Guide

## Overview

This guide explains how to integrate the APS6404L PSRAM C++ driver into your STM32 project using CMake.

---

## Quick Start

### Option 1: Submodule Integration (Recommended)

1. **Add as Git Submodule**
```bash
cd your_project
git submodule add https://github.com/yourusername/APS6404L_STM32_DRIVER.git drivers/psram
```

2. **Update your CMakeLists.txt**
```cmake
# In your project CMakeLists.txt
add_subdirectory(drivers/psram)
target_link_libraries(your_target PRIVATE psram_aps6404l)
```

### Option 2: Copy Source Files

1. **Copy the driver files**
```bash
cp -r APS6404L_STM32_DRIVER/Core/Inc drivers/psram/
cp -r APS6404L_STM32_DRIVER/Core/Src/aps6404l.cpp drivers/psram/
```

2. **Add to your project**
```cmake
# In your CMakeLists.txt
add_library(psram_aps6404l STATIC
    drivers/psram/aps6404l.cpp
)
target_include_directories(psram_aps6404l PUBLIC drivers/psram)
```

---

## Basic Integration

### Project Structure

```
your_project/
├── CMakeLists.txt
├── Core/
│   ├── Inc/
│   └── Src/
│       ├── main.c
│       └── ...
└── drivers/
    └── psram/                # APS6404L driver
        ├── CMakeLists.txt
        ├── Core/
        │   ├── Inc/
        │   └── Src/
        │       └── aps6404l.cpp
        └── README.md
```

### Parent Project CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_stm32_project)

# Add the PSRAM driver
add_subdirectory(drivers/psram)

# Your application
add_executable(my_app
    Core/Src/main.c
    # ... other source files
)

# Link with PSRAM driver
target_link_libraries(my_app
    PRIVATE
        psram_aps6404l
        STM32HAL  # Your HAL library target
)

# Required compiler flags
target_compile_options(my_app
    PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
        -std=c++17
)
```

### Application Code

#### Using the C++ Class

```cpp
#include "aps6404l.hpp"
#include <array>

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_QUADSPI1_Init();

    // QSPI and DMA handles
    extern QSPI_HandleTypeDef hqspi1;
    extern DMA_HandleTypeDef hdma_quadspi;

    // Create PSRAM instance
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);

    // Initialize PSRAM
    auto result = psram.psram_reset();
    if (!result) {
        // Handle error
        // Error codes: psram::ErrorCode::HALError, Timeout, etc.
    }

    psram.psram_enter_quad_mode();
    psram.psram_set_wrap_mode();

    // Use PSRAM with automatic error handling
    std::array<uint8_t, 256> data;
    psram.psram_fast_write(0x000000, data.data(), data.size());
    psram.psram_fast_read(0x000000, data.data(), data.size());

    // DMA operations
    psram.psram_read_dma(0x000000, data.data(), data.size());
    psram.psram_write_dma(0x000000, data.data(), data.size());

    while (1) {
        // Your application code
    }
}
```

#### Using the Buffer Class

```cpp
#include "aps6404l.hpp"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_QUADSPI1_Init();

    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);

    // Create DMA-aligned buffer (auto-aligned 32-byte)
    psram::Buffer buffer(4096);
    buffer.psram_fill(0xAA);

    // DMA operations with aligned buffer
    psram.psram_write_dma(0x000000, buffer.psram_data(), buffer.psram_size());

    // Wait for completion
    while (!dma_complete) {}

    psram.psram_read_dma(0x000000, buffer.psram_data(), buffer.psram_size());
    while (!dma_complete) {}

    // Use buffer.psram_data() to access read data
}
```
```

---

## Advanced Integration

### Custom Configuration

```cmake
# In parent CMakeLists.txt, before add_subdirectory
set(PSRAM_ENABLE_LOGGING ON CACHE BOOL "" FORCE)
set(PSRAM_ENABLE_ASSERTS ON CACHE BOOL "" FORCE)

add_subdirectory(drivers/psram)
```

### Custom Timeout

```cmake
# Set custom timeout (default: 1000 ms)
target_compile_definitions(psram_aps6404l
    PUBLIC
        APS6404L_TIMEOUT_MS=5000
)
```

---

## Configuration Options

### Compile Definitions

| Definition | Default | Description |
|------------|---------|-------------|
| `APS6404L_SIZE_BYTES` | 8388608 | PSRAM size in bytes (8 MB) |
| `APS6404L_ADDRESS_WIDTH` | 24 | Address width in bits |
| `APS6404L_DUMMY_CYCLES_STANDARD` | 4 | Dummy cycles for standard read |
| `APS6404L_DUMMY_CYCLES_FAST` | 6 | Dummy cycles for fast read |
| `APS6404L_WRAP_SIZE` | 32 | Wrap burst size in bytes |
| `APS6404L_TIMEOUT_MS` | 1000 | Command timeout in milliseconds |

### Optional Features

| Feature | Definition | Default | Description |
|---------|------------|---------|-------------|
| Logging | `PSRAM_ENABLE_LOGGING` | OFF | Enable debug logging |
| Assertions | `PSRAM_ENABLE_ASSERTS` | OFF | Enable debug assertions |

---

## C++ API Reference

### PSRAMDriver Class

```cpp
#include "aps6404l.hpp"

// Create PSRAM instance
extern QSPI_HandleTypeDef hqspi1;
extern DMA_HandleTypeDef hdma_quadspi;
psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);

// Methods
psram.psram_reset();                    // Reset PSRAM
psram.psram_enter_quad_mode();          // Enter QPI mode
psram.psram_set_wrap_mode();            // Enable wrap mode
psram.psram_read(addr, data, size);     // Blocking read
psram.psram_write(addr, data, size);    // Blocking write
psram.psram_fast_read(addr, data, size); // Fast read
psram.psram_fast_write(addr, data, size); // Fast write
psram.psram_read_dma(addr, data, size);  // DMA read
psram.psram_write_dma(addr, data, size); // DMA write

// State methods
bool initialized = psram.psram_is_initialized();
bool has_dma = psram.psram_has_dma();

// Static methods
size_t size = psram::PSRAMDriver::psram_get_size();           // 8388608
uint8_t width = psram::PSRAMDriver::psram_get_address_width(); // 24
uint32_t max_addr = psram::PSRAMDriver::psram_get_max_address(); // 8388607
```

### Buffer Class

```cpp
// Create aligned buffer (32-byte default for DMA)
psram::Buffer buffer(4096);

// Access data
uint8_t* data = buffer.psram_data();
const uint8_t* data = buffer.psram_data();
size_t size = buffer.psram_size();
size_t align = buffer.psram_alignment();

// Fill/clear
buffer.psram_clear();  // Zero fill
buffer.psram_fill(0xFF); // Fill with pattern
```

---

## Standalone Build

To build the driver standalone for testing:

```bash
cd standalone
mkdir build && cd build
cmake .. -DSTM32_DEVICE=STM32G474 -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers
make
```

To build with tests enabled:

```bash
cmake .. -DSTM32_DEVICE=STM32G474 -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers -DPSRAM_BUILD_TESTS=ON
make psramp_test
```

---

## Troubleshooting

### Issue 1: "Undefined reference to hqspi1"

**Solution**: Ensure `hqspi1` is declared in your main.c:
```c
QSPI_HandleTypeDef hqspi1;
```

### Issue 2: "Undefined reference to hdma_quadspi"

**Solution**: Ensure `hdma_quadspi` is declared in your main.c:
```c
DMA_HandleTypeDef hdma_quadspi;
```

### Issue 3: "QSPI initialization failed"

**Solution**: Verify QSPI pins are configured correctly in your HAL setup:
- PB10 → CLK
- PC1 → IO0
- PC2 → IO1
- PC3 → IO2
- PC4 → IO3
- PD3 → NCS

### Issue 4: "DMA not working"

**Solution**: 
1. Verify DMA channel is configured in `MX_DMA_Init()`
2. Ensure buffer is 32-byte aligned: `__attribute__((aligned(32)))`
3. Check DMA interrupt is enabled

---

## Example Projects

See the following files for complete examples:

- **[EXAMPLE_CMAKELISTS.txt](EXAMPLE_CMAKELISTS.txt)** - Complete parent project example
- **[standalone/CMakeLists.txt](standalone/CMakeLists.txt)** - Standalone build configuration

---

## API Reference

For detailed API documentation, see **[PSRAM_API_REFERENCE.md](PSRAM_API_REFERENCE.md)**

---

## Support

If you encounter issues:

1. Check this guide first
2. Review **[QUICK_START.md](QUICK_START.md)** for hardware setup
3. Check **[TODO.md](TODO.md)** for known limitations
4. Review the example CMakeLists.txt files

---

## License

This driver is provided as-is for use with STM32 microcontrollers.

---

**Happy Coding! 🚀**
