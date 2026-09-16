# =============================================================================
# main_cm7/CMakeLists.txt — CM7 application entry point
# Included only by the cm7 build target.
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    app_main_cm7.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
