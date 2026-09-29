# =============================================================================
# APS6404L PSRAM Driver CMake Library
# =============================================================================
# This library provides a QSPI driver for the AP Memory APS6404L 8MB PSRAM
# for use with STM32 microcontrollers using the HAL driver.
#
# Usage in parent project:
#   add_subdirectory(APS6404L_STM32_DRIVER)
#   target_link_libraries(your_target PRIVATE psram_aps6404l)
#
# Standalone build (with unit tests):
#   mkdir build && cd build
#   cmake .. -DSTM32_DEVICE=STM32G474 -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers
#   make test_aps6404l
#   ctest
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# =============================================================================
# Configuration
# =============================================================================

# Generate compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(psramp_driver
    LANGUAGES CXX C
    VERSION 1.0.0
    DESCRIPTION "APS6404L PSRAM Driver for STM32 HAL"
)

# =============================================================================
# Configuration Options
# =============================================================================

# STM32 device (adjust as needed)
set(STM32_DEVICE STM32G474 CACHE STRING "STM32 device")

# Path to STM32CubeG4 (adjust as needed)
set(STM32_CUBE_PATH "" CACHE PATH "Path to STM32CubeG4 Drivers directory")

# Optional features
option(PSRAM_ENABLE_LOGGING "Enable verbose logging" OFF)
option(PSRAM_ENABLE_ASSERTS "Enable debug assertions" OFF)
option(PSRAM_BUILD_TESTS "Build STM32 test application" OFF)
option(PSRAM_BUILD_UNIT_TESTS "Build unit tests with mock HAL" ON)
option(PSRAM_INSTALL "Install PSRAM driver" OFF)

# =============================================================================
# Find STM32 HAL
# =============================================================================

# Check if STM32_CUBE_PATH is set
if(NOT STM32_CUBE_PATH)
    message(FATAL_ERROR "STM32_CUBE_PATH must be set. Example: -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers")
endif()

# Check if path exists
if(NOT EXISTS ${STM32_CUBE_PATH})
    message(FATAL_ERROR "STM32_CUBE_PATH does not exist: ${STM32_CUBE_PATH}")
endif()

# =============================================================================
# STM32 HAL Library
# =============================================================================

# Set C language for HAL library
enable_language(C)

add_library(stm32_hal STATIC)
target_sources(stm32_hal
    PRIVATE
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal.c
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_qspi.c
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_dma.c
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_gpio.c
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_rcc.c
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_cortex.c
)

target_include_directories(stm32_hal
    PUBLIC
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
        ${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
)

target_compile_definitions(stm32_hal
    PUBLIC
        ${STM32_DEVICE}
        USE_HAL_DRIVER
        HSE_VALUE=16000000
        HSI_VALUE=16000000
)

target_compile_options(stm32_hal
    PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
        -O2
        -Wall
        -Wextra
)

# =============================================================================
# Library Definition
# =============================================================================

add_library(${PROJECT_NAME} STATIC)

# Add source files
target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/aps6404l.cpp
)

# Add include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
        ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
        ${STM32_CUBE_PATH}/CMSIS/Include
        ${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
)

# Dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        stm32_hal
)

# =============================================================================
# Requirements
# =============================================================================

# Require C++17 standard (for std::expected)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# =============================================================================
# Compile Definitions
# =============================================================================

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        # PSRAM configuration
        APS6404L_SIZE_BYTES=8388608
        APS6404L_ADDRESS_WIDTH=24
        APS6404L_DUMMY_CYCLES_STANDARD=4
        APS6404L_DUMMY_CYCLES_FAST=6
        APS6404L_WRAP_SIZE=32
        APS6404L_TIMEOUT_MS=1000
)

# =============================================================================
# Optional Features
# =============================================================================

if(PSRAM_ENABLE_LOGGING)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            PSRAM_ENABLE_LOGGING=1
    )
endif()

if(PSRAM_ENABLE_ASSERTS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            PSRAM_ENABLE_ASSERTS=1
    )
endif()

# Compiler flags
target_compile_options(${PROJECT_NAME}
    PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv4-sp-d16
        -O2
        -Wall
        -Wextra
)

# =============================================================================
# Feature Checks
# =============================================================================

# Check for required QSPI functions
include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
    #include \"stm32g4xx_hal_qspi.h\"
    int main() {
        HAL_QSPI_HandleTypeDef hqspi;
        DMA_HandleTypeDef hdma;
        HAL_QSPI_Receive(&hqspi, nullptr, 0);
        HAL_QSPI_Transmit(&hqspi, nullptr, 0);
        HAL_QSPI_Receive_DMA(&hqspi, nullptr);
        HAL_QSPI_Transmit_DMA(&hqspi, nullptr);
        return 0;
    }
" HAVE_QSPI_DMA)

if(NOT HAVE_QSPI_DMA)
    message(WARNING "QSPI DMA functions not available. DMA support disabled.")
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            PSRAM_DISABLE_DMA
    )
endif()

# =============================================================================
# Test Application (STM32)
# =============================================================================

if(PSRAM_BUILD_TESTS)
    add_executable(psramp_test
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/syscalls.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/stm32g4xx_it.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/stm32g4xx_hal_msp.c
    )

    target_include_directories(psramp_test
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    )

    target_link_libraries(psramp_test
        PRIVATE
            ${PROJECT_NAME}
            stm32_hal
    )

    # Linker script
    target_link_options(psramp_test
        PRIVATE
            -T${CMAKE_CURRENT_SOURCE_DIR}/STM32G474VETX_FLASH.ld
            -mcpu=cortex-m4
            -mthumb
            -mfloat-abi=hard
            -mfpu=fpv4-sp-d16
            -Wl,-Map=psramp_test.map
    )

    # Create HEX file
    add_custom_command(TARGET psramp_test
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex psramp_test.elf psramp_test.hex
        COMMENT "Generating HEX file"
    )

    message(STATUS "STM32 test application enabled - build with 'make psramp_test'")
endif()

# =============================================================================
# Unit Tests with Mocks (Host-based testing)
# =============================================================================

if(PSRAM_BUILD_UNIT_TESTS)
    enable_testing()

    # Create test executable
    add_executable(test_aps6404l
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_aps6404l.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/aps6404l.cpp
    )

    target_include_directories(test_aps6404l
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
            ${STM32_CUBE_PATH}/CMSIS/Include
            ${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
    )

    target_compile_definitions(test_aps6404l
        PUBLIC
            ${STM32_DEVICE}xx
            APS6404L_SIZE_BYTES=8388608
            APS6404L_ADDRESS_WIDTH=24
            APS6404L_DUMMY_CYCLES_STANDARD=4
            APS6404L_DUMMY_CYCLES_FAST=6
            APS6404L_WRAP_SIZE=32
            APS6404L_TIMEOUT_MS=1000
    )

    # Use host C++17 compiler
    set_target_properties(test_aps6404l
        PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            COMPILE_OPTIONS "-std=c++17"
    )

    # Link with standard C++ library
    target_link_libraries(test_aps6404l
        PRIVATE
            stdc++
    )

    # Add test (use full path to ensure CTest can find it)
    add_test(NAME test_aps6404l COMMAND $<TARGET_FILE:test_aps6404l>)

    message(STATUS "Unit tests enabled - build with 'make test_aps6404l'")
endif()

# =============================================================================
# Installation
# =============================================================================

if(PSRAM_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ${PROJECT_NAME}-targets
        FILE
            ${PROJECT_NAME}-targets.cmake
        NAMESPACE
            psram_aps6404l::
        DESTINATION
            lib/cmake/psram_aps6404l
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
        DESTINATION include
    )

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        DESTINATION share/doc/psram_aps6404l
    )
endif()

# =============================================================================
# Documentation
# =============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/PSRAM_API_REFERENCE.md)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PSRAM_API_REFERENCE.md
        DESTINATION share/doc/psram_aps6404l
    )
endif()

# =============================================================================
# Summary
# =============================================================================

message(STATUS "")
message(STATUS "APS6404L PSRAM Driver Configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Device:            ${STM32_DEVICE}")
message(STATUS "  Cube Path:         ${STM32_CUBE_PATH}")
message(STATUS "  Logging:           ${PSRAM_ENABLE_LOGGING}")
message(STATUS "  Assertions:        ${PSRAM_ENABLE_ASSERTS}")
message(STATUS "  Build Tests:       ${PSRAM_BUILD_TESTS}")
message(STATUS "  Build Unit Tests:  ${PSRAM_BUILD_UNIT_TESTS}")
message(STATUS "  Install:           ${PSRAM_INSTALL}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  psram_aps6404l    - Build PSRAM library")
if(PSRAM_BUILD_TESTS)
    message(STATUS "  psramp_test       - Build test application (STM32)")
endif()
if(PSRAM_BUILD_UNIT_TESTS)
    message(STATUS "  test_aps6404l     - Build unit tests (host)")
endif()
message(STATUS "")
message(STATUS "To build:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake .. -DSTM32_DEVICE=${STM32_DEVICE} -DSTM32_CUBE_PATH=${STM32_CUBE_PATH}")
message(STATUS "  make")
if(PSRAM_BUILD_TESTS)
    message(STATUS "  make psramp_test  - Build STM32 test application")
endif()
if(PSRAM_BUILD_UNIT_TESTS)
    message(STATUS "  make test_aps6404l - Build unit tests")
    message(STATUS "  ctest             - Run unit tests")
endif()
message(STATUS "")
