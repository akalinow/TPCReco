# TPCReco contributing

## Development environment

The easiest way to start developing is to run a container with the development environment
(image `elitpc/tpcreco-with_tf`, see [docker/USER_README.md](docker/USER_README.md)).

To build docker image locally:

```
docker build --no-cache --rm --target with_tf -t elitpc/tpcreco-with_tf:latest -f docker/Dockerfile .
```

To run development container:

```
docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) -v "$PWD":/scratch --workdir /scratch elitpc/tpcreco-with_tf:latest bash
```

On other platforms than Linux, and for Singularity, see [docker/USER_README.md](docker/USER_README.md).

## Updating to a newer version

To synchronize the version of software in your working directory with a newer tag do the following:

```Shell
cd TPCReco
git fetch
git checkout newer_tag
cd build
cmake ../
make install -j 4
```

You can check the tag version for your working directory with

```Shell
cd TPCReco
git branch
```

the output should look like this:

```Shell
akalinow@daqula2:~/1/TPCReco$ git branch
* (HEAD detached at v0.02_28.04.2021)
  master
```
