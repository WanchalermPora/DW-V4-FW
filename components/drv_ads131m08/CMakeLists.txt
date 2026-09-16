# =============================================================================
# components/drv_ads131m08/CMakeLists.txt — ADS131M08 ADC driver
# CM33-only (LPSPI1 and DMA owned by CM33).
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/drv_ads131m08.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
