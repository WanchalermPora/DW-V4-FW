# =============================================================================
# armgcc/CMakeLists.txt — Toolchain and linker settings for ARM GCC
#
# Produces TWO firmware images from a single cmake invocation:
#
#   DynaWatch_V4_cm33.elf + .bin  — CM33 I/O core  (240 MHz)
#   DynaWatch_V4_cm7.elf  + .bin  — CM7  compute core (600 MHz)
#
# Usage (from this directory):
#   cmake -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=<SDK>/tools/cmake_toolchain_files/armgcc.cmake \
#         -DCMAKE_BUILD_TYPE=Release \
#         ..
#   ninja
#
# Linker scripts expected in this directory:
#   MIMXRT1186xxxxx_cm33_flash.ld
#   MIMXRT1186xxxxx_cm7_flash.ld
#
# The CM7 linker script must export APP_CM7_VECTOR_TABLE_ADDRESS so that
# app_main_cm33.c can pass it to MCMGR_StartCore().
# =============================================================================

cmake_minimum_required(VERSION 3.25)

# ─── Toolchain ──────────────────────────────────────────────────────────────
set(CMAKE_SYSTEM_NAME  Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
find_program(ARM_GCC     ${TOOLCHAIN_PREFIX}gcc     REQUIRED)
find_program(ARM_GPP     ${TOOLCHAIN_PREFIX}g++     REQUIRED)
find_program(ARM_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy REQUIRED)
find_program(ARM_SIZE    ${TOOLCHAIN_PREFIX}size    REQUIRED)

set(CMAKE_C_COMPILER   ${ARM_GCC})
set(CMAKE_CXX_COMPILER ${ARM_GPP})
set(CMAKE_ASM_COMPILER ${ARM_GCC})

# ─── Common compile flags ────────────────────────────────────────────────────
set(COMMON_C_FLAGS "-ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_C_FLAGS} -fno-exceptions -fno-rtti")

# ─── Build-type flags ────────────────────────────────────────────────────────
set(CMAKE_C_FLAGS_DEBUG   "-Og -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# ─── Post-build helper ───────────────────────────────────────────────────────
function(add_arm_post_build TARGET BIN_NAME)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${ARM_OBJCOPY} -O binary
                $<TARGET_FILE:${TARGET}>
                ${BIN_NAME}.bin
        COMMAND ${ARM_SIZE} --format=berkeley $<TARGET_FILE:${TARGET}>
        COMMENT "Generating ${BIN_NAME}.bin and printing size..."
        VERBATIM
    )
endfunction()

# ===========================================================================
# CM33 IMAGE — I/O and housekeeping core
# ===========================================================================
set(CM33_TARGET DynaWatch_V4_cm33)
set(CM33_CPU_FLAGS
    "-mcpu=cortex-m33"
    "-mfpu=fpv5-sp-d16"
    "-mfloat-abi=hard"
    "-mthumb"
)
string(JOIN " " CM33_CPU_FLAGS_STR ${CM33_CPU_FLAGS})

add_executable(${CM33_TARGET})
set_target_properties(${CM33_TARGET} PROPERTIES
    OUTPUT_NAME "${CM33_TARGET}"
)

target_compile_options(${CM33_TARGET} PRIVATE
    ${CM33_CPU_FLAGS}
    -Wall -Wextra -Werror=implicit-function-declaration
    -ffunction-sections -fdata-sections
)

target_compile_definitions(${CM33_TARGET} PRIVATE
    CPU_MIMXRT1186AVM8A_cm33
    DW_CORE_CM33=1
    MBEDTLS_CONFIG_FILE="mbedtls_config.h"
)

set(CM33_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/MIMXRT1186xxxxx_cm33_flash.ld")
target_link_options(${CM33_TARGET} PRIVATE
    ${CM33_CPU_FLAGS}
    -T${CM33_LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${CM33_TARGET}.map
    --specs=nano.specs
    --specs=nosys.specs
)

# Include the root CMakeLists as CM33 so it adds the right subdirectories
set(MCUX_SDK_PROJECT_NAME ${CM33_TARGET})
set(DW_CORE "CM33")
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt)

add_arm_post_build(${CM33_TARGET} ${CM33_TARGET})

# ===========================================================================
# CM7 IMAGE — Computation-only core
# ===========================================================================
set(CM7_TARGET DynaWatch_V4_cm7)
set(CM7_CPU_FLAGS
    "-mcpu=cortex-m7"
    "-mfpu=fpv5-d16"
    "-mfloat-abi=hard"
    "-mthumb"
)
string(JOIN " " CM7_CPU_FLAGS_STR ${CM7_CPU_FLAGS})

add_executable(${CM7_TARGET})
set_target_properties(${CM7_TARGET} PROPERTIES
    OUTPUT_NAME "${CM7_TARGET}"
)

target_compile_options(${CM7_TARGET} PRIVATE
    ${CM7_CPU_FLAGS}
    -Wall -Wextra -Werror=implicit-function-declaration
    -ffunction-sections -fdata-sections
)

target_compile_definitions(${CM7_TARGET} PRIVATE
    CPU_MIMXRT1186AVM8A_cm7
    DW_CORE_CM7=1
)

set(CM7_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/MIMXRT1186xxxxx_cm7_flash.ld")
target_link_options(${CM7_TARGET} PRIVATE
    ${CM7_CPU_FLAGS}
    -T${CM7_LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${CM7_TARGET}.map
    --specs=nano.specs
    --specs=nosys.specs
)

# Include the root CMakeLists as CM7
set(MCUX_SDK_PROJECT_NAME ${CM7_TARGET})
set(DW_CORE "CM7")
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt)

add_arm_post_build(${CM7_TARGET} ${CM7_TARGET})

# ===========================================================================
# Convenience target: build both images
# ===========================================================================
add_custom_target(all_cores ALL
    DEPENDS ${CM33_TARGET} ${CM7_TARGET}
    COMMENT "Built both CM33 and CM7 firmware images"
)
