# APS6404L PSRAM Driver

QSPI driver for the AP Memory APS6404L 8MB PSRAM for STM32 microcontrollers.

[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C++17-blue.svg)](https://en.cppreference.com/w/cpp/17)

## Features

- **8MB Capacity**: 8,388,608 bytes of PSRAM memory
- **QPI Interface**: Quad SPI with 4 data lines for high throughput
- **Fast Read/Write**: Standard and fast modes with configurable dummy cycles
- **DMA Support**: Optional DMA for high-bandwidth transfers
- **Wrap Mode**: Burst transfer support with configurable wrap size (32 bytes)
- **Reset Support**: Hardware reset functionality
- **DMA-Aligned Buffers**: RAII wrapper for DMA-compatible memory

## Integration as Submodule

Add this driver as a Git submodule to your project:

```bash
git submodule add https://github.com/yourorg/APS6404L_STM32_DRIVER.git vendor/APS6404L
```

In your project's `CMakeLists.txt`:

```cmake
# Add the driver as a subdirectory
add_subdirectory(vendor/APS6404L/library)

# Link the library to your target
target_link_libraries(your_target PRIVATE psram_aps6404l)
```

## Requirements

- C++17 compiler (for `std::expected`)
- STM32 HAL library (must be defined in parent project as `stm32_hal` target)
- QSPI peripheral support
- Optional: DMA peripheral support for high-speed transfers

## Quick Start

```cpp
#include "aps6404l.hpp"

// Initialize driver with QSPI handle
psram::PSRAMDriver psram(&hqspi1);

// Reset PSRAM
psram.psram_reset();

// Enter quad mode for QPI
psram.psram_enter_quad_mode();

// Enable wrap mode for burst transfers
psram.psram_set_wrap_mode();

// Write data
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
psram.psram_write(0x000000, data, sizeof(data));

// Read data back
uint8_t buffer[4];
psram.psram_read(0x000000, buffer, sizeof(buffer));
```

## API Reference

### Device Initialization

| Method | Description |
|--------|-------------|
| `psram_reset()` | Reset PSRAM device |
| `psram_enter_quad_mode()` | Enter QPI (quad) mode |
| `psram_set_wrap_mode()` | Enable wrap burst mode |
| `psram_is_initialized()` | Check if driver is initialized |

### Memory Operations (Blocking)

| Method | Description |
|--------|-------------|
| `psram_read(address, data, size)` | Read data from PSRAM |
| `psram_write(address, data, size)` | Write data to PSRAM |
| `psram_fast_read(address, data, size)` | Fast read with dummy cycles |
| `psram_fast_write(address, data, size)` | Fast write |

### Memory Operations (DMA)

| Method | Description |
|--------|-------------|
| `psram_read_dma(address, data, size)` | Read using DMA |
| `psram_write_dma(address, data, size)` | Write using DMA |
| `psram_has_dma()` | Check if DMA is available |

### Configuration

| Method | Description |
|--------|-------------|
| `psram_set_clock_prescaler(prescaler)` | Set QSPI clock prescaler |

### Static Configuration

| Method | Description |
|--------|-------------|
| `psram_get_size()` | Total capacity (8MB) |
| `psram_get_address_width()` | Address width (24 bits) |
| `psram_get_max_address()` | Maximum valid address |
| `psram_get_dummy_cycles_standard()` | Dummy cycles for standard read |
| `psram_get_dummy_cycles_fast()` | Dummy cycles for fast read |
| `psram_get_wrap_size()` | Wrap burst size |

## Complete Example

```cpp
#include "aps6404l.hpp"
#include <cstring>

int main() {
    // Initialize PSRAM driver
    psram::PSRAMDriver psram(&hqspi1);
    
    // Reset and configure PSRAM
    psram.psram_reset();
    psram.psram_enter_quad_mode();
    psram.psram_set_wrap_mode();
    
    // Write some data
    const uint8_t write_data[] = "Hello PSRAM!";
    psram.psram_write(0x000000, write_data, strlen((char*)write_data));
    
    // Read back the data
    char buffer[64];
    psram.psram_read(0x000000, reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
    
    // Use DMA for large transfers (32-byte aligned buffer required)
    psram::Buffer dma_buffer(1024);
    psram.psram_read_dma(0x100000, dma_buffer.psram_data(), 1024);
    
    return 0;
}
```

## Buffer Class

The `Buffer` class provides RAII-managed, DMA-aligned memory:

```cpp
// Create a 32-byte aligned buffer
psram::Buffer buffer(1024);  // 1KB

// Fill with data
buffer.psram_fill(0xFF);

// Access data pointer
uint8_t* data = buffer.psram_data();
```

## Error Handling

```cpp
auto result = psram.psram_write(0x000000, data, size);
if (!result.has_value()) {
    std::error_code ec = result.error();
    
    if (ec.category() == psram::get_error_category()) {
        switch (static_cast<psram::ErrorCode>(ec.value())) {
            case psram::ErrorCode::InvalidAddress:
                // Handle invalid address
                break;
            case psram::ErrorCode::Timeout:
                // Handle timeout
                break;
            case psram::ErrorCode::QSPIError:
                // Handle QSPI error
                break;
        }
    }
}
```

## Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `APS6404L_SIZE_BYTES` | 8388608 | Total capacity in bytes |
| `APS6404L_ADDRESS_WIDTH` | 24 | Address width in bits |
| `APS6404L_DUMMY_CYCLES_STANDARD` | 4 | Dummy cycles for standard read |
| `APS6404L_DUMMY_CYCLES_FAST` | 6 | Dummy cycles for fast read |
| `APS6404L_WRAP_SIZE` | 32 | Wrap burst size in bytes |
| `APS6404L_TIMEOUT_MS` | 1000 | Command timeout in ms |

## Wiring (Typical STM32 Connection)

| PSRAM Pin | STM32 QSPI Pin | Description |
|-----------|----------------|-------------|
| D0 | IO0 | Data 0 |
| D1 | IO1 | Data 1 |
| D2 | IO2 | Data 2 |
| D3 | IO3 | Data 3 |
| CLK | CLK | Clock |
| CS | NCS | Chip select |
| RESET | GPIO | Reset (optional) |

## License

MIT License - see LICENSE file for details
