# =============================================================================
# components/tasks/cm7/CMakeLists.txt — CM7 computation task stubs
# Included only by the cm7 build target.
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    src/task_twls_pmu_p.c
    src/task_twls_pmu_m.c
    src/task_flickermeter.c
    src/task_pqm_sem.c
    src/task_dfr.c
    src/task_energy_meter.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    include
)
