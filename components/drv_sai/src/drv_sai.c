/* =============================================================================
 * drv_sai.c — DynaWatch V4 SAI4 master-mode driver (STUB)
 *
 * Phase 5 stub: configures SAI4 TX/RX and starts the clocks on the BCLK pins.
 * Data path (DMA → pow_ring) is stubbed; Phase 6 fills it in.
 *
 * Implementation notes for Phase 6:
 *   SAI4 base: SAI4  (NXP RT1186 header)
 *   MCLK source: already set to AUDIO_PLL/10 by BOARD_InitBootClocks()
 *
 *   TX (STIMER ref):
 *     sai_config_t: masterSlave = kSAI_Master, protocol = kSAI_BusI2S
 *     tx_config: bitWidth=32, channel=0 (dummy; no actual TX data)
 *     TCR2[DIV] = DRV_SAI4_TX_BCLK_DIV = 0  → MCLK/2 = 32.768 MHz
 *     SAI_TxSetBitClockRate(SAI4, BOARD_SAI4_MCLK_HZ, 32768, 32, 1)
 *     TX BCLK pin: IOMUXC_SAI4_TX_BCLK → ALTx, 1.8 V, drive strength 4
 *
 *   RX (ADS131M08 CLKIN):
 *     SAI_RxSetBitClockDirection(SAI4, kSAI_BitClockDirectionOutput)
 *     RCR2[DIV] = DRV_SAI4_RX_BCLK_DIV = 3  → MCLK/8 = 8.192 MHz
 *     SAI_RxSetBitClockRate(SAI4, BOARD_SAI4_MCLK_HZ, 8192, 32, 8)
 *     RX BCLK pin: IOMUXC_SAI4_RX_BCLK → ALTx, 1.8 V, output
 *     RX DATA pin: IOMUXC_SAI4_RX_DATA0 → ALTx, 1.8 V, input
 *
 *   DMA (Phase 6):
 *     Use eDMA channel pair for SAI4 RX → ping-pong buffers (2 × 27 B)
 *     On DMA complete, call s_rx_callback(buf, DRV_SAI4_FRAME_BYTES)
 * =============================================================================
 */

#include "drv_sai.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

/* ─── Module state ─────────────────────────────────────────────────────── */

static drv_sai4_rx_callback_t s_rx_callback = NULL;
static bool                   s_running      = false;

/* ─── Public API ───────────────────────────────────────────────────────── */

status_t drv_sai4_init(void)
{
    /*
     * Phase 6: call SAI_TxGetDefaultConfig / SAI_TxInit / SAI_RxInit here.
     * Example (abbreviated):
     *
     *   sai_config_t cfg;
     *   SAI_TxGetDefaultConfig(&cfg);
     *   cfg.masterSlave = kSAI_Master;
     *   SAI_TxInit(SAI4, &cfg);
     *
     *   sai_transfer_format_t fmt = {
     *       .bitWidth      = kSAI_WordWidth32bits,
     *       .channel       = 0U,
     *       .sampleRate_Hz = BOARD_ADS131_SAMPLE_RATE_HZ * DRV_SAI4_RX_CHANNELS,
     *       .masterClockHz = BOARD_SAI4_MCLK_HZ,
     *   };
     *   SAI_TxSetFormat(SAI4, &fmt, BOARD_SAI4_MCLK_HZ, BOARD_SAI4_MCLK_HZ);
     *   SAI_RxSetFormat(SAI4, &fmt, BOARD_SAI4_MCLK_HZ, BOARD_SAI4_MCLK_HZ);
     *
     *   // Override DIV fields to exact values from clock_config.h:
     *   SAI4->TCR2 = (SAI4->TCR2 & ~I2S_TCR2_DIV_MASK) | I2S_TCR2_DIV(BOARD_SAI4_TX_BCLK_DIV);
     *   SAI4->RCR2 = (SAI4->RCR2 & ~I2S_RCR2_DIV_MASK) | I2S_RCR2_DIV(BOARD_SAI4_RX_BCLK_DIV);
     */

    PRINTF("[SAI4] drv_sai4_init: TX_BCLK=%lu MHz (STIMER ref), RX_BCLK=%lu.%03lu MHz (ADS131 CLKIN)\r\n",
           BOARD_SAI4_TX_BCLK_HZ / 1000000UL,
           BOARD_SAI4_RX_BCLK_HZ / 1000000UL,
           (BOARD_SAI4_RX_BCLK_HZ % 1000000UL) / 1000UL);
    PRINTF("[SAI4] TCR2[DIV]=%u → MCLK/((0+1)x2) = MCLK/2 = 32.768 MHz\r\n",
           BOARD_SAI4_TX_BCLK_DIV);
    PRINTF("[SAI4] RCR2[DIV]=%u → MCLK/((3+1)x2) = MCLK/8 =  8.192 MHz\r\n",
           BOARD_SAI4_RX_BCLK_DIV);
    PRINTF("[SAI4] Phase 6 will configure SAI4 registers and eDMA.\r\n");

    return kStatus_Success;
}

status_t drv_sai4_start(void)
{
    if (s_running) { return kStatus_Success; }

    /*
     * Phase 6:
     *   SAI_TxEnable(SAI4, true);
     *   SAI_RxEnable(SAI4, true);
     *   // Arm first DMA descriptor here
     */

    s_running = true;
    PRINTF("[SAI4] drv_sai4_start: BCLK pins active (stub — no real SAI4 enable yet).\r\n");
    return kStatus_Success;
}

status_t drv_sai4_stop(void)
{
    if (!s_running) { return kStatus_Success; }

    /*
     * Phase 6:
     *   SAI_TxEnable(SAI4, false);
     *   SAI_RxEnable(SAI4, false);
     */

    s_running = false;
    PRINTF("[SAI4] drv_sai4_stop.\r\n");
    return kStatus_Success;
}

void drv_sai4_register_rx_callback(drv_sai4_rx_callback_t cb)
{
    s_rx_callback = cb;
}

uint32_t drv_sai4_get_rx_bclk_hz(void)
{
    return BOARD_SAI4_RX_BCLK_HZ;
}

uint32_t drv_sai4_get_tx_bclk_hz(void)
{
    return BOARD_SAI4_TX_BCLK_HZ;
}
