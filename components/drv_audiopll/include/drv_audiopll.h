/* =============================================================================
 * drv_audiopll.h — DynaWatch V4 AudioPLL runtime steering API
 *
 * The AudioPLL (PLL4) is initialised by BOARD_InitBootClocks() at boot:
 *   f_PLL4 = 24 MHz × (27 + 294400/960000) = 655.360 000 MHz
 *
 * This driver provides runtime adjustment of the fractional numerator (NUM)
 * register so that a PTP servo can steer STIMER, SAI4 MCLK, and the
 * ADS131M08 CLKIN frequency as a single proportional group.
 *
 * Clock steering chain (all proportional to NUM):
 *   AudioPLL → SAI4_MCLK (/10) → TX_BCLK (/2) → STIMER reference
 *                               → RX_BCLK (/8) → ADS131M08 CLKIN
 *
 * Phase 4/5: init + get_freq stubs; steer_ppb is a stub.
 * Phase 7: steer_ppb writes CCM_ANALOG_AUDIO_PLL1_NUMERATORn atomically
 *           to implement PTP clock servo corrections.
 *
 * Called from CM33 only (audio PLL owned by CM33).
 * =============================================================================
 */

#ifndef DRV_AUDIOPLL_H_
#define DRV_AUDIOPLL_H_

#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Nominal frequencies ──────────────────────────────────────────────── */
/** Nominal AudioPLL frequency in Hz (boot value) */
#define DRV_AUDIOPLL_NOM_HZ      655360000UL

/** Adjustment range: ±200 ppm (Phase 7 IEEE 1588 / IEC 61850-9-3 limit) */
#define DRV_AUDIOPLL_MAX_PPB     200000     /* +/- 200 000 ppb = 200 ppm  */

/* ─── Public API ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise AudioPLL driver state (does NOT touch PLL hardware).
 *
 * The PLL is already running after BOARD_InitBootClocks(). Call this once
 * on CM33 before the PTP servo is started to reset the internal trim state.
 *
 * @return kStatus_Success always (Phase 5 stub).
 */
status_t drv_audiopll_init(void);

/**
 * @brief Apply a frequency correction expressed in parts-per-billion.
 *
 * Adjusts the AudioPLL fractional numerator:
 *   new_NUM = NOM_NUM + round(NOM_NUM × ppb / 1e9)
 *
 * The adjustment propagates immediately to all derived clocks
 * (SAI4_MCLK, TX_BCLK/STIMER, RX_BCLK/ADS131M08).
 *
 * Clamped to ±DRV_AUDIOPLL_MAX_PPB. Thread-safe: uses a critical section.
 *
 * @param ppb  Correction in parts-per-billion (positive = speed up).
 * @return kStatus_Success, or kStatus_InvalidArgument if |ppb| >
 *         DRV_AUDIOPLL_MAX_PPB (correction is still applied after clamping).
 *
 * Phase 5 stub: records ppb internally, does NOT write PLL registers.
 * Phase 7: writes CCM_ANALOG_AUDIO_PLL1_NUMERATOR atomically.
 */
status_t drv_audiopll_steer_ppb(int32_t ppb);

/**
 * @brief Return the current AudioPLL output frequency in Hz.
 *
 * Computed from the last applied ppb correction:
 *   f = DRV_AUDIOPLL_NOM_HZ × (1 + ppb/1e9)
 *
 * @return Estimated PLL output in Hz.
 */
uint32_t drv_audiopll_get_freq_hz(void);

/**
 * @brief Return the last applied correction in ppb.
 */
int32_t drv_audiopll_get_trim_ppb(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_AUDIOPLL_H_ */
