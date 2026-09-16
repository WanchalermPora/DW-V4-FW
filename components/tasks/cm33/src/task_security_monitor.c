/* =============================================================================
 * task_security_monitor.c — CM33 ELE security health monitor (STUB)
 *
 * Target behaviour (Phase 10 — Security Polishing):
 *   - 1 Hz: query ELE firmware status via ELE_GetFirmwareStatus()
 *   - Monitor device certificate expiry (ECDSA P-256, 1-year lifetime)
 *     → trigger EdgeLock 2GO cloud renewal 30 days before expiry
 *   - Monitor ELE session validity; re-open if session ID becomes stale
 *   - Publish ELE health status over MQTT QoS 0 every 60 s (Phase 8)
 *   - On ELE fault: set PQ_ELE_FAULT_BIT in g_shared.ctrl.rms_pq_bits
 *     and trigger graceful shutdown of pmu_server_task
 *
 * Phase 4 stub: prints identity then blocks.
 * =============================================================================
 */

#include "dw_tasks_cm33.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

void task_security_monitor(void *pvParam)
{
    (void)pvParam;
    PRINTF("[SEC_MON] Task started — ELE health monitor stub (CM33)\r\n");
    PRINTF("[SEC_MON] Target: 1 Hz ELE poll, cert expiry watch, MQTT health pub\r\n");
    PRINTF("[SEC_MON] Phase 10 will replace this stub with ELE health checks.\r\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}
