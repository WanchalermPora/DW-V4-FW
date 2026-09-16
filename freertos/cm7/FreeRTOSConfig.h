/* =============================================================================
 * FreeRTOSConfig.h — DynaWatch V4 FreeRTOS configuration — CM7 CORE
 *
 * Target : NXP i.MX RT1186 Cortex-M7 @ 600 MHz
 * Role   : Computation only (TWLS PMU, PQM FFT, flicker IIR, DFR, energy)
 * Tick   : 1000 Hz (1 ms)
 * Heap   : heap_4, 256 KB in OCRAM2 (0x20200000 region, CM7-private)
 *
 * Task priority map (highest = most urgent):
 *   7  Timer daemon
 *   5  task_twls_pmu_p   — P-class synchrophasor 100 fps
 *   4  task_twls_pmu_m   — M-class + ROCOF 100 fps
 *   3  task_flickermeter — IEC 61000-4-15 IIR 8000 sps
 *   3  task_pqm_sem      — IEC 61000-4-30 FFT 5 fps
 *   2  task_dfr          — IEC 60255-24 COMTRADE on-event
 *   1  task_energy_meter — IEC 62053-22 1 Hz accumulation
 *   0  Idle
 * =============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ─── Scheduler behaviour ──────────────────────────────────────────────────── */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1   /* CLZ instruction (M7) */

/* ─── Tick ─────────────────────────────────────────────────────────────────── */
#define configCPU_CLOCK_HZ                      600000000UL
#define configTICK_RATE_HZ                      1000U

/* ─── Task priorities ──────────────────────────────────────────────────────── */
#define configMAX_PRIORITIES                    8U
#define configIDLE_TASK_STACK_DEPTH             256U
#define configMINIMAL_STACK_SIZE                128U

/* ─── Heap ─────────────────────────────────────────────────────────────────── */
/* 256 KB in OCRAM2 — CM7-private, no CM33 contention */
#define configTOTAL_HEAP_SIZE                   (256U * 1024U)

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

/* ─── Cortex-M7 interrupt priorities ──────────────────────────────────────── */
/* RT1186 NVIC_PRIO_BITS = 4; priorities 0–15, lower = higher urgency.
 * FreeRTOS requires SysTick and PendSV at lowest configurable priority.
 * ISRs that call FromISR() API must be at or below configMAX_SYSCALL. */
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
