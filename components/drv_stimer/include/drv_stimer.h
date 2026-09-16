/* =============================================================================
 * drv_stimer.h — DynaWatch V4 STIMER (Smart Timer) driver
 *
 * The STIMER is an i.MX RT1186 64-bit free-running counter that can be
 * clocked from an external reference pin.
 *
 * DynaWatch configuration:
 *   Reference: SAI4 TX_BCLK output pin  → STIMER_EXT_CLK input
 *   Frequency: 32.768 MHz (= SAI4_MCLK / 2 = AudioPLL / 20)
 *   Tick:      30.517 578 125 ns  (= 1 / 32 768 000)
 *   Ticks/ADC sample: 32 768 000 / 8 000 = 4 096 ticks  (exact integer ✓)
 *
 * The STIMER counter latches the current count when a CAPTURE event fires.
 * Phase 5: init + snapshot API.
 * Phase 7: PPS latch (1-PPS from PTP port → STIMER CAPTURE0) for
 *          time-error measurement used by the PTP servo.
 *
 * Called from CM33 (STIMER instance owned by CM33 domain).
 * =============================================================================
 */

#ifndef DRV_STIMER_H_
#define DRV_STIMER_H_

#include "fsl_common.h"
#include "clock_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Tick arithmetic helpers ──────────────────────────────────────────── */

/** Ticks per ADS131M08 sample (exact). */
#define DRV_STIMER_TICKS_PER_SAMPLE   BOARD_STIMER_TICKS_PER_SAMPLE   /* 4096 */

/** Tick period numerator (ps × 1024 for fixed-point, avoids float).
 *  1/32768000 s = 30517.578125 ns
 *  Encode as: tick_ns × 1024 = 31250  (exact: 30517.578125 × 1024 = 31250.0) */
#define DRV_STIMER_TICK_PS_X1024      BOARD_STIMER_TICK_NS_X1024       /* 31250 */

/* ─── Public API ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise STIMER with the external 32.768 MHz reference.
 *
 * Selects the external clock (SAI4 TX_BCLK pin) as STIMER_EXT_CLK,
 * resets the counter to zero, and enables the counter.
 *
 * Must be called after drv_sai4_start() so the reference clock is toggling.
 *
 * @return kStatus_Success, or an FSL error code.
 */
status_t drv_stimer_init(void);

/**
 * @brief Read the current 64-bit STIMER counter value.
 *
 * Read is atomic on Cortex-M (two 32-bit reads with high-word rollover check).
 *
 * @return 64-bit tick count since drv_stimer_init().
 */
uint64_t drv_stimer_get_count(void);

/**
 * @brief Convert a tick difference to nanoseconds (fixed-point, ±1 ns).
 *
 * Uses the 31250/1024 tick-period approximation:
 *   ns ≈ (ticks × 31250 + 512) >> 10
 *
 * @param ticks  Number of STIMER ticks.
 * @return Elapsed time in nanoseconds (unsigned).
 */
uint64_t drv_stimer_ticks_to_ns(uint64_t ticks);

/**
 * @brief Latch the STIMER count for a PPS event (Phase 7 PTP hook).
 *
 * Reads the CAPTURE0 register populated by the 1-PPS hardware input.
 * Phase 5 stub: returns drv_stimer_get_count() (no hardware latch yet).
 *
 * @param p_latched_count  Out: STIMER ticks at the PPS edge.
 * @return kStatus_Success.
 */
status_t drv_stimer_get_pps_latch(uint64_t *p_latched_count);

#ifdef __cplusplus
}
#endif

#endif /* DRV_STIMER_H_ */
