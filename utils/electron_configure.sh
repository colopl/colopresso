#!/bin/sh
set -eu

if test $# -lt 1; then
    echo "Usage: ${0} <BuildType> [targets...]" >&2
    exit 1
fi

BUILD_TYPE="${1}"
shift

TARGETS_ARGS=""
ARCH_ARG=""

while test $# -gt 0; do
    case "${1}" in
        --arch=*)
            ARCH_ARG="${1#--arch=}"
            ;;
        --arch)
            shift
            if test $# -gt 0; then
                ARCH_ARG="${1}"
            fi
            ;;
        --targets=*)
            TARGETS_ARGS="${TARGETS_ARGS} ${1#--targets=}"
            ;;
        --targets)
            shift
            if test $# -gt 0; then
                TARGETS_ARGS="${TARGETS_ARGS} ${1}"
            fi
            ;;
        *)
            TARGETS_ARGS="${TARGETS_ARGS} ${1}"
            ;;
    esac
    shift
done

TARGETS=$(printf %s "${TARGETS_ARGS}" | sed -e 's/^ *//' -e 's/ *$//')

if test -n "${npm_config_targets-}"; then
    TARGETS="${npm_config_targets}"
fi

if test -n "${npm_config_arch-}"; then
    ARCH_ARG="${npm_config_arch}"
fi

if test -z "${TARGETS}" && test -n "${npm_config_argv-}"; then
    TARGETS=$(node -e 'const argvStr = process.env.npm_config_argv; let result = ""; if (argvStr) { try { const data = JSON.parse(argvStr); const list = (data && (data.original || data.cooked)) || []; for (let i = 0; i < list.length; i++) { const arg = list[i]; if (typeof arg === "string") { if (arg.startsWith("--targets=")) { result = arg.slice(10); break; } if (arg === "--targets" && i + 1 < list.length) { result = list[i + 1]; break; } } } } catch (_) {} } process.stdout.write(result);')
fi

TARGETS=$(printf %s "${TARGETS}" | tr ',' ' ')
set -- ${TARGETS}

if test $# -eq 0; then
    if test "${OS-}" = "Windows_NT"; then
        TARGETS="--win"
    else
        TARGETS="--mac"
    fi
else
    TARGETS="${1}"
    shift
    for T in "$@"; do
        TARGETS="${TARGETS}, ${T}"
    done
fi

ARCH_CMAKE_ARG=""
if test -n "${ARCH_ARG}"; then
    ARCH_CMAKE_ARG="-DCOLOPRESSO_ELECTRON_ARCH=${ARCH_ARG}"
fi

cmake \
  -B build \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCOLOPRESSO_ELECTRON_APP=ON \
  -DCOLOPRESSO_ELECTRON_NATIVE_ADDON=ON \
  -DCOLOPRESSO_ELECTRON_TARGETS="${TARGETS}" \
  ${ARCH_CMAKE_ARG}
