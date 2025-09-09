#!/usr/bin/env zsh
# ==============================================================================
# install.zsh — Configure, build, and install CLogger as a system library
# ==============================================================================

set -euo pipefail

# ---- Defaults ---------------------------------------------------------------
PREFIX="/usr/local"
BUILD_TYPE="Release"
BUILD_KIND="static"   # static|shared|both
USE_SUDO="yes"
BUILD_DIR=""

# ---- Parse args -------------------------------------------------------------
while (( $# > 0 )); do
  case "$1" in
    --prefix)      PREFIX="${2:-}"; shift 2 ;;
    --shared)      BUILD_KIND="shared"; shift ;;
    --static)      BUILD_KIND="static"; shift ;;
    --both)        BUILD_KIND="both"; shift ;;
    --no-sudo)     USE_SUDO="no"; shift ;;
    --build-dir)   BUILD_DIR="${2:-}"; shift 2 ;;
    --debug)       BUILD_TYPE="Debug"; shift ;;
    -h|--help)     echo "Usage: $0 [--prefix /usr/local] [--shared|--static|--both] [--no-sudo] [--build-dir <dir>] [--debug]"; exit 0 ;;
    *)             echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# ---- Paths ------------------------------------------------------------------
SCRIPT_DIR="${0:A:h}"

# Hardcode the *source* directory where CMakeLists.txt lives:
ROOT_DIR="${SCRIPT_DIR:A}/../../clog"

# Optional: fail fast if CMakeLists.txt isn't there
if [[ ! -f "${ROOT_DIR}/CMakeLists.txt" ]]; then
  echo "Error: CMakeLists.txt not found at ${ROOT_DIR}" >&2
  echo "Check your tree; expected header at ${ROOT_DIR}/include/logger.h and source at ${ROOT_DIR}/logger.c" >&2
  exit 1
fi

# Build dir default
if [[ -z "${BUILD_DIR}" ]]; then
  BUILD_DIR="${ROOT_DIR}/build/install-${BUILD_KIND}-${BUILD_TYPE:l}"
fi

# ---- Parallel jobs ----------------------------------------------------------
if [[ "$OSTYPE" == darwin* ]]; then
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
  JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

# ---- Variant switches -------------------------------------------------------
cmake_static="OFF"; cmake_shared="OFF"
case "${BUILD_KIND}" in
  static) cmake_static="ON" ;;
  shared) cmake_shared="ON" ;;
  both)   cmake_static="ON"; cmake_shared="ON" ;;
esac

# ---- sudo helper ------------------------------------------------------------
SUDO=""
if [[ "${USE_SUDO}" == "yes" ]]; then
  if [[ "${EUID:-$(id -u)}" -ne 0 ]] && [[ "${PREFIX}" == /usr/* || "${PREFIX}" == /opt/* ]]; then
    SUDO="sudo"
  fi
fi

echo "==> Configuring"
echo "    Source:     ${ROOT_DIR}"
echo "    Build dir:  ${BUILD_DIR}"
echo "    Build type: ${BUILD_TYPE}"
echo "    Kind:       ${BUILD_KIND} (static=${cmake_static}, shared=${cmake_shared})"
echo "    Prefix:     ${PREFIX}"
echo "    Sudo:       ${SUDO:+yes}${SUDO:-no}"
echo

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DLOGGER_BUILD_STATIC="${cmake_static}" \
  -DLOGGER_BUILD_SHARED="${cmake_shared}" \
  -DLOGGER_BUILD_TESTS=OFF \
  -DLOGGER_INSTALL=ON

echo "==> Building (${JOBS} jobs)"
cmake --build "${BUILD_DIR}" -- -j "${JOBS}"

echo "==> Installing to ${PREFIX}"
${SUDO} cmake --install "${BUILD_DIR}"

echo "==> Done."
echo "    Headers: ${PREFIX}/include/logger.h"
if [[ "${cmake_static}" == "ON" ]]; then
  echo "    Static:  ${PREFIX}/lib/liblogger.a"
fi
if [[ "${cmake_shared}" == "ON" ]]; then
  case "$OSTYPE" in
    darwin*) soext="dylib" ;;
    msys*|cygwin*|win32) soext="dll" ;;
    *)       soext="so" ;;
  esac
  echo "    Shared:  ${PREFIX}/lib/liblogger.${soext}"
fi

# ================================================================================ 
# ================================================================================ 
# eof
