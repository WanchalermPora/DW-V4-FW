# =============================================================================
# components/security/CMakeLists.txt
#
# Security bootstrap component: ELE S200 six-phase init + mbedTLS config.
#
# External dependencies (provided by MCUXpresso SDK):
#   ele_crypto          — ELE S200 messaging-unit driver
#   mbedtls             — mbedTLS 3.x (SDK component: component.mbedtls)
#   fsl_debug_console   — PRINTF() over LPUART1
#   fsl_gpio            — LED / button GPIO
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/security_init.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)

# ---------------------------------------------------------------------------
# mbedTLS: tell the SDK mbedTLS component to use our custom config header
# instead of the default one.  The SDK checks for MBEDTLS_CONFIG_FILE.
# ---------------------------------------------------------------------------
target_compile_definitions(${MCUX_SDK_PROJECT_NAME} PRIVATE
    MBEDTLS_CONFIG_FILE="mbedtls_config.h"
)

# ---------------------------------------------------------------------------
# SDK component dependencies (MCUXpresso component name convention)
# ---------------------------------------------------------------------------
include(${SDK_PATH}/components/ele_crypto/component_ele_crypto.cmake    OPTIONAL)
include(${SDK_PATH}/components/mbedtls/component_mbedtls.cmake          OPTIONAL)
include(${SDK_PATH}/devices/MIMXRT1186/drivers/fsl_gpio.cmake           OPTIONAL)
