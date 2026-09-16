/* =============================================================================
 * security_init.h — DynaWatch V4 Security Bootstrap API
 *
 * Six-phase security initialisation sequence (D15 §11):
 *
 *   S1  ELE S200 firmware self-test             (~20 ms)
 *   S2  AHAB lifecycle state verification       (~30 ms)
 *   S3  OTFAD CTX1 key unwrap & enable          (~5  ms)
 *   A1  ELE_OpenSession                         (~5  ms)
 *   A2  ELE_OpenKeystore                        (~10 ms)
 *   A3  Calibration blob GCM verify + load      (~2  ms)
 *                                               --------
 *                                     Total:   ~72 ms
 *
 * On FRDM-IMXRT1186 (OEM_OPEN lifecycle, development):
 *   - S2 returns kSecurity_WarnDevBoard (non-fatal warning)
 *   - S3 is SKIPPED (OTFAD not keyed on dev board)
 *   - A3 loads factory default calibration constants
 *   Hold USER_BTN at reset to enter full DEV_BYPASS mode.
 * =============================================================================
 */

#ifndef SECURITY_INIT_H
#define SECURITY_INIT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * Return status codes
 * ---------------------------------------------------------------------------*/
typedef enum {
    kSecurity_Success       = 0,   /*!< All phases passed (production)      */
    kSecurity_DevMode       = 1,   /*!< Running in development bypass mode  */
    kSecurity_WarnDevBoard  = 2,   /*!< OEM_OPEN lifecycle, non-production  */
    kSecurity_EleTestFail   = -1,  /*!< S1: ELE firmware self-test failed   */
    kSecurity_LifecycleFail = -2,  /*!< S2: Unexpected lifecycle state      */
    kSecurity_OtfadFail     = -3,  /*!< S3: OTFAD CTX1 key unwrap failed   */
    kSecurity_SessionFail   = -4,  /*!< A1: ELE_OpenSession failed          */
    kSecurity_KeystoreFail  = -5,  /*!< A2: ELE_OpenKeystore failed         */
    kSecurity_CalibFail     = -6,  /*!< A3: Calibration blob auth failed    */
    kSecurity_CalibMagic    = -7,  /*!< A3: Calibration magic word mismatch */
} security_status_t;

/* ---------------------------------------------------------------------------
 * Calibration data (populated by security_init phase A3)
 * Access via g_calib after successful init.
 * ---------------------------------------------------------------------------*/
#define CALIB_MAGIC  0xCA1B1A7EU

typedef struct {
    uint32_t magic;          /*!< Must equal CALIB_MAGIC after decode       */
    uint32_t version;        /*!< Blob version for field upgrade detection   */
    float    v_gain[3];      /*!< Voltage gain  [Va, Vb, Vc]  (LSB → V)    */
    float    v_offset[3];    /*!< Voltage offset [Va, Vb, Vc] (LSB)         */
    float    i_gain[4];      /*!< Current gain  [Ia, Ib, Ic, In] (LSB → A) */
    float    i_offset[4];    /*!< Current offset [Ia, Ib, Ic, In] (LSB)     */
} calib_data_t;              /* 64 bytes plaintext */

/* Global calibration data — readable by all tasks after init */
extern calib_data_t g_calib;

/* ---------------------------------------------------------------------------
 * ELE session / keystore handles (used by Phase 5 mbedTLS CAAM engine)
 * ---------------------------------------------------------------------------*/
extern uint32_t g_ele_session_id;
extern uint32_t g_ele_keystore_id;

/* ---------------------------------------------------------------------------
 * Flash layout for calibration blob
 *   Stored in the last 4 KB page of the 4 MB OTFAD CTX1 flash region.
 *   Address: BOARD_OTFAD_CTX1_BASE + BOARD_OTFAD_CTX1_SIZE - 4096
 * ---------------------------------------------------------------------------*/
#define CALIB_BLOB_FLASH_ADDR  (BOARD_OTFAD_CTX1_BASE + BOARD_OTFAD_CTX1_SIZE - 4096U)

#pragma pack(push, 1)
typedef struct {
    uint8_t  iv[12];          /*!< AES-GCM 96-bit nonce                     */
    uint8_t  ciphertext[64];  /*!< Encrypted calib_data_t                   */
    uint8_t  tag[16];         /*!< AES-GCM 128-bit authentication tag       */
    uint32_t version;         /*!< Must match calib_data_t.version          */
} calib_blob_flash_t;         /* 96 bytes */
#pragma pack(pop)

/* ---------------------------------------------------------------------------
 * Factory default calibration (used in dev-bypass / OEM_OPEN mode)
 * Values represent unity gain and zero offset (uncompensated).
 * Replace with real measurement after hardware bring-up.
 * ---------------------------------------------------------------------------*/
#define CALIB_DEFAULT_V_GAIN     1.0f
#define CALIB_DEFAULT_V_OFFSET   0.0f
#define CALIB_DEFAULT_I_GAIN     1.0f
#define CALIB_DEFAULT_I_OFFSET   0.0f

/* ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------*/

/**
 * @brief  Run the six-phase security bootstrap.
 *
 * Must be called from main() BEFORE BOARD_InitHardware() or task creation.
 * PRINTF (debug console) must already be initialised when
 * CONFIG_SECURITY_DEBUG_UART is enabled.
 *
 * @return kSecurity_Success      Production board, all phases passed.
 *         kSecurity_DevMode      Dev-bypass mode (button held at reset).
 *         kSecurity_WarnDevBoard OEM_OPEN board, degraded mode, non-fatal.
 *         < 0                    Fatal error — caller should halt.
 */
security_status_t security_init(void);

/**
 * @brief  Return a human-readable string for a security_status_t code.
 */
const char *security_status_str(security_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_INIT_H */
