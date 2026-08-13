#!/usr/bin/env bash
# Build a Fedora RPM of kobiQ from the local tree (no git tag required).
# Produces dist-tagged packages for the host Fedora release (e.g. .fc44).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${VERSION:-0.1.5}"
RELEASE="${RELEASE:-1}"
WORKDIR="${WORKDIR:-$(mktemp -d)}"
NAME="kobiq"
OUT="${OUT_DIR:-${ROOT}/dist}"

cleanup() { rm -rf "${WORKDIR}"; }
trap cleanup EXIT

echo "Working in ${WORKDIR}"
mkdir -p "${WORKDIR}"/{SOURCES,SPECS,BUILD,RPMS,SRPMS}

# Local tarball that matches the GitHub archive layout: kobiQ-<version>/
STAGE="${WORKDIR}/kobiQ-${VERSION}"
mkdir -p "${STAGE}"
rsync -a \
  --exclude '.git' \
  --exclude 'build' \
  --exclude 'dist' \
  --exclude '*.rpm' \
  --exclude '.cache' \
  "${ROOT}/" "${STAGE}/"

tar -C "${WORKDIR}" -czf "${WORKDIR}/SOURCES/${NAME}-${VERSION}.tar.gz" "kobiQ-${VERSION}"
cp "${ROOT}/packaging/kobiq.spec" "${WORKDIR}/SPECS/kobiq.spec"

# Point Source0 at the local tarball for offline builds
sed -i \
  -e "s|^Version:.*|Version:        ${VERSION}|" \
  -e "s|^Release:.*|Release:        ${RELEASE}%{?dist}|" \
  -e "s|^Source0:.*|Source0:        ${NAME}-${VERSION}.tar.gz|" \
  "${WORKDIR}/SPECS/kobiq.spec"

rpmbuild \
  --define "_topdir ${WORKDIR}" \
  -ba "${WORKDIR}/SPECS/kobiq.spec"

mkdir -p "${OUT}"
find "${WORKDIR}/RPMS" "${WORKDIR}/SRPMS" -type f -name '*.rpm' -exec cp -v {} "${OUT}/" \;

echo
echo "RPMs written to ${OUT}:"
ls -la "${OUT}"
