/* =============================================================================
 * ipc_shared.h — DynaWatch V4 Inter-Core Shared Memory Layout
 *
 * Physical location: OCRAM1  (0x20240000, 512 KB)
 *   Accessible by both CM7 and CM33 with full coherency via LMEM.
 *   The linker scripts for both cores place g_shared at 0x20240000.
 *
 * Ownership rules (no hardware cache on shared OCRAM1):
 *   Writer       Region                   Reader
 *   ----------   -----------------------  ----------
 *   CM33         pow.ring[]               CM7
 *   CM33         pow.dbl[]                CM7
 *   CM7          pmu_p_frame              CM33
 *   CM7          pmu_m_frame              CM33
 *   CM33         ctrl.cm33_ready          CM7
 *   CM7          ctrl.cm7_ready           CM33
 *   CM33         ctrl.ads_seq             CM7   (sequence counter)
 *   CM7          ctrl.pmu_seq             CM33  (sequence counter)
 *
 * Synchronisation:
 *   Sequence counters use __atomic_store / __atomic_load (no lock needed
 *   on Cortex-M because 32-bit aligned stores are single-cycle).
 *   CM33 increments ads_seq after each 8000 sps batch write.
 *   CM7 polls / is notified via MCMGR event when ads_seq changes.
 *
 * Memory barrier:
 *   Writer calls __DSB() after updating data, before incrementing seq.
 *   Reader calls __DMB() before reading data after seq change.
 * =============================================================================
 */

#ifndef IPC_SHARED_H
#define IPC_SHARED_H

#include <stdint.h>
#include <stdatomic.h>

#include "sdkconfig.h"   /* CONFIG_POW_CHANNELS, CONFIG_POW_RING_LEN, etc. */

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * PoW ring + double buffer (written by CM33 task_ads131m08, read by CM7)
 * ---------------------------------------------------------------------------*/
typedef struct {
    int32_t ring[CONFIG_POW_CHANNELS][CONFIG_POW_RING_LEN];
    int32_t dbl [CONFIG_POW_CHANNELS][CONFIG_POW_DOUBLE_LEN];
    uint32_t write_idx;   /* current ring write position (0 … POW_RING_LEN-1) */
    uint32_t epoch;       /* UTC 500 ms epoch counter */
} ipc_pow_buf_t;

/* ---------------------------------------------------------------------------
 * PMU output frame (written by CM7 pmu tasks, read by CM33 pmu_server)
 * One slot per class; updated every 10 ms (100 fps).
 * ---------------------------------------------------------------------------*/
typedef struct {
    float    va_mag, vb_mag, vc_mag;   /* voltage phasor magnitudes (V)       */
    float    va_ang, vb_ang, vc_ang;   /* voltage phasor angles (rad)         */
    float    ia_mag, ib_mag, ic_mag;   /* current phasor magnitudes (A)       */
    float    ia_ang, ib_ang, ic_ang;
    float    freq;                     /* frequency (Hz)                      */
    float    rocof;                    /* ROCOF (Hz/s)                        */
    uint32_t soc;                      /* second-of-century (UTC)             */
    uint32_t fracsec;                  /* fraction-of-second (1/2^32)         */
    uint32_t stat;                     /* C37.118.2 STAT word                 */
} ipc_pmu_frame_t;

/* ---------------------------------------------------------------------------
 * Control / status word (heartbeat + ready flags)
 * ---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t cm33_ready;  /* set by CM33 when I/O init complete    */
    volatile uint32_t cm7_ready;   /* set by CM7 when computation ready      */
    volatile uint32_t ads_seq;     /* incremented by CM33 each ADC batch     */
    volatile uint32_t pmu_p_seq;   /* incremented by CM7 each P-frame        */
    volatile uint32_t pmu_m_seq;   /* incremented by CM7 each M-frame        */
    volatile uint32_t rms_pq_bits; /* PQ event flags (set CM7, read CM33)    */
} ipc_ctrl_t;

/* ---------------------------------------------------------------------------
 * Top-level shared memory structure — placed at OCRAM1 base by both linkers
 * ---------------------------------------------------------------------------*/
typedef struct {
    ipc_pow_buf_t    pow;        /* PoW ring + double buffer                 */
    ipc_pmu_frame_t  pmu_p;     /* Latest P-class synchrophasor frame        */
    ipc_pmu_frame_t  pmu_m;     /* Latest M-class synchrophasor frame        */
    ipc_ctrl_t       ctrl;      /* Ready flags and sequence counters         */
} dw_shared_mem_t;

/*
 * The shared memory instance is declared in the linker script of each core
 * at 0x20240000 (OCRAM1 start).  Both cores link against this header.
 * CM33 zero-initialises it at startup before releasing CM7.
 */
extern dw_shared_mem_t g_shared;

/* ---------------------------------------------------------------------------
 * PQ status bits (ctrl.rms_pq_bits) — written CM7, read CM33 for MQTT
 * ---------------------------------------------------------------------------*/
#define PQ_UV_BIT          (1U << 0)   /* Undervoltage   < 0.85 pu           */
#define PQ_OV_BIT          (1U << 1)   /* Overvoltage    > 1.10 pu           */
#define PQ_OC_BIT          (1U << 2)   /* Overcurrent    > 1.20 pu           */
#define PQ_FREQ_DEV_BIT    (1U << 3)   /* Freq deviation > threshold         */
#define PQ_FLICKER_BIT     (1U << 4)   /* Pst > 1.0 (IEC 61000-3-3)          */
#define PQ_DIP_BIT         (1U << 5)   /* Voltage dip event                  */
#define PQ_SWELL_BIT       (1U << 6)   /* Voltage swell event                */

/* ---------------------------------------------------------------------------
 * Convenience memory barriers for producers / consumers
 * ---------------------------------------------------------------------------*/
#define IPC_WRITE_BARRIER()   __asm volatile("dsb" ::: "memory")
#define IPC_READ_BARRIER()    __asm volatile("dmb" ::: "memory")

#ifdef __cplusplus
}
#endif

#endif /* IPC_SHARED_H */
