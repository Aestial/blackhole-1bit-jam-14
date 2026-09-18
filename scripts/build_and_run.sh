#!/usr/bin/env bash
# =============================================================================
# build_and_run.sh — Build Arduboy Sketch and Run in ProjectABE Emulator
# =============================================================================
#
# Usage:
#   ./build.sh [OPTIONS]
#   ./scripts/build_and_run.sh [OPTIONS]
#
# Options:
#   --skin <bare|arduboy>   Emulator skin mode:
#                             'bare'    : Pure game screen, no skin (default)
#                             'arduboy' : Compact Arduboy casing (Arduboy-off.png)
#   -b, --build-only        Compile and sync hex only, do not launch browser
#   -p, --port <port>       Local web server port (default: 8000)
#   -c, --clean             Clean build cache before compilation
#   --package               Package dist/web into dist/supermassive-whitehole-web.zip for itch.io
#   -h, --help              Show this help message
#
# =============================================================================

set -euo pipefail

# Repo root detection
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

# ANSI Colors
COLOR_RESET="\033[0m"
COLOR_BOLD="\033[1m"
COLOR_GREEN="\033[32m"
COLOR_YELLOW="\033[33m"
COLOR_CYAN="\033[36m"
COLOR_RED="\033[31m"

# Default configuration
FQBN="arduboy-homemade:avr:arduboy"
PORT=8000
BUILD_ONLY=false
CLEAN=false
PACKAGE=false
SKIN="bare" # 'bare' (game screen only) or 'arduboy' (compact casing)

# Print helper functions
log_info()    { echo -e "${COLOR_CYAN}[INFO]${COLOR_RESET} $*"; }
log_success() { echo -e "${COLOR_GREEN}[SUCCESS]${COLOR_RESET} $*"; }
log_warn()    { echo -e "${COLOR_YELLOW}[WARN]${COLOR_RESET} $*"; }
log_error()   { echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} $*"; }

show_help() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Automated build and emulation script for Supermassive Whitehole (Arduboy).

Options:
  --skin <bare|arduboy>   Emulator skin mode:
                            'bare'    : Pure game screen, no skin (default on desktop)
                            'arduboy' : Handheld Arduboy skin with buttons (Arduboy (8).png, default on mobile)
  --mobile                Shortcut for '--skin arduboy'
  -b, --build-only        Compile and copy binaries only, skip emulator launch
  -p, --port <port>       HTTP server port (default: 8000)
  -c, --clean             Clean build directory before compiling
  --package               Create dist/supermassive-whitehole-web.zip for itch.io release
  -h, --help              Show this help message

Examples:
  ./build.sh                      # Build and launch clean game screen
  ./build.sh --mobile             # Build and launch with Arduboy (8) skin and buttons
  ./build.sh --skin arduboy       # Build and launch with Arduboy (8) skin
  ./build.sh --build-only         # Compile and update dist/ without browser
  ./build.sh --package            # Build and create distribution zip for itch.io
EOF
    exit 0
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --skin)
            SKIN="$2"
            shift 2
            ;;
        --skin=*)
            SKIN="${1#*=}"
            shift 1
            ;;
        --mobile)
            SKIN="arduboy"
            shift 1
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift 1
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        --port=*)
            PORT="${1#*=}"
            shift 1
            ;;
        -c|--clean)
            CLEAN=true
            shift 1
            ;;
        --package)
            PACKAGE=true
            shift 1
            ;;
        -h|--help)
            show_help
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            ;;
    esac
done

# Validate skin choice
if [[ "${SKIN}" != "bare" && "${SKIN}" != "arduboy" ]]; then
    log_warn "Invalid skin '${SKIN}', defaulting to 'bare'."
    SKIN="bare"
fi

echo -e "${COLOR_BOLD}======================================================${COLOR_RESET}"
echo -e "${COLOR_BOLD}   Supermassive Whitehole — Arduboy Build Pipeline    ${COLOR_RESET}"
echo -e "${COLOR_BOLD}======================================================${COLOR_RESET}"

# 1. Clean if requested
if [ "${CLEAN}" = true ]; then
    log_info "Cleaning build directory..."
    rm -rf "${ROOT_DIR}/build"
fi

# 2. Check for arduino-cli
if ! command -v arduino-cli &>/dev/null; then
    log_error "arduino-cli not found in PATH. Please install arduino-cli."
    exit 1
fi

# 3. Compile sketch
log_info "Compiling sketch with FQBN: ${FQBN}..."
if ! arduino-cli compile --output-dir "${ROOT_DIR}/build" --fqbn "${FQBN}" ./; then
    log_error "Compilation failed!"
    exit 1
fi
log_success "Compilation succeeded!"

# 4. Synchronize distribution artifacts
log_info "Synchronizing distribution artifacts..."
mkdir -p "${ROOT_DIR}/dist/web"

# Dynamically discover sketch name from .ino in root
INO_FILE=$(ls -1 "${ROOT_DIR}"/*.ino 2>/dev/null | head -n 1)
SKETCH_NAME=$(basename "${INO_FILE}" .ino)

SRC_HEX="${ROOT_DIR}/build/${SKETCH_NAME}.ino.hex"
SRC_ELF="${ROOT_DIR}/build/${SKETCH_NAME}.ino.elf"

if [ ! -f "${SRC_HEX}" ]; then
    FOUND_HEX=$(find "${ROOT_DIR}/build" -name "*.hex" ! -name "*bootloader*" 2>/dev/null | head -n 1)
    if [ -n "${FOUND_HEX}" ]; then
        SRC_HEX="${FOUND_HEX}"
    else
        log_error "Built hex file not found in ${ROOT_DIR}/build"
        exit 1
    fi
fi

# Hardware binaries for flashing / debug
cp "${SRC_HEX}" "${ROOT_DIR}/dist/supermassive-whitehole.hex"
cp "${SRC_HEX}" "${ROOT_DIR}/dist/whitehole.hex"
cp "${SRC_HEX}" "${ROOT_DIR}/dist/${SKETCH_NAME}.hex"
if [ -f "${SRC_ELF}" ]; then
    cp "${SRC_ELF}" "${ROOT_DIR}/dist/supermassive-whitehole.elf"
    cp "${SRC_ELF}" "${ROOT_DIR}/dist/whitehole.elf"
    cp "${SRC_ELF}" "${ROOT_DIR}/dist/${SKETCH_NAME}.elf"
fi

# Web emulator binaries
cp "${SRC_HEX}" "${ROOT_DIR}/dist/web/ArduboyProject.hex"

# Also sync to root html5/ if present
if [ -d "${ROOT_DIR}/html5" ]; then
    cp "${SRC_HEX}" "${ROOT_DIR}/html5/ArduboyProject.hex"
fi

# Synchronize skin image
if [ -f "${ROOT_DIR}/html5/Arduboy (8).png" ] && [ ! -f "${ROOT_DIR}/dist/web/Arduboy (8).png" ]; then
    cp "${ROOT_DIR}/html5/Arduboy (8).png" "${ROOT_DIR}/dist/web/Arduboy (8).png"
fi

HEX_SIZE=$(stat -c%s "${ROOT_DIR}/dist/supermassive-whitehole.hex" 2>/dev/null || stat -f%z "${ROOT_DIR}/dist/supermassive-whitehole.hex")
log_success "Synchronized dist/supermassive-whitehole.hex (${HEX_SIZE} bytes)"
log_success "Synchronized dist/web/ArduboyProject.hex"

# 5. Packaging if requested
if [ "${PACKAGE}" = true ] || [ -n "${PACKAGE_DIST:-}" ]; then
    log_info "Creating distribution package for itch.io..."
    "${ROOT_DIR}/scripts/package_dist.sh"
fi

if [ "${BUILD_ONLY}" = true ]; then
    log_success "Build complete! (Build-only mode, skipping emulator)"
    exit 0
fi

# 6. Execute ProjectABE Emulator
WEB_DIR="${ROOT_DIR}/dist/web"
if [ ! -f "${WEB_DIR}/index.html" ]; then
    log_warn "dist/web/index.html not found, falling back to html5/"
    WEB_DIR="${ROOT_DIR}/html5"
fi

# Construct URL
URL="http://localhost:${PORT}/"
if [[ "${SKIN}" == "arduboy" ]]; then
    URL="http://localhost:${PORT}/?skin=arduboy"
fi

# Check if a server is already running on the target port and serving properly
SERVER_RUNNING=false
HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -m 1 "http://localhost:${PORT}/" 2>/dev/null || true)
if [ "${HTTP_STATUS}" = "200" ]; then
    SERVER_RUNNING=true
    log_info "Found existing web server on port ${PORT}."
else
    if [ -n "${HTTP_STATUS}" ] && [ "${HTTP_STATUS}" != "000" ]; then
        log_warn "Port ${PORT} returned HTTP ${HTTP_STATUS} (stale server). Terminating stale process..."
        fuser -k "${PORT}/tcp" 2>/dev/null || pkill -f "http.server.*${PORT}" 2>/dev/null || true
        sleep 0.5
    fi
    log_info "Starting lightweight HTTP server on port ${PORT}..."
    nohup python3 -m http.server "${PORT}" --directory "${WEB_DIR}" >/dev/null 2>&1 &
    SERVER_PID=$!
    disown "${SERVER_PID}" 2>/dev/null || true
    sleep 0.5
    log_success "HTTP server running in background (PID ${SERVER_PID})."
fi

log_info "Opening ProjectABE in default browser: ${URL}"
if command -v xdg-open &>/dev/null; then
    xdg-open "${URL}" &>/dev/null &
elif command -v firefox &>/dev/null; then
    firefox "${URL}" &>/dev/null &
else
    log_warn "No browser launcher found (xdg-open/firefox). Please open ${URL} manually."
fi

log_success "Pipeline finished successfully!"
echo -e "${COLOR_CYAN}Tip: Press F3 inside the browser at any time to toggle between skins.${COLOR_RESET}"
