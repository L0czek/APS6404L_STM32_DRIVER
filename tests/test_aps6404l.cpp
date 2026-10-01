/**
 * @file test_aps6404l.cpp
 * @brief Test file for APS6404L driver - Compilation verification
 *
 * This test verifies that the APS6404L driver compiles correctly.
 * It uses the real STM32 HAL types from the project (tests run on host).
 */

// Include expected implementation
// This is namespace-specific to avoid duplicate definitions
#include "aps6404l_expected.hpp"

// Include real STM32 HAL from project (tests run on host, not MCU)
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_qspi.h"

// Include APS6404L driver
#include "aps6404l.hpp"

// Include test framework
#include <iostream>
#include <cassert>
#include <cstring>
#include <exception>

// =============================================================================
// Global mock handles (these would be defined in main.c in real usage)
// =============================================================================

QSPI_HandleTypeDef hqspi1;
DMA_HandleTypeDef hdma_quadspi;

// =============================================================================
// Mock HAL Implementations
// =============================================================================

HAL_StatusTypeDef HAL_QSPI_Init(QSPI_HandleTypeDef* hqspi) {
    (void)hqspi;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_QSPI_Command(QSPI_HandleTypeDef* hqspi, QSPI_CommandTypeDef* cmd, uint32_t timeout) {
    (void)hqspi;
    (void)cmd;
    (void)timeout;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_QSPI_Receive(QSPI_HandleTypeDef* hqspi, uint8_t* data, uint32_t timeout) {
    (void)hqspi;
    (void)data;
    (void)timeout;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_QSPI_Transmit(QSPI_HandleTypeDef* hqspi, uint8_t* data, uint32_t timeout) {
    (void)hqspi;
    (void)data;
    (void)timeout;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_QSPI_Receive_DMA(QSPI_HandleTypeDef* hqspi, uint8_t* data) {
    (void)hqspi;
    (void)data;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_QSPI_Transmit_DMA(QSPI_HandleTypeDef* hqspi, uint8_t* data) {
    (void)hqspi;
    (void)data;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_Init(void) {
    return HAL_OK;
}

void SystemClock_Config(void) {}

void MX_GPIO_Init(void) {}

void MX_DMA_Init(void) {}

void MX_QUADSPI1_Init(void) {}

// =============================================================================
// Test Functions
// =============================================================================

bool test_constructor() {
    std::cout << "Test: Constructor..." << std::flush;
    
    // Test with QSPI handle only
    psram::PSRAMDriver psram1(&hqspi1);
    assert(psram1.psram_is_initialized() == false);
    
    // Test with both QSPI and DMA handles
    psram::PSRAMDriver psram2(&hqspi1, &hdma_quadspi);
    assert(psram2.psram_is_initialized() == false);
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_static_methods() {
    std::cout << "Test: Static methods..." << std::flush;
    
    assert(psram::PSRAMDriver::psram_get_size() == 8388608);
    assert(psram::PSRAMDriver::psram_get_address_width() == 24);
    assert(psram::PSRAMDriver::psram_get_max_address() == 8388607);
    assert(psram::PSRAMDriver::psram_get_dummy_cycles_standard() == 4);
    assert(psram::PSRAMDriver::psram_get_dummy_cycles_fast() == 6);
    assert(psram::PSRAMDriver::psram_get_wrap_size() == 32);
    assert(psram::PSRAMDriver::psram_get_timeout_ms() == 1000);
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_psram_reset() {
    std::cout << "Test: Reset..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    auto result = psram.psram_reset();
    assert(result.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_enter_quad_mode() {
    std::cout << "Test: Enter QPI mode..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    auto result = psram.psram_enter_quad_mode();
    assert(result.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_set_wrap_mode() {
    std::cout << "Test: Set wrap mode..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    auto result = psram.psram_set_wrap_mode();
    assert(result.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_psram_write() {
    std::cout << "Test: Write..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    uint8_t data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    
    auto result = psram.psram_write(0x000000, data, 256);
    assert(result.has_value());
    
    // Test invalid address
    auto result_invalid = psram.psram_write(0x1000000, data, 256);
    assert(!result_invalid.has_value());
    
    // Test zero size
    auto result_zero = psram.psram_write(0x000000, data, 0);
    assert(!result_zero.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_psram_read() {
    std::cout << "Test: Read..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    uint8_t data[256];
    std::memset(data, 0, sizeof(data));
    
    auto result = psram.psram_read(0x000000, data, 256);
    assert(result.has_value());
    
    // Test invalid address
    auto result_invalid = psram.psram_read(0x1000000, data, 256);
    assert(!result_invalid.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_psram_fast_write() {
    std::cout << "Test: Fast write..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    uint8_t data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    
    auto result = psram.psram_fast_write(0x000000, data, 256);
    assert(result.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_psram_fast_read() {
    std::cout << "Test: Fast read..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    uint8_t data[256];
    std::memset(data, 0, sizeof(data));
    
    auto result = psram.psram_fast_read(0x000000, data, 256);
    assert(result.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_psram_write_dma() {
    std::cout << "Test: Write DMA..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    uint8_t data[256];
    for (int i = 0; i < 256; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    
    auto result = psram.psram_write_dma(0x000000, data, 256);
    assert(result.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_psram_read_dma() {
    std::cout << "Test: Read DMA..." << std::flush;
    
    psram::PSRAMDriver psram(&hqspi1, &hdma_quadspi);
    
    uint8_t data[256];
    std::memset(data, 0, sizeof(data));
    
    auto result = psram.psram_read_dma(0x000000, data, 256);
    assert(result.has_value());
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_buffer_class() {
    std::cout << "Test: Buffer class..." << std::flush;
    
    // Test default alignment (32 bytes)
    psram::Buffer buffer1(4096);
    assert(buffer1.psram_size() == 4096);
    assert(buffer1.psram_alignment() == 32);
    assert(buffer1.psram_data() != nullptr);
    
    // Test custom alignment
    psram::Buffer buffer2(1024, 64);
    assert(buffer2.psram_size() == 1024);
    assert(buffer2.psram_alignment() == 64);
    
    // Test fill and clear
    buffer1.psram_fill(0xFF);
    buffer1.psram_clear();
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_has_dma() {
    std::cout << "Test: hasDMA..." << std::flush;
    
    // Without DMA handle
    psram::PSRAMDriver psram1(&hqspi1);
    assert(psram1.psram_has_dma() == false);
    
    // With DMA handle
    psram::PSRAMDriver psram2(&hqspi1, &hdma_quadspi);
    assert(psram2.psram_has_dma() == true);
    
    std::cout << " PASSED" << std::endl;
    return true;
}

bool test_error_codes() {
    std::cout << "Test: Error codes..." << std::flush;
    
    // Test error codes using make_error_code
    auto ec1 = psram::make_error_code(psram::ErrorCode::HALError);
    auto ec2 = psram::make_error_code(psram::ErrorCode::Timeout);
    auto ec3 = psram::make_error_code(psram::ErrorCode::InvalidAddress);
    
    assert(ec1.category().name() != nullptr);
    assert(ec2.category().name() != nullptr);
    assert(ec3.category().name() != nullptr);
    
    std::cout << " PASSED" << std::endl;
    return true;
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "APS6404L Driver Unit Tests" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    auto run_test = [&](bool (*test)()) {
        try {
            if (test()) {
                passed++;
            } else {
                failed++;
            }
        } catch (const std::exception& e) {
            std::cerr << " FAILED: " << e.what() << std::endl;
            failed++;
        }
    };
    
    run_test(test_constructor);
    run_test(test_static_methods);
    run_test(test_psram_reset);
    run_test(test_enter_quad_mode);
    run_test(test_set_wrap_mode);
    run_test(test_psram_write);
    run_test(test_psram_read);
    run_test(test_psram_fast_write);
    run_test(test_psram_fast_read);
    run_test(test_psram_write_dma);
    run_test(test_psram_read_dma);
    run_test(test_buffer_class);
    run_test(test_has_dma);
    run_test(test_error_codes);
    
    std::cout << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
