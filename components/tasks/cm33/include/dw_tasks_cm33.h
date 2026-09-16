/* =============================================================================
 * dw_tasks_cm33.h — CM33 I/O and housekeeping task declarations
 *
 * CM33 @ 240 MHz owns ALL I/O and housekeeping:
 *
 *  Pri  Task              Rate            Responsibility
 *  ---  ----------------  --------------  ------------------------------------
 *   6   task_ads131m08    8000 sps ISR    SPI/DMA ADC → PoW ring (OCRAM1)
 *   5   task_ptp_tc       per-packet      IEEE 1588v2 TC, NETC timestamps
 *   4   task_netc         event-driven    Ethernet sockets + TLS 1.3
 *   3   pmu_server_task   100 fps         C37.118.2 + STTP frame TX
 *   2   task_rms_sentinel 10 Hz           PQ watchdog, notifies CM7 via MCMGR
 *   1   security_monitor  1 Hz            ELE health, cert expiry countdown
 *
 * CM33 starts first, runs security_init(), zeroes g_shared,
 * then calls MCMGR_StartCore(kMCMGR_Core1, ...) to release CM7.
 * =============================================================================
 */

#ifndef DW_TASKS_CM33_H
#define DW_TASKS_CM33_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Stack sizes (32-bit words) */
#define CM33_TASK_ADS131M08_STACK    512U
#define CM33_TASK_PTP_TC_STACK       512U
#define CM33_TASK_NETC_STACK        2048U
#define CM33_TASK_PMU_SERVER_STACK   CONFIG_APP_PMU_SERVER_STACK_WORDS
#define CM33_TASK_RMS_STACK          256U
#define CM33_TASK_SEC_MONITOR_STACK  256U

/* Priorities on CM33 FreeRTOS instance */
#define CM33_PRI_ADS131M08    6U
#define CM33_PRI_PTP_TC       5U
#define CM33_PRI_NETC         4U
#define CM33_PRI_PMU_SERVER   3U
#define CM33_PRI_RMS          2U
#define CM33_PRI_SEC_MONITOR  1U

/* Task entry points */
void task_ads131m08(void *pvParam);     /* SPI DMA acquisition, DRDY IRQ     */
void task_ptp_tc(void *pvParam);        /* IEEE 1588v2 Transparent Clock      */
void task_netc(void *pvParam);          /* NETC Ethernet + TLS 1.3            */
void pmu_server_task(void *pvParam);    /* C37.118.2 / STTP publisher         */
void task_rms_sentinel(void *pvParam);  /* RMS watchdog, PQ event detector    */
void task_security_monitor(void *pvParam); /* ELE health, cert countdown      */

/* Shared task handles */
extern TaskHandle_t h_pmu_server_task;

#ifdef __cplusplus
}
#endif

#endif /* DW_TASKS_CM33_H */
