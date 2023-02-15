ARG GET_VERSION=20190315_patched
ARG ROOT_VERSION=6.08
ARG UBUNTU_VERSION=xenial

FROM elitpc/get:${GET_VERSION}-${UBUNTU_VERSION}-${ROOT_VERSION}
COPY requirements_apt.txt /tmp
COPY requirements_pip3.txt /tmp
RUN apt-get update -qq \
    && apt-get -y install $(cat /tmp/requirements_apt.txt) \ 
    gdb valgrind git \
    vim emacs nano \
    && rm -rf /var/lib/apt/lists/*
RUN yes | pip3 install --no-cache-dir -r /tmp/requirements_pip3.txt
RUN rm /tmp/requirements_*.txt
RUN useradd -ms /bin/bash woodpecker
