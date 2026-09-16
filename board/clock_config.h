/* =============================================================================
 * clock_config.h — DynaWatch V4 clock configuration
 *
 * NXP i.MX RT1186  (CM7 @ 600 MHz · CM33 @ 240 MHz)
 *
 * PLL map
 * ───────
 *  ARM_PLL  (PLL1)  = 24 MHz × 25          = 600.000 MHz  → CM7 core
 *  USB_PLL  (PLL3)  = 480 MHz (fixed)                     → CM33 core (/2)
 *  AUDIO_PLL(PLL4)  = 24 × (27 + 294400/960000)           = 655.360 MHz
 *
 * Clock root summary
 * ──────────────────
 *  M7_CLK_ROOT    = ARM_PLL  / 1  = 600 MHz
 *  M33_CLK_ROOT   = USB_PLL  / 2  = 240 MHz
 *  AHB_CLK_ROOT   = ARM_PLL  / 4  = 150 MHz
 *  IPG_CLK_ROOT   = AHB      / 2  =  75 MHz
 *  SAI4_CLK_ROOT  = AUDIO_PLL/10  =  65.536 MHz  ≤ 66 MHz MCLK limit ✓
 *  LPSPI1_CLK_ROOT= USB_PLL  / 6  =  80 MHz  (→ SCLK 20 MHz with /4 prescaler)
 *  LPUART1_CLK_ROOT= USB_PLL / 6  =  80 MHz
 *
 * SAI4 BCLK chain
 * ───────────────
 *  TX_BCLK = SAI4_MCLK / 2  =  32.768 MHz  → STIMER reference (external pin, 1.8 V)
 *  RX_BCLK = SAI4_MCLK / 8  =   8.192 MHz  → ADS131M08 CLKIN  (external pin, 1.8 V)
 *
 *  ADS131M08: CLKIN 8.192 MHz, OSR 512 → f_s = 8 192 000 / (512 × 2) = 8000 sps ✓
 *  STIMER   : 32.768 MHz → tick = 30.517578125 ns, 4096 ticks per ADC sample ✓
 * =============================================================================
 */

#ifndef CLOCK_CONFIG_H_
#define CLOCK_CONFIG_H_

#include "fsl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Crystal references ───────────────────────────────────────────────────── */
#define BOARD_XTAL0_CLK_HZ          24000000UL   /*!< 24 MHz main crystal       */
#define BOARD_XTAL32K_CLK_HZ           32768UL   /*!< 32.768 kHz RTC crystal    */

/* ─── Core clocks ──────────────────────────────────────────────────────────── */
#define BOARD_CM7_CORE_CLOCK_HZ    600000000UL   /*!< ARM_PLL × 25              */
#define BOARD_CM33_CORE_CLOCK_HZ   240000000UL   /*!< USB_PLL / 2               */
#define BOARD_AHB_CLOCK_HZ         150000000UL   /*!< ARM_PLL / 4               */
#define BOARD_IPG_CLOCK_HZ          75000000UL   /*!< AHB / 2                   */

/* ─── Audio PLL (PLL4) ─────────────────────────────────────────────────────── */
#define BOARD_AUDIO_PLL_LOOP_DIV        27U      /*!< Integer part of multiplier */
#define BOARD_AUDIO_PLL_NUM         294400U      /*!< Fractional numerator       */
#define BOARD_AUDIO_PLL_DENOM       960000U      /*!< Fractional denominator     */
/** 24 × (27 + 294400/960000) = 655.360 MHz */
#define BOARD_AUDIO_PLL_HZ         655360000UL

/* ─── SAI4 clocks ──────────────────────────────────────────────────────────── */
/** AUDIO_PLL / 10 = 65.536 MHz  (SAI_MCLK ≤ 66 MHz — margin: 0.464 MHz) */
#define BOARD_SAI4_MCLK_HZ          65536000UL
#define BOARD_SAI4_MCLK_DIV              10U

/** SAI4 TX_BCLK = MCLK / 2 = 32.768 MHz → STIMER external reference */
#define BOARD_SAI4_TX_BCLK_HZ       32768000UL
#define BOARD_SAI4_TX_BCLK_DIV           0U     /*!< TCR2[DIV]=0 → /((0+1)×2)=/2 */

/** SAI4 RX_BCLK = MCLK / 8 = 8.192 MHz → ADS131M08 CLKIN */
#define BOARD_SAI4_RX_BCLK_HZ        8192000UL
#define BOARD_SAI4_RX_BCLK_DIV           3U     /*!< RCR2[DIV]=3 → /((3+1)×2)=/8 */

/* ─── STIMER ───────────────────────────────────────────────────────────────── */
#define BOARD_STIMER_REF_HZ         32768000UL   /*!< = SAI4 TX_BCLK            */
#define BOARD_STIMER_TICKS_PER_SAMPLE   4096U    /*!< 32 768 000 / 8000 sps     */
#define BOARD_STIMER_TICK_NS_X1024    31250U      /*!< 30517.578... ps ≈ 1024 units of 30.517578125 ns — use fixed-point */

/* ─── Peripheral clocks ────────────────────────────────────────────────────── */
#define BOARD_LPSPI1_CLK_HZ          80000000UL  /*!< USB_PLL / 6               */
#define BOARD_LPUART1_CLK_HZ         80000000UL  /*!< USB_PLL / 6               */

/* ─── ADS131M08 ────────────────────────────────────────────────────────────── */
#define BOARD_ADS131_SCLK_HZ         20000000UL  /*!< LPSPI1 / 4 prescaler      */
#define BOARD_ADS131_OSR                  512U
#define BOARD_ADS131_SAMPLE_RATE_HZ      8000U   /*!< CLKIN / (OSR × 2)         */
#define BOARD_ADS131_CHANNELS               8U
#define BOARD_ADS131_FRAME_BYTES           27U   /*!< (1 cmd + 8 data) × 3 B    */

/* ─── Boot clock function ──────────────────────────────────────────────────── */
void BOARD_InitBootClocks(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_CONFIG_H_ */
