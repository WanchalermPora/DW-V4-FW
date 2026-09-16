/* =============================================================================
 * FreeRTOSConfig.h — DynaWatch V4 FreeRTOS configuration — CM33 CORE
 *
 * Target : NXP i.MX RT1186 Cortex-M33 @ 240 MHz
 * Role   : All I/O and housekeeping (ADC, Ethernet, PTP, PMU TX, PQ watch,
 *           security monitor, MCMGR coordination)
 * Tick   : 1000 Hz (1 ms)
 * Heap   : heap_4, 128 KB in OCRAM1 (CM33-private region below g_shared)
 *
 * Task priority map (highest = most urgent):
 *   7  Timer daemon
 *   6  task_ads131m08    — SPI DMA ADC acquisition (DRDY IRQ-woken)
 *   5  task_ptp_tc       — IEEE 1588v2 Transparent Clock
 *   4  task_netc         — NETC Ethernet + TLS 1.3
 *   3  pmu_server_task   — C37.118.2 / STTP frame TX 100 fps
 *   2  task_rms_sentinel — PQ watchdog 10 Hz
 *   1  task_security_monitor — ELE health 1 Hz
 *   0  Idle
 * =============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ─── Scheduler behaviour ──────────────────────────────────────────────────── */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
/* M33 has CLZ; port-optimised selection still valid */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* ─── Tick ─────────────────────────────────────────────────────────────────── */
#define configCPU_CLOCK_HZ                      240000000UL
#define configTICK_RATE_HZ                      1000U

/* ─── Task priorities ──────────────────────────────────────────────────────── */
#define configMAX_PRIORITIES                    8U
#define configIDLE_TASK_STACK_DEPTH             256U
#define configMINIMAL_STACK_SIZE                128U

/* ─── Heap ─────────────────────────────────────────────────────────────────── */
/* 128 KB in OCRAM1 — CM33 private region below g_shared at 0x20240000.
 * NETC task stack (2 KB) and pmu_server_task stack (configurable via Kconfig)
 * are the dominant consumers; 128 KB is sufficient for Phase 4–6.          */
#define configTOTAL_HEAP_SIZE                   (128U * 1024U)

/* ─── Task name length ─────────────────────────────────────────────────────── */
#define configMAX_TASK_NAME_LEN                 16U

/* ─── Trace / stats ────────────────────────────────────────────────────────── */
#define configUSE_TRACE_FACILITY                1
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* ─── Mutexes and semaphores ───────────────────────────────────────────────── */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0

/* ─── Timers ───────────────────────────────────────────────────────────────── */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1U)
#define configTIMER_QUEUE_LENGTH                8U
#define configTIMER_TASK_STACK_DEPTH            256U

/* ─── Hooks ────────────────────────────────────────────────────────────────── */
#define configUSE_IDLE_HOOK                     1   /* __WFI in idle */
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configCHECK_FOR_STACK_OVERFLOW          2   /* fill + check on switch */

/* ─── Cortex-M33 interrupt priorities ─────────────────────────────────────── */
/* RT1186 NVIC_PRIO_BITS = 4; same convention as CM7.
 * CM33 owns: LPSPI1 DRDY IRQ (SPI DMA done), NETC IRQ, ELE S3MU IRQ.
 * All must be at or below configMAX_SYSCALL_INTERRUPT_PRIORITY.            */
#ifdef __NVIC_PRIO_BITS
#  define configPRIO_BITS __NVIC_PRIO_BITS
#else
#  define configPRIO_BITS 4U
#endif
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY     15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5U
#define configKERNEL_INTERRUPT_PRIORITY         \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))

/* ─── API inclusions ───────────────────────────────────────────────────────── */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1

/* ─── Assert ───────────────────────────────────────────────────────────────── */
extern void vAssertCalled(const char *file, int line);
#define configASSERT(x) \
    do { if ((x) == 0) vAssertCalled(__FILE__, __LINE__); } while (0)

#endif /* FREERTOS_CONFIG_H */
