/* =============================================================================
 * drv_sai.h — DynaWatch V4 SAI4 driver (master mode, I²S TDM)
 *
 * SAI4 clock chain (from AudioPLL @ 655.360 MHz):
 *   SAI4_CLK_ROOT = AudioPLL / 10 = 65.536 MHz   (SAI_MCLK ≤ 66 MHz ✓)
 *   TX_BCLK       = MCLK / 2      = 32.768 MHz   → STIMER reference (1.8 V)
 *   RX_BCLK       = MCLK / 8      =  8.192 MHz   → ADS131M08 CLKIN (1.8 V)
 *
 * SAI4 configuration:
 *   - Master mode: SAI4 generates both TX_BCLK and RX_BCLK
 *   - TX: TCR2[DIV]=0  → f_BCLK = MCLK / ((0+1)×2) = MCLK/2 = 32.768 MHz
 *   - RX: RCR2[DIV]=3  → f_BCLK = MCLK / ((3+1)×2) = MCLK/8 =  8.192 MHz
 *   - BCLK pins driven at 1.8 V (IOMUXC pad configured by board_hw_init)
 *   - TX BCLK/WCLK are output-only (STIMER external reference)
 *   - RX BCLK is output (ADS131M08 CLKIN), RX DATA is input
 *
 * Phase 5 stub: init + start; DMA-linked data path is a future phase stub.
 * Phase 6 will wire SAI4 RX FIFO → DMA → pow_ring (ipc_shared).
 * =============================================================================
 */

#ifndef DRV_SAI_H_
#define DRV_SAI_H_

#include "fsl_common.h"
#include "fsl_sai.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Constants ────────────────────────────────────────────────────────── */

/** SAI4 TX BCLK divisor field value: TCR2[DIV]=0 → MCLK/((0+1)×2) = MCLK/2 */
#define DRV_SAI4_TX_BCLK_DIV      0U

/** SAI4 RX BCLK divisor field value: RCR2[DIV]=3 → MCLK/((3+1)×2) = MCLK/8 */
#define DRV_SAI4_RX_BCLK_DIV      3U

/** Number of ADS131M08 data channels */
#define DRV_SAI4_RX_CHANNELS       8U

/** Bits per sample slot (ADS131M08 outputs 24-bit words, padded to 32) */
#define DRV_SAI4_BITS_PER_SLOT    32U

/* ─── Callback type ────────────────────────────────────────────────────── */

/**
 * @brief DMA-complete callback invoked from ISR context (Phase 6).
 * @param p_rx_buf  Pointer to the received frame buffer.
 * @param frame_len Number of bytes in the buffer.
 */
typedef void (*drv_sai4_rx_callback_t)(const uint8_t *p_rx_buf, size_t frame_len);

/* ─── Public API ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise SAI4 in master mode.
 *
 * Configures:
 *   - TX: master BCLK at 32.768 MHz (STIMER reference pin)
 *   - RX: master BCLK at 8.192 MHz (ADS131M08 CLKIN pin)
 *   - I2S frame, 8 channels, 32-bit slots
 *
 * Must be called after BOARD_InitBootClocks() and BOARD_InitHardware().
 * Does NOT start the SAI; call drv_sai4_start() after.
 *
 * @return kStatus_Success or an FSL error code.
 */
status_t drv_sai4_init(void);

/**
 * @brief Start SAI4 TX and RX (enable clocks on BCLK pins).
 *
 * Phase 6: also arms the first DMA descriptor.
 *
 * @return kStatus_Success.
 */
status_t drv_sai4_start(void);

/**
 * @brief Stop SAI4 TX and RX.
 * @return kStatus_Success.
 */
status_t drv_sai4_stop(void);

/**
 * @brief Register the RX-complete callback (Phase 6 hook).
 *
 * The callback is invoked from DMA ISR when a full ADS131M08 frame has
 * been received into the DMA ping-pong buffer.
 *
 * @param cb  Callback function pointer (NULL to deregister).
 */
void drv_sai4_register_rx_callback(drv_sai4_rx_callback_t cb);

/**
 * @brief Return the RX BCLK frequency in Hz (for ADS131M08 CLKIN).
 */
uint32_t drv_sai4_get_rx_bclk_hz(void);

/**
 * @brief Return the TX BCLK frequency in Hz (STIMER reference).
 */
uint32_t drv_sai4_get_tx_bclk_hz(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SAI_H_ */
