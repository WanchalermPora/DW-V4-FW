/* =============================================================================
 * security_init.c — DynaWatch V4 Six-Phase Security Bootstrap
 *
 * Phases (D15 §11):
 *   S1  ELE S200 firmware self-test
 *   S2  AHAB lifecycle state verification
 *   S3  OTFAD CTX1 key unwrap & encryption enable
 *   A1  ELE_OpenSession
 *   A2  ELE_OpenKeystore
 *   A3  Calibration blob AES-128-GCM verify + load
 *
 * FRDM-IMXRT1186 bring-up behaviour (OEM_OPEN lifecycle):
 *   - S2 sets warn_dev_board flag (non-fatal)
 *   - S3 is SKIPPED (no OTFAD key blob in dev NOR flash)
 *   - A3 loads factory default calibration (unity gain, zero offset)
 *   - Hold USER_BTN (SW2) at power-on to enter full DEV_BYPASS mode:
 *     all ELE/OTFAD phases skipped, every PRINTF prefixed with [DEV_BYPASS]
 * =============================================================================
 */

#include "security_init.h"
#include "board.h"              /* BOARD_OTFAD_CTX1_BASE, BOARD_USER_BTN_*  */
#include "sdkconfig.h"          /* Kconfig-generated symbols                 */

/* MCUXpresso SDK */
#include "fsl_debug_console.h"  /* PRINTF()                                  */
#include "fsl_gpio.h"
#include "fsl_clock.h"

/* ELE S200 messaging unit (MCUXpresso SDK ele_crypto component) */
#include "ele_crypto.h"         /* ELE_OpenSession, ELE_GetFirmwareStatus …  */
#include "S3MU.h"               /* S3MU base address                         */

/* mbedTLS (for A3 calibration blob AES-128-GCM) */
#include "mbedtls/gcm.h"

/* FreeRTOS heap allocator for mbedTLS (Phase 3 — software only) */
#include "FreeRTOS.h"

/* ============================================================================
 * Module-level state
 * ============================================================================*/
calib_data_t g_calib;
uint32_t     g_ele_session_id  = 0U;
uint32_t     g_ele_keystore_id = 0U;

static bool  s_dev_bypass      = false;
static bool  s_warn_dev_board  = false;

/* ============================================================================
 * Internal helpers
 * ============================================================================*/

/** @brief LED feedback — green = success/running, red = fatal error. */
static void led_green(void)
{
#if defined(BOARD_LED_GREEN_GPIO)
    GPIO_PinWrite(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_PIN, 0U); /* active-low */
    GPIO_PinWrite(BOARD_LED_RED_GPIO,   BOARD_LED_RED_PIN,   1U);
#endif
}

static void led_red(void)
{
#if defined(BOARD_LED_RED_GPIO)
    GPIO_PinWrite(BOARD_LED_RED_GPIO,   BOARD_LED_RED_PIN,   0U); /* active-low */
    GPIO_PinWrite(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_PIN, 1U);
#endif
}

static void led_off(void)
{
#if defined(BOARD_LED_GREEN_GPIO)
    GPIO_PinWrite(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_PIN, 1U);
    GPIO_PinWrite(BOARD_LED_RED_GPIO,   BOARD_LED_RED_PIN,   1U);
#endif
}

/** @brief Blocking spin delay (used before FreeRTOS is running). */
static void spin_ms(uint32_t ms)
{
    /* At 600 MHz, ~600000 cycles/ms; adjust if CPU frequency differs */
    volatile uint32_t n = ms * 150000UL;
    while (n--) { __asm volatile("nop"); }
}

/** @brief Read the USER_BTN (SW2) state — true if held down at call time. */
static bool user_btn_pressed(void)
{
#if defined(BOARD_USER_BTN_GPIO)
    /* Active-low: pin reads 0 when button is pressed */
    return (GPIO_PinRead(BOARD_USER_BTN_GPIO, BOARD_USER_BTN_PIN) == 0U);
#else
    return false;
#endif
}

/** @brief Load factory default calibration (development / OEM_OPEN use). */
static void calib_load_defaults(void)
{
    g_calib.magic   = CALIB_MAGIC;
    g_calib.version = 0U;

    for (int i = 0; i < 3; i++) {
        g_calib.v_gain[i]   = CALIB_DEFAULT_V_GAIN;
        g_calib.v_offset[i] = CALIB_DEFAULT_V_OFFSET;
    }
    for (int i = 0; i < 4; i++) {
        g_calib.i_gain[i]   = CALIB_DEFAULT_I_GAIN;
        g_calib.i_offset[i] = CALIB_DEFAULT_I_OFFSET;
    }
}

/* ============================================================================
 * Phase S1 — ELE S200 firmware self-test
 *   Expected duration: ~20 ms
 *   Passes when ELE firmware is loaded and has completed its internal ROM test.
 *   On FRDM-IMXRT1186: always passes (ELE ROM is always present).
 * ============================================================================*/
static security_status_t phase_s1_ele_selftest(void)
{
    PRINTF("[SEC] S1 ELE self-test ... ");

    uint32_t fw_status = 0U;
    uint32_t fw_mode   = 0U;

    status_t st = ELE_GetFirmwareStatus(S3MU, &fw_status, &fw_mode);
    if (st != kStatus_Success) {
        PRINTF("FAIL (ELE_GetFirmwareStatus=0x%08lX)\r\n", (unsigned long)st);
        led_red();
        return kSecurity_EleTestFail;
    }

    /*
     * Bit 1 of fw_status = ELE self-test complete flag.
     * fw_mode 0x00 = standard mode (TRNG seeded, services available).
     */
    if ((fw_status & 0x02U) == 0U) {
        PRINTF("FAIL (fw_status=0x%08lX, self-test bit not set)\r\n",
               (unsigned long)fw_status);
        led_red();
        return kSecurity_EleTestFail;
    }

    PRINTF("PASS (fw_status=0x%02lX, mode=0x%02lX)\r\n",
           (unsigned long)fw_status, (unsigned long)fw_mode);
    return kSecurity_Success;
}

/* ============================================================================
 * Phase S2 — AHAB lifecycle verification
 *   OEM_CLOSED (0x80): full production, strict enforcement
 *   OEM_OPEN   (0x20): development board — non-fatal warning on FRDM
 *   Other states: fatal
 * ============================================================================*/
static security_status_t phase_s2_lifecycle(void)
{
    PRINTF("[SEC] S2 Lifecycle check  ... ");

    uint32_t lc_state = 0U;
    status_t st = ELE_GetLifecycle(S3MU, &lc_state);
    if (st != kStatus_Success) {
        PRINTF("FAIL (ELE_GetLifecycle=0x%08lX)\r\n", (unsigned long)st);
        led_red();
        return kSecurity_LifecycleFail;
    }

    if (lc_state == 0x80U) {  /* OEM_CLOSED — production */
        PRINTF("PASS (OEM_CLOSED, production mode)\r\n");
        return kSecurity_Success;
    }

    if (lc_state == 0x20U) {  /* OEM_OPEN — development board */
        PRINTF("WARN (OEM_OPEN lifecycle — FRDM dev board, non-production)\r\n");
        PRINTF("[SEC]      OTFAD CTX1 and keystore operations will be skipped.\r\n");
        s_warn_dev_board = true;

#if defined(CONFIG_SECURITY_LIFECYCLE_CHECK_ENABLE) && (CONFIG_SECURITY_LIFECYCLE_CHECK_ENABLE == 1)
        /* Strict mode: OEM_OPEN is fatal (production firmware) */
        led_red();
        return kSecurity_LifecycleFail;
#else
        /* Relaxed mode (default for development builds) */
        return kSecurity_WarnDevBoard;
#endif
    }

    /* Unexpected lifecycle state */
    PRINTF("FAIL (unknown lc=0x%08lX)\r\n", (unsigned long)lc_state);
    led_red();
    return kSecurity_LifecycleFail;
}

/* ============================================================================
 * Phase S3 — OTFAD CTX1 key unwrap & enable
 *   Requires OEM_CLOSED lifecycle and a valid key blob in NOR flash.
 *   SKIPPED on FRDM-IMXRT1186 (OEM_OPEN / no key blob provisioned).
 *   TWLS LUT bank (XiP at BOARD_OTFAD_CTX1_BASE) is read as plaintext
 *   on dev board; on production board it is transparently decrypted.
 * ============================================================================*/
static security_status_t phase_s3_otfad(void)
{
    if (s_warn_dev_board || s_dev_bypass) {
        PRINTF("[SEC] S3 OTFAD CTX1       ... SKIP (dev board / bypass mode)\r\n");
        return kSecurity_Success;   /* non-fatal skip */
    }

    PRINTF("[SEC] S3 OTFAD CTX1       ... ");

    /*
     * Key blob location: first 128 bytes of OTFAD CTX1 region.
     * The blob was written during production provisioning.
     */
    const uint8_t *keyblob = (const uint8_t *)BOARD_OTFAD_CTX1_BASE;

    status_t st = ELE_UnwrapOtfadKey(S3MU,
                                     keyblob,           /* key blob ptr      */
                                     1U,                /* CTX index = 1     */
                                     BOARD_OTFAD_CTX1_BASE,
                                     BOARD_OTFAD_CTX1_SIZE);
    if (st != kStatus_Success) {
        PRINTF("FAIL (ELE_UnwrapOtfadKey=0x%08lX)\r\n", (unsigned long)st);
        led_red();
        return kSecurity_OtfadFail;
    }

    /*
     * Enable OTFAD encryption engine on FLEXSPI1.
     * After this call, reads from [BOARD_OTFAD_CTX1_BASE .. +SIZE) are
     * transparently AES-XTS decrypted in hardware.
     */
    OTFAD_EnableEncryption(FLEXSPI1, true);

    PRINTF("PASS (CTX1 keyed, XiP region decrypted)\r\n");
    return kSecurity_Success;
}

/* ============================================================================
 * Phase A1 — ELE_OpenSession
 *   Creates a persistent ELE session handle for the life of the application.
 *   Required before any ELE key or crypto service.
 * ============================================================================*/
static security_status_t phase_a1_session(void)
{
    if (s_dev_bypass) {
        PRINTF("[SEC] A1 ELE session      ... SKIP (bypass)\r\n");
        return kSecurity_Success;
    }

    PRINTF("[SEC] A1 ELE session      ... ");

    status_t st = ELE_OpenSession(S3MU, &g_ele_session_id);
    if (st != kStatus_Success) {
        PRINTF("FAIL (ELE_OpenSession=0x%08lX)\r\n", (unsigned long)st);
        led_red();
        return kSecurity_SessionFail;
    }

    PRINTF("PASS (sid=0x%08lX)\r\n", (unsigned long)g_ele_session_id);
    return kSecurity_Success;
}

/* ============================================================================
 * Phase A2 — ELE_OpenKeystore
 *   Opens the device-bound keystore partition.
 *   Keystore must have been provisioned (write once during manufacturing).
 *   On OEM_OPEN dev board: skipped (no keystore provisioned).
 * ============================================================================*/
static security_status_t phase_a2_keystore(void)
{
    if (s_warn_dev_board || s_dev_bypass) {
        PRINTF("[SEC] A2 ELE keystore     ... SKIP (dev board / bypass)\r\n");
        return kSecurity_Success;
    }

    PRINTF("[SEC] A2 ELE keystore     ... ");

    ele_keystore_args_t ks_args = {
        .id           = CONFIG_SECURITY_KEYSTORE_ID,
        .nonce        = CONFIG_SECURITY_KEYSTORE_NONCE,
        .max_updates  = 100U,
        .create       = false,   /* open existing keystore, don't create    */
    };

    status_t st = ELE_OpenKeystore(S3MU, g_ele_session_id,
                                   &ks_args, &g_ele_keystore_id);
    if (st != kStatus_Success) {
        PRINTF("FAIL (ELE_OpenKeystore=0x%08lX)\r\n", (unsigned long)st);
        led_red();
        return kSecurity_KeystoreFail;
    }

    PRINTF("PASS (ksid=0x%08lX)\r\n", (unsigned long)g_ele_keystore_id);
    return kSecurity_Success;
}

/* ============================================================================
 * Phase A3 — Calibration blob AES-128-GCM verify + load
 *   On OEM_CLOSED: export calib key from ELE keystore, GCM-decrypt blob.
 *   On OEM_OPEN / bypass: load factory default constants (unity/zero).
 * ============================================================================*/
static security_status_t phase_a3_calib(void)
{
    if (s_warn_dev_board || s_dev_bypass) {
        PRINTF("[SEC] A3 Calib blob       ... SKIP (dev mode, loading defaults)\r\n");
        calib_load_defaults();
        PRINTF("[SEC]      v_gain=[%.4f, %.4f, %.4f]  (unity — uncompensated)\r\n",
               g_calib.v_gain[0], g_calib.v_gain[1], g_calib.v_gain[2]);
        PRINTF("[SEC]      i_gain=[%.4f, %.4f, %.4f, %.4f]\r\n",
               g_calib.i_gain[0], g_calib.i_gain[1],
               g_calib.i_gain[2], g_calib.i_gain[3]);
        return kSecurity_Success;
    }

    PRINTF("[SEC] A3 Calib blob       ... ");

    /* 1. Read blob from NOR flash (XiP direct pointer, OTFAD decrypts) */
    const calib_blob_flash_t *blob =
        (const calib_blob_flash_t *)CALIB_BLOB_FLASH_ADDR;

    /* 2. Export the calibration key from ELE keystore (index from Kconfig) */
    uint8_t calib_key[16] = {0};
    status_t st = ELE_ExportKey(S3MU, g_ele_session_id, g_ele_keystore_id,
                                CONFIG_SECURITY_CALIB_KEY_IDX,
                                calib_key, sizeof(calib_key));
    if (st != kStatus_Success) {
        PRINTF("FAIL (ELE_ExportKey=0x%08lX)\r\n", (unsigned long)st);
        led_red();
        return kSecurity_CalibFail;
    }

    /* 3. AES-128-GCM decrypt + authenticate via mbedTLS */
    mbedtls_gcm_context gcm_ctx;
    mbedtls_gcm_init(&gcm_ctx);

    int ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES,
                                 calib_key, 128U);
    if (ret != 0) {
        mbedtls_gcm_free(&gcm_ctx);
        PRINTF("FAIL (gcm_setkey=%d)\r\n", ret);
        led_red();
        return kSecurity_CalibFail;
    }

    uint8_t plaintext[sizeof(calib_data_t)] = {0};
    ret = mbedtls_gcm_auth_decrypt(&gcm_ctx,
                                   sizeof(calib_data_t),
                                   blob->iv,         sizeof(blob->iv),
                                   NULL,             0U,        /* no AAD     */
                                   blob->tag,        sizeof(blob->tag),
                                   blob->ciphertext, plaintext);
    mbedtls_gcm_free(&gcm_ctx);

    /* Zero the key immediately after use */
    __builtin_memset(calib_key, 0, sizeof(calib_key));

    if (ret != 0) {
        PRINTF("FAIL (GCM auth failed — blob corrupt or wrong key, ret=%d)\r\n",
               ret);
        led_red();
        return kSecurity_CalibFail;
    }

    /* 4. Verify magic word */
    __builtin_memcpy(&g_calib, plaintext, sizeof(g_calib));
    if (g_calib.magic != CALIB_MAGIC) {
        PRINTF("FAIL (bad magic 0x%08lX, expected 0x%08lX)\r\n",
               (unsigned long)g_calib.magic, (unsigned long)CALIB_MAGIC);
        led_red();
        return kSecurity_CalibMagic;
    }

    PRINTF("PASS (version=%lu)\r\n", (unsigned long)g_calib.version);
    PRINTF("[SEC]      v_gain=[%.4f, %.4f, %.4f]\r\n",
           g_calib.v_gain[0], g_calib.v_gain[1], g_calib.v_gain[2]);
    PRINTF("[SEC]      i_gain=[%.4f, %.4f, %.4f, %.4f]\r\n",
           g_calib.i_gain[0], g_calib.i_gain[1],
           g_calib.i_gain[2], g_calib.i_gain[3]);

    return kSecurity_Success;
}

/* ============================================================================
 * Public API
 * ============================================================================*/

const char *security_status_str(security_status_t status)
{
    switch (status) {
        case kSecurity_Success:       return "SUCCESS";
        case kSecurity_DevMode:       return "DEV_BYPASS";
        case kSecurity_WarnDevBoard:  return "WARN_DEV_BOARD (OEM_OPEN)";
        case kSecurity_EleTestFail:   return "FAIL_ELE_SELFTEST";
        case kSecurity_LifecycleFail: return "FAIL_LIFECYCLE";
        case kSecurity_OtfadFail:     return "FAIL_OTFAD";
        case kSecurity_SessionFail:   return "FAIL_ELE_SESSION";
        case kSecurity_KeystoreFail:  return "FAIL_ELE_KEYSTORE";
        case kSecurity_CalibFail:     return "FAIL_CALIB_GCM";
        case kSecurity_CalibMagic:    return "FAIL_CALIB_MAGIC";
        default:                      return "UNKNOWN";
    }
}

security_status_t security_init(void)
{
    security_status_t result = kSecurity_Success;
    uint32_t t_start_ms;

    /* ------------------------------------------------------------------
     * Read cycle counter for timing (DWT, available on CM7)
     * ------------------------------------------------------------------*/
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT  = 0U;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;
    t_start_ms   = DWT->CYCCNT;
    (void)t_start_ms;   /* suppress warning if PRINTF unavailable */

    PRINTF("\r\n");
    PRINTF("=============================================================\r\n");
    PRINTF("  DynaWatch V4 — Security Bootstrap (D15 §11)\r\n");
    PRINTF("  Build: %s %s\r\n", __DATE__, __TIME__);
    PRINTF("=============================================================\r\n");

    /* ------------------------------------------------------------------
     * Check for dev-bypass mode: USER_BTN held at power-on
     * Sample button 3 times at 10 ms intervals for debounce.
     * ------------------------------------------------------------------*/
    uint8_t btn_count = 0U;
    for (int i = 0; i < 3; i++) {
        if (user_btn_pressed()) { btn_count++; }
        spin_ms(10U);
    }
    if (btn_count >= 2U) {
        s_dev_bypass = true;
        PRINTF("[SEC] *** DEV_BYPASS MODE — USER_BTN held at reset ***\r\n");
        PRINTF("[SEC] *** All ELE/OTFAD phases SKIPPED.             ***\r\n");
        PRINTF("[SEC] *** NOT FOR PRODUCTION USE.                   ***\r\n");
        calib_load_defaults();

        uint32_t elapsed = (DWT->CYCCNT - t_start_ms) / (600000U); /* ms */
        PRINTF("=============================================================\r\n");
        PRINTF("[SEC] Security init COMPLETE in ~%lu ms — DEV_BYPASS\r\n", elapsed);
        PRINTF("=============================================================\r\n\r\n");
        led_green();
        return kSecurity_DevMode;
    }

    /* ------------------------------------------------------------------
     * S1: ELE self-test
     * ------------------------------------------------------------------*/
    result = phase_s1_ele_selftest();
    if (result < 0) { goto sec_fatal; }

    /* ------------------------------------------------------------------
     * S2: Lifecycle check
     * ------------------------------------------------------------------*/
    security_status_t s2 = phase_s2_lifecycle();
    if (s2 == kSecurity_WarnDevBoard) {
        s_warn_dev_board = true;
        result = kSecurity_WarnDevBoard;   /* downgrade result, continue   */
    } else if (s2 < 0) {
        result = s2;
        goto sec_fatal;
    }

    /* ------------------------------------------------------------------
     * S3: OTFAD CTX1
     * ------------------------------------------------------------------*/
    {
        security_status_t s3 = phase_s3_otfad();
        if (s3 < 0) { result = s3; goto sec_fatal; }
    }

    /* ------------------------------------------------------------------
     * A1: ELE session
     * ------------------------------------------------------------------*/
    {
        security_status_t a1 = phase_a1_session();
        if (a1 < 0) { result = a1; goto sec_fatal; }
    }

    /* ------------------------------------------------------------------
     * A2: ELE keystore
     * ------------------------------------------------------------------*/
    {
        security_status_t a2 = phase_a2_keystore();
        if (a2 < 0) { result = a2; goto sec_fatal; }
    }

    /* ------------------------------------------------------------------
     * A3: Calibration blob
     * ------------------------------------------------------------------*/
    {
        security_status_t a3 = phase_a3_calib();
        if (a3 < 0) { result = a3; goto sec_fatal; }
    }

    /* ------------------------------------------------------------------
     * All phases done
     * ------------------------------------------------------------------*/
    {
        uint32_t elapsed = (DWT->CYCCNT - t_start_ms) / 600000U;
        PRINTF("=============================================================\r\n");
        PRINTF("[SEC] Security init COMPLETE in ~%lu ms — %s\r\n",
               elapsed, security_status_str(result));
        PRINTF("=============================================================\r\n\r\n");
    }
    led_green();
    return result;

sec_fatal:
    PRINTF("=============================================================\r\n");
    PRINTF("[SEC] *** FATAL: Security init FAILED — %s ***\r\n",
           security_status_str(result));
    PRINTF("[SEC] *** System will blink red LED and halt.        ***\r\n");
    PRINTF("=============================================================\r\n\r\n");

    /* Blink red LED rapidly and halt — never enter FreeRTOS */
    for (;;) {
        led_red();
        spin_ms(200U);
        led_off();
        spin_ms(200U);
    }
    /* NOTREACHED */
}
