# =============================================================================
# APS6404L PSRAM Driver CMake Library
# =============================================================================
# This library provides a QSPI driver for the AP Memory APS6404L 8MB PSRAM
# for use with STM32 microcontrollers using the HAL driver.
#
# Usage in parent project (integrated mode):
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

project(psram_aps6404l
    LANGUAGES CXX C
    VERSION 1.0.0
    DESCRIPTION "APS6404L PSRAM Driver for STM32 HAL"
)

# =============================================================================
# Configuration Options
# =============================================================================

# Optional features
option(PSRAM_ENABLE_LOGGING "Enable verbose logging" OFF)
option(PSRAM_ENABLE_ASSERTS "Enable debug assertions" OFF)
option(PSRAM_BUILD_TESTS "Build STM32 test application" OFF)
option(PSRAM_BUILD_UNIT_TESTS "Build unit tests with mock HAL" ON)
option(PSRAM_INSTALL "Install PSRAM driver" OFF)

# Integrated mode: Use project's CMSIS and HAL paths
option(PSRAM_INTEGRATED_MODE "Use project-provided CMSIS/HAL paths" OFF)

# Integrated mode settings (provided by parent project)
set(PSRAM_CMSIS_INC_DIR "" CACHE STRING "Path to CMSIS include directory")
set(PSRAM_HAL_INC_DIR "" CACHE PATH "Path to HAL driver include directory")
set(PSRAM_MCU "" CACHE STRING "MCU name (e.g., STM32G474xx)")

# Standalone mode settings
set(STM32_DEVICE STM32G474 CACHE STRING "STM32 device")
set(STM32_CUBE_PATH "" CACHE PATH "Path to STM32CubeG4 Drivers directory")

# =============================================================================
# Find STM32 HAL
# =============================================================================

# Determine if we're in integrated mode or standalone mode
if(PSRAM_INTEGRATED_MODE)
    # Integrated mode: Use parent project's CMSIS and HAL paths
    if(NOT PSRAM_CMSIS_INC_DIR OR NOT PSRAM_HAL_INC_DIR OR NOT PSRAM_MCU)
        message(FATAL_ERROR "PSRAM_INTEGRATED_MODE requires PSRAM_CMSIS_INC_DIR, PSRAM_HAL_INC_DIR, and PSRAM_MCU to be set")
    endif()
    
    message(STATUS "APS6404L: Building in integrated mode with project-provided headers")
    set(STM32_DEVICE ${PSRAM_MCU})
else()
    # Standalone mode: Use STM32Cube path
    if(NOT STM32_CUBE_PATH)
        message(FATAL_ERROR "STM32_CUBE_PATH must be set. Example: -DSTM32_CUBE_PATH=/path/to/STM32CubeG4/Drivers")
    endif()
    
    if(NOT EXISTS ${STM32_CUBE_PATH})
        message(FATAL_ERROR "STM32_CUBE_PATH does not exist: ${STM32_CUBE_PATH}")
    endif()
    
    message(STATUS "APS6404L: Building in standalone mode with STM32Cube path")
endif()

# =============================================================================
# STM32 HAL Library
# =============================================================================

# Set C language for HAL library
enable_language(C)

if(PSRAM_INTEGRATED_MODE)
    # Integrated mode: Use parent project's HAL sources from external/nanopb's stm32cubemx or similar
    # The parent project should provide HAL sources via STM32_CUBE_PATH or similar
    # For integrated mode, we assume the HAL is already available from the parent project
    # Only create stm32_hal if it doesn't already exist (from another driver or parent project)
    if(NOT TARGET stm32_hal)
        add_library(stm32_hal INTERFACE)
        target_include_directories(stm32_hal INTERFACE ${PSRAM_CMSIS_INC_DIR} ${PSRAM_HAL_INC_DIR} ${PSRAM_BOARD_INC_DIR})
        target_compile_definitions(stm32_hal INTERFACE ${PSRAM_MCU} USE_HAL_DRIVER)
    endif()
    set(STM32_DEVICE ${PSRAM_MCU})
else()
    # Standalone mode: Build HAL from STM32Cube sources
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
endif()

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
)

if(PSRAM_INTEGRATED_MODE)
    # Integrated mode: Use parent project's CMSIS and HAL paths
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${PSRAM_CMSIS_INC_DIR}
            ${PSRAM_HAL_INC_DIR}
            ${PSRAM_BOARD_INC_DIR}
    )
else()
    # Standalone mode: Use STM32Cube paths
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
            ${STM32_CUBE_PATH}/CMSIS/Include
            ${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
    )
endif()

# Dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        stm32_hal
)

# Export include directory for integrated mode
set(PSRAM_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc PARENT_SCOPE)

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
    PRIVATE
        ${PSRAM_MCU}
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
        -fexceptions
        -fno-rtti
)

# =============================================================================
# Feature Checks
# =============================================================================

# Check for required QSPI functions (only in standalone mode)
if(NOT PSRAM_INTEGRATED_MODE)
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

    if(PSRAM_INTEGRATED_MODE)
        # Integrated mode: Use parent's linker script and HAL
        # Linker script path should be provided by parent project
    else()
        # Standalone mode: Use driver's linker script
        target_link_options(psramp_test
            PRIVATE
                -T${CMAKE_CURRENT_SOURCE_DIR}/STM32G474VETX_FLASH.ld
                -mcpu=cortex-m4
                -mthumb
                -mfloat-abi=hard
                -mfpu=fpv4-sp-d16
                -Wl,-Map=psramp_test.map
        )
    endif()

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
    # Since we cannot change compilers after project() is defined,
    # we create a custom target that builds the test with the host compiler directly

    set(TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_aps6404l.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/aps6404l.cpp
    )

    # Determine HAL include paths based on mode
    if(PSRAM_INTEGRATED_MODE)
        # Convert semicolon-separated list to individual -I flags
        set(HAL_INCLUDE_PATHS "")
        foreach(path IN LISTS PSRAM_CMSIS_INC_DIR)
            list(APPEND HAL_INCLUDE_PATHS -I${path})
        endforeach()
        foreach(path IN LISTS PSRAM_HAL_INC_DIR)
            list(APPEND HAL_INCLUDE_PATHS -I${path})
        endforeach()
        foreach(path IN LISTS PSRAM_BOARD_INC_DIR)
            list(APPEND HAL_INCLUDE_PATHS -I${path})
        endforeach()
    else()
        set(HAL_INCLUDE_PATHS
            -I${STM32_CUBE_PATH}/STM32G4xx_HAL_Driver/Inc
            -I${STM32_CUBE_PATH}/CMSIS/Include
            -I${STM32_CUBE_PATH}/CMSIS/Device/ST/STM32G4xx/Include
        )
    endif()

    # Create custom target that runs g++ directly
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_aps6404l.elf
        COMMAND /usr/bin/g++ -std=c++17 -fexceptions
            -I${CMAKE_CURRENT_SOURCE_DIR}
            -I${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
            -I${CMAKE_CURRENT_SOURCE_DIR}/tests
            ${HAL_INCLUDE_PATHS}
            -DAPS6404L_SIZE_BYTES=8388608
            -DAPS6404L_ADDRESS_WIDTH=24
            -DAPS6404L_DUMMY_CYCLES_STANDARD=4
            -DAPS6404L_DUMMY_CYCLES_FAST=6
            -DAPS6404L_WRAP_SIZE=32
            -DAPS6404L_TIMEOUT_MS=1000
            -DAPS6404L_UNIT_TEST
            -D${PSRAM_MCU}
            -o ${CMAKE_CURRENT_BINARY_DIR}/test_aps6404l.elf
            ${TEST_SOURCES}
        DEPENDS ${TEST_SOURCES}
        COMMENT "Building unit test with host compiler"
    )
    
    add_custom_target(test_aps6404l
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_aps6404l.elf
    )
    
    # Add test
    add_test(NAME test_aps6404l COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_aps6404l.elf)
    
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
message(STATUS "  Integrated Mode:   ${PSRAM_INTEGRATED_MODE}")
if(PSRAM_INTEGRATED_MODE)
    message(STATUS "  CMSIS Include:     ${PSRAM_CMSIS_INC_DIR}")
    message(STATUS "  HAL Include:       ${PSRAM_HAL_INC_DIR}")
else()
    message(STATUS "  Cube Path:         ${STM32_CUBE_PATH}")
endif()
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
message(STATUS "To build (integrated mode):")
message(STATUS "  Add subdirectory to your CMakeLists.txt:")
message(STATUS "    set(PSRAM_INTEGRATED_MODE ON)")
message(STATUS "    set(PSRAM_CMSIS_INC_DIR <cmsis-include-path>)")
message(STATUS "    set(PSRAM_HAL_INC_DIR <hal-include-path>)")
message(STATUS "    set(PSRAM_MCU STM32G474xx)")
message(STATUS "    add_subdirectory(APS6404L_STM32_DRIVER)")
message(STATUS "")
message(STATUS "To build (standalone mode):")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake .. -DSTM32_DEVICE=${STM32_DEVICE} -DSTM32_CUBE_PATH=<path-to-STM32CubeG4>")
message(STATUS "  make")
if(PSRAM_BUILD_TESTS)
    message(STATUS "  make psramp_test  - Build STM32 test application")
endif()
if(PSRAM_BUILD_UNIT_TESTS)
    message(STATUS "  make test_aps6404l - Build unit tests")
    message(STATUS "  ctest             - Run unit tests")
endif()
message(STATUS "")
