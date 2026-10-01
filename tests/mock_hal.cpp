/**
 * @file mock_hal.cpp
 * @brief Mock HAL implementations for host-based unit testing
 *
 * This file provides stub implementations of STM32 HAL functions
 * needed by the APS6404L driver for unit testing without real hardware.
 */

#include "mock_hal.h"
#include <cstdint>

// Global handles (referenced by test files)
extern "C" {
    QSPI_HandleTypeDef hqspi1;
    DMA_HandleTypeDef hdma_quadspi;
}

// Mock HAL functions
extern "C" {

HAL_StatusTypeDef HAL_Init(void) {
    return HAL_OK;
}

void HAL_Init(void) {
    // No-op for testing
}

void HAL_MspInit(void) {
    // No-op for testing
}

void HAL_IncTick(void) {
    // No-op for testing
}

void HAL_Delay(uint32_t Delay) {
    (void)Delay;
    // No-op for testing
}

uint32_t HAL_GetTick(void) {
    return 0;
}

void HAL_ResetTick(void) {
    // No-op for testing
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

void HAL_GPIO_Init(void* GPIOx, void* GPIO_Init) {
    (void)GPIOx;
    (void)GPIO_Init;
}

void HAL_GPIO_DeInit(void* GPIOx, uint32_t GPIO_Pin) {
    (void)GPIOx;
    (void)GPIO_Pin;
}

void HAL_DMA_Init(DMA_HandleTypeDef* hdma) {
    (void)hdma;
}

void HAL_DMA_DeInit(DMA_HandleTypeDef* hdma) {
    (void)hdma;
}

void SystemClock_Config(void) {
    // No-op for testing
}

void MX_GPIO_Init(void) {
    // No-op for testing
}

void MX_DMA_Init(void) {
    // No-op for testing
}

void MX_QUADSPI1_Init(void) {
    // No-op for testing
}

void Error_Handler(void) {
    // No-op for testing
}

} // extern "C"
