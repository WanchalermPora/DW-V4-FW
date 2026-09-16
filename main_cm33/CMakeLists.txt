# =============================================================================
# main_cm33/CMakeLists.txt — CM33 application entry point
# Included only by the cm33 build target.
# =============================================================================

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    app_main_cm33.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
