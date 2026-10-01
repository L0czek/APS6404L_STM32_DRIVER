#ifndef __APS6404L_HPP
#define __APS6404L_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <cstring>
#include <stdexcept>
#include <system_error>

// Include custom expected for C++17 compatibility
// This is namespace-specific to avoid duplicate definitions
#include "aps6404l_expected.hpp"

// HAL includes
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_qspi.h"

// PSRAM configuration
#ifndef APS6404L_SIZE_BYTES
#define APS6404L_SIZE_BYTES 8388608  // 8 MB
#endif

#ifndef APS6404L_ADDRESS_WIDTH
#define APS6404L_ADDRESS_WIDTH 24
#endif

#ifndef APS6404L_DUMMY_CYCLES_STANDARD
#define APS6404L_DUMMY_CYCLES_STANDARD 4
#endif

#ifndef APS6404L_DUMMY_CYCLES_FAST
#define APS6404L_DUMMY_CYCLES_FAST 6
#endif

#ifndef APS6404L_WRAP_SIZE
#define APS6404L_WRAP_SIZE 32
#endif

#ifndef APS6404L_TIMEOUT_MS
#define APS6404L_TIMEOUT_MS 1000
#endif

// Command definitions
constexpr uint8_t APS6404L_RSTEN_CMD       = 0x66;   // Reset Enable
constexpr uint8_t APS6404L_RST_CMD         = 0x99;   // Reset
constexpr uint8_t APS6404L_QUAD_MODE_CMD   = 0x35;   // Enter Quad Mode
constexpr uint8_t APS6404L_READ_CMD        = 0x0B;   // QPI Read
constexpr uint8_t APS6404L_WRITE_CMD       = 0x02;   // QPI Write
constexpr uint8_t APS6404L_FAST_READ_CMD   = 0xEB;   // Fast QPI Read
constexpr uint8_t APS6404L_FAST_WRITE_CMD  = 0x38;   // Fast QPI Write
constexpr uint8_t APS6404L_WRAP_TOGGLE_CMD = 0x5F;   // Wrap Boundary Toggle

namespace psram {

/**
 * @brief Error codes for PSRAM operations
 */
enum class ErrorCode : uint8_t {
    None = 0,
    HALError,
    Timeout,
    InvalidAddress,
    InvalidSize,
    NotInitialized,
    QSPIError,
    DMAError
};

/**
 * @brief Custom error_category implementation for std::expected
 */
class PSRAMErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "psram";
    }

    std::string message(int ev) const override {
        switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::None: return "No error";
            case ErrorCode::HALError: return "HAL error";
            case ErrorCode::Timeout: return "Operation timeout";
            case ErrorCode::InvalidAddress: return "Invalid address";
            case ErrorCode::InvalidSize: return "Invalid size";
            case ErrorCode::NotInitialized: return "PSRAM not initialized";
            case ErrorCode::QSPIError: return "QSPI error";
            case ErrorCode::DMAError: return "DMA error";
            default: return "Unknown error";
        }
    }
};

/**
 * @brief Get the PSRAM error category instance
 */
inline const PSRAMErrorCategory& get_error_category() {
    static PSRAMErrorCategory instance;
    return instance;
}

/**
 * @brief Create an error code from ErrorCode enum
 */
inline std::error_code make_error_code(ErrorCode ec) {
    return std::error_code(static_cast<int>(ec), get_error_category());
}

/**
 * @brief RAII wrapper for DMA-aligned buffers
 */
class Buffer {
public:
    /**
     * @brief Create a DMA-aligned buffer
     * @param size Buffer size in bytes
     * @param alignment Alignment in bytes (default: 32 for DMA)
     */
    explicit Buffer(size_t size, size_t alignment = 32)
        : buffer_(nullptr), size_(size), alignment_(alignment)
    {
        if (size == 0) {
            throw std::invalid_argument("Buffer size cannot be zero");
        }
        buffer_ = static_cast<uint8_t*>(aligned_alloc(alignment, size));
        if (!buffer_) {
            throw std::bad_alloc();
        }
        std::fill(buffer_, buffer_ + size, 0);
    }

    /**
     * @brief Destructor
     */
    ~Buffer() {
        if (buffer_) {
            free(buffer_);
        }
    }

    // Disable copy
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Enable move
    Buffer(Buffer&& other) noexcept
        : buffer_(other.buffer_), size_(other.size_), alignment_(other.alignment_)
    {
        other.buffer_ = nullptr;
        other.size_ = 0;
        other.alignment_ = 32;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (buffer_) {
                free(buffer_);
            }
            buffer_ = other.buffer_;
            size_ = other.size_;
            alignment_ = other.alignment_;
            other.buffer_ = nullptr;
            other.size_ = 0;
            other.alignment_ = 32;
        }
        return *this;
    }

    /**
     * @brief Get pointer to buffer data
     */
    uint8_t* psram_data() { return buffer_; }

    const uint8_t* psram_data() const { return buffer_; }

    /**
     * @brief Get buffer size
     */
    size_t psram_size() const { return size_; }

    /**
     * @brief Get alignment
     */
    size_t psram_alignment() const { return alignment_; }

    /**
     * @brief Clear buffer to zero
     */
    void psram_clear() {
        if (buffer_) {
            std::fill(buffer_, buffer_ + size_, 0);
        }
    }

    /**
     * @brief Fill buffer with value
     */
    void psram_fill(uint8_t value) {
        if (buffer_) {
            std::fill(buffer_, buffer_ + size_, value);
        }
    }

private:
    uint8_t* buffer_;
    size_t size_;
    size_t alignment_;
};

/**
 * @brief APS6404L PSRAM Driver Class
 *
 * This class provides a modern C++ interface for the APS6404L PSRAM chip.
 * It manages QSPI and DMA handles internally and provides methods for
 * all PSRAM operations with proper error handling via expected.
 */
class PSRAMDriver {
public:
    /**
     * @brief Construct PSRAM driver
     * @param hqspi Pointer to QSPI handle
     * @param hdma Pointer to DMA handle (for DMA operations)
     */
    PSRAMDriver(QSPI_HandleTypeDef* hqspi, DMA_HandleTypeDef* hdma = nullptr);

    /**
     * @brief Destroy PSRAM driver
     */
    ~PSRAMDriver();

    /**
     * @brief Reset PSRAM device
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_reset();

    /**
     * @brief Enter Quad (QPI) mode
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_enter_quad_mode();

    /**
     * @brief Enable wrap mode for burst transfers
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_set_wrap_mode();

    /**
     * @brief Read data from PSRAM (blocking)
     * @param address Starting address (0 to APS6404L_SIZE_BYTES-1)
     * @param data Pointer to buffer for read data
     * @param size Number of bytes to read
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_read(uint32_t address, uint8_t* data, size_t size);

    /**
     * @brief Write data to PSRAM (blocking)
     * @param address Starting address (0 to APS6404L_SIZE_BYTES-1)
     * @param data Pointer to data to write
     * @param size Number of bytes to write
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_write(uint32_t address, const uint8_t* data, size_t size);

    /**
     * @brief Fast read from PSRAM (blocking)
     * @param address Starting address (0 to APS6404L_SIZE_BYTES-1)
     * @param data Pointer to buffer for read data
     * @param size Number of bytes to read
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_fast_read(uint32_t address, uint8_t* data, size_t size);

    /**
     * @brief Fast write to PSRAM (blocking)
     * @param address Starting address (0 to APS6404L_SIZE_BYTES-1)
     * @param data Pointer to data to write
     * @param size Number of bytes to write
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_fast_write(uint32_t address, const uint8_t* data, size_t size);

    /**
     * @brief Read data from PSRAM using DMA
     * @param address Starting address (0 to APS6404L_SIZE_BYTES-1)
     * @param data Pointer to buffer for read data (must be 32-byte aligned)
     * @param size Number of bytes to read
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_read_dma(uint32_t address, uint8_t* data, size_t size);

    /**
     * @brief Write data to PSRAM using DMA
     * @param address Starting address (0 to APS6404L_SIZE_BYTES-1)
     * @param data Pointer to data to write (must be 32-byte aligned)
     * @param size Number of bytes to write
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_write_dma(uint32_t address, const uint8_t* data, size_t size);

    /**
     * @brief Get PSRAM size in bytes
     */
    static constexpr size_t psram_get_size() { return APS6404L_SIZE_BYTES; }

    /**
     * @brief Get address width in bits
     */
    static constexpr uint8_t psram_get_address_width() { return APS6404L_ADDRESS_WIDTH; }

    /**
     * @brief Get maximum valid address
     */
    static constexpr uint32_t psram_get_max_address() { return APS6404L_SIZE_BYTES - 1; }

    /**
     * @brief Get dummy cycles for standard read
     */
    static constexpr uint8_t psram_get_dummy_cycles_standard() { return APS6404L_DUMMY_CYCLES_STANDARD; }

    /**
     * @brief Get dummy cycles for fast read
     */
    static constexpr uint8_t psram_get_dummy_cycles_fast() { return APS6404L_DUMMY_CYCLES_FAST; }

    /**
     * @brief Get wrap burst size
     */
    static constexpr uint8_t psram_get_wrap_size() { return APS6404L_WRAP_SIZE; }

    /**
     * @brief Get timeout in milliseconds
     */
    static constexpr uint32_t psram_get_timeout_ms() { return APS6404L_TIMEOUT_MS; }

    /**
     * @brief Check if PSRAM is initialized
     */
    bool psram_is_initialized() const { return initialized_; }

    /**
     * @brief Check if DMA is available
     */
    bool psram_has_dma() const { return hdma_ != nullptr; }

    /**
     * @brief Get QSPI handle
     */
    QSPI_HandleTypeDef* psram_get_qspi_handle() { return hqspi_; }

    /**
     * @brief Set QSPI clock prescaler
     * @param prescaler Prescaler value (must be power of 2, 2-256)
     * @return psram_expected<void> - error on failure
     */
    psram_expected<void> psram_set_clock_prescaler(uint32_t prescaler);

private:
    QSPI_HandleTypeDef* hqspi_;
    DMA_HandleTypeDef* hdma_;
    bool initialized_;
    bool quadMode_;
    bool wrapMode_;

    /**
     * @brief Convert HAL status to expected error code
     */
    static std::error_code convertHALStatus(HAL_StatusTypeDef status);

    /**
     * @brief Validate address
     */
    static bool validateAddress(uint32_t address);

    /**
     * @brief Validate size
     */
    static bool validateSize(size_t size);

    /**
     * @brief Configure QSPI command
     */
    void configureCommand(QSPI_CommandTypeDef* cmd, uint8_t instruction,
                         uint8_t addressMode, uint8_t dataMode,
                         uint32_t address, uint8_t dummyCycles, size_t dataSize);
};

} // namespace psram

#endif // __APS6404L_HPP
