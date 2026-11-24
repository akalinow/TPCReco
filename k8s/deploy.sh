#!/usr/bin/env bash
# Deploy the tpcwebgui Helm chart on an existing cluster (minikube with ingress-nginx, e.g. the
# one serving the WordPress sites). The pod runs the ELITPC image elitpc/tpcreco-with_tf
# unchanged; TPCReco itself is a mounted installation (k8s/build-tpcreco.sh), so no image is built.
#
# Usage:  ./k8s/deploy.sh             # (load the image into minikube) + helm upgrade --install
#         ./k8s/deploy.sh --render    # only print the rendered manifests (no cluster access)
#
# Settings (environment variables):
#   VALUES           site values file    (default: k8s/values-hep.yaml)
#   RELEASE          Helm release name   (default: tpcwebgui)
#   NAMESPACE        namespace           (default: default)
#   IMAGE            runtime image       (default: elitpc/tpcreco-with_tf:latest)
#   LOAD_IMAGE       auto | no           (default: auto = load IMAGE into minikube if missing)
#   ALLOWED_OPTIONS  allowedOptions.json of the deployed TPCReco version
#                    (default: Utilities/config/allowedOptions.json of this repository)
# Extra arguments after "--" are passed to helm, e.g. ./k8s/deploy.sh -- --set urlPrefix=/gui
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"

RENDER_ONLY=0
HELM_EXTRA=()
while (( $# )); do
  case "$1" in
    --render) RENDER_ONLY=1 ;;
    --) shift; HELM_EXTRA=("$@"); break ;;
    *) echo "Unknown argument: $1 (see the header of $0)" >&2; exit 1 ;;
  esac
  shift
done

VALUES="${VALUES:-$HERE/values-hep.yaml}"
RELEASE="${RELEASE:-tpcwebgui}"
NAMESPACE="${NAMESPACE:-default}"
IMAGE="${IMAGE:-elitpc/tpcreco-with_tf:latest}"
LOAD_IMAGE="${LOAD_IMAGE:-auto}"
ALLOWED_OPTIONS="${ALLOWED_OPTIONS:-$REPO/Utilities/config/allowedOptions.json}"
CHART="$HERE/tpcwebgui"
[[ -f "$VALUES" ]] || { echo "ERROR: no values file $VALUES" >&2; exit 1; }
[[ -f "$ALLOWED_OPTIONS" ]] || { echo "ERROR: no $ALLOWED_OPTIONS" >&2; exit 1; }

HELM_ARGS=(--namespace "$NAMESPACE" --values "$VALUES"
           --set "image.repository=${IMAGE%:*}" --set "image.tag=${IMAGE##*:}"
           --set-file "allowedOptions=${ALLOWED_OPTIONS}" "${HELM_EXTRA[@]}")

if (( RENDER_ONLY )); then
  helm template "$RELEASE" "$CHART" "${HELM_ARGS[@]}"
  exit 0
fi

if [[ "$LOAD_IMAGE" == "auto" ]]; then
  if minikube image ls 2>/dev/null | grep -q "${IMAGE}$"; then
    echo "==> Image ${IMAGE} already in minikube"
  else
    echo "==> Loading ${IMAGE} from the local docker into minikube (once; 18 GB, takes a while)"
    minikube image load "$IMAGE"
  fi
fi

echo "==> helm upgrade --install ${RELEASE} (namespace ${NAMESPACE}, values $(basename "$VALUES"))"
helm upgrade --install "$RELEASE" "$CHART" "${HELM_ARGS[@]}" --create-namespace
FULLNAME=$(helm get manifest "$RELEASE" -n "$NAMESPACE" | awk '/^kind: Deployment/{d=1} d && /^  name:/{print $2; exit}')
# the TPCReco installation is a mounted directory: restart, so a rebuilt installation is used
# even when the chart values did not change
kubectl -n "$NAMESPACE" rollout restart "deployment/${FULLNAME}"
kubectl -n "$NAMESPACE" rollout status "deployment/${FULLNAME}" --timeout=900s

TOKEN=$(kubectl -n "$NAMESPACE" get secret "${FULLNAME}-token" -o jsonpath='{.data.token}' 2>/dev/null | base64 -d || true)
PREFIX=$(helm get values "$RELEASE" -n "$NAMESPACE" -a -o json | python3 -c 'import json,sys; p=(json.load(sys.stdin).get("urlPrefix") or "").rstrip("/"); print(p if not p or p.startswith("/") else "/"+p)')
HOSTS=$(kubectl -n "$NAMESPACE" get ingress "$FULLNAME" -o jsonpath='{range .spec.rules[*]}{.host}{"\n"}{end}' 2>/dev/null || true)

if [[ -n "$HOSTS" ]] && command -v minikube >/dev/null; then
  echo "==> Checking through the minikube ingress (independent of DNS / port forwarding)"
  INGRESS_IP=$(minikube ip)
  sleep 3   # let ingress-nginx load the new configuration
  for h in $HOSTS; do
    gui=$(curl -k -s -o /dev/null -w '%{http_code}' --resolve "${h}:443:${INGRESS_IP}" "https://${h}${PREFIX}/" || true)
    tok=$(curl -k -s -o /dev/null -w '%{http_code}' --resolve "${h}:443:${INGRESS_IP}" "https://${h}${PREFIX}/?token=${TOKEN}" || true)
    root=$(curl -k -s -o /dev/null -w '%{http_code}' --resolve "${h}:443:${INGRESS_IP}" "https://${h}/" || true)
    echo "  https://${h}${PREFIX}/ -> ${gui} (want 401), with token -> ${tok} (want 200) | https://${h}/ -> ${root} (other application)"
  done
fi

echo
echo "Open (once per browser, the token is then kept in a cookie):"
for h in $HOSTS; do echo "  https://${h}${PREFIX}/?token=${TOKEN}"; done
[[ -n "$HOSTS" ]] || echo "  (no ingress) kubectl -n $NAMESPACE port-forward svc/${FULLNAME} 8080:80, then http://localhost:8080${PREFIX}/?token=${TOKEN}"
