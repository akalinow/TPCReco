ARG GET_VERSION=20251024_patched
ARG TPCReco_VERSION=25_02_2020

FROM elitpc/get:${GET_VERSION} AS common_base

COPY  docker/packages.txt /tmp/packages.txt
RUN apt-get update -qq \
    && apt-get -y install $(cat /tmp/packages.txt) \
    && rm -rf /var/lib/apt/lists/* \
    && rm /tmp/packages.txt

#Geant4 and CADMesh installation
ARG GEANT4_VERSION=11.3.2
RUN wget https://github.com/Geant4/geant4/archive/refs/tags/v${GEANT4_VERSION}.tar.gz -O geant.tar.gz \
    && tar xf geant.tar.gz && rm geant.tar.gz
RUN cmake -S geant4-${GEANT4_VERSION} -Bbuild \
    -DGEANT4_INSTALL_DATA=ON -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && cmake --build build --target install -- -j 10 \
    && rm -r build geant4-${GEANT4_VERSION} \
    && ldconfig

# source geant4.sh is configured on build
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

ARG CADMESH_VERSION=1.1
RUN wget https://github.com/christopherpoole/CADMesh/archive/refs/tags/v${CADMESH_VERSION}.tar.gz -O cadmesh.tar.gz \
    && tar xf cadmesh.tar.gz && rm cadmesh.tar.gz

# Hack the CMakeLists.txt to suppress incompatible-pointer-types errors
RUN sed -i 's/CMAKE_ARGS/CMAKE_ARGS -DCMAKE_C_FLAGS=-Wno-incompatible-pointer-types /g' CADMesh-${CADMESH_VERSION}/CMakeLists.txt
RUN sed -i 's/world_logical->SetVisAttributes/ \/\/world_logical->SetVisAttributes/g' CADMesh-${CADMESH_VERSION}/examples/basic/src/DetectorConstruction.cc

RUN cmake -S CADMesh-${CADMESH_VERSION} -Bbuild \
    -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    && cmake --build build --target install \
    && rm -r build CADMesh-${CADMESH_VERSION} \
    && ldconfig

FROM common_base AS default

RUN useradd -ms /bin/bash woodpecker


FROM common_base AS with_tf

##TensorFlow installation and other python packages
# virtual environment required for pip3 management in latest Ubuntu versions
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
COPY  docker/requirements.txt /tmp/requirements.txt
RUN yes | pip3 install --no-cache-dir -r /tmp/requirements.txt \
    && rm /tmp/requirements.txt
RUN echo "source /opt/venv/bin/activate" >> /etc/bash.bashrc

# install user scripts
RUN mkdir -p /opt/scripts
COPY docker/start-jupyter.sh /opt/scripts/start-jupyter.sh
RUN chmod +x /opt/scripts/start-jupyter.sh

RUN mkdir /opt/root/etc/notebook/migrated
RUN chmod -R a+rw /opt/root/etc/notebook/migrated

RUN mkdir -p /opt/root/etc/notebook/lab/workspaces
RUN chmod -R a+rw /opt/root/etc/notebook/lab/workspaces

RUN groupadd -g 29916 user1 && \
     useradd -m -r -u 29916 -g user1 user1

FROM common_base AS user
ADD . /opt/soft/TPCReco-src
WORKDIR /opt/soft/

RUN wget https://github.com/akalinow/TPCReco/archive/refs/tags/${TPCReco_VERSION}.tar.gz -O tpcreco.tar.gz \
    && tar xf tpcreco.tar.gz && rm tpcreco.tar.gz \
    && mv TPCReco-${TPCReco_VERSION} TPCReco

RUN cmake -S TPCReco -Bbuild \
    -DCMAKE_INSTALL_PREFIX=/opt/soft/TPCReco \
    && cmake --build build --target install \
    && rm -r build
ENV PATH=${PATH}:/opt/soft/TPCReco/bin
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/opt/soft/TPCReco/lib
ENV CMAKE_PREFIX_PATH=/opt/soft/TPCReco/lib/cmake
WORKDIR /opt/soft/TPCReco/resources
