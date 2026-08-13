#!/usr/bin/env bash
# Build an RPM for the Fedora version you are running right now (no containers).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FC="$(rpm -E %fedora)"
OUT="${ROOT}/dist/fedora-${FC}"

echo "Native RPM build for Fedora ${FC}"
chmod +x "${ROOT}/packaging/build-rpm.sh"
OUT_DIR="${OUT}" "${ROOT}/packaging/build-rpm.sh"

echo
echo "Install with:"
echo "  sudo dnf install ./dist/fedora-${FC}/kobiq-*.x86_64.rpm"
