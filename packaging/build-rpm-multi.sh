#!/usr/bin/env bash
# Build kobiQ RPMs for multiple Fedora releases.
# Each Fedora version needs its own RPM (Qt ABI differs across releases).
#
# Usage:
#   ./packaging/build-rpm-multi.sh              # 42 43 44
#   ./packaging/build-rpm-multi.sh 44           # current host only (native)
#   PODMAN_TLS_VERIFY=false ./packaging/build-rpm-multi.sh 42 43 44
#
# Output: dist/fedora-<N>/kobiq-*.rpm
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${VERSION:-0.1.7}"
RELEASE="${RELEASE:-1}"
HOST_FC="$(rpm -E %fedora 2>/dev/null || echo 0)"
PODMAN_TLS_VERIFY="${PODMAN_TLS_VERIFY:-true}"

if [[ $# -gt 0 ]]; then
  RELEASES=("$@")
else
  RELEASES=(42 43 44)
fi

pull_image() {
  local ver="$1"
  local images=(
    "registry.fedoraproject.org/fedora:${ver}"
    "quay.io/fedora/fedora:${ver}"
    "docker.io/library/fedora:${ver}"
  )
  local tls_args=()
  if [[ "${PODMAN_TLS_VERIFY}" == "false" ]]; then
    tls_args=(--tls-verify=false)
  fi
  for img in "${images[@]}"; do
    echo "Trying image: ${img}" >&2
    if podman pull "${tls_args[@]}" "${img}" >&2; then
      echo "${img}"
      return 0
    fi
  done
  return 1
}

build_native() {
  local ver="$1"
  local out="${ROOT}/dist/fedora-${ver}"
  mkdir -p "${out}"
  echo "Building natively on Fedora ${HOST_FC} → ${out}"
  chmod +x "${ROOT}/packaging/build-rpm.sh"
  OUT_DIR="${out}" VERSION="${VERSION}" RELEASE="${RELEASE}" \
    "${ROOT}/packaging/build-rpm.sh"
}

build_in_container() {
  local ver="$1"
  local out="${ROOT}/dist/fedora-${ver}"
  mkdir -p "${out}"

  if ! command -v podman >/dev/null 2>&1; then
    echo "ERROR: podman required to build for Fedora ${ver} (host is ${HOST_FC})." >&2
    echo "  sudo dnf install podman" >&2
    return 1
  fi

  local image
  if ! image="$(pull_image "${ver}")"; then
    echo "ERROR: Could not pull a Fedora ${ver} image." >&2
    echo "If you see TLS/certificate errors (common on corporate networks), try:" >&2
    echo "  PODMAN_TLS_VERIFY=false ./packaging/build-rpm-multi.sh ${ver}" >&2
    echo "Or install your corporate root CA, then: sudo update-ca-trust" >&2
    return 1
  fi

  echo "Using image: ${image}"
  podman run --rm \
    --security-opt label=disable \
    -v "${ROOT}:/src:ro" \
    -v "${out}:/out:rw" \
    -e VERSION="${VERSION}" \
    -e RELEASE="${RELEASE}" \
    "${image}" \
    bash -lc '
      set -euo pipefail
      dnf -y install \
        cmake gcc-c++ qt6-qtbase-devel \
        rpm-build rpmdevtools rsync \
        >/tmp/dnf.log
      cp -a /src /work
      cd /work
      chmod +x packaging/build-rpm.sh
      OUT_DIR=/out ./packaging/build-rpm.sh
      ls -la /out
    '
}

echo "Building kobiQ ${VERSION}-${RELEASE} for Fedora: ${RELEASES[*]}"
echo "Host Fedora: ${HOST_FC}"

failed=()
for ver in "${RELEASES[@]}"; do
  echo
  echo "======== Fedora ${ver} ========"
  if [[ "${ver}" == "${HOST_FC}" ]]; then
    build_native "${ver}" || failed+=("${ver}")
  else
    build_in_container "${ver}" || failed+=("${ver}")
  fi
done

echo
if [[ ${#failed[@]} -gt 0 ]]; then
  echo "FAILED for Fedora: ${failed[*]}"
  echo
  echo "Tips:"
  echo "  • Build only for this machine: ./packaging/build-rpm-multi.sh ${HOST_FC}"
  echo "  • TLS errors: PODMAN_TLS_VERIFY=false ./packaging/build-rpm-multi.sh ..."
  echo "  • Or use GitHub Actions (tag v*) to build fc42/fc43/fc44 RPMs in the cloud"
  exit 1
fi

echo "Done. Install the RPM that matches your Fedora version:"
echo "  sudo dnf install ./dist/fedora-\$(rpm -E %fedora)/kobiq-*.x86_64.rpm"
echo
find "${ROOT}/dist" -type f -name 'kobiq-*.x86_64.rpm' ! -name '*debug*' 2>/dev/null | sort
