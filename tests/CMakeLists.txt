# =============================================================================
# APS6404L PSRAM Driver Tests
# =============================================================================

cmake_minimum_required(VERSION 3.15)

# =============================================================================
# Test Configuration
# =============================================================================

# Create test executable
add_executable(test_aps6404l
    test_aps6404l.cpp
)

# Add include directories
target_include_directories(test_aps6404l
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
)

# Require C++17 standard
set_target_properties(test_aps6404l
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Add mock HAL source if we have a .cpp version
# For now, we'll just include it inline
target_compile_definitions(test_aps6404l
    PRIVATE
        APS6404L_SIZE_BYTES=8388608
        APS6404L_ADDRESS_WIDTH=24
        APS6404L_DUMMY_CYCLES_STANDARD=4
        APS6404L_DUMMY_CYCLES_FAST=6
        APS6404L_WRAP_SIZE=32
        APS6404L_TIMEOUT_MS=1000
)

# Link with standard libraries
target_link_libraries(test_aps6404l
    PRIVATE
        stdc++m
)

# Add as test
add_test(NAME test_aps6404l COMMAND test_aps6404l)

# =============================================================================
# Test Build
# =============================================================================

# Check if we can compile (just verify the driver compiles)
try_compile(
    COMPILE_TEST
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_aps6404l.cpp
    CXX_COMPILE_FLAGS "-std=c++17 -I${CMAKE_CURRENT_SOURCE_DIR}/.. -I${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc"
)

if(COMPILE_TEST)
    message(STATUS "✓ Driver compilation test passed")
else()
    message(WARNING "✗ Driver compilation test failed")
endif()
