#!/usr/bin/env zsh
# ==============================================================================
# static.zsh — Configure & build CLogger as a static library
# ==============================================================================

set -euo pipefail

# ---- Defaults ---------------------------------------------------------------
BUILD_TYPE="Release"
PREFIX="/usr/local"
DO_INSTALL="no"

# ---- Parse args -------------------------------------------------------------
while (( $# > 0 )); do
  case "$1" in
    --debug)      BUILD_TYPE="Debug"; shift ;;
    --prefix)     PREFIX="${2:-}"; shift 2 ;;
    --no-install) DO_INSTALL="no"; shift ;;
    --install)    DO_INSTALL="yes"; shift ;;
    -h|--help)
      echo "Usage: $0 [--debug] [--prefix <dir>] [--install|--no-install]"
      exit 0
      ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# ---- Paths & environment ----------------------------------------------------
SCRIPT_DIR="${0:A:h}"

# Hardcode the *source* dir that contains CMakeLists.txt
ROOT_DIR="${SCRIPT_DIR:A}/../../clog"

if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "Error: CMakeLists.txt not found at ${ROOT_DIR}" >&2
  exit 1
fi

BUILD_DIR="${ROOT_DIR}/build/static-${BUILD_TYPE:l}"

# Detect CPU count for parallel builds
if [[ "$OSTYPE" == darwin* ]]; then
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
  JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

echo "==> Configuring static library"
echo "    Source dir:  ${ROOT_DIR}"
echo "    Build dir:   ${BUILD_DIR}"
echo "    Build type:  ${BUILD_TYPE}"
echo "    Install to:  ${PREFIX}"
echo "    Install now: ${DO_INSTALL}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DLOGGER_BUILD_STATIC=ON \
  -DLOGGER_BUILD_SHARED=OFF \
  -DLOGGER_BUILD_TESTS=OFF \
  -DLOGGER_INSTALL=ON \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}"

echo "==> Building with ${JOBS} jobs"
cmake --build "${BUILD_DIR}" -- -j "${JOBS}"

if [[ "${DO_INSTALL}" == "yes" ]]; then
  echo "==> Installing to ${PREFIX}"
  # use sudo only if prefix likely needs it and not already root
  SUDO=""
  if [[ "${EUID:-$(id -u)}" -ne 0 ]] && [[ "${PREFIX}" == /usr/* || "${PREFIX}" == /opt/* ]]; then
    SUDO="sudo"
  fi
  ${SUDO} cmake --install "${BUILD_DIR}"
fi

echo "==> Done."
echo "Static library should be at: ${BUILD_DIR}/liblogger.a (and in ${PREFIX}/lib if installed)"

# ================================================================================ 
# ================================================================================ 
# eof
