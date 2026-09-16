/* =============================================================================
 * task_ptp_tc.c — IEEE 1588v2 PTP Transparent Clock task (STUB)
 *
 * Target behaviour (Phase 6):
 *   - Operate RT1186 NETC as a Two-Step End-to-End Transparent Clock
 *   - Correct residence time in Sync and Follow_Up messages
 *   - Use NETC hardware IEEE 1588 timestamping (no software path)
 *   - Sync local stimer to GNSS 1PPS (GPIO6.10) for TAI alignment
 *   - Peer-delay measurement on all three NETC ports
 *   - Accuracy target: ±50 ns residence-time correction (IEC 61850-9-3)
 *
 * Phase 4 stub: prints identity then blocks.
 * =============================================================================
 */

#include "dw_tasks_cm33.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

void task_ptp_tc(void *pvParam)
{
    (void)pvParam;
    PRINTF("[PTP_TC] Task started — IEEE 1588v2 Transparent Clock stub\r\n");
    PRINTF("[PTP_TC] Target: E2E TC, NETC HW timestamps, ±50 ns correction\r\n");
    PRINTF("[PTP_TC] Phase 6 will replace this stub with NETC PTP driver.\r\n");

    for (;;) {
        /* Phase 6: event-driven on NETC Rx timestamp interrupt */
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}
