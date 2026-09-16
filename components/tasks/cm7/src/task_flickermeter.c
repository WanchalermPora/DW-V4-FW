/* =============================================================================
 * task_flickermeter.c — IEC 61000-4-15 Flicker Meter task (STUB)
 *
 * Target behaviour (Phase 7):
 *   - Running IIR filter bank on Va, Vb, Vc channels at 8000 sps
 *   - IEC 61000-4-15 Ed. 2 chain: demodulator → weighting filter →
 *     squaring → 1st-order smoothing → online Pst accumulator
 *   - Pst (10-minute) and Plt (2-hour) rolling values per phase
 *   - PQ_FLICKER_BIT set when Pst > 1.0 (IEC 61000-3-3 limit)
 *   - MQTT publish of Pst/Plt at end of each 10-minute window (Phase 8)
 *
 * Phase 4 stub: prints identity then blocks.
 * =============================================================================
 */

#include "dw_tasks_cm7.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

void task_flickermeter(void *pvParam)
{
    (void)pvParam;
    PRINTF("[FLKR  ] Task started — IEC 61000-4-15 Flicker Meter stub\r\n");
    PRINTF("[FLKR  ] Target: Pst (10 min) + Plt (2 hr), 3-phase, 8000 sps IIR\r\n");
    PRINTF("[FLKR  ] Phase 7 will replace this stub with flicker filter chain.\r\n");

    for (;;) {
        /* Phase 7: driven by PoW ring notification at 8000 sps */
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}
