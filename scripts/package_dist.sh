#!/usr/bin/env bash
# =============================================================================
# package_dist.sh — Package web emulator into distribution zip for itch.io
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
WEB_DIR="${DIST_DIR}/web"
ZIP_OUTPUT="${DIST_DIR}/blackhole-web.zip"

COLOR_RESET="\033[0m"
COLOR_BOLD="\033[1m"
COLOR_GREEN="\033[32m"
COLOR_CYAN="\033[36m"
COLOR_RED="\033[31m"

echo -e "${COLOR_BOLD}Packaging ProjectABE Web Distribution for itch.io...${COLOR_RESET}"

if [ ! -d "${WEB_DIR}" ] || [ ! -f "${WEB_DIR}/index.html" ]; then
    echo -e "${COLOR_RED}[ERROR] dist/web directory does not exist or is incomplete. Run ./build.sh first.${COLOR_RESET}"
    exit 1
fi

# Remove old zip if present
rm -f "${ZIP_OUTPUT}"

# Check for zip utility
if ! command -v zip &>/dev/null; then
    # Fallback to python zipfile
    python3 -c "
import zipfile, os

web_dir = '${WEB_DIR}'
zip_out = '${ZIP_OUTPUT}'

with zipfile.ZipFile(zip_out, 'w', zipfile.ZIP_DEFLATED) as zf:
    for root, dirs, files in os.walk(web_dir):
        for f in files:
            full_path = os.path.join(root, f)
            rel_path = os.path.relpath(full_path, web_dir)
            zf.write(full_path, rel_path)
"
else
    (cd "${WEB_DIR}" && zip -r -9 "${ZIP_OUTPUT}" ./*)
fi

ZIP_SIZE=$(stat -c%s "${ZIP_OUTPUT}" 2>/dev/null || stat -f%z "${ZIP_OUTPUT}")
ZIP_SIZE_KB=$((ZIP_SIZE / 1024))

echo -e "${COLOR_GREEN}[SUCCESS] Created ${ZIP_OUTPUT} (${ZIP_SIZE_KB} KB)${COLOR_RESET}"
echo ""
echo -e "${COLOR_BOLD}itch.io Upload Instructions:${COLOR_RESET}"
echo -e "  1. Go to your game edit page on itch.io (e.g. 1-Bit Game Jam 14 submission)."
echo -e "  2. Set 'Kind of project' to: ${COLOR_CYAN}HTML${COLOR_RESET}."
echo -e "  3. In 'Uploads', upload: ${COLOR_CYAN}${ZIP_OUTPUT}${COLOR_RESET}."
echo -e "  4. Check the box: ${COLOR_CYAN}'This file will be played in the browser'${COLOR_RESET}."
echo -e "  5. Embed options recommendations:"
echo -e "     - Viewport dimensions: ${COLOR_CYAN}640 x 320${COLOR_RESET} (or 800 x 600 for Arduboy casing)."
echo -e "     - Enable: ${COLOR_CYAN}'Mobile friendly'${COLOR_RESET} and ${COLOR_CYAN}'Automatically start on page load'${COLOR_RESET}."
echo -e "     - Fullscreen button: ${COLOR_CYAN}Enabled${COLOR_RESET}."
