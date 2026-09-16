# =============================================================================
# components/drv_audiopll/CMakeLists.txt — AudioPLL runtime steering driver
# CM33-only (audio PLL owned by CM33).
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/drv_audiopll.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
