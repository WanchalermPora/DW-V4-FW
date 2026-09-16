/* =============================================================================
 * app_main_cm33.c — DynaWatch V4 CM33 application entry point
 *
 * Execution sequence (CM33 boots first, holds CM7 in reset):
 *
 *   1. BOARD_InitBootClocks()    — PLL: CM33=240 MHz, CM7=600 MHz, OCRAM clocks
 *   2. BOARD_InitDebugConsole()  — LPUART1 115200 (J-Link CDC → VS Code)
 *   3. MCMGR_Init()              — Multicore manager init (must be before tasks)
 *   4. security_init()           — Six-phase ELE security bootstrap
 *   5. ipc_shared_init()         — Zero g_shared in OCRAM1, install linker sym
 *   6. BOARD_InitHardware()      — Pin mux for LPSPI1, NETC, ENET_QOS, GPIOs
 *   7. pow_buf_init()            — PoW ring and double-buffer housekeeping
 *   8. Create CM33 tasks         — Highest priority first
 *   9. Signal CM7                — MCMGR_StartCore releases CM7 from reset
 *  10. vTaskStartScheduler()     — CM33 FreeRTOS begins
 *
 * CM33 owns ALL I/O:
 *   task_ads131m08   pri 6 — SPI DMA ADC at 8000 sps, writes g_shared.pow
 *   task_ptp_tc      pri 5 — IEEE 1588v2 Transparent Clock (NETC timestamps)
 *   task_netc        pri 4 — NETC Ethernet stack, TLS 1.3 sockets
 *   pmu_server_task  pri 3 — C37.118.2 / STTP frame publisher 100 fps
 *   task_rms_sentinel pri 2 — RMS/PQ watchdog, notifies CM7 via MCMGR
 *   task_security_monitor pri 1 — ELE health, cert expiry countdown
 *
 * Phase 4 stub: all tasks print identity then block; no real I/O.
 * =============================================================================
 */

#include "board.h"
#include "fsl_debug_console.h"
#include "mcmgr.h"
#include "FreeRTOS.h"
#include "task.h"

#include "security_init.h"
#include "ipc_shared.h"
#include "dw_tasks_cm33.h"

/* ─── Shared task handles ──────────────────────────────────────────────────── */
TaskHandle_t h_pmu_server_task = NULL;

/* ─── Static task handles (CM33-local, not exposed to CM7) ────────────────── */
static TaskHandle_t s_ads_task     = NULL;
static TaskHandle_t s_ptp_task     = NULL;
static TaskHandle_t s_netc_task    = NULL;
static TaskHandle_t s_rms_task     = NULL;
static TaskHandle_t s_sec_task     = NULL;

/* --------------------------------------------------------------------------- */
/*  FreeRTOS hooks                                                             */
/* --------------------------------------------------------------------------- */

void vApplicationMallocFailedHook(void)
{
    PRINTF("[CM33] FATAL: heap allocation failed — system halted\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) { __asm volatile("wfi"); }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    PRINTF("[CM33] FATAL: stack overflow in task '%s' — system halted\r\n",
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
    PRINTF("[CM33] ASSERT failed: %s:%d\r\n", file, line);
    taskDISABLE_INTERRUPTS();
    for (;;) { __asm volatile("wfi"); }
}

/* --------------------------------------------------------------------------- */
/*  IPC shared memory initialisation                                           */
/* --------------------------------------------------------------------------- */

static void ipc_shared_init(void)
{
    /* g_shared is placed at OCRAM1 0x20240000 by the linker.
     * Zero it here before releasing CM7 so CM7 never sees stale values.    */
    memset(&g_shared, 0, sizeof(g_shared));
    /* Ensure writes are visible to CM7 before MCMGR_StartCore() fires.     */
    IPC_WRITE_BARRIER();
    PRINTF("[CM33] IPC: g_shared zeroed (%u bytes @ 0x%08X)\r\n",
           (unsigned)sizeof(g_shared), (unsigned)(uintptr_t)&g_shared);
}

/* --------------------------------------------------------------------------- */
/*  Main entry point (called by startup after C runtime init)                 */
/* --------------------------------------------------------------------------- */

int main(void)
{
    BaseType_t rc;

    /* ── 1. Clocks ──────────────────────────────────────────────────────── */
    BOARD_InitBootClocks();

    /* ── 2. Debug console (LPUART1, 115200 baud) ────────────────────────── */
    BOARD_InitDebugConsole();
    PRINTF("\r\n");
    PRINTF("╔══════════════════════════════════════════════════════════╗\r\n");
    PRINTF("║  DynaWatch V4  —  CM33 I/O Core  (Phase 4 stub build)  ║\r\n");
    PRINTF("╚══════════════════════════════════════════════════════════╝\r\n");
    PRINTF("[CM33] CPU clock : 240 MHz\r\n");
    PRINTF("[CM33] FreeRTOS  : " tskKERNEL_VERSION_NUMBER "\r\n");

    /* ── 3. Multicore Manager ───────────────────────────────────────────── */
    MCMGR_Init();
    PRINTF("[CM33] MCMGR     : initialised\r\n");

    /* ── 4. Security bootstrap ──────────────────────────────────────────── */
    PRINTF("[CM33] Security  : running six-phase ELE bootstrap...\r\n");
    security_status_t sec_status = security_init();
    switch (sec_status) {
        case kSecurity_Success:
            PRINTF("[CM33] Security  : OK — OEM_CLOSED, ELE verified\r\n");
            break;
        case kSecurity_DevMode:
            PRINTF("[CM33] Security  : DEV_BYPASS active — skipping ELE\r\n");
            break;
        case kSecurity_WarnDevBoard:
            PRINTF("[CM33] Security  : WARNING — OEM_OPEN lifecycle (dev board)\r\n");
            PRINTF("[CM33]             Factory calibration loaded; production "
                   "unit must use OEM_CLOSED.\r\n");
            break;
        default:
            /* security_init() already halted on fatal errors — unreachable */
            PRINTF("[CM33] Security  : fatal (code %d) — should not reach here\r\n",
                   (int)sec_status);
            for (;;) { __asm volatile("wfi"); }
    }

    /* ── 5. IPC shared memory ───────────────────────────────────────────── */
    ipc_shared_init();

    /* ── 6. Hardware pin mux ────────────────────────────────────────────── */
    BOARD_InitHardware();
    PRINTF("[CM33] Hardware  : pin mux configured\r\n");

    /* ── 7. PoW buffer ──────────────────────────────────────────────────── */
    /* pow_buf_init() is a lightweight ring housekeeping call — no alloc. */
    /* pow_buf_init(); */  /* Phase 5: uncomment when pow_buf component exists */
    PRINTF("[CM33] PoW ring  : ready (stub — pow_buf_init deferred to Phase 5)\r\n");

    /* ── 8. Create CM33 tasks (highest priority first) ──────────────────── */
    PRINTF("[CM33] Tasks     : creating...\r\n");

    rc = xTaskCreate(task_ads131m08, "ads131m08",
                     CM33_TASK_ADS131M08_STACK, NULL,
                     CM33_PRI_ADS131M08, &s_ads_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_ptp_tc, "ptp_tc",
                     CM33_TASK_PTP_TC_STACK, NULL,
                     CM33_PRI_PTP_TC, &s_ptp_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_netc, "netc",
                     CM33_TASK_NETC_STACK, NULL,
                     CM33_PRI_NETC, &s_netc_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(pmu_server_task, "pmu_server",
                     CM33_TASK_PMU_SERVER_STACK, NULL,
                     CM33_PRI_PMU_SERVER, &h_pmu_server_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_rms_sentinel, "rms_sentinel",
                     CM33_TASK_RMS_STACK, NULL,
                     CM33_PRI_RMS, &s_rms_task);
    configASSERT(rc == pdPASS);

    rc = xTaskCreate(task_security_monitor, "sec_monitor",
                     CM33_TASK_SEC_MONITOR_STACK, NULL,
                     CM33_PRI_SEC_MONITOR, &s_sec_task);
    configASSERT(rc == pdPASS);

    PRINTF("[CM33] Tasks     : 6 tasks created\r\n");
    PRINTF("[CM33] Heap free : %u B before scheduler\r\n",
           (unsigned)xPortGetFreeHeapSize());

    /* ── 9. Release CM7 from reset ──────────────────────────────────────── */
    /* CM7 image is linked at its own flash region (APP_CM7_VECTOR_TABLE_ADDRESS).
     * MCMGR_StartCore() copies the vector table address into the SRC_GPR
     * that CM7's boot ROM reads, then deasserts CM7 reset.                 */
    PRINTF("[CM33] MCMGR     : starting CM7 core...\r\n");
#if defined(APP_CM7_VECTOR_TABLE_ADDRESS)
    mcmgr_status_t mcmgr_rc = MCMGR_StartCore(kMCMGR_Core1,
                                                (void *)(APP_CM7_VECTOR_TABLE_ADDRESS),
                                                0,
                                                kMCMGR_Start_Synchronous);
    if (mcmgr_rc != kStatus_MCMGR_Success) {
        PRINTF("[CM33] FATAL: MCMGR_StartCore failed (%d)\r\n", (int)mcmgr_rc);
        for (;;) { __asm volatile("wfi"); }
    }
#else
    PRINTF("[CM33] WARNING: APP_CM7_VECTOR_TABLE_ADDRESS not defined — "
           "CM7 not started (Phase 4 single-core test mode)\r\n");
#endif
    PRINTF("[CM33] MCMGR     : CM7 released\r\n");

    /* ── 10. Start FreeRTOS scheduler ───────────────────────────────────── */
    PRINTF("[CM33] Scheduler : starting — output continues from tasks\r\n\r\n");
    vTaskStartScheduler();

    /* Should never reach here */
    PRINTF("[CM33] FATAL: scheduler returned — system halted\r\n");
    for (;;) { __asm volatile("wfi"); }
}
