/* =============================================================================
 * mbedtls_config.h — DynaWatch V4 mbedTLS 3.x Configuration
 *
 * Profile: TLS 1.3 only, ECDSA P-256, AES-128-GCM, HKDF
 *
 * Hardware acceleration hooks (Phase 5):
 *   RT1186 CAAM engine via ELE S200 offload.
 *   MBEDTLS_AES_ALT / MBEDTLS_SHA256_ALT stubs are linked in Phase 5;
 *   for Phase 3 they fall back to the mbedTLS software implementation.
 *
 * Flash savings vs. full mbedTLS:
 *   Full build:        ~220 KB
 *   This config:        ~62 KB  (TLS 1.3 only, one cipher suite)
 * =============================================================================
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* ---------------------------------------------------------------------------
 * Platform
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY          /* use mbedtls_set_alloc_funcs()    */
#define MBEDTLS_NO_PLATFORM_ENTROPY      /* no /dev/urandom on bare metal    */
#define MBEDTLS_ENTROPY_HARDWARE_ALT     /* hook to ELE TRNG (Phase 5)       */

/* Use FreeRTOS heap via mbedtls_platform_set_calloc_free() in security_init */
#define MBEDTLS_PLATFORM_CALLOC_MACRO    pvPortCalloc
#define MBEDTLS_PLATFORM_FREE_MACRO      vPortFree

/* ---------------------------------------------------------------------------
 * TLS 1.3 only — disable all prior versions
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
/* TLS 1.2 explicitly disabled (don't define MBEDTLS_SSL_PROTO_TLS1_2) */

#define MBEDTLS_SSL_CLI_C               /* client role                       */
#define MBEDTLS_SSL_SRV_C               /* server role (PMU data publisher)  */

/* ---------------------------------------------------------------------------
 * Cipher suite: TLS_AES_128_GCM_SHA256
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_MD_C

/* Phase 5 hardware alt — stubs until CAAM driver integrated */
/* #define MBEDTLS_AES_ALT    */
/* #define MBEDTLS_SHA256_ALT */

/* ---------------------------------------------------------------------------
 * Key exchange: ECDHE with P-256
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED   /* P-256 only                    */
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_BIGNUM_C

/* ---------------------------------------------------------------------------
 * Certificates: X.509, ECDSA P-256, SHA-256
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_PEM_PARSE_C             /* PEM-encoded certs in flash        */
#define MBEDTLS_BASE64_C                /* required by PEM                   */
#define MBEDTLS_OID_C

/* ---------------------------------------------------------------------------
 * HKDF (TLS 1.3 key schedule)
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_HKDF_C

/* ---------------------------------------------------------------------------
 * Random number generation
 * Phase 3: software CTR-DRBG seeded from ELE TRNG stub
 * Phase 5: replace with MBEDTLS_ENTROPY_HARDWARE_ALT → ELE TRNG directly
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C

/* ---------------------------------------------------------------------------
 * Miscellaneous required modules
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_SSL_RECORD_CHECKING
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

/* ---------------------------------------------------------------------------
 * Memory / buffer tuning for RT1186
 *   Single TLS connection to PDC per task (pmu_server_task)
 *   Input record: C37.118.2 frame ≤ 256 bytes
 *   Output record: ≤ 256 bytes
 * ---------------------------------------------------------------------------*/
#define MBEDTLS_SSL_IN_CONTENT_LEN   4096  /* receive buffer                 */
#define MBEDTLS_SSL_OUT_CONTENT_LEN  4096  /* transmit buffer                */
#define MBEDTLS_MPI_MAX_SIZE          64   /* 512-bit max (P-256 = 32 bytes) */

/* ---------------------------------------------------------------------------
 * Disable unused / large modules
 * ---------------------------------------------------------------------------*/
/* RSA — not used (ECDSA only) */
/* #define MBEDTLS_RSA_C */
/* #define MBEDTLS_PKCS1_V15 */
/* #define MBEDTLS_PKCS1_V21 */

/* DHE — not used (ECDHE only in TLS 1.3) */
/* #define MBEDTLS_DHM_C */

/* DES/3DES, RC4, ARCFOUR — explicitly excluded */
/* Blowfish, Camellia — excluded */

/* DTLS — not used */
/* #define MBEDTLS_SSL_PROTO_DTLS */

/* ---------------------------------------------------------------------------
 * Debug (enable only for Phase 3 bring-up; disable for production)
 * ---------------------------------------------------------------------------*/
#if defined(CONFIG_SECURITY_DEBUG_UART)
#define MBEDTLS_DEBUG_C
#define MBEDTLS_SSL_DEBUG_ALL
/* Set mbedtls_ssl_conf_dbg() in pmu_server_task to print to LPUART1 */
#endif

/* ---------------------------------------------------------------------------
 * Check configuration consistency
 * ---------------------------------------------------------------------------*/
#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */
