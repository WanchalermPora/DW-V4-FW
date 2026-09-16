# =============================================================================
# board — Board support package (BSP)
#
# Sources here are stubs; replace board.c, clock_config.c, pin_mux.c
# with MCUXpresso Config Tools output before building.
# =============================================================================

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/clock_config.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pin_mux.c
)
