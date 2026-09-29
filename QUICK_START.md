# Quick Start Guide

## Overview

This guide helps you get started with the APS6404L PSRAM driver on STM32G474.

---

## Prerequisites

### Hardware
- STM32G474VE board (or compatible)
- APS6404L PSRAM module
- ST-Link programmer/debugger
- USB cable for programming

### Software
- STM32CubeIDE 1.13.x or later
- STM32CubeMX (for configuration)
- STM32CubeG4 package

### Pin Connections

| PSRAM Pin | STM32 Pin | Description |
|-----------|-----------|-------------|
| VCC | 3.3V | Power (3.3V) |
| GND | GND | Ground |
| CLK | PB10 | QSPI Clock |
| IO0 | PC1 | QSPI Data 0 |
| IO1 | PC2 | QSPI Data 1 |
| IO2 | PC3 | QSPI Data 2 |
| IO3 | PC4 | QSPI Data 3 |
| NC | PD3 | QSPI Chip Select (NCS) |

**Note**: Ensure 3.3V power and proper grounding.

---

## Step-by-Step Setup

### 1. Open Project in STM32CubeIDE

1. Launch STM32CubeIDE
2. Select **File > Open Projects from File System**
3. Navigate to this directory and select `Second.ioc`
4. Click **Finish**

### 2. Verify Configuration

Open `Second.ioc` in STM32CubeMX and verify:

#### QSPI Configuration
```
Quad SPI:
  - Mode: Full duplex
  - Clock Prescaler: 6
  - FIFO Threshold: 1
  - Sample Shifting: None
  - Flash Size: 23 (2^24 bytes)
  - Chip Select High Time: 1 Cycle
  - Clock Mode: Mode 0
```

#### Pin Configuration
```
PB10: QUADSPI_CLK
PC1:  QUADSPI_BK2_IO0
PC2:  QUADSPI_BK2_IO1
PC3:  QUADSPI_BK2_IO2
PC4:  QUADSPI_BK2_IO3
PD3:  QUADSPI_BK2_NCS
```

#### Clock Configuration
```
System Clock Source: PLL
PLL Source: HSE (16 MHz)
PLL M: 4
PLL N: 85
PLL P: 2 (SYSCLK = 170 MHz)
QSPI Clock: SYSCLK
```

### 3. Build the Project

1. Right-click on project in Project Explorer
2. Select **Build Project**
3. Verify no errors

### 4. Program the Device

1. Connect ST-Link to board
2. Click **Run** (green play button) or **Debug**
3. Project will flash and start automatically

### 5. Monitor Output

Use STM32CubeIDE's terminal or printf ITM:
- Open **Window > Show View > Terminal**
- Or use **SWO** viewer for ITM output

Expected output:
```
DMA Fast Write throughput: 10000.00 kB/s, elapsed time: 5 ms
DMA Fast Read throughput: 10000.00 kB/s, elapsed time: 5 ms
DMA Fast Write/Read test: PASS
```

---

## Quick Test Without Hardware

If you don't have the PSRAM hardware yet, you can test the initialization sequence:

### Comment out PSRAM initialization in main.c:

```c
/* USER CODE BEGIN 2 */
// Comment out these lines for testing:
// if (PSRAM_Reset() != HAL_OK) { ... }
// if (PSRAM_EnterQuadMode() != HAL_OK) { ... }
// if (PSRAM_SetWrapMode() != HAL_OK) { ... }

// Keep the speed test but it will fail:
// ... (rest of main)
```

### Or test without PSRAM:

```c
// Test only HAL initialization
MX_GPIO_Init();
MX_DMA_Init();
MX_QUADSPI1_Init();

// Toggle LED to verify code runs
while (1) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(500);
}
```

---

## Common Issues

### Issue 1: PSRAM Not Detected

**Symptoms**: Reset command returns timeout

**Solution**:
1. Verify all pins connected correctly
2. Check 3.3V power supply
3. Verify GND connection
4. Check for short circuits

### Issue 2: Data Corruption

**Symptoms**: Read data doesn't match written data

**Solution**:
1. Enable wrap mode: `PSRAM_SetWrapMode()`
2. Reduce clock speed (increase prescaler)
3. Check signal integrity with oscilloscope

### Issue 3: Build Errors

**Symptoms**: "undefined reference" errors

**Solution**:
1. Clean project: **Project > Clean**
2. Rebuild: **Project > Build All**
3. Check `psram_aps6404l.c` is in build sources

### Issue 4: DMA Not Working

**Symptoms**: DMA transfer returns BUSY

**Solution**:
1. Verify DMA channel enabled in `MX_DMA_Init()`
2. Check `DMA2_Channel1_IRQn` interrupt enabled
3. Ensure buffer is aligned: `__attribute__((aligned(32)))`

---

## First Application

### Blink LED + PSRAM Test

```c
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_QUADSPI1_Init();

  // Initialize PSRAM
  PSRAM_Reset();
  PSRAM_EnterQuadMode();
  PSRAM_SetWrapMode();

  while (1)
  {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(1000);

    // Write test
    uint8_t test[] = "Hello PSRAM";
    PSRAM_QPI_FastWrite(0x000000, test, sizeof(test));

    // Read test
    uint8_t read_buf[16];
    PSRAM_QPI_FastRead(0x000000, read_buf, sizeof(test));

    // Check success
    if (memcmp(test, read_buf, sizeof(test)) == 0) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); // Blink twice on success
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
  }
}
```

---

## Next Steps

1. **Read Full Documentation**: See [PROJECT_DOCUMENTATION.md](PROJECT_DOCUMENTATION.md)
2. **API Reference**: See [PSRAM_API_REFERENCE.md](PSRAM_API_REFERENCE.md)
3. **Modify for Your Application**:
   - Adjust transfer sizes
   - Add your application logic
   - Implement error recovery
   - Add RTOS support if needed
4. **Optimize Performance**:
   - Use DMA for large transfers
   - Align buffers to 32-byte boundaries
   - Consider circular buffer for streaming

---

## Support

For issues:
1. Check this guide and documentation
2. Verify hardware connections
3. Use oscilloscope to check signals
4. Review STM32G474 reference manual

---

**Happy Coding! 🚀**
