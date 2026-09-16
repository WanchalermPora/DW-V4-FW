/* =============================================================================
 * clock_config.c — DynaWatch V4 BOARD_InitBootClocks()
 *
 * Configures the full clock tree for i.MX RT1186 dual-core operation.
 * Called first on both CM33 and CM7; CM33 runs it before releasing CM7.
 *
 * NXP MCUXpresso SDK 2.x API reference:
 *   fsl_clock.h — CLOCK_InitArmPll, CLOCK_InitAudioPll, CLOCK_SetRootClock
 *
 * ⚠️  SDK enum names (kCLOCK_Root_*, kCLOCK_*_ClockRoot_Mux*) must be
 *      verified against the SDK release shipped with your MCUXpresso install.
 *      Generate the authoritative version from MCUXpresso Config Tools and
 *      overlay it here, keeping the constants from clock_config.h unchanged.
 *
 * Verified clock calculations
 * ───────────────────────────
 *  ARM_PLL    = 24 MHz × 25                    = 600.000 MHz
 *  USB_PLL    = 480 MHz (fixed, loopDivider=0)
 *  AUDIO_PLL  = 24 × (27 + 294400/960000)      = 655.360 MHz
 *  M7_ROOT    = ARM_PLL / 1                    = 600 MHz
 *  M33_ROOT   = USB_PLL / 2                    = 240 MHz
 *  AHB_ROOT   = ARM_PLL / 4                    = 150 MHz
 *  IPG_ROOT   = AHB     / 2                    =  75 MHz
 *  SAI4_MCLK  = AUDIO_PLL / 10                 =  65.536 MHz  ≤ 66 MHz ✓
 *  LPSPI1     = USB_PLL / 6                    =  80 MHz
 *  LPUART1    = USB_PLL / 6                    =  80 MHz
 * =============================================================================
 */

#include "clock_config.h"
#include "fsl_clock.h"
#include "fsl_debug_console.h"

/* --------------------------------------------------------------------------- */
/*  Internal helpers                                                           */
/* --------------------------------------------------------------------------- */

/** Configure ARM PLL (PLL1) for CM7 @ 600 MHz.
 *  Formula: f = f_OSC × loopDivider = 24 MHz × 25 = 600 MHz (integer, no frac) */
static void s_init_arm_pll(void)
{
    const clock_arm_pll_config_t cfg = {
        .loopDivider = 25U,                    /* 24 × 25 = 600 MHz             */
        .postDivider = kCLOCK_PllPostDiv2Div1, /* no post-division              */
    };
    CLOCK_InitArmPll(&cfg);
}

/** Configure USB/System PLL3 for CM33 @ 240 MHz source.
 *  PLL3 fixed at 480 MHz; M33_CLK_ROOT set to /2 = 240 MHz. */
static void s_init_usb_pll(void)
{
    const clock_usb_pll_config_t cfg = {
        .loopDivider = 0U,  /* 0 → 480 MHz (PLL3 fixed frequency setting) */
        .src = 0U,
    };
    CLOCK_InitUsb1Pll(&cfg);
}

/** Configure Audio PLL (PLL4) for 655.360 MHz.
 *  Formula: f = 24 MHz × (27 + 294400/960000) = 655.360 000 MHz
 *  SAI4_MCLK = 655.360 / 10 = 65.536 MHz — SAI_MCLK ≤ 66 MHz limit: margin 0.464 MHz ✓
 */
static void s_init_audio_pll(void)
{
    const clock_audio_pll_config_t cfg = {
        .loopDivider = BOARD_AUDIO_PLL_LOOP_DIV,  /* 27                         */
        .postDivider = 1U,                         /* no post-division           */
        .numerator   = BOARD_AUDIO_PLL_NUM,        /* 294 400                    */
        .denominator = BOARD_AUDIO_PLL_DENOM,      /* 960 000                    */
    };
    CLOCK_InitAudioPll(&cfg);
}

/* --------------------------------------------------------------------------- */
/*  Public API                                                                 */
/* --------------------------------------------------------------------------- */

void BOARD_InitBootClocks(void)
{
    /* ── 1. ARM PLL → 600 MHz ──────────────────────────────────────────── */
    s_init_arm_pll();

    /* ── 2. CM7 clock root → ARM_PLL / 1 = 600 MHz ─────────────────────── */
    const clock_root_config_t m7Cfg = {
        .clockOff = false,
        .mux      = (uint8_t)kCLOCK_M7_ClockRoot_MuxArmPllOut,
        .div      = 1U,
    };
    CLOCK_SetRootClock(kCLOCK_Root_M7, &m7Cfg);

    /* ── 3. USB PLL → 480 MHz ──────────────────────────────────────────── */
    s_init_usb_pll();

    /* ── 4. CM33 clock root → USB_PLL / 2 = 240 MHz ────────────────────── */
    const clock_root_config_t m33Cfg = {
        .clockOff = false,
        .mux      = (uint8_t)kCLOCK_M33_ClockRoot_MuxSysPll3Out,
        .div      = 2U,   /* 480 / 2 = 240 MHz */
    };
    CLOCK_SetRootClock(kCLOCK_Root_M33, &m33Cfg);

    /* ── 5. AHB → ARM_PLL / 4 = 150 MHz ────────────────────────────────── */
    const clock_root_config_t ahbCfg = {
        .clockOff = false,
        .mux      = (uint8_t)kCLOCK_Bus_ClockRoot_MuxArmPllOut,
        .div      = 4U,   /* 600 / 4 = 150 MHz */
    };
    CLOCK_SetRootClock(kCLOCK_Root_Bus, &ahbCfg);
    /* IPG = AHB / 2 = 75 MHz — configured via CCM_TARGET_ROOT in SDK */

    /* ── 6. Audio PLL → 655.360 MHz ─────────────────────────────────────── */
    s_init_audio_pll();

    /* ── 7. SAI4 clock root → AUDIO_PLL / 10 = 65.536 MHz ──────────────── */
    /*       SAI_MCLK ≤ 66 MHz (RT1186 limit); 65.536 MHz gives 0.464 MHz margin */
    const clock_root_config_t sai4Cfg = {
        .clockOff = false,
        .mux      = (uint8_t)kCLOCK_Sai4_ClockRoot_MuxAudioPll1Out,
        .div      = BOARD_SAI4_MCLK_DIV,   /* 10 → 65.536 MHz */
    };
    CLOCK_SetRootClock(kCLOCK_Root_Sai4, &sai4Cfg);

    /* ── 8. LPSPI1 → USB_PLL / 6 = 80 MHz (→ SCLK 20 MHz at /4 prescaler) */
    const clock_root_config_t lpspi1Cfg = {
        .clockOff = false,
        .mux      = (uint8_t)kCLOCK_Lpspi1_ClockRoot_MuxSysPll3Out,
        .div      = 6U,   /* 480 / 6 = 80 MHz */
    };
    CLOCK_SetRootClock(kCLOCK_Root_Lpspi1, &lpspi1Cfg);

    /* ── 9. LPUART1 → USB_PLL / 6 = 80 MHz ─────────────────────────────── */
    const clock_root_config_t lpuart1Cfg = {
        .clockOff = false,
        .mux      = (uint8_t)kCLOCK_Lpuart1_ClockRoot_MuxSysPll3Out,
        .div      = 6U,   /* 480 / 6 = 80 MHz */
    };
    CLOCK_SetRootClock(kCLOCK_Root_Lpuart1, &lpuart1Cfg);

    /* ── 10. Gate peripheral clocks ─────────────────────────────────────── */
    CLOCK_EnableClock(kCLOCK_Sai4);
    CLOCK_EnableClock(kCLOCK_Lpspi1);
    CLOCK_EnableClock(kCLOCK_Lpuart1);
    CLOCK_EnableClock(kCLOCK_Gpio1);
    CLOCK_EnableClock(kCLOCK_Gpio2);
    /* NETC clocks enabled by drv_netc_init() */

    /* ── 11. Update SystemCoreClock for CMSIS ───────────────────────────── */
    SystemCoreClockUpdate();

    PRINTF("[CLK] ARM_PLL  : %lu MHz\r\n", BOARD_CM7_CORE_CLOCK_HZ / 1000000UL);
    PRINTF("[CLK] M33_ROOT : %lu MHz\r\n", BOARD_CM33_CORE_CLOCK_HZ / 1000000UL);
    PRINTF("[CLK] AHB_ROOT : %lu MHz\r\n", BOARD_AHB_CLOCK_HZ / 1000000UL);
    PRINTF("[CLK] IPG_ROOT : %lu MHz\r\n", BOARD_IPG_CLOCK_HZ / 1000000UL);
    PRINTF("[CLK] AUDIO_PLL: %lu MHz  (SAI_MCLK limit 66 MHz)\r\n",
           BOARD_AUDIO_PLL_HZ / 1000000UL);
    PRINTF("[CLK] SAI4_MCLK: %lu.%03lu MHz  (<= 66 MHz: margin 0.464 MHz ✓)\r\n",
           BOARD_SAI4_MCLK_HZ / 1000000UL,
           (BOARD_SAI4_MCLK_HZ % 1000000UL) / 1000UL);
    PRINTF("[CLK] TX_BCLK  : %lu MHz  (STIMER ref)\r\n",
           BOARD_SAI4_TX_BCLK_HZ / 1000000UL);
    PRINTF("[CLK] RX_BCLK  : %lu.%03lu MHz  (ADS131 CLKIN)\r\n",
           BOARD_SAI4_RX_BCLK_HZ / 1000000UL,
           (BOARD_SAI4_RX_BCLK_HZ % 1000000UL) / 1000UL);
}
