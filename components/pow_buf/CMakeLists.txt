# =============================================================================
# pow_buf component — Shared OCRAM1 Power-of-Words ring buffer
#
# Header-only component: pow_buf.h provides the extern declarations and the
# inline pow_window_assemble() helper.  The actual buffer variables
# (pow_ring, pow_double, pow_tail) are defined in the CM33 I/O driver layer
# (cm33/source/pow_buf.c) which owns all OCRAM1 allocations.
# =============================================================================

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# No .c sources — all functionality is inline in pow_buf.h.
# If a non-inline pow_buf.c is added later, list it here:
# target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
#     pow_buf.c
# )
