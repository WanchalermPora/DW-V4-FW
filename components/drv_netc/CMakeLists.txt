# =============================================================================
# components/drv_netc/CMakeLists.txt — NETC 3-port Ethernet switch driver
# CM33-only (NETC peripheral clocked by CM33 domain).
# Phase 8: add fsl_netc SDK component dependency here.
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/drv_netc.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
