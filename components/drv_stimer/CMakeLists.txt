# =============================================================================
# components/drv_stimer/CMakeLists.txt — STIMER 32.768 MHz reference driver
# CM33-only (SNVS/STIMER owned by CM33 secure domain).
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/drv_stimer.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
