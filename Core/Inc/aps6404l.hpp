/**
 * @file aps6404l.hpp
 * @brief APS6404L (AP Memory 64 Mbit QSPI PSRAM) driver for STM32G4 QUADSPI
 *
 * Supports one chip or two chips in QUADSPI dual-flash mode (DFM). The QUADSPI
 * peripheral configuration (prescaler, FSIZE, CS high time, DFM) is owned by
 * CubeMX; psram_init() only validates it against the datasheet timing.
 *
 * Datasheet constraints implemented here (APS6404L-3SQR rev 2.7):
 *  - tCEM: CE# low time <= 8 us (standard grade) / 3 us (extended grade):
 *    every access is split into bursts that fit inside tCEM.
 *  - Linear bursts must not cross a 1 KB page more than once (and only at
 *    <= 84 MHz): bursts are split at page boundaries.
 *  - Max clock: 84 MHz for linear bursts, 66 MHz for QPI 0Bh, 33 MHz for 9Fh.
 *  - tCPH: CE# high between bursts >= 18 ns.
 *  - Power-up: 150 us, then RESET (66h/99h) issued in SPI mode.
 *
 * Dual-flash mode: the QUADSPI halves the address for each chip and interleaves
 * the data (even bytes on bank 1, odd bytes on bank 2). Bus addresses and sizes
 * must therefore be even, the bus capacity is 16 MB and a chip page is 2 KB of
 * bus address space.
 */
#ifndef __APS6404L_HPP
#define __APS6404L_HPP

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <stdlib.h>  // newlib declares aligned_alloc only in the global namespace
#include <cstring>
#include <string>
#include <system_error>

// std::expected alias (driver is built as C++23)
#include "aps6404l_expected.hpp"

// HAL includes
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_qspi.h"

// PSRAM configuration (per chip)
#ifndef APS6404L_SIZE_BYTES
#define APS6404L_SIZE_BYTES 8388608  // 8 MB
#endif

#ifndef APS6404L_ADDRESS_WIDTH
#define APS6404L_ADDRESS_WIDTH 24
#endif

#ifndef APS6404L_PAGE_SIZE
#define APS6404L_PAGE_SIZE 1024
#endif

#ifndef APS6404L_DUMMY_CYCLES_STANDARD
#define APS6404L_DUMMY_CYCLES_STANDARD 4   // QPI 0Bh
#endif

#ifndef APS6404L_DUMMY_CYCLES_FAST
#define APS6404L_DUMMY_CYCLES_FAST 6       // QPI EBh
#endif

#ifndef APS6404L_WRAP_SIZE
#define APS6404L_WRAP_SIZE 32
#endif

#ifndef APS6404L_TIMEOUT_MS
#define APS6404L_TIMEOUT_MS 100
#endif

// Command definitions
constexpr uint8_t APS6404L_RSTEN_CMD       = 0x66;   // Reset Enable
constexpr uint8_t APS6404L_RST_CMD         = 0x99;   // Reset
constexpr uint8_t APS6404L_QUAD_MODE_CMD   = 0x35;   // Enter Quad Mode (sent in SPI mode)
constexpr uint8_t APS6404L_EXIT_QUAD_CMD   = 0xF5;   // Exit Quad Mode (sent in QPI mode)
constexpr uint8_t APS6404L_READ_CMD        = 0x0B;   // QPI Fast Read, 4 wait cycles, <= 66 MHz
constexpr uint8_t APS6404L_WRITE_CMD       = 0x02;   // QPI Write, <= 84 MHz linear burst
constexpr uint8_t APS6404L_FAST_READ_CMD   = 0xEB;   // QPI Fast Quad Read, 6 wait cycles, <= 84 MHz linear burst
constexpr uint8_t APS6404L_FAST_WRITE_CMD  = 0x02;   // QPI write (38h is the same as 02h in QPI mode)
constexpr uint8_t APS6404L_WRAP_TOGGLE_CMD = 0xC0;   // Wrap Boundary Toggle (linear <-> wrap 32)
constexpr uint8_t APS6404L_READ_ID_CMD     = 0x9F;   // Read ID (SPI mode only, <= 33 MHz)

constexpr uint8_t APS6404L_MFID            = 0x0D;   // AP Memory
constexpr uint8_t APS6404L_KGD_PASS        = 0x5D;   // Known good die

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
    DMAError,
    ClockTooSlow,      // tCEM cannot be met even for a minimal burst
    ClockTooFast,      // QSPI clock above the datasheet limit for the operation
    ConfigError,       // QUADSPI FSIZE / CS high time incompatible with the PSRAM
    NotSupported,
    IdMismatch,
    VerifyFailed
};

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
            case ErrorCode::QSPIError: return "QSPI transfer error";
            case ErrorCode::DMAError: return "DMA error";
            case ErrorCode::ClockTooSlow: return "QSPI clock too slow to meet tCEM (max CE# low time)";
            case ErrorCode::ClockTooFast: return "QSPI clock above PSRAM limit";
            case ErrorCode::ConfigError: return "QUADSPI configuration incompatible with PSRAM (FSIZE / CS high time)";
            case ErrorCode::NotSupported: return "Operation not supported in this configuration";
            case ErrorCode::IdMismatch: return "PSRAM ID mismatch";
            case ErrorCode::VerifyFailed: return "PSRAM read-back verification failed";
            default: return "Unknown error";
        }
    }
};

inline const PSRAMErrorCategory& get_error_category() {
    static PSRAMErrorCategory instance;
    return instance;
}

inline std::error_code make_error_code(ErrorCode ec) {
    return std::error_code(static_cast<int>(ec), get_error_category());
}

/**
 * @brief Aligned heap buffer (never throws; check valid())
 */
class Buffer {
public:
    explicit Buffer(size_t size, size_t alignment = 32)
        : buffer_(nullptr), size_(size), alignment_(alignment)
    {
        if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
            size_ = 0;
            return;
        }
        // aligned_alloc requires the size to be a multiple of the alignment.
        const size_t alloc_size = (size + alignment - 1) & ~(alignment - 1);
        buffer_ = static_cast<uint8_t*>(::aligned_alloc(alignment, alloc_size));
        if (!buffer_) {
            size_ = 0;
            return;
        }
        std::memset(buffer_, 0, alloc_size);
    }

    ~Buffer() { std::free(buffer_); }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept
        : buffer_(other.buffer_), size_(other.size_), alignment_(other.alignment_)
    {
        other.buffer_ = nullptr;
        other.size_ = 0;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            std::free(buffer_);
            buffer_ = other.buffer_;
            size_ = other.size_;
            alignment_ = other.alignment_;
            other.buffer_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    bool valid() const { return buffer_ != nullptr; }
    uint8_t* psram_data() { return buffer_; }
    const uint8_t* psram_data() const { return buffer_; }
    size_t psram_size() const { return size_; }
    size_t psram_alignment() const { return alignment_; }

    void psram_clear() { if (buffer_) std::memset(buffer_, 0, size_); }
    void psram_fill(uint8_t value) { if (buffer_) std::memset(buffer_, value, size_); }

private:
    uint8_t* buffer_;
    size_t size_;
    size_t alignment_;
};

/**
 * @brief APS6404L PSRAM driver
 *
 * Data transfers are split into bursts; each burst runs with interrupts masked
 * (PRIMASK) and services the QUADSPI FIFO directly. Masking is required: if an
 * interrupt delays FIFO servicing, the QUADSPI stalls the clock with CE# held
 * low, which violates tCEM and blocks the PSRAM refresh. The masked time per
 * burst is bounded by tCEM * margin (about 6 us by default); lower it with
 * Timing::max_burst_bytes if that interrupt latency is too high.
 *
 * Not thread safe: serialise access externally.
 */
class PSRAMDriver {
public:
    struct Timing {
        uint32_t tcem_ns = 8000;      // 8000 standard grade (APS6404L-3SQR), 3000 extended (-3SQRX)
        uint8_t margin_pct = 75;      // fraction of tCEM used for a burst (CPU/FIFO slack)
        size_t max_burst_bytes = 0;   // optional extra cap on bus bytes per burst (0 = none)
    };

    explicit PSRAMDriver(QSPI_HandleTypeDef* hqspi);
    PSRAMDriver(QSPI_HandleTypeDef* hqspi, const Timing& timing);
    ~PSRAMDriver();

    /**
     * @brief Full bring-up: validate QUADSPI config, reset (QPI + SPI), check ID
     *        when possible, enter QPI mode and verify with a non-destructive
     *        read/write/restore of a small block.
     *
     * Call at least 150 us after power-up (tPU).
     */
    psram_expected<void> psram_init();

    /**
     * @brief Re-read the QUADSPI configuration and recompute burst limits
     */
    psram_expected<void> psram_check_config();

    /** @brief Reset the chip(s): RSTEN/RST sent in QPI and then in SPI mode */
    psram_expected<void> psram_reset();

    /** @brief Enter QPI mode (35h, sent on one line) */
    psram_expected<void> psram_enter_quad_mode();

    /** @brief Leave QPI mode (F5h, sent on four lines) */
    psram_expected<void> psram_exit_quad_mode();

    /**
     * @brief Select wrap-32 (true) or linear burst (false); sends C0h only when
     *        the state has to change (C0h toggles).
     */
    psram_expected<void> psram_set_wrap_mode(bool wrap32);

    /**
     * @brief Read the ID (9Fh) - SPI mode, single-flash only, QSPI clock <= 33 MHz
     */
    psram_expected<void> psram_read_id(uint8_t& mfid, uint8_t& kgd);

    // ---- Data transfers (bus addresses; even address/size in dual-flash mode) ----
    psram_expected<void> psram_read(uint32_t address, uint8_t* data, size_t size);
    psram_expected<void> psram_write(uint32_t address, const uint8_t* data, size_t size);
    psram_expected<void> psram_fast_read(uint32_t address, uint8_t* data, size_t size);
    psram_expected<void> psram_fast_write(uint32_t address, const uint8_t* data, size_t size);

    /**
     * @brief Change the QUADSPI clock division factor (1..256, f = fkernel / factor)
     *        at run time (memory training). Rejected, and the previous setting
     *        restored, if the new clock violates the PSRAM timing.
     */
    psram_expected<void> psram_set_clock_prescaler(uint32_t division);

    // ---- Information ----------------------------------------------------------------
    static constexpr size_t psram_get_chip_size() { return APS6404L_SIZE_BYTES; }
    static constexpr uint8_t psram_get_address_width() { return APS6404L_ADDRESS_WIDTH; }
    static constexpr uint8_t psram_get_dummy_cycles_standard() { return APS6404L_DUMMY_CYCLES_STANDARD; }
    static constexpr uint8_t psram_get_dummy_cycles_fast() { return APS6404L_DUMMY_CYCLES_FAST; }
    static constexpr uint8_t psram_get_wrap_size() { return APS6404L_WRAP_SIZE; }
    static constexpr uint32_t psram_get_timeout_ms() { return APS6404L_TIMEOUT_MS; }

    /** @brief Bus-visible capacity: 8 MB, or 16 MB in dual-flash mode */
    size_t psram_get_size() const { return APS6404L_SIZE_BYTES * banks_; }
    bool psram_is_dual_flash() const { return banks_ == 2; }
    uint32_t psram_get_clock_hz() const { return clock_hz_; }
    /** @brief Max bus bytes in one burst for the current clock (0 = clock too slow) */
    size_t psram_get_max_burst_bytes(uint8_t dummy_cycles) const;

    /**
     * @brief Describe the QUADSPI configuration the driver checks (for logging
     *        why psram_check_config() rejected it). Writes a NUL-terminated
     *        string, e.g. "QSPI 42500000 Hz, dual-flash, FSIZE=23 (16777216 B,
     *        need 16777216), CS high 1 cyc (23 ns, need 18), burst 240 B".
     * @return number of characters that the full string needs (snprintf semantics)
     */
    int psram_describe_config(char* buffer, size_t size) const;

    bool psram_is_initialized() const { return initialized_; }
    QSPI_HandleTypeDef* psram_get_qspi_handle() { return hqspi_; }

private:
    QSPI_HandleTypeDef* hqspi_;
    Timing timing_;
    bool initialized_;
    bool quadMode_;
    bool wrapMode_;
    uint8_t banks_;        // 1, or 2 in dual-flash mode
    uint32_t clock_hz_;    // QSPI CLK

    static std::error_code convertHALStatus(HAL_StatusTypeDef status);

    psram_expected<void> validate_access(uint32_t address, size_t size, const void* data) const;
    psram_expected<void> send_command(uint8_t instruction, bool quad_lines);
    psram_expected<void> transfer(bool is_read, uint8_t instruction, uint8_t dummy_cycles,
                                  uint32_t address, uint8_t* data, size_t size);
    psram_expected<void> burst(bool is_read, uint8_t instruction, uint8_t dummy_cycles,
                               uint32_t address, uint8_t* data, size_t size);
    psram_expected<void> verify_access();
};

} // namespace psram

#endif // __APS6404L_HPP
