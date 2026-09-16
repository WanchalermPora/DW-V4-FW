/* =============================================================================
 * dw_tasks_cm7.h — CM7 computation task declarations
 *
 * CM7 @ 600 MHz handles ALL number-crunching — no peripheral registers,
 * no DMA setup, no socket calls.  All input comes from g_shared.pow (OCRAM1)
 * written by CM33; all output goes to g_shared.pmu_p / pmu_m for CM33 to Tx.
 *
 *  Pri  Task              Rate          Algorithm / Standard
 *  ---  ----------------  ------------  ------------------------------------
 *   5   task_twls_pmu_p   100 fps       IEEE C37.118.1 P-class TWLS
 *   4   task_twls_pmu_m   100 fps       IEEE C37.118.1 M-class TWLS + ROCOF
 *   3   task_flickermeter 8000 sps IIR  IEC 61000-4-15 Pst / Plt
 *   3   task_pqm_sem      5 fps (200ms) IEC 61000-4-30 Class A (FFT H1–H50)
 *   2   task_dfr          on-event      IEC 60255-24 COMTRADE encoding
 *   1   task_energy_meter 1 Hz          IEC 62053-22 Class 0.2S accumulation
 *
 * CM7 waits on g_shared.ctrl.cm33_ready before entering any task.
 * All tasks poll or block on g_shared.ctrl.ads_seq for new ADC data.
 * =============================================================================
 */

#ifndef DW_TASKS_CM7_H
#define DW_TASKS_CM7_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Stack sizes (32-bit words) */
#define CM7_TASK_PMU_P_STACK        512U
#define CM7_TASK_PMU_M_STACK        512U
#define CM7_TASK_FLICKERMETER_STACK 512U
#define CM7_TASK_PQM_STACK         1024U
#define CM7_TASK_DFR_STACK         1024U
#define CM7_TASK_ENERGY_STACK       512U

/* Priorities on CM7 FreeRTOS instance */
#define CM7_PRI_PMU_P         5U
#define CM7_PRI_PMU_M         4U
#define CM7_PRI_FLICKERMETER  3U
#define CM7_PRI_PQM           3U
#define CM7_PRI_DFR           2U
#define CM7_PRI_ENERGY        1U

/* Task entry points */
void task_twls_pmu_p(void *pvParam);    /* PMU P-class TWLS                  */
void task_twls_pmu_m(void *pvParam);    /* PMU M-class TWLS + IIR ROCOF       */
void task_flickermeter(void *pvParam);  /* IEC 61000-4-15 flicker IIR         */
void task_pqm_sem(void *pvParam);       /* IEC 61000-4-30 FFT PQ analysis     */
void task_dfr(void *pvParam);           /* IEC 60255-24 COMTRADE encoder      */
void task_energy_meter(void *pvParam);  /* IEC 62053-22 energy accumulation   */

/* Shared task handles */
extern TaskHandle_t h_twls_pmu_p_task;
extern TaskHandle_t h_twls_pmu_m_task;
extern TaskHandle_t h_dfr_task;

#ifdef __cplusplus
}
#endif

#endif /* DW_TASKS_CM7_H */
