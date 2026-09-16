/* =============================================================================
 * drv_ads131m08.h — DynaWatch V4 ADS131M08 8-channel ADC driver
 *
 * Hardware interface:
 *   Bus:    LPSPI1 @ 20 MHz (LPSPI1_CLK_ROOT=80 MHz / prescaler=4)
 *   DRDY:   GPIO1_IOxx → NVIC (rising-edge ISR, CM33 priority 6)
 *   CLKIN:  SAI4 RX_BCLK output = 8.192 MHz (driven by SAI4)
 *   CS:     GPIO-controlled (active-low, bit-bang or LPSPI PCS0)
 *
 * ADS131M08 configuration:
 *   OSR:        512  → sample rate = 8 192 000 / (512 × 2) = 8 000 sps
 *   Frame:      27 bytes = (1 status + 8 ch data) × 3 bytes, MSB-first
 *   Channels:   8  (CH0..CH7, all enabled by default)
 *   Word size:  24 bits per channel
 *   GAIN:       1× default (configurable per channel via GAIN register)
 *   CRC:        CCITT-16 on last 2 bytes of response frame (Phase 6)
 *
 * Data path:
 *   DRDY ISR → LPSPI1 DMA TX (CMD_NULL + padding) / DMA RX (27 bytes)
 *            → DMA complete ISR → push decoded samples to ipc_shared pow_ring
 *            → IPC_WRITE_BARRIER() → increment g_shared.ctrl.ads_seq
 *
 * Phase 5 stub: init + channel-config API; data path is a stub.
 * Phase 6 will implement full DRDY-ISR → DMA → pow_ring pipeline.
 * =============================================================================
 */

#ifndef DRV_ADS131M08_H_
#define DRV_ADS131M08_H_

#include "fsl_common.h"
#include "clock_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Constants ────────────────────────────────────────────────────────── */

#define DRV_ADS131_CHANNELS         BOARD_ADS131_CHANNELS        /* 8       */
#define DRV_ADS131_FRAME_BYTES      BOARD_ADS131_FRAME_BYTES     /* 27      */
#define DRV_ADS131_OSR              BOARD_ADS131_OSR             /* 512     */
#define DRV_ADS131_SAMPLE_RATE_HZ   BOARD_ADS131_SAMPLE_RATE_HZ /* 8000    */
#define DRV_ADS131_SCLK_HZ         BOARD_ADS131_SCLK_HZ        /* 20 MHz  */

/** ADS131M08 SPI command words (24-bit, MSB first) */
#define ADS131_CMD_NULL             0x000000UL  /* No-op — reads last data   */
#define ADS131_CMD_RESET            0x0011UL    /* Reset all registers        */
#define ADS131_CMD_STANDBY          0x0022UL    /* Enter standby              */
#define ADS131_CMD_WAKEUP           0x0033UL    /* Exit standby               */
#define ADS131_CMD_LOCK             0x0555UL    /* Lock register writes        */
#define ADS131_CMD_UNLOCK           0x0655UL    /* Unlock register writes      */
#define ADS131_CMD_RREG(addr,cnt)   (0xA000UL | ((addr)<<7) | ((cnt)&0x7FU))
#define ADS131_CMD_WREG(addr,cnt)   (0x6000UL | ((addr)<<7) | ((cnt)&0x7FU))

/** ADS131M08 Register addresses */
#define ADS131_REG_ID               0x00U
#define ADS131_REG_STATUS           0x01U
#define ADS131_REG_MODE             0x02U
#define ADS131_REG_CLOCK            0x03U
#define ADS131_REG_GAIN1            0x04U
#define ADS131_REG_GAIN2            0x05U
#define ADS131_REG_CFG              0x06U
#define ADS131_REG_THRSHLD_MSB      0x07U
#define ADS131_REG_THRSHLD_LSB      0x08U
#define ADS131_REG_CH0_CFG          0x09U  /* ... through CH7_CFG at 0x1EU */

/** Gain codes for GAIN1/GAIN2 registers (2 channels per nibble) */
typedef enum {
    kAds131Gain1   = 0U,  /* × 1  */
    kAds131Gain2   = 1U,  /* × 2  */
    kAds131Gain4   = 2U,  /* × 4  */
    kAds131Gain8   = 3U,  /* × 8  */
    kAds131Gain16  = 4U,  /* × 16 */
    kAds131Gain32  = 5U,  /* × 32 */
    kAds131Gain64  = 6U,  /* × 64 */
    kAds131Gain128 = 7U,  /* × 128 (for future ADS131M04 compatibility) */
} ads131_gain_t;

/* ─── Decoded sample frame ─────────────────────────────────────────────── */

/** One decoded ADC frame (all 8 channels, sign-extended to 32 bits). */
typedef struct {
    int32_t  ch[DRV_ADS131_CHANNELS]; /*!< Ch0..Ch7, raw 24-bit sign-extended */
    uint16_t status;                   /*!< Frame status word                   */
    uint16_t crc;                      /*!< Received CRC (Phase 6: verified)    */
} ads131_frame_t;

/* ─── Public API ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise ADS131M08 driver and hardware.
 *
 * Sequence:
 *   1. Configure LPSPI1 @ 20 MHz, CPHA=1, CPOL=1 (SPI mode 1)
 *   2. Assert RESET (GPIO), hold ≥1 CLKIN cycle, release
 *   3. Wait for DRDY (confirms 8.192 MHz CLKIN received)
 *   4. Send CMD_RESET, read response, verify ID register
 *   5. Write CLOCK register: OSR=512, all channels enabled
 *   6. Register DRDY ISR (GPIO1 rising-edge, priority 6)
 *   7. Phase 6: configure DMA channels and enable
 *
 * @return kStatus_Success, or kStatus_Fail if DRDY never asserts.
 */
status_t drv_ads131m08_init(void);

/**
 * @brief Set per-channel gain.
 *
 * @param channel  Channel index 0..7.
 * @param gain     Gain code from ads131_gain_t.
 * @return kStatus_Success or kStatus_InvalidArgument.
 */
status_t drv_ads131m08_set_gain(uint8_t channel, ads131_gain_t gain);

/**
 * @brief Perform a synchronous read of one frame (Phase 5 blocking helper).
 *
 * Blocks until DRDY asserts, then executes an SPI transaction and decodes
 * the 27-byte response into p_frame.
 *
 * Phase 6: replaced by the DMA pipeline; this function remains for
 * diagnostics and register-access helpers.
 *
 * @param p_frame  Out: decoded ADC frame.
 * @return kStatus_Success.
 */
status_t drv_ads131m08_read_frame(ads131_frame_t *p_frame);

/**
 * @brief Read an ADS131M08 register over SPI.
 *
 * @param reg_addr  Register address (0x00..0x1F).
 * @param p_val     Out: 16-bit register value.
 * @return kStatus_Success.
 */
status_t drv_ads131m08_read_reg(uint8_t reg_addr, uint16_t *p_val);

/**
 * @brief Write an ADS131M08 register over SPI.
 *
 * @param reg_addr  Register address.
 * @param val       16-bit value to write.
 * @return kStatus_Success.
 */
status_t drv_ads131m08_write_reg(uint8_t reg_addr, uint16_t val);

/**
 * @brief Return the number of frames received since init (diagnostic).
 */
uint32_t drv_ads131m08_get_frame_count(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_ADS131M08_H_ */
