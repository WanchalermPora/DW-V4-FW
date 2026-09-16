#!/usr/bin/env bash
# =============================================================================
# build_release.sh — DynaWatch V4 release build (ARM GCC)
#
# Usage:
#   ./armgcc/build_release.sh
#
# See build_debug.sh for environment variables.
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build/release"

if [[ -z "${MCUXPRESSO_SDK_ROOT:-}" ]]; then
    echo "ERROR: MCUXPRESSO_SDK_ROOT is not set." >&2
    echo "       export MCUXPRESSO_SDK_ROOT=<path to MCUXpresso SDK>" >&2
    exit 1
fi

# Regenerate LUTs if Python is available (idempotent)
if command -v python3 &>/dev/null; then
    echo "Regenerating TWLS LUT files…"
    python3 "${REPO_ROOT}/tools/gen_twls_lut.py" \
        --out-dir "${REPO_ROOT}/components/pmu/src/lut/"
fi

cmake -S "${REPO_ROOT}" \
      -B "${BUILD_DIR}" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DMCUXPRESSO_SDK_ROOT="${MCUXPRESSO_SDK_ROOT}" \
      -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/CMakeLists.txt"

cmake --build "${BUILD_DIR}" -- -j"$(nproc)"

echo ""
echo "Release build complete → ${BUILD_DIR}"
echo "Flash image: ${BUILD_DIR}/DynaWatch-V4.bin"
