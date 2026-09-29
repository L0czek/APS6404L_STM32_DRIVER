/**
 * @file test_main.cpp
 * @brief Simple test to verify the APS6404L driver compiles correctly
 *
 * This file is not meant to be a comprehensive test, just a compilation
 * verification that all headers and implementations work together.
 */

#include "aps6404l.hpp"
#include <iostream>

// Mock HAL handles (these would be defined in main.c in real usage)
extern "C" {
    QSPI_HandleTypeDef hqspi1;
    DMA_HandleTypeDef hdma_quadspi;
}

int main() {
    // Create PSRAM instance
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);

    // Test static methods
    std::cout << "PSRAM Size: " << psram::PSRAMDriver::psram_get_size() << " bytes" << std::endl;
    std::cout << "Address Width: " << psram::PSRAMDriver::psram_get_address_width() << " bits" << std::endl;
    std::cout << "Max Address: " << psram::PSRAMDriver::psram_get_max_address() << std::endl;

    // Test Buffer class
    psram::Buffer buffer(4096);
    std::cout << "Buffer size: " << buffer.psram_size() << " bytes" << std::endl;
    std::cout << "Buffer alignment: " << buffer.psram_alignment() << " bytes" << std::endl;

    // Test method calls (these will fail at runtime without real hardware,
    // but we're just verifying compilation)
    auto result = psram.psram_reset();
    if (!result) {
        std::cerr << "Reset failed: " << result.error().message() << std::endl;
    }

    // Test error codes
    auto ec = psram::make_error_code(psram::ErrorCode::HALError);
    std::cout << "Error category: " << ec.category().name() << std::endl;
    std::cout << "Error message: " << ec.message() << std::endl;

    return 0;
}
