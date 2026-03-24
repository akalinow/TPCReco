ARG GET_VERSION=20251024_patched
ARG TPCReco_VERSION=25_02_2020

FROM elitpc/get:${GET_VERSION} AS common_base

COPY docker/packages.txt /tmp/packages.txt
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
ENV G4LEVELGAMMADATA=/usr/local/share/Geant4/data/PhotonEvaporation6.1
#ENV G4NEUTRONXSDATA=/usr/local/share/Geant4/data/G4NEUTRONXS1.4
ENV G4LEDATA=/usr/local/share/Geant4/data/G4EMLOW8.6.1
ENV G4NEUTRONHPDATA=/usr/local/share/Geant4/data/G4NDL4.7.1
ENV G4RADIOACTIVEDATA=/usr/local/share/Geant4/data/RadioactiveDecay6.1.2
ENV G4ENSDFSTATEDATA=/usr/local/share/Geant4/data/G4ENSDFSTATE3.0
ENV G4ABLADATA=/usr/local/share/Geant4/data/G4ABLA3.3
ENV G4PIIDATA=/usr/local/share/Geant4/data/G4PII1.3
ENV G4SAIDXSDATA=/usr/local/share/Geant4/data/G4SAIDDATA1.1
ENV G4REALSURFACEDATA=/usr/local/share/Geant4/data/RealSurface2.2

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

ENV CPU_OR_GPU=gpu
#TensorFlow 2.18 was the last release of Linux x86 libtensorflow packages.
ENV TF_VERSION=2.18.0
RUN echo "Download Tensorflow-$CPU_OR_GPU-$TF_VERSION" && \
    tensorflow_url="https://storage.googleapis.com/tensorflow/versions/$TF_VERSION/libtensorflow-$CPU_OR_GPU-linux-x86_64.tar.gz" && \
    wget -O /tmp/tensorflow.tar.gz $tensorflow_url && \
    echo "Extracting..." && \
    mkdir -p /opt/soft/tensorflow-$TF_VERSION && \
    tar -C /opt/soft/tensorflow-$TF_VERSION -xzf /tmp/tensorflow.tar.gz && \
    ldconfig && \
    rm /tmp/tensorflow.tar.gz

ENV TENSORFLOW_ROOT="/opt/soft/tensorflow-$TF_VERSION"

RUN pip3 install --upgrade 'jupyterhub==4.*' 'notebook==7.*'

RUN usermod -l jovyan -d /home/jovyan -m ubuntu
RUN groupmod -n jovyan ubuntu
RUN deluser jovyan sudo 
RUN deluser jovyan adm
RUN deluser jovyan dialout 
RUN deluser jovyan cdrom
RUN deluser jovyan dip
RUN deluser jovyan plugdev

    
# install user scripts
RUN mkdir -p /opt/scripts
COPY docker/start-jupyter.sh /opt/scripts/start-jupyter.sh
COPY docker/with-libtensorflow.sh /opt/scripts/with-libtensorflow.sh
RUN chmod +x /opt/scripts/start-jupyter.sh
RUN chmod +x /opt/scripts/with-libtensorflow.sh

RUN mkdir /opt/root/etc/notebook/migrated
RUN chmod -R a+rw /opt/root/etc/notebook/migrated

RUN mkdir -p /opt/root/etc/notebook/lab/workspaces
RUN chmod -R a+rw /opt/root/etc/notebook/lab/workspaces

USER jovyan
WORKDIR /home/jovyan

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
