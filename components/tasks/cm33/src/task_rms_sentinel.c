/* =============================================================================
 * task_rms_sentinel.c — RMS watchdog and DFR trigger task (STUB)
 *
 * Target behaviour (Phase 7):
 *   - 10 Hz: read per-phase RMS from PoW ring (single-cycle window)
 *   - Compare against configurable thresholds:
 *       • Undervoltage  < 0.85 pu  → set PQ_UV_BIT,  notify h_dfr_task
 *       • Overvoltage   > 1.10 pu  → set PQ_OV_BIT,  notify h_dfr_task
 *       • Overcurrent   > 1.20 pu  → set PQ_OC_BIT,  notify h_dfr_task
 *   - De-bounced event detection (2-cycle confirmation before trigger)
 *   - Publishes live RMS values to a FreeRTOS queue for task_netc MQTT
 *
 * Phase 4 stub: prints identity then blocks.
 * =============================================================================
 */

#include "dw_tasks_cm33.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

void task_rms_sentinel(void *pvParam)
{
    (void)pvParam;
    PRINTF("[RMS   ] Task started — RMS watchdog + DFR trigger stub\r\n");
    PRINTF("[RMS   ] Target: 10 Hz, UV/OV/OC thresholds, notifies DFR on event\r\n");
    PRINTF("[RMS   ] Phase 7 will replace this stub with RMS+threshold logic.\r\n");

    for (;;) {
        /* Phase 7: vTaskDelayUntil() at 10 Hz */
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}
