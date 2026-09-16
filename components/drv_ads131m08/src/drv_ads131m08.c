/* =============================================================================
 * drv_ads131m08.c — DynaWatch V4 ADS131M08 ADC driver (STUB)
 *
 * Phase 5 stub: init sequence is documented and partially implemented;
 * the actual LPSPI1 transactions and DMA wiring are commented stubs.
 *
 * Phase 6 implementation checklist:
 *   [ ] LPSPI1 init: CLOCK_SetRootClock already done in BOARD_InitBootClocks;
 *       call LPSPI_MasterGetDefaultConfig, set baud=20 MHz, CPOL=1, CPHA=1
 *   [ ] DMA: allocate two eDMA channels (TX ping-pong, RX ping-pong)
 *       AT_NONCACHEABLE_SECTION_ALIGN(uint8_t rx_buf[2][27], 32)
 *   [ ] DRDY ISR: GPIO1_Combined_0_15_IRQHandler or similar
 *       → DMA_SubmitTransfer & DMA_StartTransfer (TX=CMD_NULL×9, RX=27B)
 *   [ ] DMA-complete ISR: s_decode_frame() → ipc_shared pow_ring push
 *       → IPC_WRITE_BARRIER() → g_shared.ctrl.ads_seq++
 *   [ ] CRC-16 (CCITT): verify last 2 bytes of response against STATUS+DATA
 *
 * SPI frame decode (27 bytes, MSB-first, 24-bit words):
 *   Byte  0- 2: STATUS word (ignore for now)
 *   Byte  3- 5: CH0 (24-bit two's complement)
 *   Byte  6- 8: CH1
 *   ...
 *   Byte 24-26: CH7
 *   (No CRC bytes in Phase 5; Phase 6 enables CRC in MODE register)
 *
 * Sign-extension of 24-bit ADC word to int32_t:
 *   raw = (int32_t)((uint32_t)(b[0]<<16 | b[1]<<8 | b[2]) << 8) >> 8;
 * =============================================================================
 */

#include "drv_ads131m08.h"
#include "ipc_shared.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

/* ─── External IPC shared memory ──────────────────────────────────────── */
extern ipc_shared_t g_shared;

/* ─── Module state ─────────────────────────────────────────────────────── */
static volatile uint32_t s_frame_count = 0U;
static uint8_t           s_gain[DRV_ADS131_CHANNELS] = {0}; /* kAds131Gain1 × 8 */

/* ─── Internal helpers (Phase 6 stubs) ────────────────────────────────── */

/** Blocking SPI transaction — Phase 5 stub (returns zeroed buffer). */
static status_t s_spi_transceive(const uint8_t *tx, uint8_t *rx, size_t len)
{
    (void)tx;
    if (rx != NULL) { __builtin_memset(rx, 0, len); }
    /* Phase 6: LPSPI_MasterTransferBlocking(LPSPI1, &xfer); */
    return kStatus_Success;
}

/** Decode 27-byte SPI response into ads131_frame_t. */
static void s_decode_frame(const uint8_t *raw, ads131_frame_t *out)
{
    out->status = (uint16_t)((raw[0] << 8) | raw[1]);
    out->crc    = (uint16_t)((raw[25] << 8) | raw[26]); /* Phase 6: verify */
    for (uint32_t ch = 0U; ch < DRV_ADS131_CHANNELS; ch++)
    {
        const uint8_t *b = &raw[3U + ch * 3U];
        /* Sign-extend 24-bit → int32_t */
        out->ch[ch] = (int32_t)((uint32_t)(((uint32_t)b[0] << 16)
                               | ((uint32_t)b[1] <<  8)
                               | (uint32_t)b[2]) << 8) >> 8;
    }
}

/* ─── Public API ───────────────────────────────────────────────────────── */

status_t drv_ads131m08_init(void)
{
    PRINTF("[ADS131] drv_ads131m08_init: SCLK=%lu MHz, OSR=%u, fs=%u sps\r\n",
           DRV_ADS131_SCLK_HZ / 1000000UL,
           DRV_ADS131_OSR,
           DRV_ADS131_SAMPLE_RATE_HZ);
    PRINTF("[ADS131] Frame = %u bytes = (1 status + 8 ch) × 3 B\r\n",
           DRV_ADS131_FRAME_BYTES);
    PRINTF("[ADS131] CLKIN = SAI4 RX_BCLK 8.192 MHz → fs = 8192000/(512×2) = 8000 sps ✓\r\n");

    /* ── Step 1: LPSPI1 already clocked (BOARD_InitBootClocks) ──────── */
    /* Phase 6: LPSPI_MasterGetDefaultConfig(&cfg); cfg.baudRate=20e6;   */
    /*          LPSPI_MasterInit(LPSPI1, &cfg, BOARD_LPSPI1_CLK_HZ);     */

    /* ── Step 2: Assert RESET via GPIO ──────────────────────────────── */
    /* Phase 6: GPIO_PinWrite(GPIO1, BOARD_ADS131_RESET_PIN, 0U);        */
    /*          SDK_DelayAtLeastUs(1U, BOARD_CM33_CORE_CLOCK_HZ);        */
    /*          GPIO_PinWrite(GPIO1, BOARD_ADS131_RESET_PIN, 1U);        */

    /* ── Step 3: Wait DRDY (CLKIN received) ─────────────────────────── */
    /* Phase 6: uint32_t timeout = 10000U;                               */
    /*          while (!GPIO_PinRead(GPIO1, BOARD_ADS131_DRDY_PIN))      */
    /*              if (!timeout--) return kStatus_Fail;                 */

    PRINTF("[ADS131] LPSPI1/DMA/DRDY-ISR stubs. Phase 6 implements full pipeline.\r\n");

    /* ── Step 4-5: Configure ADS131M08 registers (stub) ──────────────── */
    /* Phase 6: drv_ads131m08_write_reg(ADS131_REG_CLOCK, 0x0E0EU);     */
    /*   [15:8]=OSR[2:0]=0x07 (OSR=512), [7:0]=XTAL bits, EXTREF=1      */

    return kStatus_Success;
}

status_t drv_ads131m08_set_gain(uint8_t channel, ads131_gain_t gain)
{
    if (channel >= DRV_ADS131_CHANNELS) { return kStatus_InvalidArgument; }
    s_gain[channel] = (uint8_t)gain;

    /* Phase 6: recompute GAIN1/GAIN2 register values and write via SPI  */
    PRINTF("[ADS131] Channel %u gain set to x%u (register write stubbed)\r\n",
           channel, 1U << (uint8_t)gain);
    return kStatus_Success;
}

status_t drv_ads131m08_read_frame(ads131_frame_t *p_frame)
{
    if (p_frame == NULL) { return kStatus_InvalidArgument; }

    uint8_t tx[DRV_ADS131_FRAME_BYTES] = {0};
    uint8_t rx[DRV_ADS131_FRAME_BYTES] = {0};

    /* Phase 5: stub — returns zeroed frame */
    status_t ret = s_spi_transceive(tx, rx, DRV_ADS131_FRAME_BYTES);
    if (ret != kStatus_Success) { return ret; }

    s_decode_frame(rx, p_frame);
    s_frame_count++;
    return kStatus_Success;
}

status_t drv_ads131m08_read_reg(uint8_t reg_addr, uint16_t *p_val)
{
    if (p_val == NULL) { return kStatus_InvalidArgument; }
    uint8_t tx[3U] = {
        (uint8_t)((ADS131_CMD_RREG(reg_addr, 0U) >> 16) & 0xFFU),
        (uint8_t)((ADS131_CMD_RREG(reg_addr, 0U) >>  8) & 0xFFU),
        (uint8_t)( ADS131_CMD_RREG(reg_addr, 0U)        & 0xFFU),
    };
    uint8_t rx[3U] = {0};
    (void)s_spi_transceive(tx, rx, 3U);
    /* Response word is in the next SCLK frame — simplified for stub */
    *p_val = (uint16_t)((rx[1] << 8) | rx[2]);
    return kStatus_Success;
}

status_t drv_ads131m08_write_reg(uint8_t reg_addr, uint16_t val)
{
    uint8_t tx[6U] = {
        (uint8_t)((ADS131_CMD_WREG(reg_addr, 0U) >> 16) & 0xFFU),
        (uint8_t)((ADS131_CMD_WREG(reg_addr, 0U) >>  8) & 0xFFU),
        (uint8_t)( ADS131_CMD_WREG(reg_addr, 0U)        & 0xFFU),
        0U,
        (uint8_t)(val >> 8),
        (uint8_t)(val & 0xFFU),
    };
    return s_spi_transceive(tx, NULL, 6U);
}

uint32_t drv_ads131m08_get_frame_count(void)
{
    return s_frame_count;
}
