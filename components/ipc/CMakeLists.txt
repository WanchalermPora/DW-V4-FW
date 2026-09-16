# =============================================================================
# components/ipc/CMakeLists.txt
# Header-only component — shared between cm7 and cm33 build targets.
# The linker script for each core places g_shared at 0x20240000 (OCRAM1).
# =============================================================================

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
