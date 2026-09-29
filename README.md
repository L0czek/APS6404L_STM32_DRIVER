# APS6404L_STM32_DRIVER

This repository demonstrates how to interface the **AP Memory APS6404L (8MB PSRAM)** with an **STM32G474** using **Quad-SPI (QSPI) with DMA**, achieving high-speed memory transfers. The STM32G474 has **only 128 KB of internal RAM**, but this **external PSRAM significantly expands memory capacity**, making it suitable for applications like **continuous data acquisition**.

## 🛠 Hardware Details
- **Microcontroller:** STM32G474 (128 KB SRAM)
- **PSRAM:** APS6404L (8 MB, Quad-SPI)
- **Board:** Custom PCB (signal integrity issues above 25 MHz)
- **QSPI Clock:** Limited to **~24.3 MHz (Prescaler 6)**
- **Mode:** **Quad-SPI (QPI) in wrap mode (32-byte bursts)**
- **Transfer Type:** Blocking & DMA-based QSPI transfers

## 🚀 Achieved Speeds

| Transfer Mode  | Write Speed | Read Speed |
|---------------|------------|------------|
| **Blocking (CPU)**  | **1.14 MB/s** | **1.33 MB/s** |
| **DMA (Peripheral)** | **10.00 MB/s** | **10.00 MB/s** |

⚡ **Using DMA for both read & write boosted speeds to 10 MB/s!**
⚠ **Above 25 MHz, signal integrity degrades on the custom board, limiting further improvements.**

## 🛠 Features
- **QSPI initialization, wrap mode setup (0x5F)**
- **Fast read/write (0xEB / 0x38)**
- **DMA-based QSPI transfers**
- **Speed tests & data integrity checks**
- **Minimal setup required—just connect APS6404L to STM32G4's QSPI pins!**

## 📂 Code Overview
- **`Core/`** → Driver source files
- **`standalone/`** → Standalone build with unit tests
- **`tests/`** → Unit tests with mock HAL

## 🏁 Quick Start - CMake Build

```bash
# Clone and setup
cd /path/to/APS6404L_STM32_DRIVER

# Configure with CMake
mkdir build && cd build
cmake .. -DSTM32_DEVICE=STM32G474 \
         -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers \
         -DPSRAM_BUILD_UNIT_TESTS=ON

# Build and run tests
make test_aps6404l
ctest -V
```

### Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `PSRAM_ENABLE_LOGGING` | OFF | Enable verbose logging |
| `PSRAM_ENABLE_ASSERTS` | OFF | Enable debug assertions |
| `PSRAM_BUILD_TESTS` | OFF | Build STM32 test app |
| `PSRAM_BUILD_UNIT_TESTS` | ON | Build host unit tests |
| `PSRAM_INSTALL` | OFF | Install target |

## 📖 Integration Guide
