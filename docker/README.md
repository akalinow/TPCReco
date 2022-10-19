# Containerised TPCReco

## Getting started with Docker

1. Install and start docker according to instructions for your platform.
2. Load `eliptc/tpcreco` image. You can either import pre-built image or build one locally (longer).


### Import pre-built image

Download `eliptc_tpcreco_latest.tar`, then import it:

```
docker load elitpc/tpcreco:latest -i elitpc_tpcreco_latest.tar
```

### Build locally

This image uses `elitpc/get` as its base. Make sure that image is already present in your system.  Then build `elitpc/tpcreco`:

```
git clone ssh://git@dracula.hep.fuw.edu.pl:8822/akalinowski/TPCReco.git
cd TPCReco
docker build --no-cache --rm -t elitpc/tpcreco:latest -f docker/Dockerfile . 
```
## Getting started with Singularity

It's possible to convert `TPCReco` docker image to singularity image.

### From pre-built .sif file

There are no extra preparation steps. You are ready.

### From pre-built docker image

This way doesn't require docker installation, just pre-built `elitpc_tpcreco_latest.tar` file.

```
singularity build elitpc_tpcreco_latest.sif docker-archive://elitpc_tpcreco_latest.tar
```

### From local docker image

This way requires docker installation of docker. Complete [getting started](#getting-started) steps so that `elitpc/tpcreco` image is present on your system. Then:

```
singularity build elitpc_tpcreco_latest.sif docker-daemon://elitpc/tpcreco:latest
```

## Run

First start your container, then launch a reconstruction application inside the container.

### Start docker container

```
docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) -v $HOME:/scratch --workdir /scratch -v /data:/data elitpc/tpcreco:latest
```

Options `-v $HOME:/scratch --workdir /scratch` will bind your home directory from host to `/scratch` and set it to initial working directory. Option `-v /data:/data` will bind `/data` host directory.

### Start singularity container

```
singularity shell --bind /data elitpc_tpcreco_latest.sif
```

Singularity will automatically include your `$HOME` from host inside the container. If you wish to include other directories from host, you can do so using `--bind` option. `--bind /data` will bind `/data` directory on host to `/data` directory inside the container.



### Start tpcGUI

Following steps should be done inside a container.

Prepare directory for reconstruction:

```
mkdir -p reco && cd reco && cp /opt/soft/TPCReco/resources* .
```

Launch `tpcGUI`:

```
tpcGUI config_GUI__OFFLINE_250mbar_12.5MHz.json --dataFile /data/CoBo_ALL_AsAd_ALL_2021-06-22T12\:01\:56.568_0000.graw
```

Pre-made configurations files are available in `\opt\soft\TPCReco\config\`.

## Licensing

GetSoftware is distributed under [CeCILL license](http://cecill.info/licences.en.html).

`elitpc/tpcreco` docker image is not meant to be uploaded to Dockerhub or any other third part registry.