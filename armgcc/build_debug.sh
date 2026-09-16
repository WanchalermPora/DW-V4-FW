#!/usr/bin/env bash
# =============================================================================
# build_debug.sh — DynaWatch V4 debug build (ARM GCC)
#
# Usage:
#   ./armgcc/build_debug.sh
#
# Environment:
#   MCUXPRESSO_SDK_ROOT  Path to the MCUXpresso SDK root (required).
#                        e.g. ~/sdk/mcuxpresso-sdk-2.16.0
#   ARMGCC_DIR           Path to the arm-none-eabi toolchain bin dir.
#                        Defaults to the directory containing arm-none-eabi-gcc
#                        as found on PATH.
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build/debug"

if [[ -z "${MCUXPRESSO_SDK_ROOT:-}" ]]; then
    echo "ERROR: MCUXPRESSO_SDK_ROOT is not set." >&2
    echo "       export MCUXPRESSO_SDK_ROOT=<path to MCUXpresso SDK>" >&2
    exit 1
fi

cmake -S "${REPO_ROOT}" \
      -B "${BUILD_DIR}" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DMCUXPRESSO_SDK_ROOT="${MCUXPRESSO_SDK_ROOT}" \
      -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/CMakeLists.txt"

cmake --build "${BUILD_DIR}" -- -j"$(nproc)"

echo ""
echo "Debug build complete → ${BUILD_DIR}"
echo "Flash image: ${BUILD_DIR}/DynaWatch-V4.bin"
