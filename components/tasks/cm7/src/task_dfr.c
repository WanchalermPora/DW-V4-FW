/* =============================================================================
 * task_dfr.c — Digital Fault Recorder task (STUB)
 *
 * Target behaviour (Phase 7):
 *   - Blocked on ulTaskNotifyTake() until h_dfr_task is notified by
 *     task_rms_sentinel (threshold breach) or task_pqm_sem (PQ event)
 *   - On trigger: capture pre-fault window from PoW double-buffer (1888 samples)
 *     + post-fault window from PoW ring (configurable 0.5–2 s)
 *   - Encode waveform as IEC 60255-24 COMTRADE (IEEE C37.111-2013)
 *     — CFG + DAT files, 16-bit integer, 8000 sps
 *   - Write to µSD (via FatFS) and/or transmit via STTP (Phase 8)
 *   - Minimum trigger-to-record latency: < 2 ms
 *
 * Phase 4 stub: prints identity then blocks waiting for task notification.
 * =============================================================================
 */

#include "dw_tasks_cm7.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

void task_dfr(void *pvParam)
{
    (void)pvParam;
    PRINTF("[DFR   ] Task started — IEC 60255-24 Digital Fault Recorder stub\r\n");
    PRINTF("[DFR   ] Target: COMTRADE C37.111-2013, 8000 sps, pre+post fault\r\n");
    PRINTF("[DFR   ] Blocking on task notification from rms_sentinel / pqm.\r\n");

    for (;;) {
        /* Phase 7: ulTaskNotifyTake(pdTRUE, portMAX_DELAY) */
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}
