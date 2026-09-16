/**
 * @file    pow_buf.h
 * @brief   DynaWatch V4 — Shared Power-of-Words (PoW) ring buffer
 *
 * Defines the shared OCRAM1 memory layout used by both cores:
 *   - CM33 writes via DMA (LPSPI1 → EDMA → pow_ring)
 *   - CM7 reads via FreeRTOS tasks
 *
 * Memory map (OCRAM1, 164,864 bytes total):
 *   pow_ring   [7][4000] float32  — 112,000 B  500 ms ring (8000 sps)
 *   pow_double [7][1888] float32  —  52,864 B  double-buffer for cross-boundary windows
 *   pow_tail   atomic_uint        — write-head snapshot at 100 Hz tick
 *
 * UTC alignment (D11 §3):
 *   Ring Index #0 is UTC-aligned: CM33 resets write pointer to slot 0 at
 *   UTC+0 ms and UTC+500 ms (PTP-disciplined STIMER comparison).  The
 *   double-buffer holds the previous 500 ms epoch's tail to support
 *   M-class (961-sample) cross-boundary window assembly.
 *
 * Channel map (D5 v1.2):
 *   PoW[0..2] = Va, Vb, Vc  (Volts, from ADS131M08-Q1 AIN4-6)
 *   PoW[3..6] = Ia, Ib, Ic, In  (Amperes, from AIN0-3)
 *
 * ⚠  V3→V4 PORTING TRAP (Handoff §19A Trap 3):
 *   V3 ring is 8000 samples, continuous modulo.
 *   V4 ring is 4000 samples with hard UTC reset at each 500 ms boundary.
 *   (tail - N + 4000) % 4000  is WRONG when tail < N.
 *   Use pow_window_assemble() below — it handles boundary crossing correctly.
 *
 * Copyright (c) 2026 ESID CUEE, Chulalongkorn University
 */

#ifndef POW_BUF_H
#define POW_BUF_H

#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include "arm_math.h"   /* float32_t */

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Dimensions
 * ---------------------------------------------------------------------- */
#define POW_CHANNELS    7u      /**< Va Vb Vc Ia Ib Ic In                   */
#define POW_RING_LEN    4000u   /**< Ring samples per channel (500 ms)       */
#define POW_DOUBLE_LEN  1888u   /**< Double-buffer samples per channel       */
#define POW_FS_HZ       8000u   /**< ADC sample rate, Hz                     */

/* Channel indices */
#define POW_CH_VA   0u
#define POW_CH_VB   1u
#define POW_CH_VC   2u
#define POW_CH_IA   3u
#define POW_CH_IB   4u
#define POW_CH_IC   5u
#define POW_CH_IN   6u

/* -------------------------------------------------------------------------
 * Shared OCRAM1 buffers  (defined in pow_buf.c on CM33 side)
 * ---------------------------------------------------------------------- */

/** Main ring buffer — CM33 writes, CM7 reads */
extern volatile float32_t pow_ring[POW_CHANNELS][POW_RING_LEN];

/**
 * Double-buffer — holds the tail of the previous 500 ms epoch.
 * POW_DOUBLE_LEN = 1888 ≥ max window crossing:
 *   M-class N=961 at 100 fps: max cross = N - 1 = 960 samples
 *   DFR / PQM max = 1553 samples (longest FFT window at 42.5 Hz)
 *   1888 > 1553 with 335-sample guard band.
 */
extern volatile float32_t pow_double[POW_CHANNELS][POW_DOUBLE_LEN];

/**
 * Write-head snapshot delivered by task_pow_consumer to CM7 processing tasks.
 * Value = ring write index at the 100 Hz tick moment.
 * Processing tasks receive this as xTaskNotifyWait() notification value.
 */
extern volatile atomic_uint pow_tail;

/* -------------------------------------------------------------------------
 * Window assembly helper
 *
 * Assembles a contiguous N-sample window ending at ring index 'tail' for
 * channel 'ch'.  Handles the UTC 500 ms boundary crossing by splicing from
 * pow_double[] when tail < N.
 *
 * Returns a pointer to float32_t[N]:
 *   - Direct pointer into pow_ring[ch] when no boundary crossing (tail >= N)
 *   - Pointer to 'scratch' (which is filled) when crossing is needed
 *
 * @param  ch      Channel index (0..POW_CHANNELS-1)
 * @param  tail    Ring write index snapshot from task_pow_consumer
 * @param  N       Window length in samples (≤ POW_RING_LEN)
 * @param  scratch Caller-provided scratch buffer of at least N float32 elements
 * @return Pointer to N float32 samples, newest at [N-1]
 *
 * ⚠  Caller must cast away volatile before passing to arm_dot_prod_f32().
 *    The __DMB() barrier in the caller task ensures CM33's writes are visible.
 * ---------------------------------------------------------------------- */
static inline const float32_t *pow_window_assemble(
        uint32_t ch, uint32_t tail, uint32_t N, float32_t *scratch)
{
    if (tail >= N) {
        /* Common case: window fits entirely in the ring, no crossing */
        /* Cast away volatile — safe after __DMB() in caller */
        return (const float32_t *)&pow_ring[ch][tail - N];
    }

    /* Boundary crossing: part of the window is in pow_double[], part in pow_ring[] */
    uint32_t from_double = N - tail;           /* samples from previous epoch */
    uint32_t from_ring   = tail;               /* samples from current epoch  */

    /* Sanity: double-buffer must be large enough */
    /* (compile-time assertion at build would be preferable) */

    /* Copy tail of previous epoch from double-buffer */
    uint32_t double_start = POW_DOUBLE_LEN - from_double;
    memcpy(scratch,
           (const float32_t *)&pow_double[ch][double_start],
           from_double * sizeof(float32_t));

    /* Copy head of current epoch from ring */
    if (from_ring > 0u) {
        memcpy(scratch + from_double,
               (const float32_t *)&pow_ring[ch][0],
               from_ring * sizeof(float32_t));
    }

    return (const float32_t *)scratch;
}

#ifdef __cplusplus
}
#endif

#endif /* POW_BUF_H */
