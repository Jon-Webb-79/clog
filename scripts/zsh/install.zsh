#!/usr/bin/zsh
# ==============================================================================
# install.zsh — Configure, build, and install CLogger as a system library
#
# Usage:
#   scripts/zsh/install.zsh [--prefix /usr/local] [--shared|--static|--both]
#                           [--no-sudo] [--build-dir <dir>] [--debug]
#
# Examples:
#   scripts/zsh/install.zsh                       # Release, static-only, prefix=/usr/local, uses sudo
#   scripts/zsh/install.zsh --shared --no-sudo    # Release, shared-only, user-writable prefix
#   scripts/zsh/install.zsh --both --prefix /opt/clog
#   scripts/zsh/install.zsh --debug               # Debug build (for dev installs)
# ==============================================================================

set -euo pipefail

# ---- Defaults ---------------------------------------------------------------
PREFIX="/usr/local"
BUILD_TYPE="Release"
BUILD_KIND="static"   # static|shared|both
USE_SUDO="yes"
BUILD_DIR=""          # auto if empty; defaults to <repo>/build/install-<kind>-<type>

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
    -h|--help)
      echo "Usage: $0 [--prefix /usr/local] [--shared|--static|--both] [--no-sudo] [--build-dir <dir>] [--debug]"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# ---- Paths & env ------------------------------------------------------------
SCRIPT_DIR="${0:A:h}"
ROOT_DIR="${SCRIPT_DIR:A}/../.."  # repo root
if [[ -z "${BUILD_DIR}" ]]; then
  BUILD_DIR="${ROOT_DIR}/build/install-${BUILD_KIND}-${BUILD_TYPE:l}"
fi

# Parallel jobs
if [[ "$OSTYPE" == darwin* ]]; then
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
  JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

# CMake switches for static/shared selection
cmake_static="OFF"
cmake_shared="OFF"
case "${BUILD_KIND}" in
  static) cmake_static="ON" ;;
  shared) cmake_shared="ON" ;;
  both)   cmake_static="ON"; cmake_shared="ON" ;;
esac

# sudo helper
SUDO=""
if [[ "${USE_SUDO}" == "yes" ]]; then
  # Only use sudo if PREFIX is likely to need it and we’re not already root
  if [[ "${EUID:-$(id -u)}" -ne 0 ]] && [[ "${PREFIX}" == /usr/* || "${PREFIX}" == /opt/* ]]; then
    SUDO="sudo"
  fi
fi

echo "==> Configuring"
echo "    Root:       ${ROOT_DIR}"
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
echo "    Headers: $(print -r -- "${PREFIX}/include/logger.h")"
if [[ "${cmake_static}" == "ON" ]]; then
  echo "    Static:  $(print -r -- "${PREFIX}/lib/liblogger.a")"
fi
if [[ "${cmake_shared}" == "ON" ]]; then
  # Platform-specific shared suffix
  case "$OSTYPE" in
    darwin*) soext="dylib" ;;
    msys*|cygwin*|win32) soext="dll" ;;
    *)       soext="so" ;;
  esac
  echo "    Shared:  $(print -r -- "${PREFIX}/lib/liblogger.${soext}")"
fi
# ================================================================================ 
# ================================================================================ 
# eof
