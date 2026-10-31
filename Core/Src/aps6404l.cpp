#include "aps6404l.hpp"
#include <algorithm>
#include <cstring>

namespace psram {

namespace {

std::unexpected<std::error_code> fail(ErrorCode ec) {
    return std::unexpected(make_error_code(ec));
}

constexpr uint32_t kMaxLinearBurstHz = 84'000'000;  // linear burst / EBh / 02h (Table 10)
constexpr uint32_t kMaxQpiReadHz     = 66'000'000;  // QPI 0Bh
constexpr uint32_t kMaxReadIdHz      = 33'000'000;  // 9Fh
constexpr uint32_t kMinCsHighNs      = 18;          // tCPH
constexpr uint32_t kFifoBytes        = 16;          // QUADSPI FIFO depth
constexpr uint32_t kSpinGuard        = 200'000;     // bounded polling while IRQs are masked

// Instruction (2 clk) + 24-bit address on 4 lines (6 clk) + CE# setup slack (1 clk)
constexpr uint32_t kQpiHeaderClocks  = 2 + 6 + 1;

volatile uint8_t& dr8(QUADSPI_TypeDef* q) {
    return *reinterpret_cast<volatile uint8_t*>(&q->DR);
}

uint32_t fifo_level(uint32_t sr) {
    return (sr & QUADSPI_SR_FLEVEL) >> QUADSPI_SR_FLEVEL_Pos;
}

bool abort_transfer(QUADSPI_TypeDef* q) {
    q->CR |= QUADSPI_CR_ABORT;
    for (uint32_t guard = kSpinGuard; guard; --guard) {
        if ((q->CR & QUADSPI_CR_ABORT) == 0) {
            q->FCR = QUADSPI_FCR_CTCF | QUADSPI_FCR_CTEF;
            return true;
        }
    }
    return false;
}

class IrqMask {
public:
    IrqMask() : primask_(__get_PRIMASK()) { __disable_irq(); }
    ~IrqMask() { __set_PRIMASK(primask_); }
    IrqMask(const IrqMask&) = delete;
    IrqMask& operator=(const IrqMask&) = delete;
private:
    uint32_t primask_;
};

} // namespace

// =============================================================================
// Construction / configuration
// =============================================================================

PSRAMDriver::PSRAMDriver(QSPI_HandleTypeDef* hqspi)
    : PSRAMDriver(hqspi, Timing{})
{
}

PSRAMDriver::PSRAMDriver(QSPI_HandleTypeDef* hqspi, const Timing& timing)
    : hqspi_(hqspi), timing_(timing), initialized_(false), quadMode_(false), wrapMode_(false),
      banks_(1), clock_hz_(0)
{
}

PSRAMDriver::~PSRAMDriver() = default;

std::error_code PSRAMDriver::convertHALStatus(HAL_StatusTypeDef status) {
    switch (status) {
        case HAL_OK:      return make_error_code(ErrorCode::None);
        case HAL_TIMEOUT: return make_error_code(ErrorCode::Timeout);
        case HAL_BUSY:    return make_error_code(ErrorCode::QSPIError);
        case HAL_ERROR:
        default:          return make_error_code(ErrorCode::HALError);
    }
}

size_t PSRAMDriver::psram_get_max_burst_bytes(uint8_t dummy_cycles) const {
    const uint64_t budget_clk =
        static_cast<uint64_t>(clock_hz_) * timing_.tcem_ns * timing_.margin_pct / 100u / 1'000'000'000u;
    const uint32_t header_clk = kQpiHeaderClocks + dummy_cycles;
    if (budget_clk <= header_clk) {
        return 0;
    }
    // 4 data lines per chip: 2 clocks per byte and chip; in dual-flash mode both
    // chips run in parallel, so the bus moves one byte per clock.
    size_t bytes = static_cast<size_t>((budget_clk - header_clk) * banks_ / 2u);
    if (timing_.max_burst_bytes) {
        bytes = std::min(bytes, timing_.max_burst_bytes);
    }
    if (banks_ == 2) {
        bytes &= ~static_cast<size_t>(1);
    }
    return bytes;
}

psram_expected<void> PSRAMDriver::psram_check_config() {
    if (!hqspi_ || !hqspi_->Instance) {
        return fail(ErrorCode::HALError);
    }
    QUADSPI_TypeDef* q = hqspi_->Instance;

    const uint32_t kernel_hz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_QSPI);
    const uint32_t prescaler = (q->CR >> QUADSPI_CR_PRESCALER_Pos) & 0xFFu;
    clock_hz_ = kernel_hz / (prescaler + 1u);
    banks_ = (q->CR & QUADSPI_CR_DFM) ? 2 : 1;

    // FSIZE: memory = 2^(FSIZE+1) bytes; in dual-flash mode it is the total of both chips.
    const uint32_t fsize = (q->DCR & QUADSPI_DCR_FSIZE) >> QUADSPI_DCR_FSIZE_Pos;
    if ((1ull << (fsize + 1u)) < psram_get_size()) {
        return fail(ErrorCode::ConfigError);
    }

    // tCPH: CE# high between bursts >= 18 ns
    if (clock_hz_ == 0) {
        return fail(ErrorCode::ConfigError);
    }
    const uint32_t csht_cycles = ((q->DCR >> QUADSPI_DCR_CSHT_Pos) & 0x7u) + 1u;
    if (static_cast<uint64_t>(csht_cycles) * 1'000'000'000u < static_cast<uint64_t>(kMinCsHighNs) * clock_hz_) {
        return fail(ErrorCode::ConfigError);
    }

    if (clock_hz_ > kMaxLinearBurstHz) {
        return fail(ErrorCode::ClockTooFast);
    }
    // Need room for at least one full FIFO's worth of data per burst to be useful.
    if (psram_get_max_burst_bytes(APS6404L_DUMMY_CYCLES_FAST) < std::min<size_t>(kFifoBytes, 2u * banks_)) {
        return fail(ErrorCode::ClockTooSlow);
    }
    return {};
}

// =============================================================================
// Commands
// =============================================================================

psram_expected<void> PSRAMDriver::send_command(uint8_t instruction, bool quad_lines) {
    if (!hqspi_) {
        return fail(ErrorCode::HALError);
    }
    QSPI_CommandTypeDef cmd;
    std::memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode   = quad_lines ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = instruction;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_NONE;
    cmd.DummyCycles       = 0;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;

    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &cmd, APS6404L_TIMEOUT_MS);
    if (status != HAL_OK) {
        return std::unexpected(convertHALStatus(status));
    }
    return {};
}

psram_expected<void> PSRAMDriver::psram_reset() {
    if (auto r = psram_check_config(); !r) {
        return r;
    }
    // The chip may be in QPI (warm reset) or SPI mode (power-up). Send the reset
    // pair in QPI first - an SPI-mode chip sees only 2 bits and ignores it - then
    // in SPI mode, which is the reset the datasheet requires after power-up.
    for (bool quad : {true, false}) {
        if (auto r = send_command(APS6404L_RSTEN_CMD, quad); !r) {
            return r;
        }
        if (auto r = send_command(APS6404L_RST_CMD, quad); !r) {
            return r;
        }
    }
    // tRST (50 ns) has passed long before the next HAL command can be issued.
    quadMode_ = false;
    wrapMode_ = false;   // reset restores linear burst
    return {};
}

psram_expected<void> PSRAMDriver::psram_enter_quad_mode() {
    if (auto r = send_command(APS6404L_QUAD_MODE_CMD, false); !r) {
        return r;
    }
    quadMode_ = true;
    return {};
}

psram_expected<void> PSRAMDriver::psram_exit_quad_mode() {
    if (!quadMode_) {
        return {};
    }
    if (auto r = send_command(APS6404L_EXIT_QUAD_CMD, true); !r) {
        return r;
    }
    quadMode_ = false;
    return {};
}

psram_expected<void> PSRAMDriver::psram_set_wrap_mode(bool wrap32) {
    if (wrap32 == wrapMode_) {
        return {};
    }
    // C0h toggles; it must be sent on the line width of the current mode.
    if (auto r = send_command(APS6404L_WRAP_TOGGLE_CMD, quadMode_); !r) {
        return r;
    }
    wrapMode_ = wrap32;
    return {};
}

psram_expected<void> PSRAMDriver::psram_read_id(uint8_t& mfid, uint8_t& kgd) {
    if (!hqspi_) {
        return fail(ErrorCode::HALError);
    }
    // 9Fh exists only in SPI mode; in dual-flash mode both chips would drive
    // the data phase at once and the result cannot be attributed to a chip.
    if (quadMode_ || banks_ != 1) {
        return fail(ErrorCode::NotSupported);
    }
    if (clock_hz_ > kMaxReadIdHz) {
        return fail(ErrorCode::ClockTooFast);
    }

    QSPI_CommandTypeDef cmd;
    std::memset(&cmd, 0, sizeof(cmd));
    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = APS6404L_READ_ID_CMD;
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
    cmd.Address           = 0;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_1_LINE;
    cmd.DummyCycles       = 0;
    cmd.NbData            = 2;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;

    uint8_t id[2] = {0, 0};
    HAL_StatusTypeDef status = HAL_QSPI_Command(hqspi_, &cmd, APS6404L_TIMEOUT_MS);
    if (status == HAL_OK) {
        status = HAL_QSPI_Receive(hqspi_, id, APS6404L_TIMEOUT_MS);
    }
    if (status != HAL_OK) {
        return std::unexpected(convertHALStatus(status));
    }
    mfid = id[0];
    kgd = id[1];
    return {};
}

psram_expected<void> PSRAMDriver::psram_init() {
    initialized_ = false;
    if (auto r = psram_check_config(); !r) {
        return r;
    }

    HAL_Delay(1);  // tPU >= 150 us after power-up (harmless if already elapsed)

    if (auto r = psram_reset(); !r) {
        return r;
    }

    if (banks_ == 1 && clock_hz_ <= kMaxReadIdHz) {
        uint8_t mfid = 0;
        uint8_t kgd = 0;
        if (auto r = psram_read_id(mfid, kgd); !r) {
            return r;
        }
        if (mfid != APS6404L_MFID || kgd != APS6404L_KGD_PASS) {
            return fail(ErrorCode::IdMismatch);
        }
    }

    if (auto r = psram_enter_quad_mode(); !r) {
        return r;
    }

    initialized_ = true;
    if (auto r = verify_access(); !r) {
        initialized_ = false;
        return r;
    }
    return {};
}

psram_expected<void> PSRAMDriver::psram_set_clock_prescaler(uint32_t division) {
    if (!hqspi_) {
        return fail(ErrorCode::HALError);
    }
    if (division < 1 || division > 256) {
        return fail(ErrorCode::InvalidSize);
    }

    const uint32_t previous = hqspi_->Init.ClockPrescaler;
    hqspi_->Init.ClockPrescaler = division - 1u;  // fCLK = fkernel / (PRESCALER + 1)
    HAL_StatusTypeDef status = HAL_QSPI_Init(hqspi_);
    if (status != HAL_OK) {
        hqspi_->Init.ClockPrescaler = previous;
        (void)HAL_QSPI_Init(hqspi_);
        (void)psram_check_config();
        return std::unexpected(convertHALStatus(status));
    }

    if (auto r = psram_check_config(); !r) {
        // The new clock violates the PSRAM timing: go back to the old one.
        hqspi_->Init.ClockPrescaler = previous;
        (void)HAL_QSPI_Init(hqspi_);
        (void)psram_check_config();
        return r;
    }
    return {};
}

// =============================================================================
// Data transfers
// =============================================================================

psram_expected<void> PSRAMDriver::validate_access(uint32_t address, size_t size, const void* data) const {
    if (!hqspi_) {
        return fail(ErrorCode::HALError);
    }
    if (!initialized_ || !quadMode_) {
        return fail(ErrorCode::NotInitialized);
    }
    if (!data || size == 0 || size > psram_get_size()) {
        return fail(ErrorCode::InvalidSize);
    }
    if (address >= psram_get_size() || size > psram_get_size() - address) {
        return fail(ErrorCode::InvalidAddress);
    }
    if (banks_ == 2 && ((address | size) & 1u)) {
        // In dual-flash mode each byte pair maps to one address of both chips.
        return fail((address & 1u) ? ErrorCode::InvalidAddress : ErrorCode::InvalidSize);
    }
    return {};
}

psram_expected<void> PSRAMDriver::transfer(bool is_read, uint8_t instruction, uint8_t dummy_cycles,
                                           uint32_t address, uint8_t* data, size_t size) {
    const size_t max_burst = psram_get_max_burst_bytes(dummy_cycles);
    if (max_burst == 0) {
        return fail(ErrorCode::ClockTooSlow);
    }
    // Burst boundary in bus address space: 1 KB page (linear burst may not cross
    // it more than once - we never cross it) or 32 B in wrap mode; doubled in DFM.
    const uint32_t boundary = (wrapMode_ ? APS6404L_WRAP_SIZE : APS6404L_PAGE_SIZE) * banks_;

    while (size > 0) {
        const size_t to_boundary = boundary - (address % boundary);
        const size_t n = std::min({size, max_burst, to_boundary});
        if (auto r = burst(is_read, instruction, dummy_cycles, address, data, n); !r) {
            return r;
        }
        address += static_cast<uint32_t>(n);
        data += n;
        size -= n;
    }
    return {};
}

psram_expected<void> PSRAMDriver::burst(bool is_read, uint8_t instruction, uint8_t dummy_cycles,
                                        uint32_t address, uint8_t* data, size_t n) {
    QUADSPI_TypeDef* q = hqspi_->Instance;
    if (hqspi_->State != HAL_QSPI_STATE_READY) {
        return fail(ErrorCode::QSPIError);   // HAL operation in progress elsewhere
    }

    // Previous burst must be fully finished (CE# high) before starting.
    uint32_t guard = kSpinGuard;
    while ((q->SR & QUADSPI_SR_BUSY) && --guard) {
    }
    if (!guard) {
        abort_transfer(q);
        return fail(ErrorCode::Timeout);
    }

    const uint32_t ccr = (is_read ? QUADSPI_CCR_FMODE_0 : 0u)      // indirect read / write
                       | QUADSPI_CCR_DMODE                          // data on 4 lines
                       | (static_cast<uint32_t>(dummy_cycles) << QUADSPI_CCR_DCYC_Pos)
                       | QUADSPI_CCR_ADSIZE_1                       // 24-bit address
                       | QUADSPI_CCR_ADMODE                         // address on 4 lines
                       | QUADSPI_CCR_IMODE                          // instruction on 4 lines (QPI)
                       | instruction;

    bool transfer_error = false;
    bool timed_out = false;
    {
        // From AR write until TCF the FIFO must be serviced without gaps, or the
        // QUADSPI stalls CLK with CE# low (tCEM violation). Keep IRQs off.
        IrqMask mask;
        q->FCR = QUADSPI_FCR_CTCF | QUADSPI_FCR_CTEF;
        q->DLR = static_cast<uint32_t>(n - 1u);
        q->CCR = ccr;
        q->AR = address;   // starts the burst

        size_t i = 0;
        guard = kSpinGuard;
        while (i < n) {
            const uint32_t sr = q->SR;
            if (sr & QUADSPI_SR_TEF) {
                transfer_error = true;
                break;
            }
            const uint32_t level = fifo_level(sr);
            const size_t remaining = n - i;
            if (is_read) {
                if (remaining >= 4 && level >= 4) {
                    const uint32_t word = q->DR;
                    std::memcpy(data + i, &word, 4);
                    i += 4;
                    continue;
                }
                if (remaining < 4 && level > 0) {
                    data[i++] = dr8(q);
                    continue;
                }
            } else {
                if (remaining >= 4 && level <= kFifoBytes - 4) {
                    uint32_t word;
                    std::memcpy(&word, data + i, 4);
                    q->DR = word;
                    i += 4;
                    continue;
                }
                if (remaining < 4 && level < kFifoBytes) {
                    dr8(q) = data[i++];
                    continue;
                }
            }
            if (--guard == 0) {
                timed_out = true;
                break;
            }
        }

        if (!transfer_error && !timed_out) {
            guard = kSpinGuard;
            while ((q->SR & (QUADSPI_SR_TCF | QUADSPI_SR_TEF)) == 0 && --guard) {
            }
            if (q->SR & QUADSPI_SR_TEF) {
                transfer_error = true;
            } else if (!guard) {
                timed_out = true;
            }
        }

        if (transfer_error || timed_out) {
            abort_transfer(q);
        } else {
            q->FCR = QUADSPI_FCR_CTCF;
        }
    }

    if (transfer_error) {
        return fail(ErrorCode::QSPIError);
    }
    if (timed_out) {
        return fail(ErrorCode::Timeout);
    }
    return {};
}

psram_expected<void> PSRAMDriver::psram_read(uint32_t address, uint8_t* data, size_t size) {
    if (auto r = validate_access(address, size, data); !r) {
        return r;
    }
    // QPI 0Bh is limited to 66 MHz; above that use EBh (same data, 6 wait cycles).
    if (clock_hz_ <= kMaxQpiReadHz) {
        return transfer(true, APS6404L_READ_CMD, APS6404L_DUMMY_CYCLES_STANDARD, address, data, size);
    }
    return transfer(true, APS6404L_FAST_READ_CMD, APS6404L_DUMMY_CYCLES_FAST, address, data, size);
}

psram_expected<void> PSRAMDriver::psram_fast_read(uint32_t address, uint8_t* data, size_t size) {
    if (auto r = validate_access(address, size, data); !r) {
        return r;
    }
    return transfer(true, APS6404L_FAST_READ_CMD, APS6404L_DUMMY_CYCLES_FAST, address, data, size);
}

psram_expected<void> PSRAMDriver::psram_write(uint32_t address, const uint8_t* data, size_t size) {
    if (auto r = validate_access(address, size, data); !r) {
        return r;
    }
    return transfer(false, APS6404L_WRITE_CMD, 0, address, const_cast<uint8_t*>(data), size);
}

psram_expected<void> PSRAMDriver::psram_fast_write(uint32_t address, const uint8_t* data, size_t size) {
    return psram_write(address, data, size);  // 02h and 38h are the same in QPI mode
}

psram_expected<void> PSRAMDriver::verify_access() {
    // Non-destructive check: save a block, write a pattern that crosses a burst
    // boundary in both chips, read it back, restore the original content.
    constexpr size_t kLen = 32;
    const uint32_t address = APS6404L_PAGE_SIZE * banks_ - kLen / 2;  // straddles the first page boundary
    uint8_t saved[kLen];
    uint8_t pattern[kLen];
    uint8_t readback[kLen];
    for (size_t i = 0; i < kLen; ++i) {
        pattern[i] = static_cast<uint8_t>((i & 1) ? (0x5A ^ i) : (0xA5 ^ (i << 1)));
    }

    if (auto r = psram_fast_read(address, saved, kLen); !r) {
        return r;
    }
    if (auto r = psram_write(address, pattern, kLen); !r) {
        return r;
    }
    if (auto r = psram_fast_read(address, readback, kLen); !r) {
        return r;
    }
    const bool ok = std::memcmp(pattern, readback, kLen) == 0;
    if (auto r = psram_write(address, saved, kLen); !r) {
        return r;
    }
    if (!ok) {
        return fail(ErrorCode::VerifyFailed);
    }
    return {};
}

} // namespace psram
