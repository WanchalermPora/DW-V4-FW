# =============================================================================
# DynaWatch V4 — Root CMakeLists.txt  (MCUXpresso SDK / NXP i.MX RT1186)
#
# Project:  DynaWatch V4  (PMU-P/M, PQ Meter, DFR, Smart Energy Meter)
# MCU:      NXP i.MX RT1186  (CM7 @ 600 MHz computation + CM33 @ 240 MHz I/O)
# SDK:      MCUXpresso SDK 2.x
# Toolchain: ARM GCC (armgcc.cmake)
#
# DUAL-CORE BUILD
# ───────────────
# Two independent firmware images are produced from a single source tree:
#
#   DynaWatch_V4_cm33.elf   — CM33 I/O core (boots first, owns all peripherals)
#   DynaWatch_V4_cm7.elf    — CM7 computation core (released by CM33 via MCMGR)
#
# The build target is selected by passing -DDW_CORE=CM33 or -DDW_CORE=CM7 to
# cmake.  armgcc/CMakeLists.txt invokes this root twice — once per core — so
# you typically run "ninja" once from armgcc/ and both images are built.
#
# Directory layout:
#
#   board/                  MCUXpresso Config Tools generated (pin_mux, clocks)
#   components/
#     ipc/                  OCRAM1 shared-memory layout (ipc_shared.h)  ← BOTH
#     security/             ELE security bootstrap (security_init)       ← CM33
#     tasks/
#       cm33/               CM33 I/O and housekeeping task stubs         ← CM33
#       cm7/                CM7 computation task stubs                   ← CM7
#     pow_buf/              PoW ring-buffer helpers                      ← BOTH
#     pmu/                  PMU TWLS types + LUT                        ← CM7
#   freertos/
#     cm33/                 FreeRTOSConfig.h for CM33 (240 MHz, 128 KB heap)
#     cm7/                  FreeRTOSConfig.h for CM7  (600 MHz, 256 KB heap)
#   main_cm33/              CM33 app entry point (app_main_cm33.c)
#   main_cm7/               CM7  app entry point (app_main_cm7.c)
#   armgcc/                 GCC toolchain scripts + per-core linker files
#   tools/                  Offline code-generation utilities
#
# Build (from armgcc/):
#   cmake -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=<SDK>/tools/cmake_toolchain_files/armgcc.cmake \
#         -DCMAKE_BUILD_TYPE=Release \
#         ..
#   ninja
#
# Before first build — generate TWLS LUT C files:
#   python3 tools/gen_twls_lut.py --out-dir components/pmu/src/lut/
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# MCUXpresso SDK project macro (sets toolchain, SDK path, etc.)
set(SDK_PATH "$ENV{MCUXPRESSO_SDK_ROOT}" CACHE PATH "MCUXpresso SDK root")

project(DynaWatch_V4 C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 14)

# ---------------------------------------------------------------------------
# Core selection — set by armgcc/CMakeLists.txt; default to CM33 so a direct
# cmake invocation (outside armgcc/) builds something sensible.
# Valid values: CM33  CM7
# ---------------------------------------------------------------------------
if(NOT DEFINED DW_CORE)
    set(DW_CORE "CM33")
endif()
message(STATUS "DynaWatch V4: building for core = ${DW_CORE}")

# ---------------------------------------------------------------------------
# Kconfig — single centralised file at the project root.
# All symbol definitions live in Kconfig; each component's CMakeLists.txt
# reads the generated autoconf.h via the compiler's include path.
#
# To run menuconfig (requires kconfiglib):
#   pip install kconfiglib
#   menuconfig Kconfig              ← interactive TUI
#   genconfig  --header-path sdkconfig.h Kconfig   ← generate header
# ---------------------------------------------------------------------------
set(KCONFIG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Kconfig")

if(EXISTS "${CMAKE_BINARY_DIR}/sdkconfig.h")
    target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
        "${CMAKE_BINARY_DIR}"
    )
endif()

# ---------------------------------------------------------------------------
# Common components — included by BOTH cores
# ---------------------------------------------------------------------------
add_subdirectory(board)
add_subdirectory(components/ipc)
add_subdirectory(components/pow_buf)

# ---------------------------------------------------------------------------
# Core-specific components
# ---------------------------------------------------------------------------
if(DW_CORE STREQUAL "CM33")

    # FreeRTOS config for CM33 (240 MHz)
    target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
        freertos/cm33
    )

    # Security bootstrap (owned by CM33)
    add_subdirectory(components/security)

    # Phase 5 peripheral drivers — CM33-only
    add_subdirectory(components/drv_audiopll)
    add_subdirectory(components/drv_sai)
    add_subdirectory(components/drv_stimer)
    add_subdirectory(components/drv_ads131m08)
    add_subdirectory(components/drv_netc)

    # CM33 I/O task stubs
    add_subdirectory(components/tasks/cm33)

    # CM33 application entry point
    add_subdirectory(main_cm33)

elseif(DW_CORE STREQUAL "CM7")

    # FreeRTOS config for CM7 (600 MHz)
    target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
        freertos/cm7
    )

    # PMU TWLS types and LUT (computation-core only)
    add_subdirectory(components/pmu)

    # CM7 computation task stubs
    add_subdirectory(components/tasks/cm7)

    # CM7 application entry point
    add_subdirectory(main_cm7)

else()
    message(FATAL_ERROR "DW_CORE must be CM33 or CM7 (got '${DW_CORE}')")
endif()
