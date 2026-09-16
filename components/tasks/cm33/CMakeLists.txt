# =============================================================================
# components/tasks/cm33/CMakeLists.txt — CM33 I/O task stubs
# Included only by the cm33 build target.
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/task_ads131m08.c
    src/task_ptp_tc.c
    src/task_netc.c
    src/task_pmu_server.c
    src/task_rms_sentinel.c
    src/task_security_monitor.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
