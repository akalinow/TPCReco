# TPCReco contributing

## Development environment

The easiest way to start developing is to run container with developemnt enviroment.

To build docker image locally:

```
docker build --no-cache --rm --target dev -t elitpc/tpcreco-dev:latest -f docker/Dockerfile . 
```

To run development container:

```
docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --rm -it --user $(id -u) -v "$PWD":/scratch --workdir /scratch elitpc/tpcreco-dev:latest
```

In case you are on different platform than Linux consult [docker/USER_README.md](docker/USER_README.md).
