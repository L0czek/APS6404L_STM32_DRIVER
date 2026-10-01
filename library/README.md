# APS6404L PSRAM Driver

QSPI driver for the AP Memory APS6404L 8MB PSRAM for STM32 microcontrollers.

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

## API

See `../Core/Inc/aps6404l.hpp` for the complete API reference.
