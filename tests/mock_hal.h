#ifndef __MOCK_HAL_H
#define __MOCK_HAL_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>
#include <stdexcept>

// Mock HAL status codes
enum HAL_StatusTypeDef {
    HAL_OK = 0x00,
    HAL_ERROR = 0x01,
    HAL_BUSY = 0x02,
    HAL_TIMEOUT = 0x03
};

// Mock QSPI handle structure
typedef struct {
    QSPI_TypeDef* Instance;
    void* Init;
    void* State;
    void* ErrorCode;
    void* pCpltCallback;
    void* pErrorCallback;
    void* pCpltCallbackArg;
    void* pErrorCallbackArg;
} QSPI_HandleTypeDef;

// Mock DMA handle structure
typedef struct {
    DMA_TypeDef* Instance;
    void* Init;
    void* State;
    void* ErrorCode;
    void* pCpltCallback;
    void* pErrorCallback;
    void* pCpltCallbackArg;
    void* pErrorCallbackArg;
} DMA_HandleTypeDef;

// Mock HAL macros
#define QSPI_INSTRUCTION_1_LINE     0x00
#define QSPI_INSTRUCTION_4_LINES    0x04
#define QSPI_ADDRESS_NONE           0x00
#define QSPI_ADDRESS_4_LINES        0x01
#define QSPI_ADDRESS_24_BITS        0x02
#define QSPI_ALTERNATE_BYTES_NONE   0x00
#define QSPI_DATA_NONE              0x00
#define QSPI_DATA_4_LINES           0x08
#define QSPI_SIOO_INST_EVERY_CMD    0x00
#define QSPI_DDR_MODE_DISABLE       0x00
#define QSPI_DDR_HHC_ANALOG_DELAY   0x00

// QSPI command structure
typedef struct {
    uint32_t InstructionMode;
    uint8_t Instruction;
    uint32_t AddressMode;
    uint32_t AddressSize;
    uint32_t Address;
    uint32_t AlternateByteMode;
    uint8_t DummyCycles;
    uint32_t DataMode;
    uint32_t NbData;
    uint32_t SIOOMode;
    uint32_t DdrMode;
    uint8_t DdrHoldHalfCycle;
} QSPI_CommandTypeDef;

// Mock HAL functions
HAL_StatusTypeDef HAL_QSPI_Command(QSPI_HandleTypeDef* hqspi, QSPI_CommandTypeDef* cmd, uint32_t timeout);
HAL_StatusTypeDef HAL_QSPI_Receive(QSPI_HandleTypeDef* hqspi, uint8_t* data, uint32_t timeout);
HAL_StatusTypeDef HAL_QSPI_Transmit(QSPI_HandleTypeDef* hqspi, uint8_t* data, uint32_t timeout);
HAL_StatusTypeDef HAL_QSPI_Receive_DMA(QSPI_HandleTypeDef* hqspi, uint8_t* data);
HAL_StatusTypeDef HAL_QSPI_Transmit_DMA(QSPI_HandleTypeDef* hqspi, uint8_t* data);

// Mock HAL_Init
void HAL_Init(void);

// Mock System Clock
void SystemClock_Config(void);

// Mock GPIO
void MX_GPIO_Init(void);

// Mock DMA
void MX_DMA_Init(void);

// Mock QUADSPI
void MX_QUADSPI1_Init(void);

#endif // __MOCK_HAL_H
