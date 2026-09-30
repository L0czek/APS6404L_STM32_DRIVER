#include "aps6404l.hpp"
#include <cstring>

namespace psram {

// =============================================================================
// APS6404L Implementation
// =============================================================================

PSRAMDriver::PSRAMDriver(QSPI_HandleTypeDef* hqspi, DMA_HandleTypeDef* hdma)
    : hqspi_(hqspi), hdma_(hdma), initialized_(false), quadMode_(false), wrapMode_(false)
{
}

PSRAMDriver::~PSRAMDriver() = default;

std::error_code PSRAMDriver::convertHALStatus(HAL_StatusTypeDef status) {
    switch (status) {
        case HAL_OK:
            return make_error_code(ErrorCode::None);
        case HAL_TIMEOUT:
            return make_error_code(ErrorCode::Timeout);
        case HAL_ERROR:
            return make_error_code(ErrorCode::HALError);
        case HAL_BUSY:
            return make_error_code(ErrorCode::Timeout);
        default:
            return make_error_code(ErrorCode::HALError);
    }
}

bool PSRAMDriver::validateAddress(uint32_t address) {
    return address <= psram_get_max_address();
}

bool PSRAMDriver::validateSize(size_t size) {
    return size > 0 && size <= psram_get_max_address();
}

void PSRAMDriver::configureCommand(QSPI_CommandTypeDef* cmd, uint8_t instruction,
                                 uint8_t addressMode, uint8_t dataMode,
                                 uint32_t address, uint8_t dummyCycles, size_t dataSize)
{
    std::memset(cmd, 0, sizeof(QSPI_CommandTypeDef));
    
    cmd->InstructionMode   = QSPI_INSTRUCTION_4_LINES;
    cmd->Instruction       = instruction;
    cmd->AddressMode       = addressMode;
    cmd->AddressSize       = QSPI_ADDRESS_24_BITS;
    cmd->Address           = address;
    cmd->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd->DataMode          = dataMode;
    cmd->DummyCycles       = dummyCycles;
    cmd->NbData            = dataSize;
    cmd->SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    cmd->DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd->DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
}

// =============================================================================
// Public Methods
// =============================================================================

psram_expected<void> PSRAMDriver::psram_reset() {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }

    // --- Step 1: Send Reset Enable (0x66) ---
    QSPI_CommandTypeDef sCommand;
    configureCommand(&sCommand, APS6404L_RSTEN_CMD,
                     QSPI_ADDRESS_NONE, QSPI_DATA_NONE,
                     0, 0, 0);
    
    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    // --- Step 2: Send Reset (0x99) ---
    sCommand.Instruction = APS6404L_RST_CMD;
    status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    initialized_ = true;
    return {};
}

psram_expected<void> PSRAMDriver::psram_enter_quad_mode() {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }

    QSPI_CommandTypeDef sCommand;
    
    // Quad Mode Enable command (0x35) must be sent in SPI mode (1-line)
    std::memset(&sCommand, 0, sizeof(QSPI_CommandTypeDef));
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = APS6404L_QUAD_MODE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.NbData            = 0;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    quadMode_ = true;
    return {};
}

psram_expected<void> PSRAMDriver::psram_set_wrap_mode() {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }

    QSPI_CommandTypeDef sCommand;
    std::memset(&sCommand, 0, sizeof(QSPI_CommandTypeDef));
    
    // Configure command to toggle wrap boundary mode (0x5F)
    sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction       = APS6404L_WRAP_TOGGLE_CMD;
    sCommand.AddressMode       = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode          = QSPI_DATA_NONE;
    sCommand.DummyCycles       = 0;
    sCommand.NbData            = 0;
    sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    wrapMode_ = true;
    return {};
}

psram_expected<void> PSRAMDriver::psram_read(uint32_t address, uint8_t* data, size_t size) {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }
    if (!validateAddress(address)) {
        return make_error_code(ErrorCode::InvalidAddress);
    }
    if (!validateSize(size)) {
        return make_error_code(ErrorCode::InvalidSize);
    }

    QSPI_CommandTypeDef sCommand;
    configureCommand(&sCommand, APS6404L_READ_CMD,
                     QSPI_ADDRESS_4_LINES, QSPI_DATA_4_LINES,
                     address, APS6404L_DUMMY_CYCLES_STANDARD, size);

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    status = HAL_QSPI_Receive(hqspi_, data, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    return {};
}

psram_expected<void> PSRAMDriver::psram_write(uint32_t address, const uint8_t* data, size_t size) {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }
    if (!validateAddress(address)) {
        return make_error_code(ErrorCode::InvalidAddress);
    }
    if (!validateSize(size)) {
        return make_error_code(ErrorCode::InvalidSize);
    }

    QSPI_CommandTypeDef sCommand;
    configureCommand(&sCommand, APS6404L_WRITE_CMD,
                     QSPI_ADDRESS_4_LINES, QSPI_DATA_4_LINES,
                     address, 0, size);

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    status = HAL_QSPI_Transmit(hqspi_, const_cast<uint8_t*>(data), APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    return {};
}

psram_expected<void> PSRAMDriver::psram_fast_read(uint32_t address, uint8_t* data, size_t size) {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }
    if (!validateAddress(address)) {
        return make_error_code(ErrorCode::InvalidAddress);
    }
    if (!validateSize(size)) {
        return make_error_code(ErrorCode::InvalidSize);
    }

    QSPI_CommandTypeDef sCommand;
    configureCommand(&sCommand, APS6404L_FAST_READ_CMD,
                     QSPI_ADDRESS_4_LINES, QSPI_DATA_4_LINES,
                     address, APS6404L_DUMMY_CYCLES_FAST, size);

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    status = HAL_QSPI_Receive(hqspi_, data, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    return {};
}

psram_expected<void> PSRAMDriver::psram_fast_write(uint32_t address, const uint8_t* data, size_t size) {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }
    if (!validateAddress(address)) {
        return make_error_code(ErrorCode::InvalidAddress);
    }
    if (!validateSize(size)) {
        return make_error_code(ErrorCode::InvalidSize);
    }

    QSPI_CommandTypeDef sCommand;
    configureCommand(&sCommand, APS6404L_FAST_WRITE_CMD,
                     QSPI_ADDRESS_4_LINES, QSPI_DATA_4_LINES,
                     address, 0, size);

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    status = HAL_QSPI_Transmit(hqspi_, const_cast<uint8_t*>(data), APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    return {};
}

psram_expected<void> PSRAMDriver::psram_read_dma(uint32_t address, uint8_t* data, size_t size) {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }
    if (!hdma_) {
        return make_error_code(ErrorCode::DMAError);
    }
    if (!validateAddress(address)) {
        return make_error_code(ErrorCode::InvalidAddress);
    }
    if (!validateSize(size)) {
        return make_error_code(ErrorCode::InvalidSize);
    }

    QSPI_CommandTypeDef sCommand;
    configureCommand(&sCommand, APS6404L_FAST_READ_CMD,
                     QSPI_ADDRESS_4_LINES, QSPI_DATA_4_LINES,
                     address, APS6404L_DUMMY_CYCLES_FAST, size);

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    status = HAL_QSPI_Receive_DMA(hqspi_, data);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    return {};
}

psram_expected<void> PSRAMDriver::psram_write_dma(uint32_t address, const uint8_t* data, size_t size) {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }
    if (!hdma_) {
        return make_error_code(ErrorCode::DMAError);
    }
    if (!validateAddress(address)) {
        return make_error_code(ErrorCode::InvalidAddress);
    }
    if (!validateSize(size)) {
        return make_error_code(ErrorCode::InvalidSize);
    }

    QSPI_CommandTypeDef sCommand;
    configureCommand(&sCommand, APS6404L_FAST_WRITE_CMD,
                     QSPI_ADDRESS_4_LINES, QSPI_DATA_4_LINES,
                     address, 0, size);

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &sCommand, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    status = HAL_QSPI_Transmit_DMA(hqspi_, const_cast<uint8_t*>(data));
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    return {};
}

psram_expected<void> PSRAMDriver::psram_set_clock_prescaler(uint32_t prescaler) {
    if (!hqspi_) {
        return make_error_code(ErrorCode::HALError);
    }

    // Validate prescaler is power of 2 and in range (2-256)
    if (prescaler < 2 || prescaler > 256 || (prescaler & (prescaler - 1)) != 0) {
        return make_error_code(ErrorCode::InvalidSize);
    }

    // Convert prescaler to QSPI clock divider value
    // QSPI clock = HCLK / prescaler
    // Prescaler values: 2, 4, 8, 16, 32, 64, 128, 256
    // Prediv values: 1, 3, 7, 15, 31, 63, 127, 255 (Prediv + 1 = prescaler)
    uint32_t prediv = prescaler - 1;

    hqspi_->Init.ClockPrescaler = prediv;
    
    HAL_StatusTypeDef status = HAL_QSPI_Init(hqspi_);
    if (status != HAL_OK) {
        return convertHALStatus(status);
    }

    return {};
}

} // namespace psram
