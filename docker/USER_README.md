# TPCReco in a container

TPCReco as available for Linux and macOS through the container technology:

- linux:
  - [singularity](#singularity-on-linux),
  - [docker](#docker-on-linux),
- macOS:
  - [docker](#docker-on-macos) (Catalina or later),
  - [vagrant+singularity](#vagrant--singularity-on-macos).

## Getting started

### Singularity on Linux

Preparation:
- [install singularity](https://apptainer.org/docs/user/main/quick_start.html#quick-installation-steps),
- create and enter directory for reconstruction,
- copy data files and 'elitpc_tpcreco_latest.si' to that directory,
- initialize that directory by executing:

    ```
    singularity exec --bind "$PWD" elitpc_tpcreco_latest.sif cp -r /opt/soft/TPCReco/resources/. .
    ```

Running:
- go to directory created during preparation step,
- start a container:

    ```
    singularity run --bind "$PWD" elitpc_tpcreco_latest.sif
    ```

- run [`tpcGUI`](#running-tpcgui).

### Docker on Linux

Preparation:

- [install docker](https://docs.docker.com/engine/install/),
- download 'elitpc_tpcreco_latest.tar' file,
- load image:

    ```
    sudo docker load -i elitpc_tpcreco_latest.tar
    ```

- create and enter directory for reconstruction,
- copy data files to this directory,
- initialize that directory by executing:

    ```
    sudo docker run --rm  --user $(id -u) -v "$PWD:/scratch" --workdir /scratch elitpc/tpcreco:latest cp -r /opt/soft/TPCReco/resources/. .
    ```

Running:

- go to directory created during preparation step,
- start a container:

    ```
    sudo docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) -v "$PWD:/scratch" --workdir /scratch elitpc/tpcreco:latest
    ```

    NOTE: on some distributions (e.g. Arch Linux) executing `xhost local:root` is necessary. In that case consider executing also `xhost -` at the end of the session.

- run [`tpcGUI`](#running-tpcgui).

### Docker on macOS:
Preparation:
- install:
  - [XQuartz](https://www.xquartz.org/),
  - [install docker](https://docs.docker.com/engine/install/),
- start docker application,
- download 'elitpc_tpcreco_latest.tar' file,
- load docker image:

    ```
    sudo docker load -i elitpc_tpcreco_latest.tar
    ```

- Open `XQuartz`, and go to XQuartz, Preferences, select the Security tab, and tick the box "Allow connections from network clients". Then exit XQuartz,
- create and enter directory for reconstruction,
- copy data files to this directory,
- initialize that directory by executing:

    ```
    sudo docker run --rm  --user $(id -u) -v "$PWD:/scratch" --workdir /scratch elitpc/tpcreco:latest cp -r /opt/soft/TPCReco/resources/. .
    ```

Running:

- start docker application,
- go to directory created during preparation step,
- allow network X11 connection from localhost by executing:

    ```
    xhost +localhost
    ```

- start a container:

    ```
    sudo docker run -e DISPLAY=host.docker.internal:0 -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) -v "$PWD:/scratch" --workdir /scratch elitpc/tpcreco:latest
    ```

- run [`tpcGUI`](#running-tpcgui).

### Vagrant + Singularity on macOS

Preparation:

- install:
  - [virtualbox](https://www.virtualbox.org/wiki/Downloads),
  - [XQuartz](https://www.xquartz.org/),
  - [vagrant](https://www.vagrantup.com/downloads).
- create directory for reconstruction. Copy to this directory 'elitpc_tpcreco_latest.sif', 'Vagrantfile' and data files (.graw).

Running:

- start `XQuartz`,
- go to directory created during preparation step and run:

    ```
    vagrant up && vagrant ssh
    ```

    NOTE: At the end of session run `vagrant halt` otherwise the container will continue in a background.

- run [`tpcGUI`](#running-tpcgui).

## Running tpcGUI

Go to directory created during preparation step and start a container according to instructions for your system.

Start `tpcGUI`:

```
tpcGUI /opt/soft/TPCReco/config/config_GUI__OFFLINE_250mbar_12.5MHz.json --dataFile CoBo_ALL_AsAd_ALL_2021-06-22T12\:01\:56.568_0000.graw
```

Change `--dataFile` to your data file and change `config` according to experimental conditions for this run.

The output files should appear in both container and host system.
