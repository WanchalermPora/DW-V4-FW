# =============================================================================
# components/drv_sai/CMakeLists.txt — SAI4 master-mode driver
# CM33-only (SAI4 peripheral owned by CM33).
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/drv_sai.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
