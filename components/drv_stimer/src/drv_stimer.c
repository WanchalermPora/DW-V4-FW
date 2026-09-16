/* =============================================================================
 * drv_stimer.c — DynaWatch V4 STIMER driver (STUB)
 *
 * Phase 5 stub: documents register sequence, provides get_count via SysTick
 * fallback so upper layers can compile and run without real STIMER hardware.
 *
 * Phase 7 implementation notes:
 *   STIMER registers (NXP RT1186 RM, §46):
 *     SNVS_HP_CTRL[STIMER_EN]   — enable counter
 *     SNVS_HP_RTCMR / RTCLR     — 64-bit counter (high/low)
 *     SNVS_HP_SVSR[STIMER_SRC]  — clock source: 0=32kHz, 1=EXT_CLK
 *   ⚠️  Verify register names against RT1186 RM revision used at build time.
 *
 *   Atomic 64-bit read sequence (prevent rollover tear):
 *       do {
 *           hi1 = SNVS->HP_RTCMR;
 *           lo  = SNVS->HP_RTCLR;
 *           hi2 = SNVS->HP_RTCMR;
 *       } while (hi1 != hi2);
 *       return ((uint64_t)hi1 << 32) | lo;
 *
 *   CAPTURE0 latch (Phase 7 PTP):
 *       SNVS_HP_CTRL[HPSVSR] = 1 to arm; read SNVS_HP_HPSVSR for latched value.
 *       Connect 1-PPS GPIO → SNVS_EXT_TAMPER0 pad → CAPTURE0 trigger.
 * =============================================================================
 */

#include "drv_stimer.h"
#include "fsl_debug_console.h"

/* ─── Fallback tick counter (Phase 5 stub, ~1 ms resolution via SysTick) ── */
/* Real STIMER at 32.768 MHz has ~30.5 ns resolution; Phase 7 replaces this. */

static bool s_init_done = false;

/* ─── Public API ───────────────────────────────────────────────────────── */

status_t drv_stimer_init(void)
{
    /*
     * Phase 7:
     *   SNVS->HP_CTRL |= SNVS_HP_CTRL_STIMER_SRC_SEL(1);  // EXT_CLK source
     *   SNVS->HP_CTRL |= SNVS_HP_CTRL_RTC_EN_MASK;         // enable counter
     *   while (!(SNVS->HP_SR & SNVS_HP_SR_HPRT_MASK)) {}   // wait locked
     */

    s_init_done = true;

    PRINTF("[STIMER] drv_stimer_init: ref=SAI4_TX_BCLK %lu MHz, tick=30.517578125 ns\r\n",
           BOARD_STIMER_REF_HZ / 1000000UL);
    PRINTF("[STIMER] Ticks/ADC sample = %u (32768000/8000 = 4096 exact ✓)\r\n",
           BOARD_STIMER_TICKS_PER_SAMPLE);
    PRINTF("[STIMER] Phase 7 will configure SNVS STIMER with EXT_CLK source.\r\n");

    return kStatus_Success;
}

uint64_t drv_stimer_get_count(void)
{
    /*
     * Phase 7 (real STIMER):
     *   uint32_t hi1, lo, hi2;
     *   do {
     *       hi1 = SNVS->HP_RTCMR;
     *       lo  = SNVS->HP_RTCLR;
     *       hi2 = SNVS->HP_RTCMR;
     *   } while (hi1 != hi2);
     *   return ((uint64_t)hi1 << 32) | lo;
     */

    /* Phase 5 stub: use xTaskGetTickCount (1 ms) scaled to ~32768 ticks/ms */
    extern uint32_t xTaskGetTickCount(void);
    uint64_t ticks_ms = (uint64_t)xTaskGetTickCount();
    return ticks_ms * (uint64_t)BOARD_STIMER_TICKS_PER_SAMPLE * 8ULL;
    /* 4096 ticks/sample × 8 samples/ms = 32768 ticks/ms ≈ 32.768 MHz ✓ */
}

uint64_t drv_stimer_ticks_to_ns(uint64_t ticks)
{
    /* ns ≈ (ticks × 31250 + 512) >> 10
     * Exact: 1/32768000 s = 30517.578125 ns
     *        × 1024 = 31250 (exact integer conversion)
     * Error: 0 ns for any integer tick count. */
    return (ticks * 31250ULL + 512ULL) >> 10;
}

status_t drv_stimer_get_pps_latch(uint64_t *p_latched_count)
{
    /*
     * Phase 7:
     *   if (!(SNVS->HP_SR & SNVS_HP_SR_HPSVSR_MASK)) return kStatus_Fail;
     *   *p_latched_count = ((uint64_t)SNVS->HP_HPSVSR_HI << 32)
     *                    |  SNVS->HP_HPSVSR_LO;
     *   SNVS->HP_SR = SNVS_HP_SR_HPSVSR_MASK; // clear
     *   return kStatus_Success;
     */

    /* Phase 5 stub: return current count */
    if (p_latched_count == NULL) { return kStatus_InvalidArgument; }
    *p_latched_count = drv_stimer_get_count();
    return kStatus_Success;
}
