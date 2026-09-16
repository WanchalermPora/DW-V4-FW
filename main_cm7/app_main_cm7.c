/* =============================================================================
 * app_main_cm7.c — DynaWatch V4 CM7 application entry point
 *
 * CM7 is held in reset by CM33's boot ROM until CM33 calls
 * MCMGR_StartCore(kMCMGR_Core1, ...).  Once released:
 *
 *   1. BOARD_InitBootClocks() (CM7 PLL already locked by CM33 — NOP or verify)
 *   2. BOARD_InitDebugConsole() — shares LPUART1 with CM33 (Phase 4 raw write;
 *      Phase 5+ will use a FreeRTOS mutex or MCMGR message channel)
 *   3. Wait for g_shared.ctrl.cm33_ready — spin-poll with DMB barrier; CM33
 *      sets this bit after all CM33 tasks are created and the scheduler runs
 *   4. Create CM7 computation tasks (highest priority first)
 *   5. vTaskStartScheduler()
 *
 * CM7 owns ALL number-crunching; it never touches a peripheral register:
 *   task_twls_pmu_p   pri 5 — IEEE C37.118.1 P-class TWLS 100 fps
 *   task_twls_pmu_m   pri 4 — M-class TWLS + IIR ROCOF 100 fps
 *   task_flickermeter pri 3 — IEC 61000-4-15 Pst/Plt IIR 8000 sps
 *   task_pqm_sem      pri 3 — IEC 61000-4-30 FFT H1–H50 5 fps
 *   task_dfr          pri 2 — IEC 60255-24 COMTRADE encoder (on-event)
 *   task_energy_meter pri 1 — IEC 62053-22 Class 0.2S accumulation 1 Hz
 *
 * Input  : g_shared.pow   (OCRAM1) — written by CM33 task_ads131m08
 * Output : g_shared.pmu_p, g_shared.pmu_m — read and transmitted by CM33
 *
 * Phase 4 stub: all tasks print identity then block; no real computation.
 * =============================================================================
 */

#include "board.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

#include "ipc_shared.h"
#include "dw_tasks_cm7.h"

/* ─── Shared task handles ──────────────────────────────────────────────────── */
TaskHandle_t h_twls_pmu_p_task = NULL;
TaskHandle_t h_twls_pmu_m_task = NULL;
TaskHandle_t h_dfr_task        = NULL;

/* ─── Static task handles (CM7-local) ─────────────────────────────────────── */
static TaskHandle_t s_flicker_task = NULL;
static TaskHandle_t s_pqm_task     = NULL;
static TaskHandle_t s_energy_task  = NULL;

/* ─── cm33_ready poll timeout (ms) ────────────────────────────────────────── */
#define CM33_READY_TIMEOUT_MS  5000U

/* --------------------------------------------------------------------------- */
/*  FreeRTOS hooks                                                             */
/* --------------------------------------------------------------------------- */

void vApplicationMallocFailedHook(void)
{
    PRINTF("[CM7] FATAL: heap allocation failed — system halted\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) { __asm volatile("wfi"); }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    PRINTF("[CM7] FATAL: stack overflow in task '%s' — system halted\r\n",
           pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) { __asm volatile("wfi"); }
}

void vApplicationIdleHook(void)
{
    __asm volatile("wfi");
}

void vAssertCalled(const char *file, int line)
{
    PRINTF("[CM7] ASSERT failed: %s:%d\r\n", file, line);
    taskDISABLE_INTERRUPTS();
    for (;;) { __asm volatile("wfi"); }
}

/* --------------------------------------------------------------------------- */
/*  Wait for CM33 ready signal                                                 */
/* --------------------------------------------------------------------------- */

static void wait_for_cm33_ready(void)
{
    uint32_t elapsed_ms = 0U;
    const uint32_t poll_interval_ms = 10U;

    PRINTF("[CM7] Waiting for CM33 ready signal (timeout %u ms)...\r\n",
           CM33_READY_TIMEOUT_MS);

    while (1) {
        IPC_READ_BARRIER();
        if (g_shared.ctrl.cm33_ready != 0U) {
            PRINTF("[CM7] CM33 ready signal received (waited ~%u ms)\r\n",
                   elapsed_ms);
            return;
        }
        /* Busy-wait in small increments — scheduler not yet running.
         * Use DWT cycle counter if available for precise delay.           */
        for (volatile uint32_t i = 0U;
             i < (SystemCoreClock / 1000U * poll_interval_ms / 6U);
             i++) { /* spin */ }
        elapsed_ms += poll_interval_ms;

        if (elapsed_ms >= CM33_READY_TIMEOUT_MS) {
            PRINTF("[CM7] FATAL: CM33 did not signal ready within %u ms\r\n",
                   CM33_READY_TIMEOUT_MS);
            for (;;) { __asm volatile("wfi"); }
        }
    }
}

/* --------------------------------------------------------------------------- */
/*  Main entry point                                                           */
/* --------------------------------------------------------------------------- */

int main(void)
{
    BaseType_t rc;

    /* ── 1. Clocks (CM7 PLL already running; this re-enables the CM7 cache) */
    BOARD_InitBootClocks();

    /* ── 2. Debug console (LPUART1 shared with CM33 — raw in Phase 4) ───── */
    /* Note: CM33 already configured LPUART1.  On CM7 side we just hook     *
     * DbgConsole to the same peripheral; no baud-rate reconfiguration.      */
    BOARD_InitDebugConsole();
    PRINTF("[CM7] DynaWatch V4 — Computation Core — Phase 4 stub build\r\n");
    PRINTF("[CM7] CPU clock : 600 MHz\r\n");
    PRINTF("[CM7] FreeRTOS  : " tskKERNEL_VERSION_NUMBER "\r\n");

    /* ── 3. Wait for CM33 ready ─────────────────────────────────────────── */
    wait_for_cm33_ready();

    /* ── 4. Create CM7 tasks (highest priority first) ───────────────────── */
    PRINTF("[CM7] Tasks     : creating...\r\n");

    rc = xTaskCreate(task_twls_pmu_p, "twls_pmu_p",
                     CM7_TASK_PMU_P_STACK, NULL,
                     CM7_PRI_PMU_P, &h_twls_pmu_p_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_twls_pmu_m, "twls_pmu_m",
                     CM7_TASK_PMU_M_STACK, NULL,
                     CM7_PRI_PMU_M, &h_twls_pmu_m_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_flickermeter, "flickermeter",
                     CM7_TASK_FLICKERMETER_STACK, NULL,
                     CM7_PRI_FLICKERMETER, &s_flicker_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_pqm_sem, "pqm_sem",
                     CM7_TASK_PQM_STACK, NULL,
                     CM7_PRI_PQM, &s_pqm_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_dfr, "dfr",
                     CM7_TASK_DFR_STACK, NULL,
                     CM7_PRI_DFR, &h_dfr_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_energy_meter, "energy_meter",
                     CM7_TASK_ENERGY_STACK, NULL,
                     CM7_PRI_ENERGY, &s_energy_task);
    configASSERT(rc == pdPASS);

    PRINTF("[CM7] Tasks     : 6 tasks created\r\n");
    PRINTF("[CM7] Heap free : %u B before scheduler\r\n",
           (unsigned)xPortGetFreeHeapSize());

    /* ── 5. Start FreeRTOS scheduler ────────────────────────────────────── */
    PRINTF("[CM7] Scheduler : starting — output continues from tasks\r\n\r\n");
    vTaskStartScheduler();

    /* Should never reach here */
    PRINTF("[CM7] FATAL: scheduler returned — system halted\r\n");
    for (;;) { __asm volatile("wfi"); }
}
