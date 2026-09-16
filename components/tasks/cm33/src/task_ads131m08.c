/* =============================================================================
 * task_ads131m08.c — ADS131M08-Q1 ADC acquisition task
 *
 * Target behaviour (Phase 6):
 *   - drv_ads131m08_init() configures LPSPI1 @ 20 MHz + DMA + DRDY ISR
 *   - drv_sai4_start() asserts RX_BCLK 8.192 MHz → ADS131 CLKIN
 *   - drv_stimer_init() starts 64-bit tick counter on TX_BCLK 32.768 MHz
 *   - Task blocks on ulTaskNotifyTake from DMA-complete ISR
 *   - DMA ISR: decode 27-byte frame, push to ipc_shared pow_ring,
 *              set cm33_ready after first successful frame
 *
 * Phase 5 stub: initialises drivers, signals cm33_ready, then polls.
 * =============================================================================
 */

#include "dw_tasks_cm33.h"
#include "ipc_shared.h"
#include "drv_ads131m08.h"
#include "drv_sai.h"
#include "drv_stimer.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"

void task_ads131m08(void *pvParam)
{
    (void)pvParam;
    PRINTF("[ADS131] Task started — ADS131M08-Q1 acquisition (CM33)\r\n");

    /* ── Phase 5: Initialise peripheral driver stack ──────────────────── */
    /* SAI4 must start first to assert RX_BCLK (ADS131 CLKIN) */
    (void)drv_sai4_init();
    (void)drv_sai4_start();

    /* ADS131M08 init after CLKIN is running */
    status_t adc_status = drv_ads131m08_init();
    if (adc_status != kStatus_Success)
    {
        PRINTF("[ADS131] ERROR: drv_ads131m08_init failed (%d) — halting task\r\n",
               (int)adc_status);
        vTaskSuspend(NULL);
    }

    /* STIMER starts after SAI4 TX_BCLK (32.768 MHz reference) is active */
    (void)drv_stimer_init();

    /* Signal CM7 that CM33 peripheral stack is initialised.
     * Phase 6: move this after first successful DMA frame is received.   */
    IPC_WRITE_BARRIER();
    g_shared.ctrl.cm33_ready = 1U;
    IPC_WRITE_BARRIER();
    PRINTF("[ADS131] IPC: cm33_ready set — CM7 may proceed\r\n");

    for (;;) {
        /* Phase 6: ulTaskNotifyTake(pdTRUE, portMAX_DELAY); from DMA ISR */
        vTaskDelay(pdMS_TO_TICKS(125U));   /* 125 ms placeholder (8 samples) */
    }
}
