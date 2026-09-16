/**
 * @file    pmu_types.h
 * @brief   DynaWatch V4 — PMU phasor frame types and IEEE C37.118.2 STAT word
 *
 * Defines the data structures exchanged between the TWLS processing tasks
 * (task_twls_pmu_p, task_twls_pmu_m) and the PMU server task that encodes
 * C37.118.2 data frames for transmission over TLS 1.3.
 *
 * Phasor convention:
 *   phasor_f32_t.real  = X_r  (in-phase component, peak amplitude)
 *   phasor_f32_t.imag  = X_i  (quadrature component, peak amplitude)
 *   Magnitude (RMS) = sqrtf(real² + imag²) / sqrtf(2.0f)
 *   Angle           = atan2f(imag, real)   [radians, –π .. +π]
 *
 *   ⚠ Sign verification required at first power-on (Handoff §19A Trap 2):
 *   Apply a known 0° 50 Hz reference; confirm angle ≈ 0.  If off by π,
 *   negate imag (or flip Xi row in LUT gen script) and document.
 *
 * STAT word:
 *   Per IEEE C37.118.2-2011 Table 9.
 *
 * Copyright (c) 2026 ESID CUEE, Chulalongkorn University
 */

#ifndef PMU_TYPES_H
#define PMU_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "arm_math.h"   /* float32_t */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * IEEE C37.118.2-2011 STAT word  (§9.1, Table 9)
 * ===================================================================== */

/** Data error — set when phasor data is invalid (e.g. ADC fault) */
#define PMU_STAT_DATA_ERR       (1u << 15)

/** PMU sync — 1 = device is synced to a UTC-traceable time source (PTP) */
#define PMU_STAT_PMU_SYNC       (1u << 14)

/** Data sorting — 0 = data sorted by timestamp (our mode) */
#define PMU_STAT_DATA_SORT      (1u << 13)

/** PMU trigger — 1 = trigger detected (DFR trigger, fault, etc.) */
#define PMU_STAT_PMU_TRIG       (1u << 12)

/** Configuration change — 1 = config frame will change in next frame */
#define PMU_STAT_CFG_CHG        (1u << 11)

/** Data modified — 1 = data modified by post-processing */
#define PMU_STAT_DATA_MOD       (1u << 10)

/**
 * Time quality (TSQR) — bits [7:6]
 * 0b00 = locked (< 1 µs to UTC)   0b01 = < 100 µs
 * 0b10 = < 1 ms                   0b11 = > 1 ms or unavailable
 */
#define PMU_STAT_TQ_SHIFT       6u
#define PMU_STAT_TQ_MASK        (0x3u << PMU_STAT_TQ_SHIFT)
#define PMU_STAT_TQ_LOCKED      (0x0u << PMU_STAT_TQ_SHIFT)  /**< < 1 µs  */
#define PMU_STAT_TQ_100US       (0x1u << PMU_STAT_TQ_SHIFT)  /**< < 100 µs*/
#define PMU_STAT_TQ_1MS         (0x2u << PMU_STAT_TQ_SHIFT)  /**< < 1 ms  */
#define PMU_STAT_TQ_UNVAIL      (0x3u << PMU_STAT_TQ_SHIFT)  /**< > 1 ms  */

/**
 * Unlocked time indicator — bits [5:4]
 * Indicates how long since loss of sync: 0=locked, 1=<10s, 2=<100s, 3=≥100s
 */
#define PMU_STAT_UNLOCK_SHIFT   4u
#define PMU_STAT_UNLOCK_MASK    (0x3u << PMU_STAT_UNLOCK_SHIFT)
#define PMU_STAT_UNLOCK_SYNC    (0x0u << PMU_STAT_UNLOCK_SHIFT)
#define PMU_STAT_UNLOCK_10S     (0x1u << PMU_STAT_UNLOCK_SHIFT)
#define PMU_STAT_UNLOCK_100S    (0x2u << PMU_STAT_UNLOCK_SHIFT)
#define PMU_STAT_UNLOCK_OVER    (0x3u << PMU_STAT_UNLOCK_SHIFT)

/**
 * Trigger reason — bits [2:0]
 * 0=manual, 1=magnitude low, 2=magnitude high, 3=phase angle, 5=ROCOF, 7=digital
 */
#define PMU_STAT_TRIG_MASK      0x7u
#define PMU_STAT_TRIG_MANUAL    0x0u
#define PMU_STAT_TRIG_MAG_LOW   0x1u
#define PMU_STAT_TRIG_MAG_HIGH  0x2u
#define PMU_STAT_TRIG_PHASE     0x3u
#define PMU_STAT_TRIG_ROCOF     0x5u
#define PMU_STAT_TRIG_DIGITAL   0x7u

/* =========================================================================
 * Phasor type
 * ===================================================================== */

/**
 * @brief Single-precision complex phasor (rectangular form).
 *
 * Components are peak amplitude (not RMS).  To obtain the C37.118.2
 * floating-point phasor format (which uses magnitude in engineering units
 * and angle in radians), the STTP encoder applies:
 *   magnitude = hypotf(real, imag) / sqrtf(2.0f)
 *   angle     = atan2f(imag, real)
 */
typedef struct {
    float32_t real;     /**< In-phase peak amplitude (V or A)        */
    float32_t imag;     /**< Quadrature peak amplitude (V or A)      */
} phasor_f32_t;

/* =========================================================================
 * PMU frame types
 * ===================================================================== */

/** Number of voltage phasors per frame (Va, Vb, Vc) */
#define PMU_V_COUNT     3u
/** Number of current phasors per frame (Ia, Ib, Ic, In) */
#define PMU_I_COUNT     4u
/** Total phasors per frame */
#define PMU_PHASOR_COUNT  (PMU_V_COUNT + PMU_I_COUNT)

/** Voltage phasor index base in phasor[] array */
#define PMU_V_BASE      0u
/** Current phasor index base in phasor[] array */
#define PMU_I_BASE      PMU_V_COUNT

/**
 * @brief  P-class PMU data frame (100 fps).
 *
 * Produced by task_twls_pmu_p and passed (via pointer notification) to
 * pmu_server_task for C37.118.2 encoding.
 *
 * Double-buffered: task allocates from a 2-frame pool; pmu_server_task
 * holds a read reference until the next frame arrives.
 */
typedef struct {
    uint64_t       soc_ns;                      /**< UTC timestamp, ns since epoch    */
    uint16_t       stat;                        /**< IEEE C37.118.2 STAT word         */
    uint16_t       _pad;                        /**< Reserved (alignment)             */
    phasor_f32_t   phasor[PMU_PHASOR_COUNT];    /**< [0..2]=Va,Vb,Vc  [3..6]=Ia,Ib,Ic,In */
    float32_t      freq_hz;                     /**< Estimated fundamental frequency  */
} pmu_frame_p_t;

/**
 * @brief  M-class PMU data frame (100 fps, adds ROCOF).
 *
 * Produced by task_twls_pmu_m.  In addition to the P-class fields,
 * includes ROCOF (Hz/s) estimated by finite difference of successive
 * frequency measurements (see IEEE C37.118.1-2011 §5.6).
 */
typedef struct {
    uint64_t       soc_ns;
    uint16_t       stat;
    uint16_t       _pad;
    phasor_f32_t   phasor[PMU_PHASOR_COUNT];
    float32_t      freq_hz;
    float32_t      rocof_hz_s;                  /**< Rate of change of frequency, Hz/s */
} pmu_frame_m_t;

/* =========================================================================
 * Double-buffer pool (defined in task_twls_pmu_p.c / _m.c)
 * ===================================================================== */
extern pmu_frame_p_t g_pmu_frame_p[2];   /**< P-class double-buffer pool  */
extern pmu_frame_m_t g_pmu_frame_m[2];   /**< M-class double-buffer pool  */

/* =========================================================================
 * External interfaces used by the TWLS tasks
 * ===================================================================== */

/**
 * @brief  Task handle for the PMU server task (C37.118.2 encoder).
 *         Defined in pmu_server.c; TWLS tasks use xTaskNotify() to
 *         pass a pmu_frame_p_t* or pmu_frame_m_t* pointer as value.
 */
extern TaskHandle_t h_pmu_server_task;

/**
 * @brief  Event group bits used for inter-task coordination.
 *         Defined in app_events.c.
 */
extern EventGroupHandle_t g_pq_event_group;
#define PQ_FREQ_DEV_BIT     (1u << 0)   /**< Freq deviation > threshold     */
#define SEM_DATA_READY_BIT  (1u << 1)   /**< SEM energy data updated        */
#define PQM_FFT_READY_BIT   (1u << 2)   /**< PQM FFT window ready           */

/**
 * @brief  Push current M-class frequency and ROCOF to app_state.
 *         Defined in app_state.c; called by task_twls_pmu_m.
 */
void app_state_push_mfreq(float32_t freq_hz, float32_t rocof_hz_s);

/**
 * @brief  Retrieve current PTP synchronisation status.
 * @param[out] locked   True if UTC lock achieved (< 1 µs)
 * @param[out] tsqr     Time quality code (0..3, for STAT bits [7:6])
 * @param[out] unlock   Unlocked indicator (0..3, for STAT bits [5:4])
 */
void ptp_get_stat(bool *locked, uint8_t *tsqr, uint8_t *unlock);

/**
 * @brief  Retrieve current UTC timestamp (nanoseconds since Unix epoch).
 *         Implemented in ptp_tc.c using STIMER capture.
 */
uint64_t ptp_get_utc_ns(void);

#ifdef __cplusplus
}
#endif

#endif /* PMU_TYPES_H */
