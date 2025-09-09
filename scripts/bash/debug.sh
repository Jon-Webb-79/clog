#!/usr/bin/env zsh
# ================================================================================
# debug.zsh — Configure & build CLogger in Debug mode (with tests)
# ================================================================================

set -euo pipefail

# Hardcoded project source dir (directory containing CMakeLists.txt)
SRC_DIR="/home/jonwebb/Code_Dev/C/clog/clog"
BUILD_DIR="${SRC_DIR}/build/debug"

# Detect CPU count for parallel build jobs
if [[ "$OSTYPE" == darwin* ]]; then
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
  JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

echo "==> Source: ${SRC_DIR}"
echo "==> Build:  ${BUILD_DIR}"
echo "==> Configuring (Debug, tests ON)"

cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLOGGER_BUILD_TESTS=ON \
  -DLOGGER_BUILD_STATIC=ON \
  -DLOGGER_BUILD_SHARED=OFF

echo "==> Building with ${JOBS} jobs"
cmake --build "${BUILD_DIR}" -- -j "${JOBS}"

echo "==> Running unit tests"
ctest --test-dir "${BUILD_DIR}" --output-on-failure
# ================================================================================ 
# ================================================================================ 
# eof
