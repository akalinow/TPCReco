# TPCReco in a container

TPCReco is available for Linux and macOS through containers (Docker or Singularity/Apptainer).
Two kinds of images are built from [Dockerfile](Dockerfile):

| Image | Dockerfile stage | Content |
|---|---|---|
| `elitpc/tpcreco-with_tf` | `with_tf` | build and run environment: ROOT, GET software, Geant4, TensorFlow, Python packages. TPCReco is built from source inside the container. **Recommended.** |
| `elitpc/tpcreco` | `user` | TPCReco of a release tag installed in `/opt/soft/TPCReco` (`bin/` in `PATH`) |

Inside the container, always set:

```
export LC_ALL=$LANG
```

## Recommended: build TPCReco in the `with_tf` container

### Singularity on Linux

- [install singularity](https://apptainer.org/docs/user/main/quick_start.html#quick-installation-steps),
- get the image, e.g. convert the Docker image: `singularity build elitpc_tpcreco-with_tf.sif docker-archive://elitpc_tpcreco-with_tf_latest.tar`,
- start the container, binding the directories with the source code and the data:

    ```
    singularity run --bind /path/to/work:/path/to/work,/path/to/data:/path/to/data elitpc_tpcreco-with_tf.sif
    ```

- build and install TPCReco as described in the [main README](../README.md#installation-instructions):

    ```
    cd /path/to/work/TPCReco
    mkdir build; cd build
    cmake -DBUILD_TEST=ON ../
    make install -j 4
    ```

- run the applications from any directory, e.g. [`tpcGUI`](#running-tpcgui).

### Docker on Linux

- [install docker](https://docs.docker.com/engine/install/),
- load the image (`sudo docker load -i elitpc_tpcreco-with_tf_latest.tar`) or build it
  (see [CONTRIBUTING.md](../CONTRIBUTING.md)),
- start the container with X11 forwarding and the work directory mounted:

    ```
    sudo docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) \
       -v /path/to/work:/scratch --workdir /scratch elitpc/tpcreco-with_tf:latest bash
    ```

    NOTE: on some distributions (e.g. Arch Linux) executing `xhost local:root` is necessary. In that case consider executing also `xhost -` at the end of the session.

- build and install TPCReco as above (in `/scratch/TPCReco`).

## Prebuilt TPCReco: the `elitpc/tpcreco` image

The applications are in `PATH` and the configuration files in `/opt/soft/TPCReco/config`.
The container starts in `/opt/soft/TPCReco/resources`. Images built from TPCReco versions that
find the resource files automatically can run the applications from any directory; with older
images start them from `/opt/soft/TPCReco/resources`.

### Singularity on Linux

```
singularity shell --bind "$PWD" elitpc_tpcreco_latest.sif
```

### Docker on Linux

```
sudo docker load -i elitpc_tpcreco_latest.tar
sudo docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) -v "$PWD:/scratch" elitpc/tpcreco:latest
```

### Docker on macOS

Preparation:
- install [XQuartz](https://www.xquartz.org/) and [docker](https://docs.docker.com/engine/install/),
- start the docker application,
- load the image: `sudo docker load -i elitpc_tpcreco_latest.tar`,
- open `XQuartz`, go to XQuartz, Preferences, select the Security tab, and tick the box "Allow connections from network clients". Then exit XQuartz.

Running:
- start the docker application,
- allow network X11 connections from localhost: `xhost +localhost`,
- start a container in the directory with your data:

    ```
    sudo docker run -e DISPLAY=host.docker.internal:0 -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) -v "$PWD:/scratch" elitpc/tpcreco:latest
    ```

### Vagrant + Singularity on macOS

Preparation:
- install [virtualbox](https://www.virtualbox.org/wiki/Downloads), [XQuartz](https://www.xquartz.org/) and [vagrant](https://www.vagrantup.com/downloads),
- create a directory and copy to it `elitpc_tpcreco_latest.sif`, [Vagrantfile](Vagrantfile) and the data files (.graw).

Running:
- start `XQuartz`,
- in that directory run `vagrant up && vagrant ssh`; the container shell starts in `/vagrant` (the directory on the host).

NOTE: at the end of the session run `vagrant halt`, otherwise the virtual machine keeps running in the background.

## Running tpcGUI

Start a container as described above for your system, then start `tpcGUI` with a configuration file
matching the experimental conditions of the run and your data file(s):

```
tpcGUI /opt/soft/TPCReco/config/config_GUI__OFFLINE_250mbar_12.5MHz.json --input.dataFile /scratch/CoBo_ALL_AsAd_ALL_2021-06-22T12:01:56.568_0000.graw
```

With TPCReco built in the `with_tf` container use `<build>/bin/tpcGUI` and `<build>/config/...`
instead. More examples and the browser version `tpcWebGUI`: [GUI README](../GUI/README.md),
[Web GUI README](../WebServerClient/README.md).

Output files are written to the current directory and are visible both in the container and on the host
(for directories mounted with `--bind` or `-v`).
