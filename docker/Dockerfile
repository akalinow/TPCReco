ARG GET_VERSION=20190315_patched
ARG ROOT_VERSION=6.08
ARG UBUNTU_VERSION=xenial

FROM elitpc/get:${GET_VERSION}-${UBUNTU_VERSION}-${ROOT_VERSION} AS common_base

RUN apt-get update -qq \
    && apt-get -y install git \
    && rm -rf /var/lib/apt/lists/*
ARG GEANT4_VERSION=10.4.3
ARG CADMESH_VERSION=1.1
RUN wget https://github.com/Geant4/geant4/archive/refs/tags/v${GEANT4_VERSION}.tar.gz -O geant.tar.gz \
    && tar xf geant.tar.gz && rm geant.tar.gz
RUN cmake -S geant4-${GEANT4_VERSION} -Bbuild \
    -DGEANT4_INSTALL_DATA=ON -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && cmake --build build --target install -- -j 10 \
    && rm -r build geant4-${GEANT4_VERSION} \
    && ldconfig

RUN wget https://github.com/christopherpoole/CADMesh/archive/refs/tags/v${CADMESH_VERSION}.tar.gz -O cadmesh.tar.gz \
    && tar xf cadmesh.tar.gz && rm cadmesh.tar.gz
RUN cmake -S CADMesh-${CADMESH_VERSION} -Bbuild \
    -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && cmake --build build --target install \
    && rm -r build CADMesh-${CADMESH_VERSION} \
    && ldconfig

# grrr source geant4.sh is configured on build
ENV G4LEVELGAMMADATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/PhotonEvaporation5.2
ENV G4NEUTRONXSDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/G4NEUTRONXS1.4
ENV G4LEDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/G4EMLOW7.3
ENV G4NEUTRONHPDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/G4NDL4.5
ENV G4RADIOACTIVEDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/RadioactiveDecay5.2
ENV G4ENSDFSTATEDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/G4ENSDFSTATE2.2
ENV G4ABLADATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/G4ABLA3.1
ENV G4PIIDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/G4PII1.3
ENV G4SAIDXSDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/G4SAIDDATA1.1
ENV G4REALSURFACEDATA=/usr/local/share/Geant4-${GEANT4_VERSION}/data/RealSurface2.1.1

COPY requirements_apt.txt /tmp
COPY requirements_pip3.txt /tmp
RUN apt-get update -qq \
    && apt-get -y install $(cat /tmp/requirements_apt.txt) \ 
    less vim emacs nano \
    && rm -rf /var/lib/apt/lists/*
RUN yes | pip3 install --no-cache-dir -r /tmp/requirements_pip3.txt
RUN rm /tmp/requirements_*.txt


FROM common_base AS dev
RUN apt-get update -qq \
    && apt-get -y install \
    gdb valgrind git ccache \
    binutils linux-tools-generic \
    && rm -rf /var/lib/apt/lists/*
RUN useradd -ms /bin/bash woodpecker

FROM common_base AS user
ADD . /opt/soft/TPCReco-src
WORKDIR /opt/soft/
RUN cmake -S TPCReco-src -Bbuild \
    -DCMAKE_INSTALL_PREFIX=/opt/soft/TPCReco \
    && cmake --build build --target install \
    && rm -r build
ENV PATH=${PATH}:/opt/soft/TPCReco/bin
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/opt/soft/TPCReco/lib
ENV CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:/opt/soft/TPCReco/lib/cmake
WORKDIR /opt/soft/TPCReco/resources
