# tpcWebGUI on Kubernetes

Helm chart and scripts to run the web GUI (`tpcWebGUI`, see
[WebServerClient/README.md](../WebServerClient/README.md)) on an existing Kubernetes cluster
with an ingress-nginx controller, e.g. the minikube cluster that serves the WordPress sites
(`wordpress-k8s`). The GUI is published under a path of the existing host names, by default
`https://hep.fuw.edu.pl/tpcgui/`, so no new DNS name or certificate is needed.

The pod runs the ELITPC image `elitpc/tpcreco-with_tf:latest` unchanged (no image is built).
TPCReco itself is an installation compiled with that image and mounted into the pod.

```
k8s/
├── build-tpcreco.sh  # compile + install TPCReco inside elitpc/tpcreco-with_tf (docker run)
├── deploy.sh         # load the image into minikube (once) + helm upgrade --install
├── values-hep.yaml   # site settings: hosts, TLS secret, mounts, input files
└── tpcwebgui/        # Helm chart: Deployment, Service, Ingress, ConfigMap, Secret, PVC
```

## Prerequisites

On the machine running the cluster (the WordPress server):

* the cluster with ingress-nginx and the `wordpress-tls` certificate (`wordpress-k8s/deploy.sh`),
* `helm`, `kubectl`, `minikube`, `docker`, `git`, a clone of TPCReco,
* the image `elitpc/tpcreco-with_tf:latest` in the local docker (see [docker/](../docker/README.md),
  or `docker load -i elitpc_tpcreco-with_tf_latest.tar`). `deploy.sh` copies it into minikube
  on the first run (18 GB, once).

## TPCReco installation

Compile and install TPCReco on the host inside the same image as the pod (`docker run`), so the
binaries match the runtime libraries (ROOT, GET, TensorFlow):

```bash
cd TPCReco
./k8s/build-tpcreco.sh /scratch/tpcwebgui/TPCReco                   # committed state (git HEAD)
SOURCE=worktree ./k8s/build-tpcreco.sh /scratch/tpcwebgui/TPCReco   # with uncommitted changes
```

The installation contains `bin/ lib/ web/ resources/` and `allowedOptions.json`, the list of
configuration options of this version, which `deploy.sh` passes to the pod
(`ALLOWED_OPTIONS=/scratch/tpcwebgui/TPCReco/allowedOptions.json`).

Use a dedicated installation directory. A build directory that is rebuilt while the GUI runs, or
by several people, can end up with libraries of different versions, and the GUI then crashes at
start-up.

## Mounts

The pod mounts the TPCReco installation (`tpcreco.hostPath`) and the data (`dataVolumes`).
With minikube a `hostPath` volume is a directory of the minikube node, so the host directories
have to be mounted into minikube first. For the installation above and the HIgS 2022 data
(CIFS share `//10.4.5.16/backup_daqula2/HIgS_2022`, mounted on the host e.g. at `/mnt/HIgS_2022`):

```bash
minikube mount /scratch/tpcwebgui/TPCReco:/tpcreco &     # must keep running
minikube mount /mnt/HIgS_2022:/data/HIgS_2022 &           # must keep running
```

To keep them running across reboots, use systemd units, as for the socat port forwarding of
`wordpress-k8s/set_socat.sh`. Example unit `/etc/systemd/system/minikube-mount-higs.service`
(replace `USER` and the minikube path):

```ini
[Unit]
Description=Mount HIgS 2022 data into minikube
After=docker.service network-online.target remote-fs.target

[Service]
User=USER
ExecStart=/usr/local/bin/minikube mount /mnt/HIgS_2022:/data/HIgS_2022
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

then `sudo systemctl daemon-reload && sudo systemctl enable --now minikube-mount-higs`.

On other clusters use NFS or PersistentVolumeClaims instead of `hostPath`
(`tpcreco.existingClaim`, `dataVolumes[].nfs` or `persistentVolumeClaim`), e.g.

```yaml
dataVolumes:
  - name: higs2022
    mountPath: /data/HIgS_2022
    readOnly: true
    nfs: { server: 10.4.5.16, path: /HIgS_2022 }
```

`config.input.dataFile` (files opened at start-up) must point to files under `mountPath`.
Other files and the following chunks of a run are opened from the GUI.

## Deploy

```bash
cd TPCReco
ALLOWED_OPTIONS=/scratch/tpcwebgui/TPCReco/allowedOptions.json ./k8s/deploy.sh
```

The script

1. loads `elitpc/tpcreco-with_tf:latest` into minikube if it is not there yet,
2. installs or upgrades the Helm release `tpcwebgui` with `values-hep.yaml` and restarts the pod
   (so a rebuilt installation is used),
3. checks the URLs through the minikube ingress and prints the URL with the access token:

```
https://hep.fuw.edu.pl/tpcgui/?token=...
```

Open it once per browser: the token is kept in a cookie and removed from the address bar.
Requests without it get HTTP 401. The URL must end with `/` before `?token=`.

Options (environment variables, see the header of `deploy.sh`):

| command | effect |
|---|---|
| `./k8s/deploy.sh --render` | print the manifests only, no cluster access |
| `VALUES=my-site.yaml ./k8s/deploy.sh` | other site settings |
| `LOAD_IMAGE=no IMAGE=registry.example.org/elitpc/tpcreco-with_tf:latest ./k8s/deploy.sh` | image from a registry |
| `./k8s/deploy.sh -- --set urlPrefix=/gui` | extra helm arguments |

Code update: run `build-tpcreco.sh` again, then `deploy.sh` (it restarts the pod).
Configuration update (values file): run `deploy.sh`.

## Access

* **Access token** (always on): generated on the first install and stored in the Secret
  `tpcwebgui-token`; it is kept across upgrades and uninstall. Show it with
  `kubectl get secret tpcwebgui-token -o jsonpath='{.data.token}' | base64 -d`.
  New token: delete the Secret and run `deploy.sh` again.
* **Basic auth** (optional, per-person accounts as for mt-viewer): create the htpasswd Secret
  and enable it in the values file (`ingress.basicAuth.enabled: true`):

  ```bash
  htpasswd -c auth jkowalski        # or: printf 'jkowalski:%s\n' "$(openssl passwd -apr1)" > auth
  kubectl create secret generic tpcwebgui-basic-auth --from-file=auth && rm auth
  ```

All browsers share one GUI state: one event source and one output file on the server.

## Output files

The working directory `/work` is a PersistentVolumeClaim (`tpcwebgui-work`, kept on uninstall)
with the output file `TPCReco_<input>.root` (recreated when the pod starts) and the event type
logs `<input>.log`. List and copy them with

```bash
kubectl exec deploy/tpcwebgui -- ls -l /work
kubectl cp <pod name>:/work/TPCReco_2022-04-12T08-03-44.540_0000.root .
```

Stopping the pod (upgrade, `kubectl rollout restart`, uninstall) sends SIGTERM: the server
closes the output file before it exits.

## Check / remove

```bash
kubectl get pods,svc,ingress,pvc -l app.kubernetes.io/name=tpcwebgui
kubectl logs deploy/tpcwebgui
helm uninstall tpcwebgui                                  # keeps the PVC and the token Secret
kubectl delete pvc tpcwebgui-work && kubectl delete secret tpcwebgui-token   # remove them too
```

## Notes

* The pod starts `tpcWebGUI` through `bash -c`, which adds the installation's `lib/` and
  `$TENSORFLOW_ROOT/lib` to `LD_LIBRARY_PATH` (the image does not have TensorFlow on it), and sets
  `TPCRECO_RESOURCES` (the compiled-in resource path is the one of the build machine).
* The GUI serves its own path prefix (`--web.urlPrefix /tpcgui`); the Ingress forwards the path
  unchanged. A `rewrite-target` annotation is deliberately not used: in ingress-nginx it turns all
  paths of the host into regular expressions, also those of WordPress.
* The Ingress reuses the `wordpress-tls` Secret and has no cert-manager annotation, so it never
  competes for the WordPress certificate (same scheme as `wordpress-k8s/mt-viewer`).
* WebSocket and long requests (opening a GRAW chunk) use 1 h proxy timeouts.
* One replica only (`Recreate` strategy): the GUI state and the output file belong to one process.
