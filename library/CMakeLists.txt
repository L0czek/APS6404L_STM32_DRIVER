# =============================================================================
# APS6404L PSRAM Driver - Library Integration CMakeLists.txt
# =============================================================================
# Usage in parent project:
#   add_subdirectory(path/to/APS6404L_STM32_DRIVER/library)
#   target_link_libraries(your_target PRIVATE psram_aps6404l)
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# =============================================================================
# Library Definition
# =============================================================================

add_library(psram_aps6404l STATIC)

# Add source files
target_sources(psram_aps6404l
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Src/aps6404l.cpp
)

# Add include directories
# Core/Inc is at ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
target_include_directories(psram_aps6404l
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
)

# Dependencies (HAL library must be provided by parent project)
target_link_libraries(psram_aps6404l
    PUBLIC
        $<TARGET_NAME_IF_EXISTS:stm32_hal>
)

# =============================================================================
# Requirements
# =============================================================================

set_target_properties(psram_aps6404l
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# =============================================================================
# Compile Definitions
# =============================================================================

target_compile_definitions(psram_aps6404l
    PUBLIC
        APS6404L_SIZE_BYTES=8388608
        APS6404L_ADDRESS_WIDTH=24
        APS6404L_DUMMY_CYCLES_STANDARD=4
        APS6404L_DUMMY_CYCLES_FAST=6
        APS6404L_WRAP_SIZE=32
        APS6404L_TIMEOUT_MS=1000
)
