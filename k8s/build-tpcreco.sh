#!/usr/bin/env bash
# Compile and install TPCReco for the tpcwebgui pod, inside the runtime image
# elitpc/tpcreco-with_tf (docker run; no new image is built), so the binaries match the pod.
#
# Usage:  ./k8s/build-tpcreco.sh [INSTALL_DIR]
#   INSTALL_DIR  installation (bin/ lib/ web/ resources/), default /scratch/tpcwebgui/TPCReco;
#                mount it into minikube as tpcreco.hostPath of the values file (README.md)
# Settings (environment variables):
#   IMAGE      runtime image         (default: elitpc/tpcreco-with_tf:latest)
#   SOURCE     head | worktree       (default: head = committed state, git archive HEAD)
#   BUILD_DIR  cmake build directory (default: INSTALL_DIR.build, reused for faster rebuilds)
#   JOBS       parallel jobs         (default: nproc)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
INSTALL_DIR="$(realpath -m "${1:-/scratch/tpcwebgui/TPCReco}")"
BUILD_DIR="$(realpath -m "${BUILD_DIR:-${INSTALL_DIR}.build}")"
IMAGE="${IMAGE:-elitpc/tpcreco-with_tf:latest}"
SOURCE="${SOURCE:-head}"
JOBS="${JOBS:-$(nproc)}"

SRC_DIR="${BUILD_DIR}/src"
mkdir -p "$INSTALL_DIR" "$BUILD_DIR"
rm -rf "$SRC_DIR" && mkdir -p "$SRC_DIR"
case "$SOURCE" in
  head)     git -C "$REPO" archive --format=tar HEAD | tar -xf - -C "$SRC_DIR"
            echo "==> TPCReco $(git -C "$REPO" rev-parse --short HEAD) (committed state)" ;;
  worktree) git -C "$REPO" ls-files -z | (cd "$REPO" && tar --null -T - -cf -) | tar -xf - -C "$SRC_DIR"
            echo "==> TPCReco working tree (with uncommitted changes)" ;;
  *) echo "ERROR: SOURCE must be head or worktree" >&2; exit 1 ;;
esac

echo "==> Building in ${IMAGE}: ${BUILD_DIR} -> ${INSTALL_DIR}"
# HOME inside the build directory: the build installs allowedOptions.json to $HOME/.tpcreco
docker run --rm --user "$(id -u):$(id -g)" \
  -v "${BUILD_DIR}:${BUILD_DIR}" -v "${INSTALL_DIR}:${INSTALL_DIR}" \
  -e HOME="${BUILD_DIR}/home" \
  "$IMAGE" bash -c "
    set -e
    mkdir -p \"\$HOME\"
    cmake -S '${SRC_DIR}' -B '${BUILD_DIR}/cmake' -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX='${INSTALL_DIR}' -DBUILD_TEST=OFF
    cmake --build '${BUILD_DIR}/cmake' --target install -j${JOBS}"

cp -f "${BUILD_DIR}/home/.tpcreco/config/allowedOptions.json" "${INSTALL_DIR}/allowedOptions.json"
echo "==> Done. Installed: ${INSTALL_DIR} (bin/ lib/ web/ resources/ allowedOptions.json)"
echo "    deploy with ALLOWED_OPTIONS=${INSTALL_DIR}/allowedOptions.json ./k8s/deploy.sh"
