/* =============================================================================
 * drv_audiopll.c — DynaWatch V4 AudioPLL runtime steering (STUB)
 *
 * Phase 4/5 stub: tracks ppb in RAM, does NOT touch PLL hardware registers.
 *
 * Phase 7 implementation notes:
 *   - AudioPLL NUM register: CCM_ANALOG_AUDIO_PLL1_NUMERATORn (offset 0x14)
 *   - Write sequence (NXP AN13391):
 *       1. CLOCK_SetAudioPllBypass(true)        — bypass PLL, output OSC
 *       2. CCM_ANALOG->AUDIO_PLL1_NUMERATOR = new_num;
 *       3. CLOCK_SetAudioPllBypass(false)       — re-engage PLL
 *       4. Spin until CCM_ANALOG_AUDIO_PLL1_CTRL & PLL_CTRL_POWERUP (≤100 µs)
 *   - The bypass causes a brief (~10 µs) clock glitch on SAI4 MCLK.
 *     Both SAI4 TX and RX must be stopped (or tolerant) during update.
 *     ADS131M08 will see ≤1 extra/missing CLKIN cycle — recoverable.
 *   - Correction granularity: ≈ 24 Hz per NUM LSB (655.360 MHz / 960000 denom)
 * =============================================================================
 */

#include "drv_audiopll.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

/* ─── Module state ─────────────────────────────────────────────────────── */

static int32_t  s_trim_ppb   = 0;   /* last applied correction, ppb      */
static bool     s_init_done  = false;

/* ─── Public API ───────────────────────────────────────────────────────── */

status_t drv_audiopll_init(void)
{
    taskENTER_CRITICAL();
    s_trim_ppb  = 0;
    s_init_done = true;
    taskEXIT_CRITICAL();

    PRINTF("[AudioPLL] drv_audiopll_init: NOM=%lu MHz, max_trim=±%d ppb\r\n",
           DRV_AUDIOPLL_NOM_HZ / 1000000UL, DRV_AUDIOPLL_MAX_PPB);
    PRINTF("[AudioPLL] Phase 7 will implement CCM_ANALOG NUM write.\r\n");

    return kStatus_Success;
}

status_t drv_audiopll_steer_ppb(int32_t ppb)
{
    status_t ret = kStatus_Success;

    /* Clamp */
    if (ppb > DRV_AUDIOPLL_MAX_PPB)
    {
        ppb = DRV_AUDIOPLL_MAX_PPB;
        ret = kStatus_InvalidArgument;
    }
    else if (ppb < -DRV_AUDIOPLL_MAX_PPB)
    {
        ppb = -DRV_AUDIOPLL_MAX_PPB;
        ret = kStatus_InvalidArgument;
    }

    taskENTER_CRITICAL();
    s_trim_ppb = ppb;
    /*
     * Phase 7: compute new_NUM and write CCM_ANALOG_AUDIO_PLL1_NUMERATOR
     *   int64_t delta_num = ((int64_t)294400 * ppb + 500000000LL) / 1000000000LL;
     *   uint32_t new_num  = (uint32_t)(294400LL + delta_num);
     *   CCM_ANALOG->AUDIO_PLL1_NUMERATOR = new_num;
     */
    taskEXIT_CRITICAL();

    return ret;
}

uint32_t drv_audiopll_get_freq_hz(void)
{
    /* f ≈ NOM_HZ × (1 + ppb/1e9) — integer approximation, ±1 Hz */
    int32_t ppb = s_trim_ppb;
    int64_t f   = (int64_t)DRV_AUDIOPLL_NOM_HZ
                + ((int64_t)DRV_AUDIOPLL_NOM_HZ * ppb + 500000000LL) / 1000000000LL;
    return (uint32_t)f;
}

int32_t drv_audiopll_get_trim_ppb(void)
{
    return s_trim_ppb;
}
